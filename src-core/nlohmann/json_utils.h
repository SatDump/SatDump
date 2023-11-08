#pragma once

#include "json.hpp"

nlohmann::ordered_json loadJsonFile(std::string path);
void saveJsonFile(std::string path, nlohmann::ordered_json j);

// Apply a diff JSON object onto another
nlohmann::ordered_json merge_json_diffs(nlohmann::ordered_json master, nlohmann::ordered_json diff);

// Get a diff JSON object
nlohmann::ordered_json perform_json_diff(nlohmann::ordered_json master, nlohmann::ordered_json modified);

template <typename T>
T getValueOrDefault(nlohmann::json obj, T v)
{
    try
    {
        return obj.get<T>();
    }
    catch (std::exception &)
    {
        return v;
    }
}

template <typename T>
void setValueIfExists(nlohmann::json obj, T &v)
{
    try
    {
        v = obj.get<T>();
    }
    catch (std::exception &)
    {
    }
}

nlohmann::ordered_json loadCborFile(std::string path);
