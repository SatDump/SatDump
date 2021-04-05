#include "global.h"

std::shared_ptr<std::vector<std::shared_ptr<ProcessingModule>>> uiCallList;
std::shared_ptr<std::mutex> uiCallListMutex;

ctpl::thread_pool processThreadPool(4);

std::string downlink_pipeline = "";
std::string input_level = "";
std::string input_file = "";
std::string output_level = "products";
std::string output_file = "";
std::map<std::string, std::string> parameters;

//bool processing = false;

int category_id = 0;
int pipeline_id = -1;
int input_level_id = 0;

int baseband_type_option = 2;

char samplerate[100];
int frequency_id = 0;
float frequency;
std::string baseband_format;
bool dc_block;
bool iq_swap;

bool livedemod = false;

char error_message[100];