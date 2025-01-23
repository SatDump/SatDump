#include "equation.h"
#include "core/exception.h"
#include "libs/muparser/muParser.h"
#include "common/utils.h"
#include "common/image/meta.h"

namespace image
{
    image::Image generate_image_equation(std::vector<EquationChannel> channels, std::string equation)
    {
        if (!channels.size())
            throw satdump_exception("No channels provided!");

        int width = channels[0].img->width();
        int height = channels[0].img->height();
        int depth = channels[0].img->depth();

        try
        {
            mu::Parser equParser;
            equParser.SetExpr(equation);

            nlohmann::json proj_cfg;

            for (auto &p : channels)
            {
                if (p.img->width() != width || p.img->height() != height)
                    throw satdump_exception("All channels must have the same width!");

                if (p.img->channels() == 1)
                {
                    equParser.DefineVar(p.tkt, &p.val[0]);
                }
                else
                {
                    auto tkts = splitString(p.tkt, ',');
                    for (int i = 0; i < p.img->channels(); i++)
                        equParser.DefineVar(tkts.size() > i ? tkts[i] : (p.tkt + "_" + std::to_string(i + 1)), &p.val[i]);
                }

                if (p.img->depth() > depth)
                    depth = p.img->depth();

                if (image::has_metadata_proj_cfg(*p.img))
                    proj_cfg = image::get_metadata_proj_cfg(*p.img);
            }

            int nout_channels;
            equParser.Eval(nout_channels);

            // We can't handle over 4
            if (nout_channels > 4)
                throw satdump_exception("Can't have more than 4 output channels!");

            image::Image out(depth,
                             channels[0].img->width(),
                             channels[0].img->height(),
                             nout_channels);
            image::set_metadata_proj_cfg(out, proj_cfg);

            size_t x, y, c;
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    for (auto &p : channels)
                    {
                        if (p.img->channels() == 1)
                        {
                            p.val[0] = p.img->getf(0, x, y);
                        }
                        else
                        {
                            for (int i = 0; i < p.img->channels(); i++)
                                p.val[i] = p.img->getf(i, x, y);
                        }
                    }

                    // Run the parser
                    double *equOut = equParser.Eval(nout_channels);

                    // Set output
                    for (c = 0; c < nout_channels; c++)
                        out.setf(c, x, y, out.clampf(equOut[c]));
                }
            }

            return out;
        }
        catch (mu::ParserError &e)
        {
            throw satdump_exception(e.GetMsg());
        }
    }
}