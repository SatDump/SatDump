#include "contains.h"

bool contains(std::vector<double> tm, double n){
    for (unsigned int i = 0; i < tm.size(); i++){
        if (tm[i] == n)
            return true;
    }
    return false;
}