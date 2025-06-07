#pragma once

// namespace satdump
// {
//     namespace pipeline
//     {
enum instrument_status_t
{
    DECODING,
    PROCESSING,
    SAVING,
    DONE,
};

void drawStatus(instrument_status_t status);
//     } // namespace pipeline
// } // namespace satdump