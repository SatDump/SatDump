#include "../ndsp_test_helpers.h"
#include "dsp/io/file_sink.h"
#include "dsp/io/file_source.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    // Runs a file source until its terminator and returns what it output
    template <typename T>
    ndsp_test::OutputResult<T> runSource(std::shared_ptr<FileSourceBlock<T>> b)
    {
        ndsp_test::OutputResult<T> r;
        ndsp_test::Collector<T> col(b->get_output(0, 4).fifo);
        b->start();
        bool got = col.waitForTerminator();
        REQUIRE(ndsp_test::stopBlock(b));
        col.finish();
        REQUIRE(got);
        r.data = col.data();
        r.sizes = col.sizes();
        r.terminators = col.terminators();
        r.propagating_terminators = col.propagatingTerminators();
        return r;
    }
} // namespace

///////////////////////////////////////////////////////////////////////////////
// File source
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp FileSourceBlock reads the file in buffer_size chunks", "[dsp][io][file_source]")
{
    ndsp_test::TempFile tf;
    std::vector<float> data(1000);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (float)i * 0.5f;
    ndsp_test::writeFile(tf.path(), data);

    auto b = std::make_shared<FileSourceBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"file", tf.path()}, {"buffer_size", 256}}) == Block::RES_OK);
    auto r = runSource(b);

    // The file content comes first, in order (what follows is covered by the bug test below)
    REQUIRE(r.data.size() >= data.size());
    CHECK(std::vector<float>(r.data.begin(), r.data.begin() + data.size()) == data);
    for (auto s : r.sizes)
        CHECK(s == 256);
    CHECK(r.propagating_terminators == 1);
    CHECK(b->d_filesize == data.size() * sizeof(float));
    CHECK(b->is_async());
}

TEST_CASE("ndsp FileSourceBlock reads complex and integer files", "[dsp][io][file_source]")
{
    ndsp_test::TempFile tf;
    auto data = ndsp_test::randomComplex(512, 61);
    ndsp_test::writeFile(tf.path(), data);
    auto b = std::make_shared<FileSourceBlock<complex_t>>();
    b->set_cfg("file", tf.path());
    b->set_cfg("buffer_size", 128);
    auto r = runSource(b);
    REQUIRE(r.data.size() >= data.size());
    for (size_t i = 0; i < data.size(); i++)
        REQUIRE(ndsp_test::sameBits(r.data[i], data[i]));

    ndsp_test::TempFile tf2;
    std::vector<int16_t> s16 = {-32768, -1, 0, 1, 32767, 1234, -4321, 7};
    ndsp_test::writeFile(tf2.path(), s16);
    auto b2 = std::make_shared<FileSourceBlock<int16_t>>();
    b2->set_cfg("file", tf2.path());
    b2->set_cfg("buffer_size", 8);
    auto r2 = runSource(b2);
    REQUIRE(r2.data.size() >= s16.size());
    CHECK(std::vector<int16_t>(r2.data.begin(), r2.data.begin() + 8) == s16);
}

TEST_CASE("ndsp FileSourceBlock with a missing file only sends a terminator", "[dsp][io][file_source]")
{
    ndsp_test::TempFile tf; // Never created
    auto b = std::make_shared<FileSourceBlock<uint8_t>>();
    b->set_cfg("file", tf.path());
    auto r = runSource(b);
    CHECK(r.data.empty());
    CHECK(r.propagating_terminators == 1);
}

TEST_CASE("ndsp FileSourceBlock can be stopped early", "[dsp][io][file_source]")
{
    ndsp_test::TempFile tf;
    ndsp_test::writeFile(tf.path(), std::vector<float>(1 << 20, 1.0f));
    auto b = std::make_shared<FileSourceBlock<float>>();
    b->set_cfg("file", tf.path());
    b->set_cfg("buffer_size", 1024);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();
    REQUIRE(col.waitFor(4096));
    REQUIRE(ndsp_test::stopBlock(b, true));
    REQUIRE(col.waitForTerminator());
    col.finish();
    CHECK(col.size() < (1u << 20));
    CHECK(b->d_progress > 0);
    CHECK(b->d_progress <= b->d_filesize);
}

TEST_CASE("ndsp FileSourceBlock id and config round-trip", "[dsp][io][file_source]")
{
    CHECK(FileSourceBlock<complex_t>().d_id == "file_source_c");
    CHECK(FileSourceBlock<float>().d_id == "file_source_f");
    CHECK(FileSourceBlock<int16_t>().d_id == "file_source_s");
    CHECK(FileSourceBlock<int8_t>().d_id == "file_source_h");
    CHECK(FileSourceBlock<uint8_t>().d_id == "file_source_b");

    FileSourceBlock<float> b;
    Block &bb = b;
    CHECK(b.get_inputs().empty());
    CHECK(b.get_cfg("buffer_size") == 8192);
    REQUIRE(bb.set_cfg(nlohmann::json{{"file", "/some/file.f32"}, {"buffer_size", 100}}) == Block::RES_OK);
    nlohmann::json all = bb.get_cfg();
    CHECK(all["file"] == "/some/file.f32");
    CHECK(all["buffer_size"] == 100);
    auto list = b.get_cfg_list();
    CHECK(list["file"]["disable"] == false);
    CHECK(list["buffer_size"]["disable"] == false);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

// Bug : file_source.cpp:32, the output size is always d_buffer_size instead of
// the number of samples actually read (file_reader.gcount()), so the last
// partial buffer is padded with stale data, and as EOF is only detected after
// a short read, a file whose size is a multiple of buffer_size gets a whole
// extra buffer of stale data.
TEST_CASE("ndsp FileSourceBlock outputs exactly the samples of a partial last buffer", "[dsp][io][file_source][!shouldfail]")
{
    ndsp_test::TempFile tf;
    ndsp_test::writeFile(tf.path(), std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    auto b = std::make_shared<FileSourceBlock<float>>();
    b->set_cfg("file", tf.path());
    b->set_cfg("buffer_size", 4);
    auto r = runSource(b);
    CHECK(r.data.size() == 10);
}

TEST_CASE("ndsp FileSourceBlock outputs exactly the samples of a buffer-aligned file", "[dsp][io][file_source][!shouldfail]")
{
    ndsp_test::TempFile tf;
    ndsp_test::writeFile(tf.path(), std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8});
    auto b = std::make_shared<FileSourceBlock<float>>();
    b->set_cfg("file", tf.path());
    b->set_cfg("buffer_size", 4);
    auto r = runSource(b);
    CHECK(r.data.size() == 8);
}

///////////////////////////////////////////////////////////////////////////////
// File sink
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp FileSinkBlock writes raw samples", "[dsp][io][file_sink]")
{
    ndsp_test::TempFile tf;
    auto data = ndsp_test::randomComplex(3000, 62);
    auto b = std::make_shared<FileSinkBlock<complex_t>>();
    REQUIRE(ndsp_test::configure(*b, {{"file", tf.path()}}) == Block::RES_OK);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, data, 1000);
    REQUIRE(r.completed);
    CHECK(r.outputs.empty());
    auto back = ndsp_test::readFile<complex_t>(tf.path());
    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); i++)
        REQUIRE(ndsp_test::sameBits(back[i], data[i]));
}

TEST_CASE("ndsp FileSinkBlock truncates on restart and handles non-propagating terminators", "[dsp][io][file_sink]")
{
    ndsp_test::TempFile tf;
    auto b = std::make_shared<FileSinkBlock<int8_t>>();
    b->set_cfg("file", tf.path());
    auto r1 = ndsp_test::runSimple<int8_t, int8_t>(b, {1, 2, 3, 4, 5}, 2);
    REQUIRE(r1.completed);
    CHECK(ndsp_test::readFile<int8_t>(tf.path()) == std::vector<int8_t>{1, 2, 3, 4, 5});

    auto r2 = ndsp_test::runSimple<int8_t, int8_t>(b, {-9, -8}, 2, 2);
    REQUIRE(r2.completed);
    CHECK(ndsp_test::readFile<int8_t>(tf.path()) == std::vector<int8_t>{-9, -8});
}

TEST_CASE("ndsp FileSinkBlock throws on start when the file cannot be opened", "[dsp][io][file_sink]")
{
    FileSinkBlock<float> b;
    b.set_cfg("file", (std::filesystem::temp_directory_path() / "satdump_ndsp_no_such_dir" / "x" / "y.f32").string());
    CHECK_THROWS_AS(b.start(), satdump::satdump_exception_t);
}

TEST_CASE("ndsp FileSinkBlock id and config round-trip", "[dsp][io][file_sink]")
{
    CHECK(FileSinkBlock<complex_t>().d_id == "file_sink_c");
    CHECK(FileSinkBlock<float>().d_id == "file_sink_f");
    CHECK(FileSinkBlock<int16_t>().d_id == "file_sink_s");
    CHECK(FileSinkBlock<int8_t>().d_id == "file_sink_h");
    CHECK(FileSinkBlock<uint8_t>().d_id == "file_sink_b");

    FileSinkBlock<float> b;
    CHECK(b.get_outputs().empty());
    CHECK(b.get_inputs().size() == 1);
    CHECK_FALSE(b.is_async());
    b.set_cfg("file", "/x/y");
    CHECK(b.get_cfg("file") == "/x/y");
    CHECK(b.get_cfg_list()["file"]["type"] == "string");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

TEST_CASE("ndsp FileSourceBlock to FileSinkBlock copies a file", "[dsp][io][file_source][file_sink]")
{
    ndsp_test::TempFile src, dst;
    // Multiple of the buffer size, and the extra stale buffer (see bug above)
    // is avoided by checking the prefix only
    std::vector<uint8_t> data(4096);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (uint8_t)(i * 7);
    ndsp_test::writeFile(src.path(), data);

    auto source = std::make_shared<FileSourceBlock<uint8_t>>();
    auto sink = std::make_shared<FileSinkBlock<uint8_t>>();
    source->set_cfg("file", src.path());
    source->set_cfg("buffer_size", 512);
    sink->set_cfg("file", dst.path());
    sink->link(source.get(), 0, 0, 4);
    sink->start();
    source->start();
    REQUIRE(ndsp_test::stopBlock(sink));
    REQUIRE(ndsp_test::stopBlock(source));
    auto back = ndsp_test::readFile<uint8_t>(dst.path());
    REQUIRE(back.size() >= data.size());
    CHECK(std::vector<uint8_t>(back.begin(), back.begin() + data.size()) == data);
}
