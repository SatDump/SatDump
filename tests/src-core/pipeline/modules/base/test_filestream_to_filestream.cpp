#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace pipeline_test;
namespace pl = satdump::pipeline;

namespace
{
    // Copies 8-byte chunks from input to output, XORing every byte with 0xFF
    class XorModule : public pl::base::FileStreamToFileStreamModule
    {
    public:
        int chunks = 0;
        nlohmann::json stats_mid;

        XorModule(std::string in, std::string hint, nlohmann::json params, std::string ext = ".bin", bool output = true) : FileStreamToFileStreamModule(in, hint, params)
        {
            fsfsm_file_ext = ext;
            fsfsm_enable_output = output;
        }

        void process()
        {
            uint8_t buf[8];
            while (should_run())
            {
                read_data(buf, 8);
                // Inputs used here are multiples of 8 bytes: stop on the read that hit EOF
                if (input_data_type == pl::DATA_FILE && !should_run())
                    break;
                if (input_data_type == pl::DATA_FILE && chunks == 0)
                    stats_mid = getModuleStats();
                for (auto &b : buf)
                    b ^= 0xFF;
                write_data(buf, 8);
                chunks++;
            }
            cleanup();
        }

        void drawUI(bool) {}
        std::string getIDM() { return "unit_xor"; }

        void publicInit() { init(); }
        nlohmann::json stats() { return getModuleStats(); }
    };

    std::string xorString(const std::string &s)
    {
        std::string out = s;
        for (auto &c : out)
            c = (char)((uint8_t)c ^ 0xFF);
        return out;
    }
} // namespace

TEST_CASE("FileStreamToFileStreamModule advertises file and stream types", "[pipeline][modules][filestream_to_filestream]")
{
    XorModule with_output("in", "hint", {});
    CHECK(with_output.getInputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});
    CHECK(with_output.getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE, pl::DATA_STREAM});

    XorModule without_output("in", "hint", {}, ".bin", false);
    CHECK(without_output.getOutputTypes() == std::vector<pl::ModuleDataType>{pl::DATA_FILE});
}

TEST_CASE("FileStreamToFileStreamModule::init opens <hint><ext> as the output file", "[pipeline][modules][filestream_to_filestream]")
{
    TempDir tmp("pipe_fsfs");
    std::string input = tmp.file("in.raw");
    writeFile(input, "ABCDEFGH12345678");

    XorModule m(input, tmp.file("result"), {}, ".xor");
    m.setInputType(pl::DATA_FILE);
    m.setOutputType(pl::DATA_FILE);
    m.publicInit();

    CHECK(m.getOutput() == tmp.file("result") + ".xor");
    CHECK(m.stats()["progress"] == 0.0);

    m.process();

    CHECK(m.stats_mid["progress"].get<double>() == Catch::Approx(50.0));
    CHECK(readFile(tmp.file("result.xor")) == xorString("ABCDEFGH12345678"));
}

TEST_CASE("FileStreamToFileStreamModule without output does not create a file", "[pipeline][modules][filestream_to_filestream]")
{
    TempDir tmp("pipe_fsfs");
    std::string input = tmp.file("in.raw");
    writeFile(input, "ABCDEFGH");

    XorModule m(input, tmp.file("result"), {}, ".bin", false);
    m.setInputType(pl::DATA_FILE);
    m.setOutputType(pl::DATA_FILE);
    m.publicInit();

    CHECK(m.getOutput() == "");
    m.process();
    CHECK_FALSE(std::filesystem::exists(tmp.file("result.bin")));
}

// Bug: filestream_to_filestream.cpp:53 : once the input is exhausted, the last read fails and
// data_in.tellg() returns -1, which is stored in the unsigned `progress`, so the final stats
// report an astronomically large percentage instead of 100%.
TEST_CASE("FileStreamToFileStreamModule reports 100% progress after processing a whole file", "[pipeline][modules][filestream_to_filestream][!shouldfail]")
{
    TempDir tmp("pipe_fsfs");
    std::string input = tmp.file("in.raw");
    writeFile(input, "ABCDEFGH12345678");

    XorModule m(input, tmp.file("result"), {});
    m.setInputType(pl::DATA_FILE);
    m.setOutputType(pl::DATA_FILE);
    m.publicInit();
    m.process();

    CHECK(m.stats()["progress"].get<double>() == Catch::Approx(100.0));
}

TEST_CASE("FileStreamToFileStreamModule streams FIFO input to FIFO output", "[pipeline][modules][filestream_to_filestream]")
{
    XorModule m("", "unused_hint", {});
    m.input_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000);
    m.output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000);
    m.setInputType(pl::DATA_STREAM);
    m.setOutputType(pl::DATA_STREAM);
    m.publicInit();
    m.input_active = true;

    // No file progress when streaming
    CHECK_FALSE(m.stats().contains("progress"));

    std::thread t([&m]() { m.process(); });

    std::string payload = "0123456789abcdef";
    m.input_fifo->write((uint8_t *)payload.data(), payload.size());

    bool got = test_helpers::waitUntil([&]() { return m.output_fifo->getReadable() >= 16; }, std::chrono::seconds(30));

    m.input_active = false;
    m.input_fifo->stopReader();
    m.input_fifo->stopWriter();
    t.join();

    REQUIRE(got);
    std::string out(16, '\0');
    m.output_fifo->read((uint8_t *)out.data(), 16);
    CHECK(out == xorString(payload));
}
