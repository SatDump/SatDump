#include "logger.h"
#include <catch2/catch_session.hpp>

int main(int argc, char *argv[])
{
    // Some of the code under test logs on error paths, so the
    // global logger has to exist before any test runs.
    initLogger();
    logger->set_level(slog::LOG_OFF);

    return Catch::Session().run(argc, argv);
}
