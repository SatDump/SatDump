#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <atomic>
#include "imgui/imgui_flags.h"
#include "dll_export.h"
#include "common/dsp/complex.h"
#include "common/dsp/buffer.h"
#include "nlohmann/json.hpp"
#include "plugin.h"

// Utils
#define WRITE_IMAGE(image, path)                  \
{                                                 \
    std::string newPath = path;                   \
    image.append_ext(&newPath);                   \
    image.save_img(std::string(newPath).c_str()); \
    d_output_files.push_back(newPath);            \
}
#define UITO_C_STR(input) "%s", std::to_string(input).c_str()

// Colors
#define IMCOLOR_NOSYNC ImColor::HSV(0 / 360.0, 1, 1, 1.0)
#define IMCOLOR_SYNCING ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0)
#define IMCOLOR_SYNCED ImColor::HSV(113.0 / 360.0, 1, 1, 1.0)

#define REGISTER_MODULE(module) modules_registry.emplace(module::getID(), module::getInstance)
#define REGISTER_MODULE_EXTERNAL(registry, module) registry.emplace(module::getID(), module::getInstance)

enum ModuleDataType
{
    DATA_STREAM,     // Generic data, for which a circular buffer will be used
    DATA_DSP_STREAM, // DSP Data using the specialized buffer and complex floats
    DATA_FILE,       // Just generic data from a file
};

class ProcessingModule
{
protected:
    const std::string d_input_file;
    const std::string d_output_file_hint;
    std::vector<std::string> d_output_files;
    nlohmann::json d_parameters;
    ModuleDataType input_data_type, output_data_type;

    bool streamingInput; // Used to know if we should treat the input as a stream or file

public:
    ProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    virtual std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE}; }
    virtual std::vector<ModuleDataType> getOutputTypes() { return {DATA_FILE}; }
    void setInputType(ModuleDataType type);
    void setOutputType(ModuleDataType type);
    ModuleDataType getInputType();
    ModuleDataType getOutputType();
    virtual void init();
    virtual void stop();
    virtual void process() = 0;
    virtual void drawUI(bool window) = 0;
    virtual bool hasUI();
    std::vector<std::string> getOutputs();

public:
    std::shared_ptr<dsp::RingBuffer<uint8_t>> input_fifo;
    std::shared_ptr<dsp::RingBuffer<uint8_t>> output_fifo;
    std::shared_ptr<dsp::stream<complex_t>> input_stream;
    std::shared_ptr<dsp::stream<complex_t>> output_stream;
    std::atomic<bool> input_active;

public:
    nlohmann::json module_stats;

public:
    static std::string getID();
    virtual std::string getIDM() = 0;
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
};

SATDUMP_DLL extern std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)>> modules_registry;

// Event where modules are registered, so plugins can load theirs
struct RegisterModulesEvent
{
    std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)>> &modules_registry;
};

void registerModules();

SATDUMP_DLL extern float ui_scale;               // UI Scaling factor, for DPI scaling
SATDUMP_DLL extern int demod_constellation_size; // Demodulator constellation size

// Status stuff
enum instrument_status_t
{
    DECODING,
    PROCESSING,
    SAVING,
    DONE,
};

void drawStatus(instrument_status_t status);
