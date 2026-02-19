#include "logger.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "pipeline/pipeline.h"
#include "utils/string.h"
#include <filesystem>
#include <fstream>

#include "init.h"

#include <list>

nlohmann::ordered_json run_standalone_tests_offline(nlohmann::ordered_json config, std::string input_path1, std::string output_path1)
{
    nlohmann::ordered_json report;

    std::string excutable = config["config"]["command"];
    bool catch_logs = config["config"]["catch_logs"];

    for (auto pipeline_test : config["pipeline_tests"].items())
    {
        std::string name = pipeline_test.key();
        std::string pipeline = pipeline_test.value()["pipeline"];
        std::string input_file = pipeline_test.value()["input_file"];
        std::string input_level = pipeline_test.value()["input_level"];
        std::string parameters = pipeline_test.value()["parameters"];

        std::string input_path = input_path1 + "/" + input_file;
        std::string output_path = output_path1 + "/" + name;

        if (!std::filesystem::exists(output_path))
            std::filesystem::create_directories(output_path);

        std::string command = excutable + " " + pipeline + " " + input_level + " \"" + input_path + "\" \"" + output_path + "\" " + parameters;

        if (catch_logs)
            command += " > \"" + output_path + "/logs.txt\"";

        logger->info("Running %s", name.c_str());
        logger->trace("Command %s", command.c_str());

        auto test_start = std::chrono::system_clock::now();
        int return_value = system(command.c_str());
        auto test_time = (std::chrono::system_clock::now() - test_start);

        if (return_value == 256)
            return_value = 0;

        logger->info("Done! Return was %d. Time was %f", return_value, test_time.count() / 1e9);

        report[name]["command"] = command;
        report[name]["time"] = test_time.count() / 1e9;
        report[name]["return"] = return_value;
    }

    return report;
}

#include "common/cli_utils.h"

nlohmann::ordered_json run_singleinstance_tests_offline(nlohmann::ordered_json config, std::string input_path1, std::string output_path1)
{
    nlohmann::ordered_json report;

    // std::string excutable = config["config"]["command"];
    // bool catch_logs = config["config"]["catch_logs"];

    for (auto pipeline_test : config["pipeline_tests"].items())
    {
        std::string name = pipeline_test.key();
        std::string pipeline = pipeline_test.value()["pipeline"];
        std::string input_file = pipeline_test.value()["input_file"];
        std::string input_level = pipeline_test.value()["input_level"];
        std::string parameters = pipeline_test.value()["parameters"];

        std::string input_path = input_path1 + "/" + input_file;
        std::string output_path = output_path1 + "/" + name;

        if (!std::filesystem::exists(output_path))
            std::filesystem::create_directories(output_path);

        logger->info("Running %s", name.c_str());
        // logger->trace("Command %s", command.c_str());

        auto test_start = std::chrono::system_clock::now();
        // int return_value = system(command.c_str());

        {
            std::optional<satdump::pipeline::Pipeline> pipeliner = satdump::pipeline::getPipelineFromID(pipeline);

            std::vector<std::string> split_str = satdump::splitString(parameters, ' ');
            std::vector<char *> v;
            for (std::string str : split_str)
            {
                char *arg = new char[str.size() + 1];
                copy(str.begin(), str.end(), arg);
                arg[str.size()] = '\0';
                v.push_back(arg);
            }

            auto params = parse_common_flags(v.size(), &v[0]);

            for (size_t i = 0; i < v.size(); i++)
                delete[] v[i];

            pipeliner->run(input_path, output_path, params, input_level);
        }

        auto test_time = (std::chrono::system_clock::now() - test_start);

        // if (return_value == 256)
        //     return_value = 0;

        logger->info("Done! Time was %f", test_time.count() / 1e9);

        // report[name]["command"] = command;
        report[name]["time"] = test_time.count() / 1e9;
        // report[name]["return"] = return_value;
    }

    return report;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
        printf("Usage : test_satdump configuration.json source_files_directory output_directory\n");

    nlohmann::ordered_json config = loadJsonFile(argv[1]);
    nlohmann::ordered_json final_report;

    std::string input_folder = argv[2];
    std::string output_folder = argv[3];

    // Standalone tests
    if (config["run"]["standalone_offline"].get<bool>())
        final_report["standalone_offline"] = run_standalone_tests_offline(config, input_folder, output_folder + "/standalone_offline");

    initLogger();
    satdump::initSatDump();
    completeLoggerInit();

    // Single-Instance tests
    if (config["run"]["singleinstance_offline"].get<bool>())
        final_report["singleinstance_offline"] = run_singleinstance_tests_offline(config, input_folder, output_folder + "/singleinstance_offline");

    std::ofstream output_file(std::string(argv[3]) + "/tests_results.json");
    output_file << final_report.dump(4);
    output_file.close();

    satdump::exitSatDump();
}
