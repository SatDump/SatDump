#include "angelscript/scriptarray/scriptarray.h"
#include "bind_satdump.h"

#include "projection/projection.h"

#include "bind_geodetic.h"
#include "bind_image.h"
#include "bind_json.h"
#include "projection/reproject_img.h"

namespace satdump
{
    namespace script
    {
        namespace
        {
            class CScriptProjection
            {
            public:
                // Constructors
                CScriptProjection()
                {
                    refCount = 1;
                    p = new projection::Projection();
                }

                CScriptProjection(projection::Projection i)
                {
                    refCount = 1;
                    p = new projection::Projection();
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
                CScriptProjection &operator=(const CScriptProjection &other)
                {
                    *p = *other.p;
                    return *this;
                }

            protected:
                virtual ~CScriptProjection() { delete p; }

                mutable int refCount;

            public:
                projection::Projection *p;

            public:
                bool init(bool fwd, bool inv) { return p->init(fwd, inv); }
                bool forward(CScriptGeodeticCoords *pos, double &x, double &y) { return p->forward(*pos->p, x, y); }
                bool inverse(double x, double y, CScriptGeodeticCoords *pos) { return p->inverse(x, y, *pos->p); }

                CScriptProjection &operator=(const CScriptJson &v)
                {
                    *p = *v.j;
                    return *this;
                }

                CScriptJson *opConv_json() { return new CScriptJson(*p); }
            };

            static CScriptProjection *ScriptProjectionFactory() { return new CScriptProjection(); }
        } // namespace

        namespace pro
        {
            CScriptImage *reprojectImage(CScriptImage *img, CScriptProjection *p) { return new CScriptImage(projection::reprojectImage(*img->img, *p->p)); }
            CScriptImage *reprojectImageJ(CScriptImage *img, CScriptJson *p) { return new CScriptImage(projection::reprojectImage(*img->img, *p->j)); }
        } // namespace pro

        void registerProj(asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("proj");

            ////////////////////////////////
            //////// Core Projection Class
            ////////////////////////////////

            // Base type
            engine->RegisterObjectType("Projection", sizeof(CScriptProjection), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_FACTORY, "Projection@ f()", asFUNCTION(ScriptProjectionFactory), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptProjection, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptProjection, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("Projection", "Projection &opAssign(const Projection &in)", asMETHODPR(CScriptProjection, operator=, (const CScriptProjection &), CScriptProjection &),
                                         asCALL_THISCALL);
            engine->RegisterObjectMethod("Projection", "Projection &opAssign(const nlohmann::json &in)", asMETHODPR(CScriptProjection, operator=, (const CScriptJson &), CScriptProjection &),
                                         asCALL_THISCALL);

            engine->RegisterObjectMethod("Projection", "nlohmann::json@ opImplConv()", asMETHOD(CScriptProjection, opConv_json), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod("Projection", "bool init(bool, bool)", asMETHOD(CScriptProjection, init), asCALL_THISCALL);
            engine->RegisterObjectMethod("Projection", "bool forward(geodetic::geodetic_coords_t@, double &out, double &out)", asMETHOD(CScriptProjection, forward), asCALL_THISCALL);
            engine->RegisterObjectMethod("Projection", "bool inverse(double, double, geodetic::geodetic_coords_t@)", asMETHOD(CScriptProjection, inverse), asCALL_THISCALL);

            ////////////////////////////////
            //////// Other funcs
            ////////////////////////////////

            engine->RegisterGlobalFunction("image::Image@ reprojectImage(image::Image &, nlohmann::json &)", asFUNCTION(pro::reprojectImageJ), asCALL_CDECL);
            engine->RegisterGlobalFunction("image::Image@ reprojectImage(image::Image &, Projection &)", asFUNCTION(pro::reprojectImage), asCALL_CDECL);
        }
    } // namespace script
} // namespace satdump