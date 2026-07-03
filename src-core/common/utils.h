#pragma once

#include <cstdint>
#include <ctime>
#include <string>


#if defined(_WIN32)
#include <time.h>

#ifdef timegm
#undef timegm
#endif

inline time_t timegm(struct tm *const t) { return _mkgmtime(t); }
#endif

// Return filesize
uint64_t getFilesize(std::string filepath);

std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder = "");
std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency);
