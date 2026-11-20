#include "common/map/maidenhead.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <string>

using Catch::Matchers::WithinAbs;

namespace
{
    std::string mh(double lat, double lon, int size) { return std::string(get_mh(lat, lon, size)); }

    double lonOf(std::string loc) { return mh2lon(&loc[0]); }
    double latOf(std::string loc) { return mh2lat(&loc[0]); }

    // complete_mh pads short locators from a static buffer that earlier calls can dirty (see the
    // [!shouldfail] test below). Decoding a 9-character locator rewrites it to its initial state.
    void resetPadding() { lonOf("LL55LL55L"); }
} // namespace

TEST_CASE("get_mh encodes well-known locations", "[common][map][maidenhead]")
{
    // ARRL HQ (W1AW), Newington CT: FN31pr
    CHECK(mh(41.714775, -72.727260, 6) == "FN31PR");
    // Munich: JN58td
    CHECK(mh(48.14666, 11.60833, 6) == "JN58TD");
    // Origin of the grid and the point (0, 0)
    CHECK(mh(-90, -180, 6) == "AA00AA");
    CHECK(mh(0, 0, 6) == "JJ00AA");
    // Just inside the far corner
    CHECK(mh(89.999, 179.999, 6) == "RR99XX");
}

TEST_CASE("get_mh output length follows the requested size", "[common][map][maidenhead]")
{
    CHECK(mh(41.714775, -72.727260, 2) == "FN");
    CHECK(mh(41.714775, -72.727260, 4) == "FN31");
    CHECK(mh(41.714775, -72.727260, 6) == "FN31PR");
    CHECK(mh(41.714775, -72.727260, 8).size() == 8);
    CHECK(mh(41.714775, -72.727260, 10).size() == 10);
    CHECK(mh(41.714775, -72.727260, 8).substr(0, 6) == "FN31PR");
    CHECK(mh(41.714775, -72.727260, 10).substr(0, 8) == mh(41.714775, -72.727260, 8));

    // Odd sizes are rounded down, invalid ones fall back to 6
    CHECK(mh(41.714775, -72.727260, 5) == "FN31");
    CHECK(mh(41.714775, -72.727260, 0) == "FN31PR");
    CHECK(mh(41.714775, -72.727260, -3) == "FN31PR");
    CHECK(mh(41.714775, -72.727260, 12) == "FN31PR");
}

TEST_CASE("get_mh extended square digits are correct", "[common][map][maidenhead]")
{
    // 8 characters: the extended square splits a subsquare into 10x10.
    // lon -72.72726 -> 107.27274 east of -180: field F (100), square 3 (6),
    // subsquare P (15/12 = 1.25), remainder 0.02274 / (1/120) = 2.7 -> '2'
    // lat 41.714775 -> 131.714775 north of -90: field N (130), square 1, subsquare R (17/24),
    // remainder 0.006442 / (1/240) = 1.5 -> '1'
    CHECK(mh(41.714775, -72.727260, 8) == "FN31PR21");
}

TEST_CASE("mh2lon and mh2lat decode the south-west corner of full locators", "[common][map][maidenhead]")
{
    CHECK_THAT(lonOf("AA00AA00AA"), WithinAbs(-180, 1e-12));
    CHECK_THAT(latOf("AA00AA00AA"), WithinAbs(-90, 1e-12));
    CHECK_THAT(lonOf("JJ00AA00AA"), WithinAbs(0, 1e-12));
    CHECK_THAT(latOf("JJ00AA00AA"), WithinAbs(0, 1e-12));
    // FN31PR00AA: -180 + 5*20 + 3*2 + 15/12 = -72.75; -90 + 13*10 + 1 + 17/24
    CHECK_THAT(lonOf("FN31PR00AA"), WithinAbs(-72.75, 1e-12));
    CHECK_THAT(latOf("FN31PR00AA"), WithinAbs(41.0 + 17.0 / 24.0, 1e-12));
}

TEST_CASE("mh2lon and mh2lat place short locators inside their square", "[common][map][maidenhead]")
{
    // Missing characters are padded with "LL55LL", i.e. a point near (not exactly at) the centre
    resetPadding();
    double lon = lonOf("FN31");
    double lat = latOf("FN31");
    CHECK(lon > -74);
    CHECK(lon < -72);
    CHECK(lat > 41);
    CHECK(lat < 42);
    CHECK_THAT(lon, WithinAbs(-74 + 11.0 / 12 + 5.0 / 120 + 11.0 / 2880, 1e-12));
    CHECK_THAT(lat, WithinAbs(41 + 11.0 / 24 + 5.0 / 240 + 11.0 / 5760, 1e-12));

    double lon6 = lonOf("FN31PR");
    double lat6 = latOf("FN31PR");
    CHECK(lon6 >= -72.75);
    CHECK(lon6 < -72.75 + 1.0 / 12);
    CHECK(lat6 >= 41 + 17.0 / 24);
    CHECK(lat6 < 41 + 18.0 / 24);
}

TEST_CASE("mh2lon and mh2lat reject over-long locators", "[common][map][maidenhead]")
{
    CHECK(lonOf("FN31PR21AB1") == 0);
    CHECK(latOf("FN31PR21AB1") == 0);
}

TEST_CASE("get_mh then mh2lon/mh2lat round-trips to within one 10-character cell", "[common][map][maidenhead]")
{
    // A 10-character cell is 1/2880 deg of longitude by 1/5760 deg of latitude. The decoded value is
    // the cell's corner, so the error is at most one cell (plus a little for the rounded factors
    // used by get_mh).
    for (double lat = -89.3; lat < 90; lat += 7.77)
    {
        for (double lon = -179.7; lon < 180; lon += 13.13)
        {
            INFO("lat " << lat << " lon " << lon);
            std::string loc = mh(lat, lon, 10);
            REQUIRE(loc.size() == 10);
            double dlon = lon - lonOf(loc);
            double dlat = lat - latOf(loc);
            CHECK(dlon >= -1e-6);
            CHECK(dlon < 1.0 / 2880 + 1e-4);
            CHECK(dlat >= -1e-6);
            CHECK(dlat < 1.0 / 5760 + 1e-4);
        }
    }
}

// Bug: complete_mh (maidenhead.c:80) memcpy's the short locator into a static buffer that is never
// reset to "LL55LL55LL", so characters from a previously decoded longer locator leak into the
// padding of the next short one.
TEST_CASE("mh2lon of a short locator does not depend on previous calls", "[common][map][maidenhead][!shouldfail]")
{
    // JO22 padded with LL55LL: -180 + 9*20 + 2*2 + 11/12 + 5/120 + 11/2880
    const double expected = 4 + 11.0 / 12 + 5.0 / 120 + 11.0 / 2880;
    resetPadding();
    CHECK_THAT(lonOf("JO22"), WithinAbs(expected, 1e-12));
    lonOf("FN31AA");
    CHECK_THAT(lonOf("JO22"), WithinAbs(expected, 1e-12));
}
