#include "procfile.h"
#include "core/exception.h"
#include "logger.h"
#include "product_info.h"
#include "type.h"
#include "utils/file/file_iterators.h"
#include "utils/file/folder_file_iterators.h"
#include "utils/file/zip_file_iterators.h"

namespace satdump
{
    namespace official
    {
        OfficialProductInfo identifyOfficialProductFile(std::string fpath)
        {
            // Identify the product to process. If a single file, just work off that
            // and figure out if it's multi-file, if it's a zip just identify the first
            // one we can.

            OfficialProductInfo toproc_info;

            bool is_zip = std::filesystem::path(fpath).extension() == ".zip";
            bool is_file = std::filesystem::is_regular_file(fpath);

            if (is_zip)
            {
                satdump::utils::ZipFilesIterator fit(fpath);
                std::unique_ptr<satdump::utils::FilesIteratorItem> f;

                while (fit.getNext(f))
                {
                    if (f)
                    {
                        toproc_info = parseOfficialInfo(f);
                        break;
                    }
                }
            }
            else if (is_file)
            {
                std::unique_ptr<satdump::utils::FilesIteratorItem> f = std::make_unique<satdump::utils::FolderFileIteratorItem>(fpath);
                toproc_info = parseOfficialInfo(f);
            }
            else
            {
                logger->error("Not a file or ZIP! => " + fpath);
            }

            return toproc_info;
        }

        std::shared_ptr<products::Product> processOfficialProductFile(OfficialProductInfo toproc_info, std::string fpath)
        {
            // Check it is valid & setup processor

            if (toproc_info.type == PRODUCT_NONE)
                throw satdump_exception("No product present in file " + fpath + "!");

            logger->debug("Processing product : " + toproc_info.name);

            auto processor = getProcessorForProduct(toproc_info);

            // Run all required files through.
            bool is_zip = std::filesystem::path(fpath).extension() == ".zip";

            if (toproc_info.group_id.size() || is_zip) // If we have a group, we need to iterate through all of them (also do so if we got a single one in a file)
            {
                logger->trace("Group : " + toproc_info.group_id);

                std::unique_ptr<satdump::utils::FilesIterator> fit;
                if (is_zip)
                    fit = std::make_unique<satdump::utils::ZipFilesIterator>(fpath);
                else
                    fit = std::make_unique<satdump::utils::FolderFilesIterator>(std::filesystem::path(fpath).parent_path());

                std::unique_ptr<satdump::utils::FilesIteratorItem> f;
                while (fit->getNext(f))
                {
                    if (!f)
                        continue;

                    if (toproc_info.group_id == parseOfficialInfo(f).group_id)
                    {
                        logger->trace(" - " + f->name);
                        processor->ingestFile(f->getPayload());
                    }
                }
            }
            else
            {
                std::unique_ptr<satdump::utils::FilesIteratorItem> f = std::make_unique<satdump::utils::FolderFileIteratorItem>(fpath);
                logger->trace(" - " + f->name);
                processor->ingestFile(f->getPayload());
            }

            // Finally, return product
            return processor->getProduct();
        }
    } // namespace official
} // namespace satdump