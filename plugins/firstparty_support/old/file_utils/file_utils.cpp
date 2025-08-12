#include "file_utils.h"
#include "libs/miniz/miniz.h"
#include <filesystem>

#include <hdf5.h>
#include <H5LTpublic.h>

namespace satdump
{
    std::string try_get_eumetsat_id(std::string zippath)
    {
        std::string f = "";

        if (!file_is_zip(zippath))
            return f;

        mz_zip_archive zip{};
        mz_zip_reader_init_file(&zip, zippath.c_str(), 0);

        int numfiles = mz_zip_reader_get_num_files(&zip);

        for (int fi = 0; fi < numfiles && f == ""; fi++)
        {
            if (mz_zip_reader_is_file_supported(&zip, fi))
            {
                char filename[1000];
                mz_zip_reader_get_filename(&zip, fi, filename, 1000);
                std::string ext = std::filesystem::path(filename).extension().string();

                if (ext == ".nc")
                {
                    size_t filesize = 0;
                    void *file_ptr = mz_zip_reader_extract_to_heap(&zip, fi, &filesize, 0);
                    hid_t file = H5LTopen_file_image(file_ptr, filesize, H5F_ACC_RDONLY);

                    if (file > 0)
                    {
                        if (H5Aexists(file, "product_id"))
                        {
                            hid_t att = H5Aopen(file, "product_id", H5P_DEFAULT);
                            hid_t att_type = H5Aget_type(att);
                            char *str;
                            if (H5Tget_class(att_type) == H5T_STRING)
                            {
                                H5Aread(att, att_type, &str);
                                f = std::string(str);
                                H5free_memory(str);
                            }
                            H5Tclose(att_type);
                            H5Aclose(att);
                        }
                        else if (H5Aexists(file, "product_name"))
                        {
                            hid_t att = H5Aopen(file, "product_name", H5P_DEFAULT);
                            hid_t att_type = H5Aget_type(att);
                            char str[10000];
                            if (H5Tget_class(att_type) == H5T_STRING)
                            {
                                H5Aread(att, att_type, &str);
                                f = std::string(str);
                            }
                            H5Tclose(att_type);
                            H5Aclose(att);
                        }
                        H5Fclose(file);
                    }
                    mz_free(file_ptr);
                }
            }
        }

        mz_zip_reader_end(&zip);
        return f;
    }
}
