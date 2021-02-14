#pragma once

#include "module.h"

class MetOpMHSDecoderModule : public ProcessingModule
{
protected:


public:
    MetOpMHSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    void process();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};