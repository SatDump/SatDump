#pragma once

#include "generic.h"

/*
 * Calculate Free-Space path-loss based on ITU R-REC-P.619
 */
class FreeSpacePathLossAttenuation : public GenericAttenuation
{

public:
    FreeSpacePathLossAttenuation();

    ~FreeSpacePathLossAttenuation();

    double get_attenuation();
};