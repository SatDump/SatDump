#include "../io.h"
#include "H5Zpublic.h"
#include "core/exception.h"
#include "image/image.h"
#include "logger.h"
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <H5Cpp.h>

namespace satdump
{
    namespace image
    {
        void save_hdf(Image &img, std::string file)
        {
            auto d_depth = img.depth();
            auto d_channels = img.channels();
            auto d_height = img.height();
            auto d_width = img.width();

            if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
            {
                logger->trace("Tried to save empty HDF!");
                return;
            }

            H5::H5File hfile(file, H5F_ACC_TRUNC);

            hsize_t fdim[] = {d_height, d_width, (size_t)d_channels}; // dim sizes of ds (on disk)
            H5::DataSpace fspace(3, fdim);

            H5::DSetCreatPropList plist;
            plist.setDeflate(6);
            /// plist.setFillValue(H5::PredType::NATIVE_INT, &fillvalue);

            hsize_t fdims[] = {d_height / 4, d_width / 4, 1};
            plist.setChunk(3, fdims);

            if (d_depth > 8)
            {
                H5::DataSet dataset(hfile.createDataSet("image", H5::PredType::NATIVE_UINT16, fspace, plist));
                dataset.write(img.raw_data(), H5::PredType::NATIVE_UINT16);
            }
            else
            {
                H5::DataSet dataset(hfile.createDataSet("image", H5::PredType::NATIVE_UINT8, fspace, plist));
                dataset.write(img.raw_data(), H5::PredType::NATIVE_UINT8);
            }
        }

        void load_hdf(Image &img, std::string file)
        {
            if (!std::filesystem::exists(file))
                return;

            H5::H5File hfile(file, H5F_ACC_RDONLY);

            H5::DataSet s = hfile.openDataSet("image");

            if (s.getSpace().getSimpleExtentNdims() != 3)
            {
                logger->error("Rank is not 3!");
                return;
            }

            hsize_t image_dims[3];
            s.getSpace().getSimpleExtentDims(image_dims);
            logger->critical("%d %d %d", image_dims[1], image_dims[0], image_dims[2]);

            if (s.getDataType() == H5::PredType::NATIVE_UINT16)
            {
                img = image::Image(16, image_dims[1], image_dims[0], image_dims[2]);
                s.read(img.raw_data(), H5::PredType::NATIVE_UINT16);
            }
            else
            {
                img = image::Image(8, image_dims[1], image_dims[0], image_dims[2]);
                s.read(img.raw_data(), H5::PredType::NATIVE_UINT8);
            }
        }
    } // namespace image
} // namespace satdump