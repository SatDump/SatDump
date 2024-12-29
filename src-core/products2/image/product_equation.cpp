#include "product_equation.h"
#include "libs/muparser/muParser.h"
#include "logger.h"

namespace satdump
{
    namespace products
    {
        std::vector<std::string> get_required_tokens(std::string equation, int *numout = nullptr)
        {
            mu::Parser rgbParser;
            int outValsCnt = 0;

            std::vector<std::string> tokens;

            while (true)
            {
                try
                {
                    rgbParser.SetExpr(equation);
                    rgbParser.Eval(outValsCnt);
                    if (numout != nullptr)
                        *numout = outValsCnt;
                }
                catch (mu::ParserError &e)
                {
                    if (e.GetCode() == mu::ecUNASSIGNABLE_TOKEN)
                    {
                        std::string token = e.GetToken();
                        rgbParser.DefineConst(token, 1);
                        tokens.push_back(token);
                        continue;
                    }
                    throw e;
                }
                break;
            }

            return tokens;
        }

        image::Image generate_equation_product_composite(ImageProducts *products, std::string equation, float *progess)
        {
            int nout_channels = 0;
            auto tokens = get_required_tokens(equation, &nout_channels);

            if (nout_channels > 4)
                throw satdump_exception("Can't have more than 4 output channels!");

            struct TokenS
            {
                image::Image &img;
                ChannelTransform &transform;
                double px = 0, py = 0;
                double v = 0;

                TokenS(image::Image &img, ChannelTransform &transform) : img(img), transform(transform) {}
            };

            std::vector<std::shared_ptr<TokenS>> tkts;
            mu::Parser equParser;
            equParser.SetExpr(equation);

            for (auto &tkt : tokens)
            {
                if (tkt.find("ch") == 0 && tkt.size() > 2)
                {
                    std::string index = tkt.substr(2);
                    logger->trace("Needs channel " + index);
                    auto &h = products->get_channel_image(index);
                    auto nt = std::make_shared<TokenS>(h.image, h.ch_transform);
                    equParser.DefineVar(tkt, &nt->v);
                    tkts.push_back(nt);
                }
                else
                    throw satdump_exception("Token " + tkt + " is invalid!");
            }

            std::shared_ptr<TokenS> rtkt = tkts[0];
            image::Image out(rtkt->img.depth(),
                             rtkt->img.width(),
                             rtkt->img.height(),
                             nout_channels);

            for (size_t x = 0; x < out.width(); x++)
            {
                for (size_t y = 0; y < out.height(); y++)
                {
                    // Update raw values
                    for (auto &t : tkts)
                    {
                        t->px = x, t->py = y;
                        rtkt->transform.forward(&t->px, &t->py);
                        t->transform.reverse(&t->px, &t->py);
                        if (t->px > 0 && t->px < t->img.width() && t->py > 0 && t->py < t->img.height())
                            t->v = (double)t->img.get_pixel_bilinear(0, t->px, t->py) / (double)t->img.maxval();
                    }

                    // Do the math
                    double *equOut = equParser.Eval(nout_channels);

                    // Set output
                    for (int c = 0; c < nout_channels; c++)
                        out.setf(c, x, y, out.clampf(equOut[c]));
                }

                if (progess != nullptr)
                    *progess = (float)x / (float)out.width();
            }

            return out;
        }
    }
}