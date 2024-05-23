#include "core/plugin.h"
#include "logger.h"
#include "imgui/file_dialog/file_dialog.h"

#include <gtk/gtk.h>
#include <thread>

GtkApplication *gtk_app;

namespace satdump
{
    class GTKFileDialog : public FileDialog
    {
    private:
        std::thread l_th;
        std::string final_path = "";

    private:
        int stat = 0;

        static void fileselection_dialog_response(GtkWidget *widget, int response)
        {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);
            if (response == GTK_RESPONSE_ACCEPT)
            {
                g_autoptr(GListModel) model = gtk_file_chooser_get_files(chooser);
                guint items = g_list_model_get_n_items(model);

                for (guint i = 0; i < items; ++i)
                {
                    g_autoptr(GFile) file = (GFile *)g_list_model_get_item(model, i);
                    ((GTKFileDialog *)widget->priv)->final_path = g_file_get_path(file);
                }
            }
            g_application_release(g_application_get_default());
        }

        static void app_activate(GApplication *a)
        {
            GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
            GtkFileChooserNative *dialog = gtk_file_chooser_native_new("Open file...", NULL, action, "_OK", "_Cancel");
            gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
            g_signal_connect(dialog, "response", G_CALLBACK(fileselection_dialog_response), NULL);
            gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
            g_application_hold(g_application_get_default());
        }

    public:
        GTKFileDialog(file_dialog_type_t type, std::string name, std::string directory)
            : FileDialog(type, name, directory)
        {
            g_signal_connect(gtk_app, "activate", G_CALLBACK(app_activate), this);

            l_th = std::thread([this]()
                               { stat = g_application_run(G_APPLICATION(gtk_app), 0, 0); });
        }

        ~GTKFileDialog()
        {
            //  g_object_unref(app);
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

class GTKFileDialogSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "gtk_filedialog_support";
    }

    void init()
    {
        gtk_app = gtk_application_new("org.satdump.SatDump", G_APPLICATION_FLAGS_NONE);
        satdump::eventBus->register_handler<satdump::RequestFileDialogEvent>(provideFileDialogHandler);
    }

    static void provideFileDialogHandler(const satdump::RequestFileDialogEvent &evt)
    {
        //   evt.constructors.push_back([](satdump::file_dialog_type_t type, std::string name, std::string directory)
        //                              { return std::make_shared<satdump::GTKFileDialog>(type, name, directory); });
    }
};

PLUGIN_LOADER(GTKFileDialogSupport)