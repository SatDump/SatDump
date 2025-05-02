#include "bind_geodetic.h"
#include "bind_satdump.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    namespace script
    {
        namespace
        {
            static CScriptGeodeticCoords *ScriptGeodecticCoordsFactory() { return new CScriptGeodeticCoords(); }
            static CScriptGeodeticCoords *ScriptGeodecticCoordsFactory2(double lat, double lon, double alt) { return new CScriptGeodeticCoords(lat, lon, alt); }
        } // namespace

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
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_FACTORY, "geodetic_coords_t@ f(double, double, double)", asFUNCTION(ScriptGeodecticCoordsFactory2), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptGeodeticCoords, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("geodetic_coords_t", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptGeodeticCoords, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t &opAssign(const geodetic_coords_t &in)",
                                         asMETHODPR(CScriptGeodeticCoords, operator=, (const CScriptGeodeticCoords &), CScriptGeodeticCoords &), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t@ toRads()", asMETHOD(CScriptGeodeticCoords, toRads), asCALL_THISCALL);
            engine->RegisterObjectMethod("geodetic_coords_t", "geodetic_coords_t@ toDegs()", asMETHOD(CScriptGeodeticCoords, toDegs), asCALL_THISCALL);

            // Values
            engine->RegisterObjectProperty("geodetic_coords_t", "double lat", asOFFSET(geodetic::geodetic_coords_t, lat), asOFFSET(CScriptGeodeticCoords, p), true);
            engine->RegisterObjectProperty("geodetic_coords_t", "double lon", asOFFSET(geodetic::geodetic_coords_t, lon), asOFFSET(CScriptGeodeticCoords, p), true);
            engine->RegisterObjectProperty("geodetic_coords_t", "double alt", asOFFSET(geodetic::geodetic_coords_t, alt), asOFFSET(CScriptGeodeticCoords, p), true);
        }
    } // namespace script
} // namespace satdump