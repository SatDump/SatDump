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

#include "logger.h"
#include <gtk/gtk.h>
#include <qt5/QtWidgets/qfiledialog.h>
#include <qt5/QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

#if 1
    int v = 0;
    QApplication a(v, NULL);

    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        QObject::tr("Open Document"),
        QDir::currentPath(),
        QObject::tr("All files (*.*)"));

    if (!filename.isNull())
    {
        logger->info(filename.toStdString());
    }

#else
    gtk_init();

    GtkFileDialog *dialog = gtk_file_dialog_new();
    // gtk_file_dialog_set_title(dialog, "Title");
    gtk_file_dialog_open(dialog, NULL, NULL, NULL, NULL);
    sleep(10);
#endif
}