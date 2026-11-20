#include "common/physics_constants.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("SPEED_OF_LIGHT_M_S is the exact SI value", "[common][physics_constants]")
{
    CHECK(SPEED_OF_LIGHT_M_S == 299792458.0);
}
