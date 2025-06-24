#pragma once

#include "handlers/handler.h"
#include "htmltest.h"
#include "imgui/imgui.h"
#include <litehtml.h>
#include <litehtml/document.h>

namespace satdump
{
    class DocTestHandler : public handlers::Handler
    {
    protected:
        void drawMenu();
        void drawContents(ImVec2 win_size);
        void drawMenuBar();

        LiteHtmlView viewtest;
        litehtml::document::ptr doc;

    public:
        DocTestHandler();
        ~DocTestHandler();

    public:
        std::string getID() { return "doctest_handler"; }
        std::string getName() { return "WIP Documentation"; }
    };
}; // namespace satdump