#pragma once

#include "core/backend.h"
#include <string>
#include <thread>

namespace fileutils
{
    class DirSelTh
    {
    private:
        std::thread th;
        bool ready = false;
        std::string res;

    public:
        DirSelTh(std::string default_path)
        {
            th = std::thread(
                [this, default_path]()
                {
                    res = backend::selectFolderDialog(default_path);
                    ready = true;
                });
        }

        bool is_ready() { return ready; }
        std::string result() { return res; }

        ~DirSelTh()
        {
            if (th.joinable())
                th.join();
        }
    };

    class FileSelTh
    {
    private:
        std::thread th;
        bool ready = false;
        std::string res;

    public:
        FileSelTh(std::vector<std::pair<std::string, std::string>> filters, std::string default_path)
        {
            th = std::thread(
                [this, default_path, filters]()
                {
                    res = backend::selectFileDialog(filters, default_path);
                    ready = true;
                });
        }

        bool is_ready() { return ready; }
        std::string result() { return res; }

        ~FileSelTh()
        {
            if (th.joinable())
                th.join();
        }
    };
} // namespace fileutils

class FileSelectWidget
{
public:
    FileSelectWidget(std::string label, std::string selection_text, bool directory = false, bool allow_url = false);
    ~FileSelectWidget();
    bool draw(std::string hint = "");
    bool isValid();
    std::string getPath();
    void setPath(std::string new_path);
    void setDefaultDir(std::string new_path);

private:
    std::string label, selection_text, id, btnid, default_dir, path;
    bool directory, waiting_for_res, allow_url;
    fileutils::FileSelTh *file_select;
    fileutils::DirSelTh *dir_select;
    bool file_valid, url_valid;
};
