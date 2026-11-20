#include "dsp/block.h"
#include "dsp/ndsp_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

using namespace satdump::ndsp;

namespace
{
    // Minimal block : forwards float buffers, counts init() calls and
    // exposes a few protected helpers for testing
    class PassBlock : public Block
    {
    public:
        int init_count = 0;
        float gain = 1;
        int ival = 3;
        std::atomic<int> work_calls{0};

        PassBlock() : Block("pass_ff", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        void init() override { init_count++; }

        bool work() override
        {
            work_calls++;
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();
            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }
            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(iblk.size, sizeof(float));
            for (uint32_t i = 0; i < iblk.size; i++)
                oblk.getSamples<float>()[i] = iblk.getSamples<float>()[i] * gain;
            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            inputs[0].fifo->free(iblk);
            return false;
        }

        nlohmann::ordered_json get_cfg_list() override
        {
            nlohmann::ordered_json p;
            add_param_simple(p, "gain", "float");
            add_param_simple(p, "ival", "int");
            return p;
        }

        nlohmann::json get_cfg(std::string key) override
        {
            if (key == "gain")
                return gain;
            else if (key == "ival")
                return ival;
            throw satdump_exception(key);
        }

        cfg_res_t set_cfg(std::string key, nlohmann::json v) override
        {
            if (key == "gain")
                gain = v;
            else if (key == "ival")
            {
                ival = v;
                return RES_LISTUPD;
            }
            else if (key == "io")
                return RES_IOUPD;
            else if (key == "bad")
                return RES_ERR;
            else
                throw satdump_exception(key);
            return RES_OK;
        }

        using Block::cfg_res_t;
        bool running() { return is_work_running(); }

        template <typename T>
        void callSetVal(T &v, nlohmann::json p)
        {
            setValFromJSONIfExists(v, p);
        }
    };

    // Source that only stops when asked to (work_should_exit), sending a terminator
    class TickSource : public Block
    {
    public:
        std::atomic<int> sent{0};

        TickSource() : Block("tick_f", {}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        bool work() override
        {
            if (work_should_exit)
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }
            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(4, sizeof(float));
            for (int i = 0; i < 4; i++)
                oblk.getSamples<float>()[i] = (float)(sent * 4 + i);
            oblk.size = 4;
            outputs[0].fifo->wait_enqueue(oblk);
            sent++;
            return false;
        }

        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };

    // Loops on work() until force-stopped, without ever blocking
    class SpinBlock : public Block
    {
    public:
        std::atomic<long> spins{0};
        SpinBlock() : Block("spin", {}, {}) {}
        bool work() override
        {
            spins++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return false;
        }
        nlohmann::json get_cfg(std::string key) override { throw satdump_exception(key); }
        cfg_res_t set_cfg(std::string key, nlohmann::json) override { throw satdump_exception(key); }
    };
} // namespace

TEST_CASE("ndsp Block exposes its id and IO configuration", "[dsp][block]")
{
    PassBlock b;
    REQUIRE(b.d_id == "pass_ff");

    auto in = b.get_inputs();
    auto out = b.get_outputs();
    REQUIRE(in.size() == 1);
    REQUIRE(out.size() == 1);
    CHECK(in[0].name == "in");
    CHECK(in[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(in[0].fifo == nullptr);
    CHECK(out[0].name == "out");
    CHECK(out[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(out[0].samplerate == 0);
    CHECK(out[0].frequency == 0);
}

TEST_CASE("ndsp Block is_async depends on having inputs", "[dsp][block]")
{
    PassBlock withInput;
    TickSource source;
    CHECK_FALSE(withInput.is_async());
    CHECK(source.is_async());
}

TEST_CASE("ndsp Block get_output creates a stream only when nbuf is positive", "[dsp][block]")
{
    PassBlock b;

    BlockIO o0 = b.get_output(0, 0);
    CHECK(o0.fifo == nullptr);

    BlockIO o1 = b.get_output(0, 3);
    REQUIRE(o1.fifo != nullptr);
    CHECK(o1.fifo->max_capacity() == 3);

    // nbuf == 0 returns the existing stream
    BlockIO o2 = b.get_output(0, 0);
    CHECK(o2.fifo == o1.fifo);

    // A new positive nbuf replaces it
    BlockIO o3 = b.get_output(0, 5);
    CHECK(o3.fifo != o1.fifo);
    CHECK(o3.fifo->max_capacity() == 5);

    CHECK_THROWS_AS(b.get_output(1, 4), satdump::satdump_exception_t);
}

TEST_CASE("ndsp Block set_input checks index and type", "[dsp][block]")
{
    PassBlock b;

    BlockIO good = {"x", DSP_SAMPLE_TYPE_F32, std::make_shared<DSPStream>(2)};
    BlockIO bad = {"x", DSP_SAMPLE_TYPE_CF32, std::make_shared<DSPStream>(2)};

    CHECK_THROWS_AS(b.set_input(good, 1), satdump::satdump_exception_t);
    CHECK_THROWS_AS(b.set_input(bad, 0), satdump::satdump_exception_t);
    CHECK(b.get_inputs()[0].fifo == nullptr);

    b.set_input(good, 0);
    CHECK(b.get_inputs()[0].fifo == good.fifo);
    // Name and type of the input itself are kept
    CHECK(b.get_inputs()[0].name == "in");
}

TEST_CASE("ndsp Block link connects an output to an input", "[dsp][block]")
{
    PassBlock a, b;
    b.link(&a, 0, 0, 6);
    REQUIRE(a.get_outputs()[0].fifo != nullptr);
    CHECK(b.get_inputs()[0].fifo == a.get_outputs()[0].fifo);
    CHECK(a.get_outputs()[0].fifo->max_capacity() == 6);

    TickSource s;
    CHECK_THROWS_AS(s.link(&a, 0, 0, 2), satdump::satdump_exception_t); // s has no input
}

TEST_CASE("ndsp Block set_cfg with JSON returns the highest status", "[dsp][block][cfg]")
{
    PassBlock pb;
    Block &b = pb; // The JSON overloads are hidden in derived classes
    PassBlock &raw = pb;

    CHECK(b.set_cfg(nlohmann::json{{"gain", 2.5}}) == Block::RES_OK);
    CHECK(raw.gain == 2.5f);

    CHECK(b.set_cfg(nlohmann::json{{"gain", 1.0}, {"ival", 7}}) == Block::RES_LISTUPD);
    CHECK(raw.ival == 7);

    CHECK(b.set_cfg(nlohmann::json{{"ival", 1}, {"io", 0}}) == Block::RES_IOUPD);
    CHECK(b.set_cfg(nlohmann::json{{"bad", 0}}) == Block::RES_ERR);

    // An exception while applying a key is reported as RES_ERR, other keys still apply
    CHECK(b.set_cfg(nlohmann::json{{"unknown", 1}, {"gain", 4.0}}) == Block::RES_ERR);
    CHECK(raw.gain == 4.0f);

    // Wrong JSON type throws inside, also reported as RES_ERR
    CHECK(b.set_cfg(nlohmann::json{{"gain", "nope"}}) == Block::RES_ERR);

    CHECK(b.set_cfg(nlohmann::json::object()) == Block::RES_OK);
}

TEST_CASE("ndsp Block get_cfg returns every listed parameter", "[dsp][block][cfg]")
{
    PassBlock b;
    b.gain = 0.5;
    b.ival = 42;
    nlohmann::json all = ((Block &)b).get_cfg();
    REQUIRE(all.size() == 2);
    CHECK(all["gain"].get<float>() == 0.5f);
    CHECK(all["ival"].get<int>() == 42);

    // The default list is empty
    TickSource s;
    CHECK(s.get_cfg_list().empty());
    CHECK(((Block &)s).get_cfg().is_null());
}

TEST_CASE("ndsp Block setValFromJSONIfExists ignores null", "[dsp][block][cfg]")
{
    PassBlock b;
    int v = 5;
    b.callSetVal(v, nlohmann::json());
    CHECK(v == 5);
    b.callSetVal(v, nlohmann::json(9));
    CHECK(v == 9);

    nlohmann::json obj = {{"a", 3}};
    b.callSetVal(v, obj["missing"]);
    CHECK(v == 9);
}

TEST_CASE("ndsp add_param helpers build the parameter description", "[dsp][block][cfg]")
{
    nlohmann::ordered_json p;
    add_param_simple(p, "a", "float");
    add_param_simple(p, "b", "int", "Bee");
    add_param_range(p, "c", "float", 0, 10, 0.5, "Sea");
    add_param_list(p, "d", "string", {"x", "y"});

    CHECK(p["a"]["type"] == "float");
    CHECK_FALSE(p["a"].contains("name"));
    CHECK(p["b"]["name"] == "Bee");
    CHECK(p["c"]["range"] == nlohmann::ordered_json({0, 10, 0.5}));
    CHECK(p["c"]["name"] == "Sea");
    CHECK(p["d"]["list"] == nlohmann::ordered_json({"x", "y"}));
    CHECK_FALSE(p["d"].contains("name"));

    // Insertion order is kept
    std::vector<std::string> keys;
    for (auto &it : p.items())
        keys.push_back(it.key());
    CHECK(keys == std::vector<std::string>{"a", "b", "c", "d"});
}

TEST_CASE("ndsp Block start calls init and runs until a terminator", "[dsp][block]")
{
    auto b = std::make_shared<PassBlock>();
    b->gain = 3;
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2, 3, 4, 5}, 2);
    REQUIRE(r.completed);
    CHECK(b->init_count == 1);
    CHECK(r.out() == std::vector<float>{3, 6, 9, 12, 15});
    CHECK(r.outputs[0].sizes == std::vector<uint32_t>{2, 2, 1});
    CHECK(r.outputs[0].propagating_terminators == 1);
    CHECK_FALSE(b->running());
}

TEST_CASE("ndsp Block non-propagating terminator stops without forwarding", "[dsp][block]")
{
    auto b = std::make_shared<PassBlock>();
    auto r = ndsp_test::runSimple<float, float>(b, {1, 2}, 8, 2);
    REQUIRE(r.completed);
    CHECK(r.out() == std::vector<float>{1, 2});
    CHECK(r.outputs[0].terminators == 0);
}

TEST_CASE("ndsp Block can be restarted after stopping", "[dsp][block]")
{
    auto b = std::make_shared<PassBlock>();
    auto r1 = ndsp_test::runSimple<float, float>(b, {1}, 8);
    auto r2 = ndsp_test::runSimple<float, float>(b, {2, 3}, 8);
    REQUIRE(r1.completed);
    REQUIRE(r2.completed);
    CHECK(r1.out() == std::vector<float>{1});
    CHECK(r2.out() == std::vector<float>{2, 3});
    CHECK(b->init_count == 2);
}

TEST_CASE("ndsp Block start twice without stop throws", "[dsp][block]")
{
    auto b = std::make_shared<PassBlock>();
    auto in = std::make_shared<DSPStream>(2);
    b->set_input({"x", DSP_SAMPLE_TYPE_F32, in}, 0);
    auto out = b->get_output(0, 2).fifo;
    ndsp_test::Collector<float> col(out);

    b->start();
    CHECK(b->running());
    CHECK_THROWS_AS(b->start(), satdump::satdump_exception_t);
    CHECK(b->init_count == 1);

    ndsp_test::pushTerminator(*in);
    REQUIRE(ndsp_test::stopBlock(b));
    REQUIRE(col.waitForTerminator());
    col.finish();
}

TEST_CASE("ndsp Block stop on a never started block is a no-op", "[dsp][block]")
{
    PassBlock b;
    b.stop();
    b.stop(true, true);
    CHECK_FALSE(b.running());
}

TEST_CASE("ndsp Block stop_now makes a source send its terminator", "[dsp][block]")
{
    auto s = std::make_shared<TickSource>();
    ndsp_test::Collector<float> col(s->get_output(0, 4).fifo);
    s->start();
    REQUIRE(col.waitFor(40));
    REQUIRE(ndsp_test::stopBlock(s, true));
    REQUIRE(col.waitForTerminator());
    col.finish();

    // Samples are a ramp, in order, with no gaps
    auto d = col.data();
    REQUIRE(d.size() >= 40);
    for (size_t i = 0; i < d.size(); i++)
        REQUIRE(d[i] == (float)i);
    CHECK(col.terminators() == 1);
    CHECK(d.size() == (size_t)s->sent * 4);
}

TEST_CASE("ndsp Block force stop exits a looping work function", "[dsp][block]")
{
    auto s = std::make_shared<SpinBlock>();
    s->start();
    while (s->spins < 3)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto done = std::async(std::launch::async, [s]() { s->stop(false, true); });
    REQUIRE(done.wait_for(ndsp_test::kTimeout) == std::future_status::ready);
    long spins = s->spins;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(s->spins == spins);
}
