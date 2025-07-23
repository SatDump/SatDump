#include "product_expression.h"
#include "common/calibration.h"
#include "common/physics_constants.h"
#include "core/exception.h"
#include "image/image.h"
#include "image/meta.h"
#include "image/processing.h"
#include "libs/muparser/muParser.h"
#include "logger.h"

#include "products/image/image_calibrator.h"
#include "utils/string.h"

#include "image/io.h"

#include "core/resources.h"
#include "projection/projection.h"
#include "projection/utils/equirectangular.h"
#include "utils/unit_parser.h"

namespace satdump
{
    namespace products
    {
        /// TODOREWORK
        namespace
        {
            struct EquP
            {
                image::Image img;
                size_t *x;
                size_t *y;
                projection::Projection p;
                geodetic::geodetic_coords_t pos;
                projection::EquirectangularProjection ep;
                int ox, oy;
            };

            // Equp apply function
            double equpProcess(void *userdata, double ch)
            {
                EquP *p = (EquP *)userdata;
                image::Image &img = p->img;
                if (img.size() == 0)
                    return -1;
                if (ch > img.channels())
                    return -1;

                if (p->p.inverse(*p->x, *p->y, p->pos))
                    return 0;

                p->ep.forward(p->pos.lon, p->pos.lat, p->ox, p->oy);

                return img.getf(ch, p->ox, p->oy);
            }
            ///

            // Equp config
            struct EqupCfg
            {
                bool valid;
                std::string token;
                std::string path;
            };

            // Example :
            // bluemarble=equp(blue_marble.png); bluemarble(0), bluemarble(1), bluemarble(2)
            EqupCfg tryParseEqup(std::string str)
            {
                EqupCfg c;
                c.valid = false;

                if (str.find("=equp") == std::string::npos)
                    return c;

                c.token = str.substr(0, str.find("=equp"));
                std::string parenthesis = str.substr(str.find_first_of("(") + 1, str.find_first_of(")") - str.find_first_of("(") - 1);
                auto parts = splitString(parenthesis, ',');

                if (parts.size() == 1)
                {
                    c.path = parts[0];
                    c.valid = true;
                }

                return c;
            }
        } // namespace

        namespace
        {
            // Lut apply function
            double lutProcess(void *userdata, double chvalx, double chvaly, double ch)
            {
                image::Image *img = (image::Image *)userdata;
                if (img->size() == 0)
                    return -1;
                if (ch > img->channels())
                    return -1;
                if (chvalx < 0)
                    chvalx = 0;
                if (chvalx > 1)
                    chvalx = 1;
                if (chvaly < 0)
                    chvaly = 0;
                if (chvaly > 1)
                    chvaly = 1;
                // Nan guard
                if (chvalx != chvalx)
                    chvalx = 0;
                if (chvaly != chvaly)
                    chvaly = 0;

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

                if (str.find("=lut") == std::string::npos)
                    return c;

                c.token = str.substr(0, str.find("=lut"));
                std::string parenthesis = str.substr(str.find_first_of("(") + 1, str.find_first_of(")") - str.find_first_of("(") - 1);
                auto parts = splitString(parenthesis, ',');

                if (parts.size() == 1)
                {
                    c.path = parts[0];
                    c.valid = true;
                }

                return c;
            }
        } // namespace

        namespace
        {
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

                c.token = str.substr(0, str.find_first_of("="));
                std::string parenthesis = str.substr(str.find_first_of("(") + 1, str.find_first_of(")") - str.find_first_of("(") - 1);
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
        } // namespace

        std::vector<std::string> get_required_tokens(std::string expression, std::vector<LutCfg> lut_cfgs, std::vector<EqupCfg> equp_cfgs, int *numout = nullptr)
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
                for (auto &l : equp_cfgs)
                    equParser.DefineFunUserData(l.token.c_str(), equpProcess, &img);

                while (true)
                {
                    try
                    {
                        equParser.SetExpr(expression);
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

        image::Image generate_expression_product_composite(ImageProduct *product, std::string expression, float *progress)
        {
            // Dummy mode, a special mode where no actual work is done, only checking
            bool dummy_mode = progress ? (*progress == -1) : false;

            // Sanitize string
            expression.erase(std::remove(expression.begin(), expression.end(), '\r'), expression.end());
            expression.erase(std::remove(expression.begin(), expression.end(), '\n'), expression.end());
            expression.erase(std::remove(expression.begin(), expression.end(), ' '), expression.end());

            // See if we have some token macros
            {
                std::vector<std::string> all_found;
                std::string curr;
                bool in_tkt = false;
                for (auto &c : expression)
                {
                    if (c == '{')
                    {
                        in_tkt = true;
                    }
                    else if (c == '}' && in_tkt)
                    {
                        all_found.push_back(curr);
                        curr.clear();
                        in_tkt = false;
                    }
                    else if (in_tkt)
                    {
                        curr += c;
                    }
                }

                for (auto tkt : all_found)
                {
                    auto atkt = product->get_channel_image_by_unitstr(tkt).channel_name;
                    replaceAllStr(expression, "{" + tkt + "}", atkt);
                }
            }

            // See if we ave some setup string present
            std::map<std::string, CalibChannelCfg> calib_cfgs;
            std::vector<LutCfg> lut_cfgs;
            std::vector<EqupCfg> equp_cfgs;

            // Also see if the reference channel is getting overwritten
            std::string reference_channel = "";

            if (expression.find(';') != std::string::npos)
            {
                try
                {
                    auto split_cfg = splitString(expression, ';');
                    expression = split_cfg[split_cfg.size() - 1];
                    for (auto &cfg : split_cfg)
                    {
                        if (cfg == expression)
                            continue; // Skip actual expression!

                        auto pcfg = tryParse(cfg);
                        auto lcfg = tryParseLut(cfg);
                        auto ecfg = tryParseEqup(cfg);
                        if (pcfg.valid)
                            calib_cfgs.emplace(pcfg.token, pcfg);
                        else if (lcfg.valid)
                            lut_cfgs.push_back(lcfg);
                        else if (ecfg.valid)
                            equp_cfgs.push_back(ecfg);
                        else if (cfg.find("=") != std::string::npos) // Macro
                        {
                            auto macro = splitString(cfg, '=');
                            if (macro.size() == 2)
                            {
                                if (macro[0] == "ref_channel")
                                { // Special case to override the ref
                                    reference_channel = macro[1];
                                    logger->info("Using reference channel : " + reference_channel);
                                }
                                else
                                    replaceAllStr(expression, macro[0], "(" + macro[1] + ")");
                            }
                            else
                                logger->warn("Invalid macro config!");
                        }
                    }
                }
                catch (std::exception &e)
                {
                    throw satdump_exception("Error parsing equation parameters! " + std::string(e.what()));
                }
            }

            if (!dummy_mode)
                logger->debug("Final Expression " + expression);

            // Get required variables & number of output channels
            int nout_channels = 0;
            auto tokens = get_required_tokens(expression, lut_cfgs, equp_cfgs, &nout_channels);

            // We can't handle over 4
            if (nout_channels > 4)
                throw satdump_exception("Can't have more than 4 output channels!");

            // If in dummy mode, we just check the channels are present/can be calibrated
            // Let it error out if need be!
            if (dummy_mode)
            {
                *progress = 1;

                for (auto &t : tokens)
                    if (t.find("ch") == 0 && t.size() > 2)
                        product->get_channel_image(t.substr(2));

                for (auto &t : calib_cfgs)
                {
                    product->get_channel_image(t.second.channel);
                    if (!product->has_calibration() && t.second.unit != "equalized")
                        *progress = 0;
                }

                return image::Image();
            }

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

            // Index of the reference channel
            int reference_channel_index = reference_channel == "" ? 0 : -1;

            try
            {
                // Setup expression parser
                size_t ntkts = 0;
                TokenS **tkts = new TokenS *[tokens.size()];
                image::Image *other_images = new image::Image[tokens.size()];
                mu::Parser equParser;
                equParser.SetExpr(expression);

                // Add LUTs to expression parser
                std::vector<std::shared_ptr<image::Image>> lut_imgs;
                for (auto &l : lut_cfgs)
                {
                    logger->trace("Needs LUT " + l.path + " as " + l.token);
                    std::shared_ptr<image::Image> lutimg = std::make_shared<image::Image>();
                    image::load_img(*lutimg, resources::getResourcePath(l.path));
                    equParser.DefineFunUserData(l.token.c_str(), lutProcess, lutimg.get());
                    lut_imgs.push_back(lutimg);
                }

                // Add EquPs to expression parser
                std::vector<std::shared_ptr<EquP>> equp_imgs;
                for (auto &l : equp_cfgs)
                {
                    logger->trace("Needs EquP " + l.path + " as " + l.token);
                    std::shared_ptr<EquP> e = std::make_shared<EquP>();
                    image::load_img(e->img, resources::getResourcePath(l.path));
                    e->ep.init(e->img.width(), e->img.height(), -180, 90, 180, -90);
                    equParser.DefineFunUserData(l.token.c_str(), equpProcess, e.get(), false);
                    equp_imgs.push_back(e);
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

                        if (reference_channel != "" && reference_channel == index)
                            reference_channel_index = ntkts - 1; // Try to set reference
                    }
                    else if (calib_cfgs.count(tkt))
                    { // Calibrated channel case
                        auto &c = calib_cfgs[tkt];
                        std::string index = calib_cfgs[tkt].channel;
                        logger->trace("Needs calibrated channel " + index + " as " + c.unit);
                        auto &h = product->get_channel_image(index);
                        if (c.unit == "equalized") // TODOREWORK this is an exception?
                        {
                            other_images[ntkts] = h.image;
                            image::equalize(other_images[ntkts]);
                        }
                        else
                            other_images[ntkts] = generate_calibrated_product_channel(product, index, c.min, c.max, c.unit, progress); // TODOREWORK handle progress?
                        auto &image = other_images[ntkts];
                        auto nt = new TokenS(image, h.ch_transform);
                        nt->ch_idx = h.abs_index;
                        nt->width = image.width();
                        nt->height = image.height();
                        nt->maxval = image.maxval();
                        equParser.DefineVar(tkt, &nt->v);
                        tkts[ntkts++] = nt;

                        if (reference_channel != "" && reference_channel == index)
                            reference_channel_index = ntkts - 1; // Try to set reference
                    }
                    else // Abort if we can't recognize token
                        throw satdump_exception("Token " + tkt + " is invalid!");
                }

                if (ntkts == 0)
                    throw satdump_exception("No channel in expression!");

                // Select reference channel
                if (reference_channel_index == -1)
                    throw satdump_exception("Reference channel " + reference_channel + " not found! Must be present/used in equation!");
                TokenS *rtkt = tkts[reference_channel_index];

                // Setup output image
                image::Image out(rtkt->img.depth(), rtkt->img.width(), rtkt->img.height(), nout_channels);

                // Variables to utilize
                size_t x, y, i, c;

                // EquPs
                for (auto &i : equp_imgs)
                {
                    i->x = &x;
                    i->y = &y;
                    i->p = product->get_proj_cfg(rtkt->ch_idx);
                    i->p.init(0, 1);
                }

                // Main loop
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
                            if (t->px >= 0 && t->px < t->width && t->py >= 0 && t->py < t->height)
                                t->v = (((t->px - (uint32_t)t->px) == 0 && (t->py - (uint32_t)t->py) == 0) ? (double)t->img.get(0, t->px, t->py) : (double)t->img.get_pixel_bilinear(0, t->px, t->py)) /
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

        bool check_expression_product_composite(satdump::products::ImageProduct *product, std::string expression)
        {
            float dummy = -1;
            try
            {
                satdump::products::generate_expression_product_composite(product, expression, &dummy);
            }
            catch (std::exception &)
            {
                return false;
            }
            return dummy == 1;
        }
    } // namespace products
} // namespace satdump