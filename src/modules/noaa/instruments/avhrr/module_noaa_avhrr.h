#pragma once

#include "module.h"
#include <complex>
#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/costas_loop.h>
#include <dsp/clock_recovery_mm.h>

class NOAAAVHRRDecoderModule : public ProcessingModule
{
protected:


public:
    NOAAAVHRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    void process();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};