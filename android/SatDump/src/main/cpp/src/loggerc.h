//
// Created by sf on 6/28/17.
//

#ifndef XPLATDEV_LOGGER_H
#define XPLATDEV_LOGGER_H

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "NativeApp"
#else
#include <iostream>
#endif
#include <sstream>

namespace _Logger {
    enum Severity {
        LOG_DEBUG = 0,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_FATAL
    };

    class Logger {
    private:
        std::stringstream logbuf;
        Severity severity;

        std::string sevStr()
        {
            switch(severity){
                case LOG_DEBUG:
                    return "[DEBUG] ";
                case LOG_INFO:
                    return "[INFO] ";
                case LOG_WARN:
                    return "[WARN] ";
                case LOG_ERROR:
                    return "[ERROR] ";
                case LOG_FATAL:
                    return "[FATAL] ";
            }
        }

#ifdef __ANDROID__
        int getPrio()
        {
            switch(severity)
            {
                case LOG_DEBUG:
                    return ANDROID_LOG_DEBUG;
                case LOG_INFO:
                    return ANDROID_LOG_INFO;
                case LOG_WARN:
                    return ANDROID_LOG_WARN;
                case LOG_ERROR:
                    return ANDROID_LOG_ERROR;
                case LOG_FATAL:
                    return ANDROID_LOG_FATAL;
                default:
                    return ANDROID_LOG_DEFAULT;
            }
        }
#else
        static std::ostream& getStream(Severity s)
        {
            if (s < LOG_ERROR)
            {
                return std::cout;
            }
            else
            {
                return std::cerr;
            }
        }
#endif

    public:
        static Severity& minSeverity()
        {
            static Severity minSeverity = LOG_DEBUG;
            return minSeverity;
        }

        Logger(Severity s) : severity(s)
        {

        }
        ~Logger()
        {
#ifdef __ANDROID__
            __android_log_print(getPrio(), LOG_TAG, "%s", logbuf.str().c_str());
#else
            getStream(severity) << sevStr() << logbuf.str() << std::endl;
#endif
        }
        std::stringstream& log()
        {
            return logbuf;
        }

    };
};

#define Log(LOGLEVEL) _Logger::Logger(_Logger::LOGLEVEL).log()

#endif //XPLATDEV_LOGGER_H
