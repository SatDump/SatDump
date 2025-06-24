#include "doctest.h"
#include "imgui/imgui.h"
#include "litehtml/types.h"
#include "utils/string.h"
#include <litehtml.h>
#include <litehtml/document.h>

namespace satdump
{
    DocTestHandler::DocTestHandler() : viewtest(720, 480, ".")
    {
        handler_tree_icon = u8"\uf471";
        auto htmlstr = loadFileToString("/home/alan/Documents/SatDump/build/documentation/html/md_docs_2pages_2ImageProductExpression.html");
        logger->trace(htmlstr);
        doc = litehtml::document::createFromString(htmlstr, &viewtest);
    }

    DocTestHandler::~DocTestHandler() {}

    void DocTestHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("NothingYet", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ////
        }
    }

    void DocTestHandler::drawMenuBar() {}

    void DocTestHandler::drawContents(ImVec2 win_size)
    {
        ImGui::BeginChild("ChildDocTest");

        doc->render(win_size.x);

        litehtml::position p;
        //   doc->render(win_size.x);
        p.width = win_size.x;
        p.height = win_size.y;
        p.x = 0;
        p.y = 0;
        doc->draw((litehtml::uint_ptr) nullptr, 0, 0, &p);

        ImGui::EndChild();
    }
} // namespace satdump
