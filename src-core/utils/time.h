#pragma once

/**
 * @file time.h
 * @brief A collection of time-related utility functions
 */

#include <chrono>
#include <string>

namespace satdump
{
    /**
     * @brief Get high-precision current UNIX
     * time (milliseconds-level).
     *
     * @return UNIX timestamps in seconds
     */
    double getTime();

    /**
     * @brief Format an unit timestamp to
     * string.
     *
     * @param timestamp UNIX Timestamps in seconds
     * @param local format to local time. UTC (false)
     * by default
     * @return String in YYYY/MM/DD HH:MM:SS format
     */
    std::string timestamp_to_string(double timestamp, bool local = false);

    /**
     * @brief Get a UNIX Timestamp
     * in seconds from a formatted
     * timestamp in a filename.
     *
     * @param filename Entire file name string
     * @return timestamp Double in UNIX Timestamps in seconds
     */
    double timestamp_from_filename(std::string filename);

} // namespace satdump
