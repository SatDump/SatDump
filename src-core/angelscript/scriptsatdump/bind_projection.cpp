#include "bind_satdump.h"
#include "angelscript/scriptarray/scriptarray.h"

#include "projection/projection.h"

#include "bind_json.h"

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
                    p = new proj::Projection();
                }

                CScriptProjection(proj::Projection i)
                {
                    refCount = 1;
                    p = new proj::Projection();
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
                virtual ~CScriptProjection()
                {
                    delete p;
                }

                mutable int refCount;

            public:
                proj::Projection *p;

            public:
                bool init(bool fwd, bool inv) { return p->init(fwd, inv); }
                bool forward(geodetic::geodetic_coords_t pos, double &x, double &y) { return p->forward(pos, x, y); }

                CScriptProjection &operator=(const CScriptJson &v)
                {
                    *p = *v.j;
                    return *this;
                }

                CScriptJson *opConv_json() { return new CScriptJson(*p); }
            };

            static CScriptProjection *ScriptProjectionFactory() { return new CScriptProjection(); }
        }

        namespace pro
        {

        }

        void registerProj(asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("proj");

            ////////////////////////////////
            //////// Core Image Class
            ////////////////////////////////

            // Base type
            engine->RegisterObjectType("Projection", sizeof(CScriptProjection), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_FACTORY, "Projection@ f()", asFUNCTION(ScriptProjectionFactory), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptProjection, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("Projection", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptProjection, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("Projection", "Projection &opAssign(const Projection &in)", asMETHODPR(CScriptProjection, operator=, (const CScriptProjection &), CScriptProjection &), asCALL_THISCALL);
            engine->RegisterObjectMethod("Projection", "Projection &opAssign(const nlohmann::json &in)", asMETHODPR(CScriptProjection, operator=, (const CScriptJson &), CScriptProjection &), asCALL_THISCALL);

            engine->RegisterObjectMethod("Projection", "nlohmann::json@ opImplConv()", asMETHOD(CScriptProjection, opConv_json), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod("Projection", "bool init(bool, bool)", asMETHOD(CScriptProjection, init), asCALL_THISCALL);
        }
    }
}