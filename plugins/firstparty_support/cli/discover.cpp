#include "discover.h"

#include "../product_info.h"
#include "logger.h"
#include "utils/file/file_iterators.h"
#include "utils/file/folder_file_iterators.h"
#include "utils/file/zip_file_iterators.h"

namespace satdump
{
    void DiscoverProductsCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("prodisc", "Discover First-Party products in a file (or folder)");
        sub_module->add_option("file");
    }

    void DiscoverProductsCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        std::string path = subcom->get_option("file")->as<std::string>();

        firstparty::FirstPartyProductInfo info;

        bool is_zip = std::filesystem::path(path).extension() == ".zip";
        bool is_file = std::filesystem::is_regular_file(path);

        if (is_zip)
        {
            utils::ZipFilesIterator fit(path);
            std::shared_ptr<satdump::utils::FilesIteratorItem> f;

            while (fit.getNext(f))
            {
                if (f)
                {
                    info = firstparty::parseFirstPartyInfo(f);
                    if (info.type != firstparty::PRODUCT_NONE)
                        break;
                }
            }
        }
        else if (is_file)
        {
            std::shared_ptr<satdump::utils::FilesIteratorItem> f = std::make_shared<satdump::utils::FolderFileIteratorItem>(path);
            info = firstparty::parseFirstPartyInfo(f);
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

                if (info.group_id == firstparty::parseFirstPartyInfo(f).group_id)
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
} // namespace satdump