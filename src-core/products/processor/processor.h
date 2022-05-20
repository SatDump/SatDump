#pragma once

#include <string>
#include <functional>
#include <map>
#include "../products.h"

namespace satdump
{
    void process_product(std::string product_path);
    void process_dataset(std::string dataset_path);
}