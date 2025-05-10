#include "bind_json.h"
#include "nlohmann/json_utils.h"

namespace satdump
{
    namespace script
    {
        namespace json
        {
            CScriptJson *parse(std::string str) { return new CScriptJson(nlohmann::json::parse(str)); }
            CScriptJson *loadJsonFile(std::string str) { return new CScriptJson(::loadJsonFile(str)); }
        } // namespace json

        void registerJson(asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("nlohmann");

            // Base type
            engine->RegisterObjectType("json", sizeof(CScriptJson), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour("json", asBEHAVE_FACTORY, "json@ f()", asFUNCTION(ScriptJsonFactory), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("json", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptJson, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("json", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptJson, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("json", "json &opAssign(const json &in)", asMETHODPR(CScriptJson, operator=, (const CScriptJson &), CScriptJson &), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "json &opAssign(const bool &in)", asMETHODPR(CScriptJson, operator=, (const bool &), CScriptJson &), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "json &opAssign(const double &in)", asMETHODPR(CScriptJson, operator=, (const double &), CScriptJson &), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "json &opAssign(const string &in)", asMETHODPR(CScriptJson, operator=, (const std::string &), CScriptJson &), asCALL_THISCALL);

            // Conversion operators
            engine->RegisterObjectMethod("json", "bool opImplConv()", asMETHOD(CScriptJson, opConv_bool), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "int opImplConv()", asMETHOD(CScriptJson, opConv_int), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "double opImplConv()", asMETHOD(CScriptJson, opConv_double), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "string opImplConv()", asMETHOD(CScriptJson, opConv_string), asCALL_THISCALL);

            // Index operator
            engine->RegisterObjectMethod("json", "json& opIndex(const string &in)", asMETHODPR(CScriptJson, opIndex, (std::string), CScriptJson *), asCALL_THISCALL);
            engine->RegisterObjectMethod("json", "json& opIndex(const int)", asMETHODPR(CScriptJson, opIndex, (int), CScriptJson *), asCALL_THISCALL);

            // Dump
            engine->RegisterObjectMethod("json", "string dump(int)", asMETHOD(CScriptJson, dump), asCALL_THISCALL);
            engine->RegisterGlobalFunction("json@ parse(string)", asFUNCTION(json::parse), asCALL_CDECL);

            // Load
            engine->RegisterGlobalFunction("json@ loadJsonFile(string)", asFUNCTION(json::loadJsonFile), asCALL_CDECL);
        }
    } // namespace script
} // namespace satdump