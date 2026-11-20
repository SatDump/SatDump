#include "common/utils.h" // timegm on MSVC
#include "helpers/temp_files.h"
#include "logger.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <mutex>
#include <regex>
#include <string>
#include <vector>

// These tests use their own slog::Logger instances and never touch the global logger or its
// sinks. Note that Logger::add_sink replays the process-wide init buffer (messages logged before
// completeLoggerInit(), which the test main never calls), so recording sinks are cleared after
// being added.

namespace
{
    class RecordingSink : public slog::LoggerSink
    {
    public:
        std::mutex mtx;
        std::vector<slog::LogMsg> msgs;

        void receive(slog::LogMsg log)
        {
            if (log.lvl >= sink_lvl)
            {
                std::lock_guard<std::mutex> l(mtx);
                msgs.push_back(log);
            }
        }

        void clear()
        {
            std::lock_guard<std::mutex> l(mtx);
            msgs.clear();
        }

        // Exposes the protected formatter
        std::string format(slog::LogMsg m, bool color, int *cpos = nullptr) { return format_log(m, color, cpos); }
    };

    struct LoggerWithSink
    {
        slog::Logger logger;
        std::shared_ptr<RecordingSink> sink = std::make_shared<RecordingSink>();
        LoggerWithSink()
        {
            logger.add_sink(sink);
            sink->clear();
        }
    };

    using test_helpers::readFile;

    const std::string TIMESTAMP_RE = "\\[\\d\\d:\\d\\d:\\d\\d - \\d\\d/\\d\\d/\\d{4}\\]";
} // namespace

TEST_CASE("Logger level defaults to trace and can be changed", "[logger]")
{
    slog::Logger l;
    CHECK(l.get_level() == slog::LOG_TRACE);
    l.set_level(slog::LOG_ERROR);
    CHECK(l.get_level() == slog::LOG_ERROR);
}

TEST_CASE("Logger delivers each level with the right tag", "[logger]")
{
    LoggerWithSink t;
    t.logger.trace("t");
    t.logger.debug("d");
    t.logger.info("i");
    t.logger.notice("n");
    t.logger.warn("w");
    t.logger.error("e");
    t.logger.critical("c");

    REQUIRE(t.sink->msgs.size() == 7);
    const slog::LogLevel expected[] = {slog::LOG_TRACE, slog::LOG_DEBUG, slog::LOG_INFO, slog::LOG_NOTICE, slog::LOG_WARN, slog::LOG_ERROR, slog::LOG_CRIT};
    for (int i = 0; i < 7; i++)
        CHECK(t.sink->msgs[i].lvl == expected[i]);
    CHECK(std::string(t.sink->msgs[0].str.c_str()) == "t");
    CHECK(std::string(t.sink->msgs[6].str.c_str()) == "c");
}

TEST_CASE("Logger drops messages below its level", "[logger]")
{
    LoggerWithSink t;
    t.logger.set_level(slog::LOG_WARN);
    t.logger.debug("hidden");
    t.logger.info("hidden");
    t.logger.warn("shown warn");
    t.logger.critical("shown crit");

    REQUIRE(t.sink->msgs.size() == 2);
    CHECK(t.sink->msgs[0].lvl == slog::LOG_WARN);
    CHECK(t.sink->msgs[1].lvl == slog::LOG_CRIT);

    t.sink->clear();
    t.logger.set_level(slog::LOG_OFF);
    t.logger.critical("nothing gets through");
    CHECK(t.sink->msgs.empty());
}

TEST_CASE("Logger printf-style formatting", "[logger]")
{
    LoggerWithSink t;
    t.logger.info("%d-%s-%.2f-%c-%%", 42, "abc", 3.14159, 'z');
    t.logger.warn("no arguments");
    REQUIRE(t.sink->msgs.size() == 2);
    CHECK(std::string(t.sink->msgs[0].str.c_str()) == "42-abc-3.14-z-%");
    CHECK(std::string(t.sink->msgs[1].str.c_str()) == "no arguments");
}

TEST_CASE("Logger formats messages longer than its 1024 byte buffer", "[logger]")
{
    LoggerWithSink t;
    std::string big(3000, 'q');
    t.logger.error("<%s>", big.c_str());
    REQUIRE(t.sink->msgs.size() == 1);
    std::string got = t.sink->msgs[0].str.c_str();
    CHECK(got.size() == big.size() + 2);
    CHECK((got == "<" + big + ">"));
}

// Bug: Logger::logf (logger.cpp:202) checks `size <= output.size()`, but vsnprintf needs room
// for the terminating NUL, so a message of exactly 1024 characters loses its last character.
TEST_CASE("Logger keeps every character of a 1024 character message", "[logger][!shouldfail]")
{
    LoggerWithSink t;
    std::string exact(1024, 'x');
    t.logger.info("%s", exact.c_str());
    REQUIRE(t.sink->msgs.size() == 1);
    std::string got = t.sink->msgs[0].str.c_str();
    CHECK(got.size() == exact.size());
}

// Bug: Logger::logf (logger.cpp:192-212) formats into a 1024 byte std::string and never shrinks it,
// so sinks receive LogMsg::str with the text followed by NUL padding (and one trailing NUL for long
// messages). Sinks that use log.str directly, rather than through c_str(), get the padding.
TEST_CASE("Logger formatted messages have no trailing NUL padding", "[logger][!shouldfail]")
{
    LoggerWithSink t;
    t.logger.info("value=%d", 5);
    REQUIRE(t.sink->msgs.size() == 1);
    CHECK(t.sink->msgs[0].str == "value=5");
    CHECK(t.sink->msgs[0].str.size() == 7);
}

TEST_CASE("Logger int overloads log the decimal value", "[logger]")
{
    LoggerWithSink t;
    t.logger.trace(-12);
    t.logger.info(0);
    t.logger.warn(123456);
    t.logger.error(7);
    t.logger.critical(8);
    REQUIRE(t.sink->msgs.size() == 5);
    CHECK(t.sink->msgs[0].str == "-12");
    CHECK(t.sink->msgs[0].lvl == slog::LOG_TRACE);
    CHECK(t.sink->msgs[1].str == "0");
    CHECK(t.sink->msgs[2].str == "123456");
    CHECK(t.sink->msgs[2].lvl == slog::LOG_WARN);
    CHECK(t.sink->msgs[3].lvl == slog::LOG_ERROR);
    CHECK(t.sink->msgs[4].lvl == slog::LOG_CRIT);
}

// Bug: logger.h:117 `void notice(int v) { log(LOG_INFO, v); }` logs at INFO instead of NOTICE.
TEST_CASE("Logger notice int overload logs at notice level", "[logger][!shouldfail]")
{
    LoggerWithSink t;
    t.logger.notice(3);
    REQUIRE(t.sink->msgs.size() == 1);
    CHECK(t.sink->msgs[0].lvl == slog::LOG_NOTICE);
}

TEST_CASE("Logger log with a string is passed through unformatted", "[logger]")
{
    LoggerWithSink t;
    t.logger.log(slog::LOG_NOTICE, std::string("100% literal %s"));
    REQUIRE(t.sink->msgs.size() == 1);
    CHECK(t.sink->msgs[0].str == "100% literal %s");
    CHECK(t.sink->msgs[0].lvl == slog::LOG_NOTICE);
}

TEST_CASE("Logger fans out to every sink and del_sink removes one", "[logger]")
{
    slog::Logger l;
    auto a = std::make_shared<RecordingSink>();
    auto b = std::make_shared<RecordingSink>();
    l.add_sink(a);
    l.add_sink(b);
    a->clear();
    b->clear();

    l.info("both");
    CHECK(a->msgs.size() == 1);
    CHECK(b->msgs.size() == 1);

    l.del_sink(a);
    l.info("only b");
    CHECK(a->msgs.size() == 1);
    CHECK(b->msgs.size() == 2);

    // Removing a sink that is not attached is a no-op
    auto stranger = std::make_shared<RecordingSink>();
    CHECK_NOTHROW(l.del_sink(stranger));
    CHECK_NOTHROW(l.del_sink(a));
    l.info("still b");
    CHECK(b->msgs.size() == 3);
}

TEST_CASE("LoggerSink format_log plain layout", "[logger]")
{
    RecordingSink s;
    std::string out = s.format({"hello world", slog::LOG_INFO}, false);
    CHECK(std::regex_match(out, std::regex(TIMESTAMP_RE + " \\(I\\) hello world\n")));

    const char tags[] = {'T', 'D', 'I', 'N', 'W', 'E', 'C'};
    for (int lvl = slog::LOG_TRACE; lvl <= slog::LOG_CRIT; lvl++)
    {
        std::string o = s.format({"m", (slog::LogLevel)lvl}, false);
        CHECK(o.substr(o.size() - 6) == std::string("(") + tags[lvl] + ") m\n");
    }
}

TEST_CASE("LoggerSink format_log timestamp is the current UTC time", "[logger]")
{
    RecordingSink s;
    time_t before = time(nullptr);
    std::string out = s.format({"x", slog::LOG_INFO}, false);
    time_t after = time(nullptr);

    std::tm tm{};
    REQUIRE(sscanf(out.c_str(), "[%d:%d:%d - %d/%d/%d]", &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tm.tm_mday, &tm.tm_mon, &tm.tm_year) == 6);
    tm.tm_mon -= 1;
    tm.tm_year -= 1900;
    time_t parsed = timegm(&tm);
    CHECK(parsed >= before);
    CHECK(parsed <= after);
}

TEST_CASE("LoggerSink format_log colored layout and color position", "[logger]")
{
    RecordingSink s;
    int cpos = -1;
    std::string out = s.format({"careful", slog::LOG_WARN}, true, &cpos);
    CHECK(std::regex_match(out, std::regex(TIMESTAMP_RE + " \033\\[33m\033\\[1m\\(W\\) careful\033\\[m\n")));

    // cpos marks where the level tag starts in the uncolored layout
    std::string plain = s.format({"careful", slog::LOG_WARN}, false, &cpos);
    CHECK(cpos == 24);
    CHECK(plain.substr(cpos) == "(W) careful\n");
}

// Bug: slog::vformat (logger.cpp:55,67) retries with a buffer of exactly `needed` bytes, leaving no
// room for the NUL, so any formatted line of 1024 characters or more loses its final '\n'.
TEST_CASE("LoggerSink format_log keeps the newline of long lines", "[logger][!shouldfail]")
{
    RecordingSink s;
    std::string big(1500, 'b');
    std::string out = s.format({big, slog::LOG_INFO}, false);
    CHECK(out.size() == 24 + 4 + big.size() + 1);
    CHECK(out.back() == '\n');
}

TEST_CASE("FileSink writes formatted lines and respects its level", "[logger]")
{
    test_helpers::TempDir tmp("logger");
    auto base = tmp.path / "log";
    std::filesystem::path file = base.string() + ".log";
    {
        auto sink = std::make_shared<slog::FileSink>(base.string());
        REQUIRE(std::filesystem::exists(file));
        sink->receive({"first line", slog::LOG_INFO});
        sink->set_level(slog::LOG_ERROR);
        sink->receive({"filtered", slog::LOG_WARN});
        sink->receive({"second line", slog::LOG_ERROR});
    }

    std::string content = readFile(file);
    CHECK(std::regex_match(content, std::regex(TIMESTAMP_RE + " \\(I\\) first line\n" + TIMESTAMP_RE + " \\(E\\) second line\n")));
}

TEST_CASE("FileSink never overwrites an existing log", "[logger]")
{
    test_helpers::TempDir tmp("logger");
    auto base = (tmp.path / "rotate").string();
    {
        slog::FileSink a(base);
        slog::FileSink b(base);
        slog::FileSink c(base);
        a.receive({"from a", slog::LOG_INFO});
        b.receive({"from b", slog::LOG_INFO});
        c.receive({"from c", slog::LOG_INFO});
    }
    CHECK(readFile(base + ".log").find("from a") != std::string::npos);
    CHECK(readFile(base + "-1.log").find("from b") != std::string::npos);
    CHECK(readFile(base + "-2.log").find("from c") != std::string::npos);
}

TEST_CASE("FileSink attached to a Logger receives messages", "[logger]")
{
    test_helpers::TempDir tmp("logger");
    auto base = (tmp.path / "attached").string();
    {
        slog::Logger l;
        auto sink = std::make_shared<slog::FileSink>(base);
        // Ignore anything replayed from the init buffer
        sink->set_level(slog::LOG_CRIT);
        l.add_sink(sink);
        l.critical("attached %d", 99);
        l.error("below sink level");
    }
    std::string content = readFile(base + ".log");
    CHECK(content.find("(C) attached 99\n") != std::string::npos);
    CHECK(content.find("below sink level") == std::string::npos);
}
