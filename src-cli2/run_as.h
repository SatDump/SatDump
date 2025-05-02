#pragma once

#include <string>

namespace satdump
{
    int runAngelScript(std::string file, bool lint, bool predef);
}