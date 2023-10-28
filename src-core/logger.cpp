#define SATDUMP_DLL_EXPORT 1
#include "logger.h"
#include <iostream>
#include <ctime>
#include <algorithm>
#include <filesystem>
#ifdef __ANDROID__
#include <android/log.h>
static char ag_LogTag[] = "SatDump";
#endif
#if defined(_WIN32)
#include <windows.h>
#include <wincon.h>
#endif

#include "init.h"

// Logger and sinks. We got a console sink and file sink
#ifdef __ANDROID__
std::shared_ptr<slog::AndroidSink> console_sink;
#elif defined(_WIN32)
std::shared_ptr<slog::WinOutSink> console_sink;
#else
std::shared_ptr<slog::StdOutSink> console_sink;
#endif
std::shared_ptr<slog::FileSink> file_sink;
SATDUMP_DLL std::shared_ptr<slog::Logger> logger;

namespace slog
{
    const std::string log_schar[] = {"T", "D", "I", "W", "E", "C"};
    const std::string colors[] = {"\033[37m",
                                  "\033[36m",
                                  "\033[32m",
                                  "\033[33m\033[1m",
                                  "\033[31m\033[1m",
                                  "\033[1m\033[41m"};

    template <typename... T>
    std::string vformat(const char *fmt, T &&...args)
    {
        // Allocate a buffer on the stack that's big enough for us almost
        // all the time.
        size_t size = 1024;
        std::vector<char> buf;
        buf.resize(size);

        // Try to vsnprintf into our buffer.
        size_t needed = snprintf((char *)&buf[0], size, fmt, args...);
        // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
        // buffer.  On Linux & OSX, it returns the length it would have needed.

        if (needed <= size)
        {
            // It fit fine the first time, we're done.
            return std::string(&buf[0]);
        }
        else
        {
            // vsnprintf reported that it wanted to write more characters
            // than we allotted.  So do a malloc of the right size and try again.
            // This doesn't happen very often if we chose our initial size
            // well.
            size = needed;
            buf.resize(size);
            needed = snprintf((char *)&buf[0], size, fmt, args...);
            return std::string(&buf[0]);
        }
    }

    std::string LoggerSink::format_log(LogMsg m, bool color, int *cpos)
    {
        time_t ct = time(0);
        std::tm *tmr = gmtime(&ct);

        std::string timestamp =
            (tmr->tm_hour < 10 ? "0" : "") + std::to_string(tmr->tm_hour) + ":" +       // Hour
            (tmr->tm_min < 10 ? "0" : "") + std::to_string(tmr->tm_min) + ":" +         // Min
            (tmr->tm_sec < 10 ? "0" : "") + std::to_string(tmr->tm_sec) + " - " +       // Sec
            (tmr->tm_mday < 10 ? "0" : "") + std::to_string(tmr->tm_mday) + "/" +       // Day
            (tmr->tm_mon + 1 < 10 ? "0" : "") + std::to_string(tmr->tm_mon + 1) + "/" + // Mon
            (tmr->tm_year < 10 ? "0" : "") + std::to_string(tmr->tm_year + 1900);       // Year

        if (cpos != nullptr)
            *cpos = timestamp.size() + 3;

        if (color)
            return vformat("[%s] %s(%s) %s\033[m\n",
                           timestamp.c_str(),
                           colors[m.lvl].c_str(), log_schar[m.lvl].c_str(), m.str.c_str());
        else
            return vformat("[%s] (%s) %s\n",
                           timestamp.c_str(),
                           log_schar[m.lvl].c_str(), m.str.c_str());
    }

    void StdOutSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, true);
            fwrite(s.c_str(), sizeof(char), s.size(), stderr);
        }
    }

#if defined(_WIN32)
    const int colors_win[] = {FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                              FOREGROUND_GREEN | FOREGROUND_BLUE,
                              FOREGROUND_GREEN,
                              FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
                              FOREGROUND_RED | FOREGROUND_INTENSITY,
                              BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY};

    int win_set_foreground_color(HANDLE &out_handle_, int attribs)
    {
        CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
        if (!::GetConsoleScreenBufferInfo(out_handle_, &orig_buffer_info))
        {
            // just return white if failed getting console info
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }

        // change only the foreground bits (lowest 4 bits)
        auto new_attribs = static_cast<int>(attribs) | (orig_buffer_info.wAttributes & 0xfff0);
        auto ignored = ::SetConsoleTextAttribute(out_handle_, static_cast<WORD>(new_attribs));
        (void)(ignored);
        return orig_buffer_info.wAttributes; // return orig attribs
    }

    void WinOutSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            int color_pos = 0;
            std::string s = format_log(log, false, &color_pos);
            std::string s1 = s.substr(0, color_pos);
            std::string s2 = s.substr(color_pos, s.size() - color_pos - 1);
            std::string s3 = s.substr(s.size() - 1, 1);
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            fwrite(s1.c_str(), sizeof(char), s1.size(), stderr);
            int rst = win_set_foreground_color(hConsole, colors_win[log.lvl]);
            fwrite(s2.c_str(), sizeof(char), s2.size(), stderr);
            SetConsoleTextAttribute(hConsole, rst);
            fwrite(s3.c_str(), sizeof(char), s3.size(), stderr);
        }
    }
#endif

    FileSink::FileSink(std::string path)
    {
        int suffix = 0;
        std::string log_path = path + ".log";
        while(std::filesystem::exists(log_path))
        {
            suffix++;
            log_path = path + "-" + std::to_string(suffix) + ".log";
        }

        outf = std::ofstream(log_path);
    }
    FileSink::~FileSink()
    {
        outf.close();
    }

    void FileSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, false);
            outf.write(s.c_str(), s.size());
            outf.flush(); // Too annoying when there's a segault not letting it flush
        }
    }

    void Logger::log(LogLevel lvl, std::string v)
    {
        sink_mtx.lock();
        LogMsg m;
        m.str = v;
        m.lvl = lvl;
        if (m.lvl >= logger_lvl)
            for (auto &l : sinks)
                l->receive(m);
        sink_mtx.unlock();
    }

    void Logger::logf(LogLevel lvl, std::string fmt, va_list args)
    {
        std::string output;
        output.resize(1024);

        va_list argscopy;
        va_copy(argscopy, args);

        size_t size = vsnprintf((char *)output.c_str(), output.size(), fmt.c_str(), args);

        if (output.size() <= size)
        {
            log(lvl, output);
        }
        else
        {
            output.resize(size + 1);
            size = vsnprintf((char *)output.c_str(), output.size(), fmt.c_str(), argscopy);
            log(lvl, output);
        }
    }

#ifdef __ANDROID__
    const android_LogPriority log_lvls_a[] = {
        ANDROID_LOG_VERBOSE,
        ANDROID_LOG_DEBUG,
        ANDROID_LOG_INFO,
        ANDROID_LOG_WARN,
        ANDROID_LOG_ERROR,
        ANDROID_LOG_FATAL,
    };

    void AndroidSink::receive(LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            std::string s = format_log(log, false);
            __android_log_print(log_lvls_a[log.lvl], ag_LogTag, "%s", log.str.c_str());
        }
    }
#endif

    void Logger::set_level(LogLevel lvl)
    {
        logger_lvl = lvl;
    }

    void Logger::add_sink(std::shared_ptr<LoggerSink> sink)
    {
        sink_mtx.lock();
        sinks.push_back(sink);
        sink_mtx.unlock();
    }

    void Logger::del_sink(std::shared_ptr<LoggerSink> sink)
    {
        sink_mtx.lock();
        auto it = std::find_if(sinks.begin(), sinks.end(), [&sink](std::shared_ptr<LoggerSink>& c)
            { return sink.get() == c.get(); });
        if (it != sinks.end())
            sinks.erase(it);
        sink_mtx.unlock();
    }
}

void initLogger()
{
    try
    {
// Initialize everything
#ifdef __ANDROID__
        console_sink = std::make_shared<slog::AndroidSink>();
#elif defined(_WIN32)
        console_sink = std::make_shared<slog::WinOutSink>();
#else
        console_sink = std::make_shared<slog::StdOutSink>();
#endif

        logger = std::make_shared<slog::Logger>();
        logger->add_sink(console_sink);

        // Use a custom, nicer log pattern. No color in the file
        // console_sink->set_pattern("[%D - %T] %^(%L) %v%$");

        // Default log level
        console_sink->set_level(slog::LOG_TRACE);
        logger->set_level(slog::LOG_TRACE);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

void initFileSink()
{
    try
    {
        //Make sure the user folder is created
        if (!std::filesystem::exists(std::filesystem::path(satdump::user_path + "/")))
            std::filesystem::create_directories(std::filesystem::path(satdump::user_path + "/"));

        //Clear old logs
        char timebuffer[16];
        struct tm *timeinfo;
        time_t current_time = time(NULL);
        timeinfo = localtime(&current_time);
        strftime(timebuffer, sizeof(timebuffer), "%Y%m%d", timeinfo);
        int delete_before = atoi(timebuffer) - 2;
        for (const auto& user_file : std::filesystem::directory_iterator(satdump::user_path + "/"))
        {
            std::string file_name = user_file.path().filename().string();
            if (file_name.size() < 26)
                continue;
            int log_date = atoi(file_name.substr(8, 8).c_str());
            if (log_date != 0 && log_date < delete_before)
                std::filesystem::remove(user_file);
        }

        //Create new log
        strftime(timebuffer, sizeof(timebuffer), "%Y%m%dT%H%M%S", timeinfo);
        file_sink = std::make_shared<slog::FileSink>(satdump::user_path + "/satdump-" + std::string(timebuffer));
        logger->add_sink(file_sink);
        // file_sink->set_pattern("[%D - %T] (%L) %v");
        file_sink->set_level(slog::LOG_TRACE);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

void setConsoleLevel(slog::LogLevel level)
{
    // Just change our log level
    console_sink->set_level(level);
}
