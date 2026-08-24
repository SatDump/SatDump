#pragma once

#include "elektro_arktika/ggak/ggak.h"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKMerger
        {
        private:
            enum state_t
            {
                RESET = 0,
                SYNCED = 1,
                WAIT = 2,
            };

        private:
            const int WINDOW_SIZE = 20;

        private:
            state_t state = RESET;
            uint16_t last_valid_frame_ctr;
            std::vector<GGAKFrame> recovery_queue;
            int num_not_in_window = 0;

        private:
            bool is_in_window(uint16_t ctr)
            {
                for (uint16_t i = 0; i < WINDOW_SIZE; i++)
                    if (uint16_t(last_valid_frame_ctr + i) == ctr)
                        return true;
                return false;
            }

        public:
            std::vector<GGAKFrame> process(GGAKFrame frm)
            {
                // Check CRC
                if (!checkCRC(&frm))
                    return {};

                // Skip filler, bad counter
                if (frm.id == 119)
                    return {};

                std::vector<GGAKFrame> ofrms;

                // State : RESET
                // In this state, we are simply syncing on
                // the first frame that arrives.
                if (state == RESET)
                {
                    ofrms.push_back(frm);
                    last_valid_frame_ctr = frm.master_counter;
                    state = SYNCED;
                }
                // State : SYNCED
                // In this state, we simply validate the frames
                // that arrive are in the expected window and in
                // order, without duplicates.
                else if (state == SYNCED)
                {
                    // Skip duplicates
                    if (last_valid_frame_ctr == frm.master_counter)
                        return ofrms;

                    // Check it's the frame we expected, and if it is, carry on
                    if (uint16_t(last_valid_frame_ctr + 1) == frm.master_counter)
                    {
                        ofrms.push_back(frm);
                        last_valid_frame_ctr = frm.master_counter;
                    }
                    // Otherwise, if check if the frame is in the "recovery" window, and buffer this frame
                    else if (is_in_window(frm.master_counter))
                    {
                        recovery_queue.clear();
                        num_not_in_window = 0;
                        recovery_queue.push_back(frm);
                        state = WAIT;
                    }
                    // And at last, if it's entirely out of the window, ignore.
                    else
                    {
                    }
                }
                // State : WAIT
                // Here the idea is to wait and see if we do get the missing frame
                // in order to go back to SYNC, or run out of time and go
                // into RESET.
                else if (state == WAIT)
                {
                    // Push into queue if we're in the window
                    if (is_in_window(frm.master_counter))
                    {
                        recovery_queue.push_back(frm);
                        num_not_in_window = 0;

                        // If the queue is over the window size, reset!
                        if (recovery_queue.size() > WINDOW_SIZE)
                        {
                            recovery_queue.clear();
                            state = RESET;
                        }
                        // Otherwise, check if we got the frame we were expecting
                        else
                        {
                        retry:
                            for (int i = 0; i < recovery_queue.size(); i++)
                            {
                                // If we find it, push it
                                if (recovery_queue[i].master_counter == uint16_t(last_valid_frame_ctr + 1))
                                {
                                    ofrms.push_back(recovery_queue[i]);
                                    last_valid_frame_ctr = recovery_queue[i].master_counter;

                                    std::remove_if(recovery_queue.begin(), recovery_queue.end(), [this](auto &v) { return v.master_counter == last_valid_frame_ctr; });

                                    goto retry;
                                }
                            }

                            // After a few tries, the queue should end up empty
                            if (recovery_queue.size() == 0)
                                state = SYNCED;
                        }
                    }
                    // Otherwise increase invalid counter.
                    else
                    {
                        num_not_in_window++;
                        if (num_not_in_window > WINDOW_SIZE)
                        {
                            recovery_queue.clear();
                            num_not_in_window = 0;
                            state = RESET;
                        }
                    }
                }

                return ofrms;
            }

            int getState() { return state; }
        };
    } // namespace ggak
} // namespace elektro_arktika