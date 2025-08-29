#pragma once

/**
 * @file dsp_buffer.h
 */

#include "dll_export.h"
#include <cstddef>
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief DSP Buffer content type
         *
         * - DSP_BUFFER_TYPE_TERMINATOR_PROPAGATING : Tells the following block to stop,
         * while also sending terminators to following blocks to terminate the entire
         * processing chain.
         * - DSP_BUFFER_TYPE_TERMINATOR_NON_PROPAGATING : Only stops the following blocks,
         * leaving the rest hanging and ready to ingest more buffers.
         * - DSP_BUFFER_TYPE_SAMPLES : Buffer containing actual samples.
         * - DSP_BUFFER_TYPE_INVALID : Only useful to catch an error commmited by the "thing"
         * between the keyboard and the chair :-)
         */
        enum dsp_buffer_type_t
        {
            DSP_BUFFER_TYPE_TERMINATOR_PROPAGATING = 0,
            DSP_BUFFER_TYPE_TERMINATOR_NON_PROPAGATING = 1,
            DSP_BUFFER_TYPE_SAMPLES = 2,
            DSP_BUFFER_TYPE_INVALID = 255,
        };

        SATDUMP_DLL extern const size_t volk_alignment;

        /**
         * @brief DSP Buffer class
         *
         * This is a simple helper class to make it easier to manage sample
         * buffers used in blocks. It holds a pointer with raw samples, the
         * size of said buffer as well as actual useful sample count.
         *
         * @param type of the buffer
         * @param typesize size of the type, (eg, float = 4 bytes)
         * @param max_size size of the buffer, in number of elements. This is
         * the size of the allocated buffer, NOT sample count!
         * @param size number of actual samples
         */
        class DSPBuffer
        {
        private:
            void *ptr = nullptr;
            size_t ptr_size = 0;

        public:
            dsp_buffer_type_t type = DSP_BUFFER_TYPE_INVALID;
            uint8_t typesize = 0;
            uint32_t max_size = 0;
            uint32_t size = 0;

        public:
            friend class DSPStream;

        public:
            /**
             * @brief Get sample pointer
             * @return sample pointer
             */
            template <typename T>
            inline T *getSamples()
            {
                return (T *)ptr;
            }

            /**
             * @brief Checks if this buffer is a terminator.
             * This covers BOTH propagating and normal terminator markers.
             * @return true if terminator
             */
            inline bool isTerminator() { return type == DSP_BUFFER_TYPE_TERMINATOR_PROPAGATING || type == DSP_BUFFER_TYPE_TERMINATOR_NON_PROPAGATING; }

            /**
             * @brief Checks if this buffer is a propagating terminator.
             * If it is, a propagating terminator should be sent down to
             * the next blocks as well!
             * @return true if propagating
             */
            inline bool terminatorShouldPropagate() { return type == DSP_BUFFER_TYPE_TERMINATOR_PROPAGATING; }

            // /**
            //  * @brief Frees the pointer. This MUST be called once you're
            //  * done doing your DSP with this buffer! Otherwise things will
            //  * leak. BADLY leak!
            //  *
            //  * NOTE TODOREWORK : probably only needed for SAMPLES buffers?
            //  */
            // inline void free() { volk_free(ptr); }

            // public:
            //     /**
            //      * @brief Create a terminator buffer
            //      * @param prop Whether this terminator should be propagating
            //      * @return the new DSP buffer
            //      */
            //     static DSPBuffer newBufferTerminator(bool prop = true)
            //     {
            //         DSPBuffer b;
            //         b.type = prop ? DSP_BUFFER_TYPE_TERMINATOR_PROPAGATING : DSP_BUFFER_TYPE_TERMINATOR_NON_PROPAGATING;
            //         return b;
            //     }

            //     /**
            //      * @brief Create a sample buffer
            //      * @param size size of the new buffer (max_size,
            //      * size is left at 0)
            //      * @return the new DSP buffer
            //      */
            //     template <typename T>
            //     static DSPBuffer newBufferSamples(uint32_t size)
            //     {
            //         DSPBuffer b;
            //         b.type = DSP_BUFFER_TYPE_SAMPLES;
            //         b.max_size = size;
            //         b.ptr_size = size * sizeof(T);
            //         b.ptr = volk_malloc(b.ptr_size, volk_alignment);
            //         return b;
            //     }
        };
    } // namespace ndsp
} // namespace satdump