#pragma once

#include <mutex>
#include <atomic>

extern std::mutex source_stream_mtx;
extern bool source_should_stream;
extern std::atomic<int> streaming_bit_depth;

void sourceStreamThread();