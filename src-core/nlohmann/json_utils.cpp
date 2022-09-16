#include "json.hpp"
#include <fstream>
#include <filesystem>

nlohmann::ordered_json loadJsonFile(std::string path)
{
    nlohmann::ordered_json j;

    if (std::filesystem::exists(path))
    {
        std::ifstream istream(path);
        istream >> j;
        istream.close();
    }

    return j;
}

void saveJsonFile(std::string path, nlohmann::ordered_json j)
{
    std::ofstream ostream(path);
    ostream << j.dump(4);
    ostream.close();
}

nlohmann::ordered_json merge_json_diffs(nlohmann::ordered_json master, nlohmann::ordered_json diff)
{
    nlohmann::ordered_json output = master;

    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : master.items())
    {
        if (diff.contains(cfg.key()))
        {
            if (cfg.value().is_object())
                output[cfg.key()] = merge_json_diffs(cfg.value(), diff[cfg.key()]);
            else
                output[cfg.key()] = diff[cfg.key()];
        }
    }

    return output;
}

nlohmann::ordered_json perform_json_diff(nlohmann::ordered_json master, nlohmann::ordered_json modified)
{
    nlohmann::ordered_json diff;

    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : master.items())
    {
        if (modified[cfg.key()] != cfg.value())
        {
            if (cfg.value().is_object())
                diff[cfg.key()] = perform_json_diff(cfg.value(), modified[cfg.key()]);
            else
                diff[cfg.key()] = modified[cfg.key()];
        }
    }

    return diff;
}