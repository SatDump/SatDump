#include "product_equation.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "common/image/meta.h"

#include "common/utils.h"
#include "products2/image/image_calibrator.h"

#include "common/image/io.h"

namespace satdump
{
    namespace products
    {
        /// TODOREWORK
        // Lut apply function
        double lutProcess(void *userdata, double chvalx, double chvaly, double ch)
        {
            image::Image *img = (image::Image *)userdata;
            if (img->size() == 0)
                return -1;
            if (chvalx < 0)
                chvalx = 0;
            if (chvalx > 1)
                chvalx = 1;
            if (chvaly < 0)
                chvaly = 0;
            if (chvaly > 1)
                chvaly = 1;
            return img->getf(ch, chvalx * (img->width() - 1), chvaly * (img->height() - 1));
        }
        ///

        // Lut config
        struct LutCfg
        {
            bool valid;
            std::string token;
            std::string path;
        };

        // Example :
        // lut=lut(lut_path.png); lut(ch1, 0), lut(ch1, 1), lut(ch1, 2)
        LutCfg tryParseLut(std::string str)
        {
            LutCfg c;
            c.valid = false;

            if (str.find_first_of('=lut') == std::string::npos)
                return c;

            c.token = str.substr(0, str.find_first_of('=lut') + 1);
            std::string parenthesis = str.substr(str.find_first_of('(') + 1, str.find_first_of(')') - str.find_first_of('(') - 1);
            auto parts = splitString(parenthesis, ',');

            if (parts.size() == 1)
            {
                c.path = parts[0];
                c.valid = true;
            }

            return c;
        }

        // Calibrated channels config
        struct CalibChannelCfg
        {
            bool valid;
            std::string token;
            std::string channel;
            std::string unit;
            double min;
            double max;
        };

        // Example :
        // cch2=(2,sun_angle_compensated_reflective_radiance,0,50);cch1=(1,sun_angle_compensated_reflective_radiance,0,50); cch2, cch2, cch1
        CalibChannelCfg tryParse(std::string str)
        {
            CalibChannelCfg c;
            c.valid = false;

            c.token = str.substr(0, str.find_first_of('='));
            std::string parenthesis = str.substr(str.find_first_of('(') + 1, str.find_first_of(')') - str.find_first_of('(') - 1);
            auto parts = splitString(parenthesis, ',');

            if (parts.size() == 4)
            {
                c.channel = parts[0];
                c.unit = parts[1];
                c.min = std::stod(parts[2]);
                c.max = std::stod(parts[3]);
                c.valid = true;
            }

            return c;
        }

        std::vector<std::string> get_required_tokens(std::string equation, std::vector<LutCfg> lut_cfgs, int *numout = nullptr)
        {
            std::vector<std::string> tokens;

            try
            {
                mu::Parser equParser;
                int outValsCnt = 0;

                // Define lut functions so it can detect tokens reliably
                image::Image img;
                for (auto &l : lut_cfgs)
                    equParser.DefineFunUserData(l.token.c_str(), lutProcess, &img);

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
            // Sanitize string
            equation.erase(std::remove(equation.begin(), equation.end(), '\r'), equation.end());
            equation.erase(std::remove(equation.begin(), equation.end(), '\n'), equation.end());
            equation.erase(std::remove(equation.begin(), equation.end(), ' '), equation.end());

            // See if we ave some setup string present
            std::map<std::string, CalibChannelCfg> calib_cfgs;
            std::vector<LutCfg> lut_cfgs;
            if (equation.find(';') != std::string::npos)
            {
                auto split_cfg = splitString(equation, ';');
                equation = split_cfg[split_cfg.size() - 1];
                for (auto &cfg : split_cfg)
                {
                    auto pcfg = tryParse(cfg);
                    if (pcfg.valid)
                        calib_cfgs.emplace(pcfg.token, pcfg);
                    auto lcfg = tryParseLut(cfg);
                    if (lcfg.valid)
                        lut_cfgs.push_back(lcfg);
                }
            }

            // Get required variables & number of output channels
            int nout_channels = 0;
            auto tokens = get_required_tokens(equation, lut_cfgs, &nout_channels);

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
                image::Image *other_images = new image::Image[tokens.size()];
                mu::Parser equParser;
                equParser.SetExpr(equation);

                // Add LUTs to equation parser
                std::vector<std::shared_ptr<image::Image>> lut_imgs;
                for (auto &l : lut_cfgs)
                {
                    logger->trace("Needs LUT " + l.path + " as " + l.token);
                    std::shared_ptr<image::Image> lutimg = std::make_shared<image::Image>();
                    image::load_img(*lutimg, l.path);
                    equParser.DefineFunUserData(l.token.c_str(), lutProcess, lutimg.get());
                    lut_imgs.push_back(lutimg);
                }

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
                    else if (calib_cfgs.count(tkt))
                    {
                        auto &c = calib_cfgs[tkt];
                        std::string index = calib_cfgs[tkt].channel;
                        logger->trace("Needs calibrated channel " + index + " as " + c.unit);
                        auto &h = product->get_channel_image(index);
                        other_images[ntkts] = generate_calibrated_product_channel(product, index, c.min, c.max, c.unit, progress); // TODOREWORK handle progress?
                        auto &image = other_images[ntkts];
                        auto nt = new TokenS(image, h.ch_transform);
                        nt->ch_idx = h.abs_index;
                        nt->width = image.width();
                        nt->height = image.height();
                        nt->maxval = image.maxval();
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
                delete[] other_images;

                return out;
            }
            catch (mu::ParserError &e)
            {
                throw satdump_exception(e.GetMsg());
            }
        }
    }
}