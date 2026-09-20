#include "crashdump.h"

#ifdef SATDUMP_CRASHDUMP
#include "utils/format.h"
#include <backtrace.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#endif

namespace satdump
{
#ifdef SATDUMP_CRASHDUMP
    static backtrace_state *bt_state = nullptr;

    static int bt_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
    {
        std::string &report = *((std::string *)data);
        report += satdump::svformat("  at %s (%s:%d)\n", function ? function : "??", filename ? filename : "??", lineno);
        return 0;
    }

    static void bt_error(void *data, const char *msg, int errnum)
    {
        std::string &report = *((std::string *)data);
        report += satdump::svformat("  [backtrace error: %s (%d)]\n", msg, errnum);
    }

    static void crash_handler(int sig)
    {
        std::string report;

        if (time(NULL) % 5 == 0)
        {
            report += "                  KI                |-------------------------------|      \n";
            report += "                KING                | SPECIAL MESSAGE FROM THE KING |      \n";
            report += "               NOAA15               |-------------------------------|      \n";
            report += "            NOAA15 KIN                                                        \n";
            report += "           NOAA15 N         AAN                                                \n";
            report += "          NOAA15-AAN       KINGN                                               \n";
            report += "         NOAA15 NOAA15N   N  AAN                                               \n";
            report += "        NOAA15 NOAA15KINGN KINGN                               KING              \n";
            report += "       NOAA15-NOAA15NOAA15AAN AN                              NOAA15KING      \n";
            report += "       NOAA15 NOAA15N/--\5NOAA15N                           NOAA15KING        \n";
            report += "      NOAA15 NOAA15NO|  |NOAA15KING  AAN                  ----------          \n";
            report += "      NOAA15-KINGNOAA\\--/1/--\5KINGNOAA15KI             NOAA15KING            \n";
            report += "     NOAA1        NOAA15KI|  |NGNOAA15KINGN          NOAA15KING              \n";
            report += "                   NOAA15K\\--/5N/--\5NOAA15        ----------                \n";
            report += "                   N NOAA15NOAA1|  |A15KING######NOAA15KING                  \n";
            report += "                   N  AAN  NOAA1\\--/A15KING###NOAA15NOAA15                    \n";
            report += "                 -/N\\-    NOAA15NOAA15KINGN  -----------                      \n";
            report += "                 ||N||       N  NOAA15AAN  NOAA15KING                        \n";
            report += "                 ||N||            NOAA15 -----------                          \n";
            report += "                                    N  NOAA15KING                            \n";
            report += "                                     NOAA15KING                              \n";
            report += "                                   -----------                                \n";
            report += "                                  NOAA15KING                                  \n";
            report += "                                NOAA15KING                                     \n";
            report += "                                                                              \n";
            report += "|------------------------------------------------------------------|\n";
            report += "|SUBJECT: CONCERNS ABOUT YOUR LACK OF WORSHIPMENT TO HIS MAJESTY   |\n";
            report += "|------------------------------------------------------------------|\n";
            report += "|Dear SatDump user, it has come to my attention that you didn't pay|\n";
            report += "|the homages I was expecting. I have therefore decided to make your|\n";
            report += "|SatDump copy crash.                                               |\n";
            report += "|You may copy this crash report to the developers and they might be|\n";
            report += "|able to figure something out for you.                             |\n";
            report += "|                                                                  |\n";
            report += "|Glory to the eternal POES kingdom.                                |\n";
            report += "|                                                                  |\n";
            report += "|HIS MAJESTY THE KING NOAA 15                                      |\n";
            report += "|------------------------------------------------------------------|\n";
        }

        report += "\n";
        report += "   ______                __    ____                      \n";
        report += "  / ____/________ ______/ /_  / __ \\__  ______ ___  ____ \n";
        report += " / /   / ___/ __ `/ ___/ __ \\/ / / / / / / __ `__ \\/ __ \\\n";
        report += "/ /___/ /  / /_/ (__  ) / / / /_/ / /_/ / / / / / / /_/ /\n";
        report += "\\____/_/   \\__,_/____/_/ /_/_____/\\__,_/_/ /_/ /_/ .___/ \n";
        report += "                                                /_/      \n";

        report += "SatDump ran into an error and had to terminate.\n";
        report += "You can find a backtrace below to share with the\n";
        report += "developers for debugging purposes.\n";
        report += "\n=== START BACKTRACE ===\n\n";

        report += satdump::svformat("Crash report (signal %d: %s)\n", sig, strsignal(sig));

        backtrace_full(bt_state, 0, bt_callback, bt_error, &report);
        fprintf(stderr, "\n%s\n", report.c_str());

        std::ofstream("satdump_crash_" + std::to_string(time(NULL)) + ".txt").write((char *)report.c_str(), report.size());

        signal(sig, SIG_DFL);
        exit(1);
    }
#endif

    void initCrashDump()
    {
#ifdef SATDUMP_CRASHDUMP
        bt_state = backtrace_create_state(NULL, 1, bt_error, nullptr);

        struct sigaction sa{};
        sa.sa_handler = crash_handler;
        sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
        sigaction(SIGFPE, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGBUS, &sa, nullptr);
#endif
    }
} // namespace satdump
