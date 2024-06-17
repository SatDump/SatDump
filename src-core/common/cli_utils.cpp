#include "cli_utils.h"

nlohmann::json parse_common_flags(int argc, char *argv[], std::map<std::string, std::optional<std::type_index>> strong_types)
{
    nlohmann::json parameters;

    if (argc > 0)
    {
        for (int i = 0; i < argc; i++)
        {
            if (i != argc)
            {
                std::string flag = argv[i];

                if (flag[0] == '-' && flag[1] == '-') // Detect a flag
                {
                    flag = flag.substr(2, flag.length()); // Remove the "--"

                    if (i + 1 == argc) // Check if there is a next element, if not, consider this is a switch
                    {
                        parameters[flag] = true;
                        continue;
                    }

                    std::string value = argv[i + 1]; // Get value

                    // Check if the next value is a flag as well. If so, treat as a switch and set it to true
                    if (value[0] == '-' && value[1] == '-')
                    {
                        parameters[flag] = true;
                        continue;
                    }

                    // Force specified flags to be parsed as the specified type
                    if (strong_types.count(flag) > 0)
                    {
                        if (strong_types[flag] == typeid(std::string))
                        {
                            parameters[flag] = value;
                            i++;
                            continue;
                        }
                    }

                    // Is this a boolean?
                    if (value == "false" || value == "true")
                    {
                        parameters[flag] = (value == "true");
                        continue;
                    }

                    try // Attempt to parse it as a number
                    {
                        for (char &c : std::string(argv[i + 1]))
                            if (c < '0' || '9' < c)
                                if (c != '.' && c != 'e')
                                    if (c != '-')
                                        throw std::runtime_error("");

                        int points_cnt = 0;
                        for (char &c : std::string(argv[i + 1]))
                            if (c == '.')
                                points_cnt++;
                        if (points_cnt > 1)
                            throw std::runtime_error(""); // Numbers should not have more than one dot! If it does, probably an IP address

                        double val = std::stod(argv[i + 1]);
                        double integral, fractional = modf(val, &integral); // Split to fractional & integral parts

                        if (fractional != 0) // If this is not an integer, put it as a double
                            parameters[flag] = val;
                        else // Otherwse, cast to an integer
                            parameters[flag] = (long long)integral;
                    }
                    catch (std::exception &) // If it fails, parse to a string
                    {
                        parameters[flag] = value;
                    }

                    i++; // Skip the value
                }
            }
        }
    }

    return parameters;
}