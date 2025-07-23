
#pragma once

#include "bind_json.h"
#include "products/product.h"

namespace satdump
{
    namespace script
    {
        class CScriptProduct
        {
        public:
            // Constructors
            CScriptProduct()
            {
                refCount = 1;
                p = std::make_shared<products::Product>();
                j = new CScriptJson(&p->contents);
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

        protected:
            virtual ~CScriptProduct() { j->Release(); }

            mutable int refCount;

        public:
            std::shared_ptr<products::Product> p;
            CScriptJson *j;

        public:
            void set_product_timestamp(double t) { p->set_product_timestamp(t); }
            bool has_product_timestamp() { return p->has_product_timestamp(); }
            double get_product_timestamp() { return p->get_product_timestamp(); }

            void set_product_source(std::string t) { p->set_product_source(t); }
            bool has_product_source() { return p->has_product_source(); }
            std::string get_product_source() { return p->get_product_source(); }

            void save(std::string d) { return p->save(d); }
            void load(std::string d) { return p->load(d); }
        };

        template <typename T>
        static T *ScriptProductFactory()
        {
            return new T();
        }

        template <typename T>
        void registerProduct(std::string name, asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("products");

            ////////////////////////////////
            //////// Core Product Class
            ////////////////////////////////

            // Base type
            engine->RegisterObjectType(name.c_str(), sizeof(T), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_FACTORY, (name + "@ f()").c_str(), asFUNCTION(ScriptProductFactory<T>), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);

            // Other funcs
            engine->RegisterObjectMethod(name.c_str(), "void set_product_timestamp(double)", asMETHOD(T, set_product_timestamp), asCALL_THISCALL);
            engine->RegisterObjectMethod(name.c_str(), "bool has_product_timestamp()", asMETHOD(T, has_product_timestamp), asCALL_THISCALL);
            engine->RegisterObjectMethod(name.c_str(), "double get_product_timestamp()", asMETHOD(T, get_product_timestamp), asCALL_THISCALL);

            engine->RegisterObjectMethod(name.c_str(), "void set_product_source(string)", asMETHOD(T, set_product_source), asCALL_THISCALL);
            engine->RegisterObjectMethod(name.c_str(), "bool has_product_source()", asMETHOD(T, has_product_source), asCALL_THISCALL);
            engine->RegisterObjectMethod(name.c_str(), "string get_product_source()", asMETHOD(T, get_product_source), asCALL_THISCALL);

            engine->RegisterObjectMethod(name.c_str(), "void save(string &in)", asMETHOD(T, save), asCALL_THISCALL);
            engine->RegisterObjectMethod(name.c_str(), "void load(string &in)", asMETHOD(T, load), asCALL_THISCALL);

            // Values
            engine->RegisterObjectProperty(name.c_str(), "nlohmann::json@ contents", asOFFSET(T, j), false);
            engine->RegisterObjectProperty(name.c_str(), "string instrument_name", asOFFSET(products::Product, instrument_name), asOFFSET(T, p), true);
            engine->RegisterObjectProperty(name.c_str(), "string type", asOFFSET(products::Product, type), asOFFSET(T, p), true);
        }
    } // namespace script
} // namespace satdump