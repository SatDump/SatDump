/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "init.h"
#include "logger.h"

#include "new/procfile.h"
#include "new/product_info.h"
#include "new/type.h"
#include "utils/file/file_iterators.h"
#include "utils/file/folder_file_iterators.h"
#include "utils/file/zip_file_iterators.h"
#include "utils/time.h"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

#if 1
    std::string path = argv[1];

    {
        satdump::official::OfficialProductInfo info;

        bool is_zip = std::filesystem::path(path).extension() == ".zip";
        bool is_file = std::filesystem::is_regular_file(path);

        if (is_zip)
        {
            satdump::utils::ZipFilesIterator fit(path);
            std::shared_ptr<satdump::utils::FilesIteratorItem> f;

            while (fit.getNext(f))
            {
                if (f)
                {
                    info = satdump::official::parseOfficialInfo(f);
                    if (info.type != satdump::official::PRODUCT_NONE)
                        break;
                }
            }
        }
        else if (is_file)
        {
            std::shared_ptr<satdump::utils::FilesIteratorItem> f = std::make_shared<satdump::utils::FolderFileIteratorItem>(path);
            info = satdump::official::parseOfficialInfo(f);
        }
        else
        {
            logger->error("Not a file or ZIP!");
        }

        logger->info(info.name);

        if (info.group_id.size() || is_zip)
        {
            logger->trace("Group : " + info.group_id);

            std::shared_ptr<satdump::utils::FilesIterator> fit;
            if (is_zip)
                fit = std::make_shared<satdump::utils::ZipFilesIterator>(path);
            else
                fit = std::make_shared<satdump::utils::FolderFilesIterator>(std::filesystem::path(path).parent_path());

            std::shared_ptr<satdump::utils::FilesIteratorItem> f;
            while (fit->getNext(f))
            {
                if (!f)
                    continue;

                if (info.group_id == satdump::official::parseOfficialInfo(f).group_id)
                {
                    logger->trace(" - " + f->name);
                }
            }
        }
        else
        {
            std::shared_ptr<satdump::utils::FilesIteratorItem> f = std::make_shared<satdump::utils::FolderFileIteratorItem>(path);
            logger->trace(" - " + f->name);
        }
    }

#else
#if 1
    std::unique_ptr<satdump::utils::FilesIterator> fit = std::make_unique<satdump::utils::FolderFilesIterator>(argv[1]);
#else
    std::unique_ptr<satdump::utils::FilesIterator> fit = std::make_unique<satdump::utils::ZipFilesIterator>(
        "/tmp/satdump_official/W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI1+FCI-1C-RRAD-FDHSI-FD--x-x---x_C_EUMT_20250724120349_IDPFI_OPE_20250724120007_20250724120935_N__O_0073_0000.zip");
#endif

    std::unique_ptr<satdump::utils::FilesIteratorItem> f;

    while (fit->getNext(f))
    {
        if (f)
        {
            std::string identified;
            for (auto &p : satdump::official::getRegisteredProducts())
            {
                auto finfo = p.testFile(f);
                if (finfo.type != satdump::official::PRODUCT_NONE)
                    identified = (finfo.group_id.size() ? finfo.group_id + " | " : std::string()) + finfo.name; //+ " " = f->name);
            }

            if (identified.size())
                logger->info(f->name + " =======> " + identified);
            else
                logger->error(f->name);

#if 0
            uint32_t test1, test2, test3;
            if (sscanf(f->name.c_str(), "W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI%*d+FCI-1C-RRAD-FDHSI-FD--CHK-BODY---NC4E_C_EUMT_%14u_IDPFI_OPE_%14u_%14u_N__O_%*d_%*d.nc", &test1, &test2, &test3) == 3)
            {
                logger->trace(f->name + " MTG FCI Images");
            }
            else
            {
                logger->info(f->name);
            }
#endif
        }
    }
#endif
}
