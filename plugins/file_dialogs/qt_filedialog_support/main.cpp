#include "core/plugin.h"
#include "logger.h"
#include "imgui/file_dialog/file_dialog.h"

#include <qt5/QtWidgets/qfiledialog.h>
#include <qt5/QtWidgets/QApplication>
#include <thread>

namespace satdump
{
    class QTFileDialog : public FileDialog
    {
    private:
        std::thread l_th;
        std::string final_path = "";

    public:
        QTFileDialog(file_dialog_type_t type, std::string name, std::string directory)
            : FileDialog(type, name, directory)
        {
            auto fun = [this]()
            {
                int v = 0;
                QApplication *a = new QApplication(v, NULL);

                {
                    QString filename = QFileDialog::getOpenFileName(
                        nullptr,
                        QObject::tr("Open Document"),
                        QDir::currentPath(),
                        QObject::tr("All files (*.*);;Document files (*.doc *.rtf)"));

                    if (!filename.isNull())
                        final_path = filename.toStdString();
                }

                a->closeAllWindows();
                a->quit();

                logger->info("Closing!");
                delete a;
                logger->info("Closing2!");
            };
            l_th = std::thread(fun);
        }

        ~QTFileDialog()
        {
            logger->info("Exiting???");
            if (l_th.joinable())
                l_th.join();
            logger->info("Exiting!!!!");
        }

        bool ready()
        {
            return final_path.size() > 0;
        }

        std::string result()
        {
            return final_path;
        }
    };
}

class QTFileDialogSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "qt_filedialog_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<satdump::RequestFileDialogEvent>(provideFileDialogHandler);
    }

    static void provideFileDialogHandler(const satdump::RequestFileDialogEvent &evt)
    {
        evt.constructors.push_back([](satdump::file_dialog_type_t type, std::string name, std::string directory)
                                   { return std::make_shared<satdump::QTFileDialog>(type, name, directory); });
    }
};

PLUGIN_LOADER(QTFileDialogSupport)