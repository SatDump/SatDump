#pragma once

#include "tcp_test.h"
#include "common/dsp_source_sink/dsp_sample_source.h"

extern TCPServer *tcp_server;
extern std::mutex source_mtx;
extern bool source_is_open;
extern bool source_is_started;
extern std::shared_ptr<dsp::DSPSampleSource> current_sample_source;