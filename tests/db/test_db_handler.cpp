#include "core/exception.h"
#include "db/db_handler.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <random>

// Uses in-memory databases or files under the temp directory; never the global satdump::db.

namespace
{
    std::filesystem::path uniqueTempPath(const std::string &name)
    {
        std::mt19937 rng(std::random_device{}());
        return std::filesystem::temp_directory_path() / ("satdump_test_db_" + std::to_string(rng()) + "_" + name);
    }

    struct TempDir
    {
        std::filesystem::path path;
        TempDir(const std::string &name) : path(uniqueTempPath(name)) {}
        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    std::shared_ptr<satdump::DBHandler> memoryDb()
    {
        auto db = std::make_shared<satdump::DBHandler>(":memory:");
        db->init();
        return db;
    }

    class RecordingSubhandler : public satdump::DBHandlerBase
    {
    public:
        int inits = 0;
        bool table_existed = false;
        RecordingSubhandler(std::shared_ptr<satdump::DBHandler> h) : satdump::DBHandlerBase(h) {}
        void init()
        {
            inits++;
            // The base tables are created before subhandlers run
            table_existed = h->get_table_size("meta") == 0;
        }
    };
} // namespace

TEST_CASE("DBHandler init creates the meta and user tables", "[db][db_handler]")
{
    satdump::DBHandler db(":memory:");
    CHECK(db.db != nullptr);
    // Before init the tables do not exist
    CHECK(db.get_table_size("meta") == -1);
    db.init();
    CHECK(db.get_table_size("meta") == 0);
    CHECK(db.get_table_size("user") == 0);
    // init is idempotent
    CHECK_NOTHROW(db.init());
}

TEST_CASE("DBHandler init runs every subhandler", "[db][db_handler]")
{
    auto db = std::make_shared<satdump::DBHandler>(":memory:");
    auto a = std::make_shared<RecordingSubhandler>(db);
    auto b = std::make_shared<RecordingSubhandler>(db);
    db->subhandlers.push_back(a);
    db->subhandlers.push_back(b);
    db->init();
    CHECK(a->inits == 1);
    CHECK(b->inits == 1);
    CHECK(a->table_existed);
    // Break the reference cycle (subhandlers hold the DBHandler)
    db->subhandlers.clear();
}

TEST_CASE("DBHandler meta values round-trip and update", "[db][db_handler]")
{
    auto db = memoryDb();
    CHECK(db->get_meta("missing") == "");
    CHECK(db->get_meta("missing", "fallback") == "fallback");

    db->set_meta("key", "value one");
    CHECK(db->get_meta("key") == "value one");
    CHECK(db->get_table_size("meta") == 1);

    // Setting again updates in place
    db->set_meta("key", "value two");
    CHECK(db->get_meta("key", "fallback") == "value two");
    CHECK(db->get_table_size("meta") == 1);

    db->set_meta("other", "");
    CHECK(db->get_meta("other", "fallback") == "");
    CHECK(db->get_table_size("meta") == 2);
}

TEST_CASE("DBHandler escapes quotes in values", "[db][db_handler]")
{
    auto db = memoryDb();
    db->set_meta("q", "it's a 'quoted' value; DROP TABLE meta; --");
    CHECK(db->get_meta("q") == "it's a 'quoted' value; DROP TABLE meta; --");
    db->set_user("q", "O'Brien");
    CHECK(db->get_user("q") == "O'Brien");
    CHECK(db->get_table_size("meta") == 1);
}

// Bug: DBHandler::set_meta/get_meta/set_user/get_user (db_handler.cpp:54,66,80,92) escape the value
// but splice the id into the SQL unescaped, so an id containing a quote breaks the statement: the
// value is never stored and the getter returns the default.
TEST_CASE("DBHandler handles quotes in ids", "[db][db_handler][!shouldfail]")
{
    auto db = memoryDb();
    db->set_meta("it's", "v");
    CHECK(db->get_meta("it's", "default") == "v");
    db->set_user("user's", "w");
    CHECK(db->get_user("user's", "default") == "w");
}

TEST_CASE("DBHandler user and meta tables are independent", "[db][db_handler]")
{
    auto db = memoryDb();
    db->set_user("shared_id", "user value");
    db->set_meta("shared_id", "meta value");
    CHECK(db->get_user("shared_id") == "user value");
    CHECK(db->get_meta("shared_id") == "meta value");
    CHECK(db->get_user("only_meta", "none") == "none");
}

TEST_CASE("DBHandler user JSON values round-trip", "[db][db_handler]")
{
    auto db = memoryDb();
    nlohmann::json v = {{"name", "it's"}, {"list", {1, 2, 3}}, {"nested", {{"x", 1.5}}}};
    db->set_user_json("cfg", v);
    CHECK(db->get_user_json("cfg") == v);

    // Missing ids give the default, which is null unless specified
    CHECK(db->get_user_json("missing").is_null());
    CHECK(db->get_user_json("missing", {{"d", true}}) == nlohmann::json({{"d", true}}));

    // A stored value that is not JSON also gives the default
    db->set_user("not_json", "{ broken");
    CHECK(db->get_user_json("not_json", 5) == 5);
}

TEST_CASE("DBHandler get_table_size counts rows and reports missing tables", "[db][db_handler]")
{
    auto db = memoryDb();
    CHECK(db->get_table_size("no_such_table") == -1);
    for (int i = 0; i < 25; i++)
        db->set_user("k" + std::to_string(i), "v");
    CHECK(db->get_table_size("user") == 25);
}

TEST_CASE("DBHandler run_sql returns true on error", "[db][db_handler]")
{
    auto db = memoryDb();
    CHECK_FALSE(db->run_sql("CREATE TABLE t(a INT);"));
    CHECK_FALSE(db->run_sql("INSERT INTO t VALUES (1), (2);"));
    CHECK(db->get_table_size("t") == 2);
    CHECK(db->run_sql("THIS IS NOT SQL;"));
    CHECK(db->run_sql("INSERT INTO missing_table VALUES (1);"));
}

TEST_CASE("DBHandler transactions commit their writes", "[db][db_handler]")
{
    auto db = memoryDb();
    db->tr_begin();
    for (int i = 0; i < 100; i++)
        db->set_meta("m" + std::to_string(i), std::to_string(i * i));
    db->tr_end();
    CHECK(db->get_table_size("meta") == 100);
    CHECK(db->get_meta("m12") == "144");

    // An END without BEGIN only logs an error
    CHECK_NOTHROW(db->tr_end());
}

TEST_CASE("DBHandler file databases create their folder and persist", "[db][db_handler]")
{
    TempDir dir("persist");
    std::string path = (dir.path / "deep" / "nested" / "test.db").string();
    {
        satdump::DBHandler db(path);
        db.init();
        db.set_meta("persisted", "yes");
        db.set_user_json("j", {1, 2});
    }
    CHECK(std::filesystem::exists(path));
    {
        satdump::DBHandler db(path);
        db.init();
        CHECK(db.get_meta("persisted") == "yes");
        CHECK(db.get_user_json("j") == nlohmann::json({1, 2}));
    }
}

TEST_CASE("DBHandler throws when the database cannot be opened", "[db][db_handler]")
{
    TempDir dir("unopenable");
    std::filesystem::create_directories(dir.path);
    // A directory cannot be opened as a database file
    CHECK_THROWS_AS(satdump::DBHandler(dir.path.string()), satdump::satdump_exception_t);
}
