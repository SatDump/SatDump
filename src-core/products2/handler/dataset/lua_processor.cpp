#include "lua_processor.h"
#include "libs/sol2/sol.hpp"
#include "logger.h"

#include "lua/lua_bind.h"

namespace satdump
{
    namespace viewer
    {
        Lua_DatasetProductProcessor::Lua_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p)
            : DatasetProductProcessor(dh, dp, p)
        {
            editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
            if (p.contains("lua"))
                editor.SetText(p["lua"].get<std::string>());
        }

        nlohmann::json Lua_DatasetProductProcessor::getCfg()
        {
            nlohmann::json cfg;
            cfg["lua"] = editor.GetText();
            cfg["cfg"] = params;
            return cfg;
        }

        bool Lua_DatasetProductProcessor::can_process()
        {
            return true; // TODOREWORK
        }

        void Lua_DatasetProductProcessor::process(float *progress)
        {
            std::string lua_code = editor.GetText();

            try
            {
                sol::state lua;

                lua.open_libraries(sol::lib::base);
                lua.open_libraries(sol::lib::string);
                lua.open_libraries(sol::lib::math);
                lua.open_libraries(sol::lib::table);
                lua.open_libraries(sol::lib::io);

                lua::bind_logger(lua);
                lua::bind_image(lua);
                lua::bind_product(lua);
                lua::bind_geodetic(lua);
                lua::bind_projection(lua);
                lua::bind_handler_image(lua);

                // Experimental
                lua["add_handler_to_products"] = [this](std::shared_ptr<Handler> p) // TODO, find why THIS specifically crashes...
                {
                    add_handler_to_products(p);
                };

                lua["get_instrument_products"] = [this](std::string v, int index)
                {
                    return get_instrument_products(v, index);
                };

                // Run
                lua.script(lua_code);
            }
            catch (sol::error &e)
            {
                logger->error(e.what());
            }
        }

        void Lua_DatasetProductProcessor::renderUI()
        {
            editor.Render("LuaEditor");
        }
    }
}