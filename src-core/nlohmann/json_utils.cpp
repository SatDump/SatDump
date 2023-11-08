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

    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : diff.items())
    {
        if (master.contains(cfg.key()))
        {
            if (cfg.value().is_object())
                output[cfg.key()] = merge_json_diffs(master[cfg.key()], cfg.value());
            else
                output[cfg.key()] = diff[cfg.key()];
        }
        else
            output[cfg.key()] = cfg.value();
    }

    return output;
}

nlohmann::ordered_json perform_json_diff(nlohmann::ordered_json master, nlohmann::ordered_json modified)
{
    nlohmann::ordered_json diff;

    for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : modified.items())
    {
        if (master[cfg.key()] != cfg.value())
        {
            if (cfg.value().is_object())
                diff[cfg.key()] = perform_json_diff(master[cfg.key()], cfg.value());
            else
                diff[cfg.key()] = cfg.value();
        }
    }

    return diff;
}

nlohmann::ordered_json loadCborFile(std::string path)
{
    // Read file
    std::vector<uint8_t> cbor_data;
    std::ifstream in_file(path, std::ios::binary);
    while (!in_file.eof())
    {
        uint8_t b;
        in_file.read((char *)&b, 1);
        cbor_data.push_back(b);
    }
    in_file.close();
    cbor_data.pop_back();
    return nlohmann::json::from_cbor(cbor_data);
}
