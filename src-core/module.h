#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <atomic>
#include "imgui/imgui_flags.h"
#include "pipe.h"
#include "dll_export.h"

#define WRITE_IMAGE(image, path)               \
    image.save_png(std::string(path).c_str()); \
    d_output_files.push_back(path);

enum ModuleDataType
{
    DATA_STREAM,
    DATA_FILE
};

class ProcessingModule
{
protected:
    const std::string d_input_file;
    const std::string d_output_file_hint;
    std::vector<std::string> d_output_files;
    const std::map<std::string, std::string> d_parameters;
    ModuleDataType input_data_type, output_data_type;

public:
    ProcessingModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    virtual std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE}; }
    virtual std::vector<ModuleDataType> getOutputTypes() { return {DATA_FILE}; }
    void setInputType(ModuleDataType type);
    void setOutputType(ModuleDataType type);
    virtual void process() = 0;
    virtual void drawUI() = 0;
    std::vector<std::string> getOutputs();

public:
    std::shared_ptr<satdump::Pipe> input_fifo;
    std::shared_ptr<satdump::Pipe> output_fifo;
    std::atomic<bool> input_active;

public:
    static std::string getID();
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
};

SATDUMP_DLL extern std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> modules_registry;

void registerModules();