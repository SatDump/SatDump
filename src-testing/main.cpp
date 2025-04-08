/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#if 1
#include "angelscript/angelscript.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
// #include "aatc/aatc.hpp"
// #include "aatc/aatc_container_vector.hpp"
#include "angelscript/scriptarray/scriptarray.h"
#include "angelscript/scriptjson/scriptjson.h"

// Implement a simple message callback function
void MessageCallback(const asSMessageInfo *msg, void *param)
{
    const char *type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING)
        type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION)
        type = "INFO";
    // printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);

    if (msg->type == asMSGTYPE_INFORMATION)
        logger->info("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
    else if (msg->type == asMSGTYPE_WARNING)
        logger->warn("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
    else if (msg->type == asMSGTYPE_ERROR)
        logger->error("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
}

// Print the script string to the standard output stream
void print(std::string &msg)
{
    printf("%s", msg.c_str());
}

#define IMAGELIB_NOINLINE_SETGET 1
#include "common/image/image.h"
#include "common/image/io.h"

namespace img
{
    void ImageConstructor(void *memory)
    {
        // Initialize the pre-allocated memory by calling the
        // object constructor with the placement-new operator
        new (memory) image::Image();
    }

    void ImageDestructor(void *memory)
    {
        logger->error("Destruct %d", ((image::Image *)memory)->size());
        // Uninitialize the memory by calling the object destructor
        ((image::Image *)memory)->~Image();
    }

    /*void ImageSet(void *memory, int p, int v)
    {
        ((image::Image *)memory)->set(p, v);
    }

    int ImageGet(void *memory, int p)
    {
        return ((image::Image *)memory)->get(p);
    }*/

    void ImageDrawCircle(void *memory, int x0, int y0, int radius, CScriptArray *color, bool fill)
    {
        ((image::Image *)memory)->draw_circle(x0, y0, radius, std::vector<double>((double *)color->At(0), (double *)color->At(0) + color->GetSize()), fill);
    }
}

namespace namelog
{
    void trace(std::string &f) { logger->trace("[AngelScript] " + f); }
    void debug(std::string &f) { logger->debug("[AngelScript] " + f); }
    void info(std::string &f) { logger->info("[AngelScript] " + f); }
    void warn(std::string &f) { logger->warn("[AngelScript] " + f); }
    void error(std::string &f) { logger->error("[AngelScript] " + f); }
    void critical(std::string &f) { logger->critical("[AngelScript] " + f); }
}

#include "nlohmann/json.hpp"

namespace j
{
    void JConstructor(void *memory)
    {
        // Initialize the pre-allocated memory by calling the
        // object constructor with the placement-new operator
        new (memory) nlohmann::basic_json<>();
    }

    void JDestructor(void *memory)
    {
        // Uninitialize the memory by calling the object destructor
        ((nlohmann::basic_json<> *)memory)->~basic_json();
    }
}
#endif

int main(int argc, char *argv[])
{
#if 1
    initLogger();

    // Create the script engine
    asIScriptEngine *engine = asCreateScriptEngine();

    // Set the message callback to receive information on errors in human readable form.
    int r = engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
    //    assert(r >= 0);

    // AngelScript doesn't have a built-in string type, as there is no definite standard
    // string type for C++ applications. Every developer is free to register its own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to implement the registration yourself if you don't want to.
    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterScriptJson(engine);
    // aatc::RegisterAllContainers(engine);

    // Register the function that we want the scripts to call
    r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);
    //  assert(r >= 0);

    {
        engine->SetDefaultNamespace("logger");
        r = engine->RegisterGlobalFunction("void trace(const string &in)", asFUNCTION(namelog::trace), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void debug(const string &in)", asFUNCTION(namelog::debug), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void info(const string &in)", asFUNCTION(namelog::info), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void warn(const string &in)", asFUNCTION(namelog::error), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void error(const string &in)", asFUNCTION(namelog::error), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void critical(const string &in)", asFUNCTION(namelog::critical), asCALL_CDECL);
    }

    {
        engine->SetDefaultNamespace("image");

        r = engine->RegisterObjectType("Image", sizeof(image::Image), asOBJ_VALUE | asGetTypeTraits<image::Image>());
        //        assert(r >= 0);
        // Register the behaviours
        r = engine->RegisterObjectBehaviour("Image", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(img::ImageConstructor), asCALL_CDECL_OBJLAST);
        // assert(r >= 0);
        r = engine->RegisterObjectBehaviour("Image", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(img::ImageDestructor), asCALL_CDECL_OBJLAST);
        //  assert(r >= 0);

        r = engine->RegisterObjectMethod("Image", "void init(int, uint64, uint64, int)", asMETHOD(image::Image, init), asCALL_THISCALL); // assert( r >= 0 );
        r = engine->RegisterObjectMethod("Image", "void fill(int)", asMETHOD(image::Image, fill), asCALL_THISCALL);                      // assert( r >= 0 );
        r = engine->RegisterObjectMethod("Image", "uint64 size()", asMETHOD(image::Image, size), asCALL_THISCALL);

        r = engine->RegisterObjectMethod("Image", "void set(uint64, int)", asMETHODPR(image::Image, set, (size_t, int), void), asCALL_THISCALL);
        r = engine->RegisterObjectMethod("Image", "void set(uint64, uint64, int)", asMETHODPR(image::Image, set, (size_t, size_t, int), void), asCALL_THISCALL);
        r = engine->RegisterObjectMethod("Image", "void set(uint64, uint64, uint64, int)", asMETHODPR(image::Image, set, (size_t, size_t, size_t, int), void), asCALL_THISCALL);
        r = engine->RegisterObjectMethod("Image", "int get(uint64)", asMETHODPR(image::Image, get, (size_t), int), asCALL_THISCALL);
        r = engine->RegisterObjectMethod("Image", "int get(uint64, uint64)", asMETHODPR(image::Image, get, (size_t, size_t), int), asCALL_THISCALL);
        r = engine->RegisterObjectMethod("Image", "int get(uint64, uint64, uint64)", asMETHODPR(image::Image, get, (size_t, size_t, size_t), int), asCALL_THISCALL);

        r = engine->RegisterObjectMethod("Image", "void draw_circle(int, int, int, array<double>@, bool)", asFUNCTION(img::ImageDrawCircle), asCALL_CDECL_OBJFIRST);

        engine->RegisterObjectMethod("Image", "Image &opAssign(const Image &in)", asMETHODPR(image::Image, operator=, (const image::Image &), image::Image &), asCALL_THISCALL);

        // r = engine->RegisterGlobalFunction("Image load_img(string)", asFUNCTIONPR(image::load_img, (std::string), image::Image), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void load_img(Image &out, string)", asFUNCTIONPR(image::load_img, (image::Image &, std::string), void), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void save_img(const Image &in, string, bool)", asFUNCTIONPR(image::save_img, (image::Image &, std::string, bool), void), asCALL_CDECL);
    }

    nlohmann::json test2j;
    test2j["hello"] = true;

    {

        engine->RegisterGlobalProperty("json testj", &test2j);
        // engine->SetDefaultNamespace("nlohmann");
        // r = engine->RegisterObjectType("json", sizeof(nlohmann::json), asOBJ_VALUE | asGetTypeTraits<nlohmann::json>());

        // r = engine->RegisterObjectBehaviour("json", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(j::JConstructor), asCALL_CDECL_OBJLAST);
        // r = engine->RegisterObjectBehaviour("json", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(j::JDestructor), asCALL_CDECL_OBJLAST);

        // engine->RegisterObjectMethod("json", "json &opAssign(const json &in)", asMETHODPR(nlohmann::json, operator=, (const nlohmann::json &), nlohmann::json &), asCALL_THISCALL);
    }

    ////////////////////////////////////////

    {
        // The CScriptBuilder helper is an add-on that loads the file,
        // performs a pre-processing pass if necessary, and then tells
        // the engine to build a script module.
        CScriptBuilder builder;
        int r = builder.StartNewModule(engine, "MyModule");
        if (r < 0)
        {
            // If the code fails here it is usually because there
            // is no more memory to allocate the module
            printf("Unrecoverable error while starting a new module.\n");
            return 1;
        }
        r = builder.AddSectionFromFile("test.as");
        if (r < 0)
        {
            // The builder wasn't able to load the file. Maybe the file
            // has been removed, or the wrong name was given, or some
            // preprocessing commands are incorrectly written.
            printf("Please correct the errors in the script and try again.\n");
            return 1;
        }
        r = builder.BuildModule();
        if (r < 0)
        {
            // An error occurred. Instruct the script writer to fix the
            // compilation errors that were listed in the output stream.
            printf("Please correct the errors in the script and try again.\n");
            return 1;
        }

        //////////////

        // Find the function that is to be called.
        asIScriptModule *mod = engine->GetModule("MyModule");
        asIScriptFunction *func = mod->GetFunctionByDecl("void main()");
        if (func == 0)
        {
            // The function couldn't be found. Instruct the script writer
            // to include the expected function in the script.
            printf("The script must have the function 'void main()'. Please add it and try again.\n");
            return 1;
        }

        // Create our context, prepare it, and then execute
        asIScriptContext *ctx = engine->CreateContext();
        ctx->Prepare(func);
        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED)
        {
            // The execution didn't complete as expected. Determine what happened.
            if (r == asEXECUTION_EXCEPTION)
            {
                // An exception occurred, let the script writer know what happened so it can be corrected.
                printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
            }
        }

        // Clean up
        ctx->Release();
        engine->ShutDownAndRelease();
    }
#endif
}
