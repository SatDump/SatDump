#pragma once
#include <atomic>
#include <c_plugin_loader.h>

#include "common/dsp/utils/random.h"
#include "core/module.h"

#include <complex>
#include <fstream>

namespace c_plugin
{
class CPluginLoader : public ProcessingModule
{
  protected:
    std::string plugin_path;

    void *dynlib = nullptr;

    std::ifstream data_in;
    std::ofstream data_out;

    nlohmann::json params;

    plugin_interface_t plugin_interface;

    static const char* query_plugin_param(void *loader, const char *param_name);

  public:
    CPluginLoader(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    ~CPluginLoader();
    void process();
    void drawUI(bool window);
    bool hasUI();
    std::vector<ModuleDataType> getInputTypes();
    std::vector<ModuleDataType> getOutputTypes();

  public:
    static std::string getID();
    virtual std::string getIDM()
    {
        return getID();
    };
    static std::vector<std::string> getParameters();
    static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint,
                                                         nlohmann::json parameters);
};
} // namespace c_plugin
