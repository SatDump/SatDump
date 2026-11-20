#pragma once

/*
In-memory Kepler (TLE) database, shared by the whole suite. Everything is
inline and lives in the test_helpers namespace, as every test file is linked
into the same executable.

Code looking TLEs up (pass prediction, the object tracker, the autotrack
scheduler...) reads the global satdump::db_keplers, which is null in the
tests. KeplerDbFixture points it at an in-memory SQLite database holding only
the elements a test adds, and restores the previous handler afterwards. Use
it as a local variable or through TEST_CASE_METHOD.
*/

#include "db/db_handler.h"
#include "db/kepler/kepler_handler.h"
#include "init.h"
#include <memory>
#include <stdexcept>

namespace test_helpers
{
    // Points satdump::db_keplers at an in-memory database, restores the previous handler on destruction
    class KeplerDbFixture
    {
    private:
        std::shared_ptr<satdump::KeplerDBHandler> previous;
        std::shared_ptr<satdump::DBHandler> db;

    public:
        KeplerDbFixture()
        {
            previous = satdump::db_keplers;
            db = std::make_shared<satdump::DBHandler>(":memory:");
            db->init();
            // Same schema as KeplerDBHandler::init(), which cannot be called here because it
            // needs the SatDump config and task scheduler
            if (db->run_sql("CREATE TABLE IF NOT EXISTS kepler("
                            "id TEXT PRIMARY KEY NOT NULL, satellite_number INT NOT NULL, element_number INT NOT NULL,"
                            "name TEXT NOT NULL, designator TEXT NOT NULL, epoch REAL NOT NULL, inclination REAL NOT NULL,"
                            "right_ascension REAL NOT NULL, eccentricity REAL NOT NULL, argument_of_perigee REAL NOT NULL,"
                            "mean_anomaly REAL NOT NULL, mean_motion REAL NOT NULL, derivative_mean_motion REAL NOT NULL,"
                            "second_derivative_mean_motion REAL NOT NULL, bstar_drag_term REAL NOT NULL, revolutions_at_epoch INT NOT NULL);"))
                throw std::runtime_error("Could not create the test Kepler table");
            satdump::db_keplers = std::make_shared<satdump::KeplerDBHandler>(db);
        }

        ~KeplerDbFixture() { satdump::db_keplers = previous; }

        void add(satdump::KeplerData kep) { satdump::db_keplers->putKepler(kep); }

        // The TLE getPassesForSatellite will actually use (regenerated from the stored elements)
        satdump::TLE stored(int norad)
        {
            auto tle = satdump::db_keplers->get_from_norad(norad);
            if (!tle.has_value())
                throw std::runtime_error("Test TLE missing from the database");
            return *tle;
        }
    };
} // namespace test_helpers
