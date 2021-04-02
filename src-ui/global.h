#pragma once

#include "modules/common/ctpl/ctpl_stl.h"
#include "module.h"

extern std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
extern std::shared_ptr<std::mutex> uiCallListMutex;

extern ctpl::thread_pool processThreadPool;

extern std::string downlink_pipeline;
extern std::string input_level;
extern std::string input_file;
extern std::string output_level;
extern std::string output_file;
extern std::map<std::string, std::string> parameters;

//extern bool processing;

extern int category_id;
extern int pipeline_id;
extern int input_level_id;

extern int baseband_type_option;

extern char samplerate[100];
extern int frequency_id;
extern float frequency;
extern std::string baseband_format;
extern bool dc_block;

extern bool livedemod;

extern char error_message[100];