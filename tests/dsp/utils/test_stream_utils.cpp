#include "../ndsp_test_helpers.h"
#include "dsp/block_helpers.h"
#include "dsp/utils/psk_snr_estimator.h"
#include "dsp/utils/reshape_buffer.h"
#include "dsp/utils/samplerate_meter.h"
#include "dsp/utils/throttle.h"
#include "dsp/utils/zero_fill.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

///////////////////////////////////////////////////////////////////////////////
// PSK SNR estimator
///////////////////////////////////////////////////////////////////////////////

namespace
{
    std::vector<complex_t> noisyBpsk(size_t n, double sigma, uint32_t seed)
    {
        ndsp_test::Lcg rng(seed);
        std::vector<complex_t> v(n);
        for (auto &x : v)
        {
            float s = rng.sign();
            float ni = sigma * rng.gauss();
            float nq = sigma * rng.gauss();
            x = complex_t(s + ni, nq);
        }
        return v;
    }
} // namespace

TEST_CASE("ndsp PSKSnrEstimatorBlock estimates the SNR of noisy BPSK", "[dsp][utils][psk_snr_estimator]")
{
    // Signal power 1, noise power 2 * sigma^2
    for (double sigma : {0.05, 0.1, 0.2})
    {
        INFO("sigma = " << sigma);
        double snr_db = 10 * log10(1.0 / (2 * sigma * sigma));
        auto b = std::make_shared<PSKSnrEstimatorBlock>();
        auto r = ndsp_test::runSimple<complex_t, float>(b, noisyBpsk(40000, sigma, 31), 4096);
        REQUIRE(r.completed);
        CHECK(r.outputs.empty());

        // get_cfg() reads snr first, which also updates signal / noise
        nlohmann::json st = ((Block &)*b).get_cfg();
        CHECK(std::fabs(st["snr"].get<float>() - snr_db) < 1.5);
        CHECK(std::fabs(st["signal"].get<float>()) < 0.5);
        CHECK(std::fabs(st["noise"].get<float>() - 10 * log10(2 * sigma * sigma)) < 1.5);
    }
}

TEST_CASE("ndsp PSKSnrEstimatorBlock lower noise gives a higher SNR", "[dsp][utils][psk_snr_estimator]")
{
    auto run = [](double sigma)
    {
        auto b = std::make_shared<PSKSnrEstimatorBlock>();
        auto r = ndsp_test::runSimple<complex_t, float>(b, noisyBpsk(30000, sigma, 32), 2048);
        REQUIRE(r.completed);
        return b->get_cfg("snr").get<float>();
    };
    float a = run(0.3), c = run(0.1);
    CHECK(c > a + 5);
}

TEST_CASE("ndsp PSKSnrEstimatorBlock id, IO and config", "[dsp][utils][psk_snr_estimator]")
{
    PSKSnrEstimatorBlock b;
    CHECK(b.d_id == "psk_snr_estimator_c");
    CHECK(b.get_inputs().size() == 1);
    CHECK(b.get_outputs().empty());
    CHECK_FALSE(b.is_async());
    auto list = b.get_cfg_list();
    CHECK(list["snr"]["type"] == "stat");
    CHECK(list["noise"]["type"] == "stat");
    CHECK(list["signal"]["type"] == "stat");
    // Stats are read-only, setting them is accepted and ignored
    CHECK(b.set_cfg("snr", 99) == Block::RES_OK);
    CHECK(b.set_cfg("noise", 99) == Block::RES_OK);
    CHECK(b.set_cfg("signal", 99) == Block::RES_OK);
    CHECK_THROWS(b.set_cfg("x", 1));
    CHECK_THROWS(b.get_cfg("x"));
}

///////////////////////////////////////////////////////////////////////////////
// Reshape buffer
///////////////////////////////////////////////////////////////////////////////

namespace
{
    // The reshape block has a second thread writing to the output, which is
    // only safe while the work thread itself is idle. So wait for all expected
    // output before sending the terminator (see report).
    template <typename T>
    std::vector<uint32_t> runReshape(int bufsize, const std::vector<T> &in, size_t chunk, size_t expected_out, std::vector<T> &out)
    {
        auto b = std::make_shared<ReshapeBufferBlock<T>>();
        b->set_cfg("buffer_size", bufsize);
        auto is = std::make_shared<DSPStream>(4);
        b->set_input({"in", getTypeSampleType<T>(), is}, 0);
        ndsp_test::Collector<T> col(b->get_output(0, 4).fifo);
        // Workaround for the start() race (see report) : initialize the ring
        // buffer before start() launches the second thread. start() runs
        // init() again, leaking the first ring allocation.
        b->init();
        b->start();
        {
            ndsp_test::Feeder<T> feed(is, in, chunk, 0);
            REQUIRE(feed.wait());
        }
        REQUIRE(col.waitFor(expected_out));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ndsp_test::pushTerminator(*is);
        REQUIRE(col.waitForTerminator());
        REQUIRE(ndsp_test::stopBlock(b));
        col.finish();
        out = col.data();
        return col.sizes();
    }
} // namespace

TEST_CASE("ndsp ReshapeBufferBlock outputs fixed size buffers", "[dsp][utils][reshape_buffer]")
{
    std::vector<float> in(1000);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (float)i;
    std::vector<float> out;
    auto sizes = runReshape<float>(100, in, 37, 1000, out);
    CHECK(out == in);
    CHECK(sizes == std::vector<uint32_t>(10, 100));
}

TEST_CASE("ndsp ReshapeBufferBlock merges small buffers into larger ones", "[dsp][utils][reshape_buffer]")
{
    auto in = ndsp_test::randomComplex(2048, 33);
    std::vector<complex_t> out;
    auto sizes = runReshape<complex_t>(512, in, 16, 2048, out);
    CHECK(sizes == std::vector<uint32_t>(4, 512));
    REQUIRE(out.size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(ndsp_test::sameBits(out[i], in[i]));
}

TEST_CASE("ndsp ReshapeBufferBlock keeps an incomplete last buffer", "[dsp][utils][reshape_buffer]")
{
    // 250 samples, 100 per buffer : the last 50 are never output
    std::vector<int8_t> in(250);
    for (size_t i = 0; i < in.size(); i++)
        in[i] = (int8_t)(i % 100);
    std::vector<int8_t> out;
    auto sizes = runReshape<int8_t>(100, in, 64, 200, out);
    CHECK(sizes == std::vector<uint32_t>{100, 100});
    CHECK(out == std::vector<int8_t>(in.begin(), in.begin() + 200));
}

TEST_CASE("ndsp ReshapeBufferBlock ids and config", "[dsp][utils][reshape_buffer]")
{
    CHECK(ReshapeBufferBlock<float>().d_id == "reshape_buffer_ff");
    CHECK(ReshapeBufferBlock<complex_t>().d_id == "reshape_buffer_cc");
    CHECK(ReshapeBufferBlock<int8_t>().d_id == "reshape_buffer_hh");
    ReshapeBufferBlock<float> b;
    CHECK(b.get_cfg("buffer_size") == 1024);
    b.set_cfg("buffer_size", 77);
    CHECK(b.get_cfg("buffer_size") == 77);
    CHECK(b.get_cfg_list()["buffer_size"]["type"] == "int");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Zero fill
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ZeroFillBlock fills gaps with zeros and forwards data in order", "[dsp][utils][zero_fill]")
{
    auto b = std::make_shared<ZeroFillBlock<float>>();
    b->set_cfg("zero_buffer_size", 16);
    auto is = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_F32, is}, 0);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();

    // With no input, zero buffers keep coming
    REQUIRE(col.waitFor(64));

    std::vector<float> data(100);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = (float)(i + 1);
    ndsp_test::pushSamples(*is, data);
    REQUIRE(ndsp_test::waitUntil(
        [&]()
        {
            auto s = col.sizes();
            return std::find(s.begin(), s.end(), 100u) != s.end();
        }));
    ndsp_test::pushTerminator(*is);
    REQUIRE(col.waitForTerminator());
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();

    auto d = col.data();
    auto sizes = col.sizes();
    // Every buffer is either a zero buffer of 16 samples, or the 100 data samples
    size_t pos = 0, data_buffers = 0;
    for (auto s : sizes)
    {
        if (s == 100)
        {
            data_buffers++;
            for (size_t i = 0; i < 100; i++)
                REQUIRE(d[pos + i] == data[i]);
        }
        else
        {
            REQUIRE(s == 16);
            for (size_t i = 0; i < 16; i++)
                REQUIRE(d[pos + i] == 0.0f);
        }
        pos += s;
    }
    CHECK(data_buffers == 1);
    CHECK(col.terminators() == 1);
}

TEST_CASE("ndsp ZeroFillBlock non-propagating terminator stops silently", "[dsp][utils][zero_fill]")
{
    auto b = std::make_shared<ZeroFillBlock<complex_t>>();
    auto is = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_CF32, is}, 0);
    ndsp_test::Collector<complex_t> col(b->get_output(0, 4).fifo);
    b->start();
    REQUIRE(col.waitFor(64));
    ndsp_test::pushTerminator(*is, false);
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();
    CHECK(col.terminators() == 0);
    for (auto s : col.sizes())
        REQUIRE(s == 64);
}

TEST_CASE("ndsp ZeroFillBlock ids and config", "[dsp][utils][zero_fill]")
{
    CHECK(ZeroFillBlock<float>().d_id == "zero_fill_ff");
    CHECK(ZeroFillBlock<complex_t>().d_id == "zero_fill_cc");
    CHECK(ZeroFillBlock<int8_t>().d_id == "zero_fill_hh");
    ZeroFillBlock<float> b;
    CHECK(b.get_cfg("zero_buffer_size") == 64);
    b.set_cfg("zero_buffer_size", 8);
    CHECK(b.get_cfg("zero_buffer_size") == 8);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Throttle
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp ThrottleBlock limits the sample rate", "[dsp][utils][throttle]")
{
    // 20000 samples at 100 kS/s take at least 0.2 s
    auto in = ndsp_test::randomFloats(20000, 34);
    auto b = std::make_shared<ThrottleBlock<float>>();
    REQUIRE(ndsp_test::configure(*b, {{"samplerate", 100e3}}) == Block::RES_OK);
    auto t0 = std::chrono::steady_clock::now();
    auto r = ndsp_test::runSimple<float, float>(b, in, 1000);
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    REQUIRE(r.completed);
    CHECK(r.out() == in);
    CHECK(elapsed >= 0.19);
    CHECK(elapsed < 10.0);
}

TEST_CASE("ndsp ThrottleBlock passes complex samples unchanged", "[dsp][utils][throttle]")
{
    auto in = ndsp_test::randomComplex(5000, 35);
    auto b = std::make_shared<ThrottleBlock<complex_t>>();
    b->set_cfg("samplerate", 1e7);
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 1000);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(ndsp_test::sameBits(r.out()[i], in[i]));
}

TEST_CASE("ndsp ThrottleBlock ids and config", "[dsp][utils][throttle]")
{
    ThrottleBlock<complex_t> c;
    ThrottleBlock<float> f;
    CHECK(c.d_id == "throttle_cc");
    CHECK(f.d_id == "throttle_ff");
    CHECK(f.get_cfg("samplerate") == 1e6);
    f.set_cfg("samplerate", 2.5e6);
    CHECK(f.get_cfg("samplerate") == 2.5e6);
    CHECK(f.get_cfg_list()["samplerate"]["name"] == "Samplerate");
    CHECK_THROWS(f.get_cfg("x"));
    CHECK_THROWS(f.set_cfg("x", 1));
}

///////////////////////////////////////////////////////////////////////////////
// Samplerate meter
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("ndsp SamplerateMeterBlock passes samples through", "[dsp][utils][samplerate_meter]")
{
    auto in = ndsp_test::randomComplex(3000, 36);
    auto b = std::make_shared<SamplerateMeterBlock<complex_t>>();
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 512);
    REQUIRE(r.completed);
    REQUIRE(r.out().size() == in.size());
    for (size_t i = 0; i < in.size(); i++)
        REQUIRE(ndsp_test::sameBits(r.out()[i], in[i]));
    CHECK(r.outputs[0].propagating_terminators == 1);
}

TEST_CASE("ndsp SamplerateMeterBlock measures the input rate", "[dsp][utils][samplerate_meter]")
{
    // ~20 kS/s for a bit more than 2 s : the first second only sets the reference
    auto b = std::make_shared<SamplerateMeterBlock<float>>();
    auto is = std::make_shared<DSPStream>(4);
    b->set_input({"in", DSP_SAMPLE_TYPE_F32, is}, 0);
    ndsp_test::Collector<float> col(b->get_output(0, 4).fifo);
    b->start();
    std::vector<float> chunk(1000, 1.0f);
    auto t0 = std::chrono::steady_clock::now();
    size_t sent = 0;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(2300))
    {
        ndsp_test::pushSamples(*is, chunk);
        sent += chunk.size();
        std::this_thread::sleep_until(t0 + std::chrono::milliseconds(50) * (sent / 1000));
    }
    ndsp_test::pushTerminator(*is);
    REQUIRE(col.waitForTerminator());
    REQUIRE(ndsp_test::stopBlock(b));
    col.finish();
    CHECK(col.size() == sent);
    // Wide bounds : a loaded machine can only slow the feeder down
    CHECK(b->measured_samplerate > 5000);
    CHECK(b->measured_samplerate < 30000);
}

TEST_CASE("ndsp SamplerateMeterBlock ids and config", "[dsp][utils][samplerate_meter]")
{
    CHECK(SamplerateMeterBlock<float>().d_id == "samplerate_meter_ff");
    CHECK(SamplerateMeterBlock<complex_t>().d_id == "samplerate_meter_cc");
    SamplerateMeterBlock<float> b;
    CHECK(b.get_cfg_list().empty());
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}
