#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

#define WRITE_IMAGE(image, path)               \
    image.save_png(std::string(path).c_str()); \
    d_output_files.push_back(path);

class ProcessingModule
{
protected:
    const std::string d_input_file;
    const std::string d_output_file_hint;
    std::vector<std::string> d_output_files;
    const std::map<std::string, std::string> d_parameters;

public:
    ProcessingModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    virtual void process() = 0;
    std::vector<std::string> getOutputs();

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};

extern std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> modules_registry;

void registerModules();