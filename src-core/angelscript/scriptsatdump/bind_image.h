#pragma once

#include "angelscript/angelscript.h"
#include "common/image/image.h"
namespace satdump
{
    namespace script
    {
        namespace
        {
            class CScriptImage
            {
            public:
                // Constructors
                CScriptImage()
                {
                    refCount = 1;
                    img = new image::Image();
                }

                CScriptImage(int bit_depth, size_t width, size_t height, int channels)
                {
                    refCount = 1;
                    img = new image::Image(bit_depth, width, height, channels);
                }

                CScriptImage(image::Image i)
                {
                    refCount = 1;
                    img = new image::Image();
                    *img = i;
                }

                // Memory management
                int AddRef() const
                {
                    // Increase counter
                    return asAtomicInc(refCount);
                }

                int Release() const
                {
                    // Decrease the ref counter
                    if (asAtomicDec(refCount) == 0)
                    {
                        // Delete this object as no more references to it exists
                        delete this;
                        return 0;
                    }

                    return refCount;
                }

                // Copy the stored value from another any object
                CScriptImage &operator=(const CScriptImage &other)
                {
                    *img = *other.img;
                    return *this;
                }

            protected:
                virtual ~CScriptImage() { delete img; }

                mutable int refCount;

            public:
                image::Image *img;
            };

            static CScriptImage *ScriptImageFactory() { return new CScriptImage(); }
            static CScriptImage *ScriptImageFactory2(int bit_depth, size_t width, size_t height, int channels) { return new CScriptImage(bit_depth, width, height, channels); }
        } // namespace
    } // namespace script
} // namespace satdump