#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <climits>

#ifdef _MSC_VER
#define timegm _mkgmtime
#endif

// Return filesize
uint64_t getFilesize(std::string filepath);

std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder = "");
std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency);
