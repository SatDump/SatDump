#include "product_equation.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "common/image/meta.h"

#include "products2/image/image_calibrator.h"

namespace satdump
{
    namespace products
    {

        std::vector<std::string> get_required_tokens(std::string equation, int *numout = nullptr)
        {
            std::vector<std::string> tokens;

            try
            {
                mu::Parser equParser;
                int outValsCnt = 0;

                while (true)
                {
                    try
                    {
                        equParser.SetExpr(equation);
                        equParser.Eval(outValsCnt);
                        if (numout != nullptr)
                            *numout = outValsCnt;
                    }
                    catch (mu::ParserError &e)
                    {
                        if (e.GetCode() == mu::ecUNASSIGNABLE_TOKEN)
                        {
                            std::string token = e.GetToken();
                            equParser.DefineConst(token, 1);
                            tokens.push_back(token);
                            continue;
                        }
                        throw e;
                    }
                    break;
                }
            }
            catch (mu::ParserError &e)
            {
                throw satdump_exception(e.GetMsg());
            }

            return tokens;
        }

        image::Image generate_equation_product_composite(ImageProduct *product, std::string equation, float *progress)
        {
            // Get required variables & number of output channels
            int nout_channels = 0;
            auto tokens = get_required_tokens(equation, &nout_channels);

            // We can't handle over 4
            if (nout_channels > 4)
                throw satdump_exception("Can't have more than 4 output channels!");

            // Handles variables for each input channel
            struct TokenS
            {
                int ch_idx;                   // Channel ID, absolute
                image::Image &img;            // Channel image
                ChannelTransform &transform;  // Channel transform
                double px = 0, py = 0;        // Chanel X/Y to be transformed
                double v = 0;                 // Channel value passed to muParser
                size_t width = 0, height = 0; // Channel width/height for bounds checks
                double maxval = 0;            // Maxval to convert to double

                TokenS(image::Image &img, ChannelTransform &transform) : img(img), transform(transform) {}
            };

            try
            {
                // Setup expression parser
                size_t ntkts = 0;
                TokenS **tkts = new TokenS *[tokens.size()];
                mu::Parser equParser;
                equParser.SetExpr(equation);

                // Iterate through tokens to set them up
                for (auto &tkt : tokens)
                {
                    if (tkt.find("ch") == 0 && tkt.size() > 2)
                    { // Raw channel case
                        std::string index = tkt.substr(2);
                        logger->trace("Needs channel " + index);
                        auto &h = product->get_channel_image(index);
                        auto nt = new TokenS(h.image, h.ch_transform);
                        nt->ch_idx = h.abs_index;
                        nt->width = h.image.width();
                        nt->height = h.image.height();
                        nt->maxval = h.image.maxval();
                        equParser.DefineVar(tkt, &nt->v);
                        tkts[ntkts++] = nt;
                    }
                    else // Abort if we can't recognize token
                        throw satdump_exception("Token " + tkt + " is invalid!");
                }

                if (ntkts == 0)
                    throw satdump_exception("No channel in equation!");

                // Select reference channel, setup output
                TokenS *rtkt = tkts[0];
                image::Image out(rtkt->img.depth(),
                                 rtkt->img.width(),
                                 rtkt->img.height(),
                                 nout_channels);

                size_t x, y, i, c;
                for (y = 0; y < rtkt->height; y++)
                {
                    for (x = 0; x < rtkt->width; x++)
                    {
                        // Update raw values, apply transforms if needed
                        for (i = 0; i < ntkts; i++)
                        {
                            TokenS *t = tkts[i];
                            t->px = x, t->py = y;
                            rtkt->transform.forward(&t->px, &t->py);
                            t->transform.reverse(&t->px, &t->py);
                            if (t->px > 0 && t->px < t->width && t->py > 0 && t->py < t->height)
                                t->v = (((t->px - (uint32_t)t->px) == 0 && (t->py - (uint32_t)t->py) == 0)
                                            ? (double)t->img.get(0, t->px, t->py)
                                            : (double)t->img.get_pixel_bilinear(0, t->px, t->py)) /
                                       t->maxval; // TODOREWORK optimize
                            else
                                t->v = 0; // TODOREWORK DETECT AND CROP
                        }

                        // Run the parser
                        double *equOut = equParser.Eval(nout_channels);

                        // Set output
                        for (c = 0; c < nout_channels; c++)
                            out.setf(c, x, y, out.clampf(equOut[c]));
                    }

                    if (progress != nullptr)
                        *progress = (float)y / (float)rtkt->height;
                }

                // Add metadata
                if (product->has_proj_cfg())
                    image::set_metadata_proj_cfg(out, product->get_proj_cfg(rtkt->ch_idx));

                // Free up tokens
                for (int i = 0; i < ntkts; i++)
                    delete tkts[i];

                return out;
            }
            catch (mu::ParserError &e)
            {
                throw satdump_exception(e.GetMsg());
            }
        }
    }
}