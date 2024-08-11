#pragma once

#include "nafifo.hpp"

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>

#include <volk/volk.h>

#include "nlohmann/json.hpp"

namespace ndsp
{
    namespace buf
    {
        template <typename T>
        struct StdBuf
        {
            uint64_t max;
            uint64_t cnt;
            T *dat;
        };

        template <typename T>
        void init_nafifo_stdbuf(std::shared_ptr<NaFiFo> ptr, int nbuf, int size)
        {
            auto alloc = [size]() -> void *
            {
                StdBuf<T> *ptr = new StdBuf<T>;
                ptr->max = size;
                ptr->cnt = 0;
                ptr->dat = (T *)volk_malloc(sizeof(T) * size, volk_get_alignment());
                return ptr;
            };

            auto dealloc = [](void *p) -> void
            {
                StdBuf<T> *ptr = (StdBuf<T> *)p;
                volk_free(ptr->dat);
                delete ptr;
            };

            ptr->init(nbuf, sizeof(T), alloc, dealloc);
        }
    }

    struct BlockInOutCfg
    {
        int typesize;
    };

    class Block
    {
    private:
        std::string d_id;
        std::vector<BlockInOutCfg> d_incfg;
        std::vector<BlockInOutCfg> d_oucfg;

    protected:
        std::vector<std::shared_ptr<NaFiFo>> inputs;
        std::vector<std::shared_ptr<NaFiFo>> outputs;

    private:
        std::atomic<bool> d_work_run;
        std::thread d_work_th;

    private:
        void loop();
        virtual void work() = 0;

    public:
        Block(std::string id, std::vector<BlockInOutCfg> incfg, std::vector<BlockInOutCfg> oucfg);
        ~Block();

        std::string get_id() { return d_id; }
        std::shared_ptr<NaFiFo> get_output(int ou_n);
        void set_input(int in_n, std::shared_ptr<NaFiFo> out);

        virtual nlohmann::json get_params() { return {}; }
        virtual void set_params(nlohmann::json p = {}) {}

        virtual void start();
        virtual void stop();
    };
}