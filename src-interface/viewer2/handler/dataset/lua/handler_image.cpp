#include "lua_bind.h"
#include "../../image/image_handler.h"

SOL_BASE_CLASSES(satdump::viewer::ImageHandler, satdump::viewer::Handler);
SOL_DERIVED_CLASSES(satdump::viewer::Handler, satdump::viewer::ImageHandler);

namespace satdump
{
    namespace lua
    {
        void bind_handler_image(sol::state &lua)
        {
            sol::usertype<viewer::ImageHandler> image_handler_type = lua.new_usertype<viewer::ImageHandler>("ImageHandler",
                                                                                                            // sol::constructors<std::shared_ptr<ImageHandler>(), std::shared_ptr<ImageHandler>(image::Image)>(),
                                                                                                            sol::call_constructor,
                                                                                                            sol::factories([](image::Image img)
                                                                                                                           { return std::make_shared<viewer::ImageHandler>(img); },
                                                                                                                           [](std::shared_ptr<viewer::Handler> h)
                                                                                                                           { return std::dynamic_pointer_cast<viewer::ImageHandler>(h); }),
                                                                                                            sol::base_classes, sol::bases<viewer::Handler>());
        }
    }
}