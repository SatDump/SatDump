/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "handlers/experimental/decoupled/rec_backend.h"
#include "handlers/experimental/decoupled/rec_frontend.h"
#include "handlers/experimental/decoupled/test/test_http.h"
#include "init.h"
#include "logger.h"
#include <cmath>

int main(int argc, char *argv[])
{
    initLogger();
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatDump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    auto h = std::make_shared<satdump::handlers::RecFrontendHandler>(std::make_shared<satdump::handlers::TestHttpBackend>(std::make_shared<satdump::handlers::RecBackend>()));

    while (1)
        ;
}
