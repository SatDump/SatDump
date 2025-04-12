#pragma once

#include "bind_satdump.h"
#include "angelscript/scriptarray/scriptarray.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace script
    {
        namespace
        {
            class CScriptJson
            {
            public:
                // Constructors
                CScriptJson()
                {
                    refCount = 1;
                    j = new nlohmann::json();
                }

                CScriptJson(nlohmann::json j)
                {
                    refCount = 1;
                    this->j = new nlohmann::json();
                    *this->j = j;
                }

                CScriptJson(nlohmann::json *j)
                {
                    refCount = 1;
                    do_free = false;
                    this->j = j;
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

            public:
                // Copy the stored value from another any object
                CScriptJson &operator=(const CScriptJson &other)
                {
                    *j = *other.j;
                    return *this;
                }

                CScriptJson &operator=(const bool &v)
                {
                    *j = v;
                    return *this;
                }

                CScriptJson &operator=(const int &v)
                {
                    *j = v;
                    return *this;
                }

                CScriptJson &operator=(const double &v)
                {
                    *j = v;
                    return *this;
                }

                CScriptJson &operator=(const std::string &v)
                {
                    *j = v;
                    return *this;
                }

            public:
                bool opConv_bool() { return *j; }
                int opConv_int() { return *j; }
                double opConv_double() { return *j; }
                std::string opConv_string() { return *j; }

            public:
                CScriptJson *opIndex(std::string k) { return new CScriptJson(&(*j)[k]); }
                CScriptJson *opIndex(int k) { return new CScriptJson(&(*j)[k]); }

            protected:
                virtual ~CScriptJson()
                {
                    if (do_free)
                        delete j;
                }

                mutable int refCount;

            public:
                bool do_free = true;
                nlohmann::json *j;

            public:
                std::string dump(int v) { return (*j).dump(v); }
            };

            static CScriptJson *ScriptJsonFactory() { return new CScriptJson(); }
        }
    }
}