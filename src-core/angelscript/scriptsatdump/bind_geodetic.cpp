#include "bind_satdump.h"
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
                virtual ~CScriptGeodeticCoords()
                {
                    delete p;
                }

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

            static CScriptGeodeticCoords *ScriptGeodecticCoordsFactory() { return new CScriptGeodeticCoords(); }
            static CScriptGeodeticCoords *ScriptGeodecticCoordsFactory2(double lat, double lon, double alt) { auto v=new CScriptGeodeticCoords(); return v;}
        }

        namespace pro
        {

        }

        void registerGeodetic(asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("geodetic");

            ////////////////////////////////
            //////// Core Image Class
            ////////////////////////////////

            // Base type
            engine->RegisterObjectType("geodetic_coords_t", sizeof(CScriptGeodeticCoords), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_FACTORY, "geodetic_coords_t@ f()", asFUNCTION(ScriptGeodecticCoordsFactory), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptGeodeticCoords, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptGeodeticCoords, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t &opAssign(const geodetic_coords_t &in)", asMETHODPR(CScriptGeodeticCoords, operator=, (const CScriptGeodeticCoords &), CScriptGeodeticCoords &), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t@ toRads()", asMETHOD(CScriptGeodeticCoords, toRads), asCALL_THISCALL);
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t@ toDegs()", asMETHOD(CScriptGeodeticCoords, toDegs), asCALL_THISCALL);
        }
    }
}