#pragma once

#include "angelscript/angelscript.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    namespace script
    {
        namespace
        {
            class CScriptGeodeticCoords
            {
            public:
                // Constructors
                CScriptGeodeticCoords()
                {
                    refCount = 1;
                    p = new geodetic::geodetic_coords_t();
                }

                CScriptGeodeticCoords(double lat, double lon, double alt)
                {
                    refCount = 1;
                    p = new geodetic::geodetic_coords_t(lat, lon, alt);
                }

                CScriptGeodeticCoords(geodetic::geodetic_coords_t i)
                {
                    refCount = 1;
                    p = new geodetic::geodetic_coords_t();
                    *p = i;
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
                CScriptGeodeticCoords &operator=(const CScriptGeodeticCoords &other)
                {
                    *p = *other.p;
                    return *this;
                }

            protected:
                virtual ~CScriptGeodeticCoords() { delete p; }

                mutable int refCount;

            public:
                geodetic::geodetic_coords_t *p;

            public:
                CScriptGeodeticCoords *toRads()
                {
                    p->toRads();
                    return this;
                }

                CScriptGeodeticCoords *toDegs()
                {
                    p->toDegs();
                    return this;
                }
            };
        } // namespace
    } // namespace script
} // namespace satdump