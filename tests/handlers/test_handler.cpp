#include "handlers/dummy_handler.h"
#include "handlers/handler.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <thread>

namespace
{
    using satdump::handlers::DummyHandler;
    using satdump::handlers::Handler;

    std::shared_ptr<Handler> dummy(const std::string &name) { return std::make_shared<DummyHandler>(name); }

    std::vector<std::string> names(const std::vector<std::shared_ptr<Handler>> &v)
    {
        std::vector<std::string> n;
        for (auto &h : v)
            n.push_back(h->getName());
        return n;
    }

    // Handler that only relies on the base class defaults
    class BareHandler : public Handler
    {
    public:
        void drawMenu() {}
        void drawContents(ImVec2) {}
        std::string getID() { return "unit_bare"; }
    };
} // namespace

TEST_CASE("DummyHandler reports its name and ID", "[handlers][handler]")
{
    DummyHandler h("My Folder");
    CHECK(h.getName() == "My Folder");
    CHECK(h.getID() == "dummy");
    CHECK(h.handler_tree_icon == "N");
    CHECK_FALSE(h.hasSubhandlers());
    CHECK(h.getAllSubHandlers().empty());
}

TEST_CASE("Handler defaults: name, config and tree ID", "[handlers][handler]")
{
    BareHandler h;
    CHECK(h.getName() == "!!!Invalid!!!");
    CHECK(h.getConfig().is_null());
    h.setConfig({{"anything", 1}});
    CHECK(h.getConfig().is_null());
    h.resetConfig();
    h.drawMenuBar();
    h.drawContextMenu();
}

TEST_CASE("Handler::getTreeID combines the name with a per-instance suffix", "[handlers][handler]")
{
    DummyHandler a("Same"), b("Same");

    std::string ida = a.getTreeID();
    CHECK(ida.rfind("Same##", 0) == 0);
    CHECK(ida == "Same##" + std::to_string((size_t)&a));
    CHECK(ida == a.getTreeID()); // stable
    CHECK(ida != b.getTreeID()); // unique per instance, even with the same name
}

TEST_CASE("Handler::addSubHandler appends or prepends", "[handlers][handler]")
{
    DummyHandler root("root");
    root.addSubHandler(dummy("a"));
    root.addSubHandler(dummy("b"));
    root.addSubHandler(dummy("top"), true);
    root.addSubHandler(dummy("c"), false);

    CHECK(root.hasSubhandlers());
    CHECK(names(root.getAllSubHandlers()) == std::vector<std::string>{"top", "a", "b", "c"});
}

TEST_CASE("Handler::getAllSubHandlers returns a copy of the list", "[handlers][handler]")
{
    DummyHandler root("root");
    auto a = dummy("a");
    root.addSubHandler(a);

    auto list = root.getAllSubHandlers();
    list.push_back(dummy("extra"));
    list.clear();

    auto again = root.getAllSubHandlers();
    REQUIRE(again.size() == 1);
    CHECK(again[0] == a); // same shared instance
}

TEST_CASE("Handler::delSubHandler defers deletion unless asked to delete now", "[handlers][handler]")
{
    DummyHandler root("root");
    auto a = dummy("a");
    auto b = dummy("b");
    auto c = dummy("c");
    root.addSubHandler(a);
    root.addSubHandler(b);
    root.addSubHandler(c);

    // Deferred: only marked, still present until the next tree draw or immediate delete
    root.delSubHandler(b);
    CHECK(names(root.getAllSubHandlers()) == std::vector<std::string>{"a", "b", "c"});

    // Immediate deletion also flushes previously marked handlers
    root.delSubHandler(a, true);
    CHECK(names(root.getAllSubHandlers()) == std::vector<std::string>{"c"});

    root.delSubHandler(c, true);
    CHECK_FALSE(root.hasSubhandlers());
    CHECK(root.getAllSubHandlers().empty());
}

TEST_CASE("Handler::delSubHandler releases the removed handler", "[handlers][handler]")
{
    DummyHandler root("root");
    auto a = dummy("a");
    std::weak_ptr<Handler> weak = a;
    root.addSubHandler(a);
    a.reset();
    CHECK_FALSE(weak.expired());

    root.delSubHandler(weak.lock(), true);
    CHECK(weak.expired());
}

TEST_CASE("Handler sub-handlers form an independent tree per node", "[handlers][handler]")
{
    auto root = std::make_shared<DummyHandler>("root");
    auto folder = dummy("folder");
    auto leaf1 = dummy("leaf1");
    auto leaf2 = dummy("leaf2");

    root->addSubHandler(folder);
    folder->addSubHandler(leaf1);
    folder->addSubHandler(leaf2);

    CHECK(names(root->getAllSubHandlers()) == std::vector<std::string>{"folder"});
    CHECK(names(folder->getAllSubHandlers()) == std::vector<std::string>{"leaf1", "leaf2"});
    CHECK_FALSE(leaf1->hasSubhandlers());

    // "Move" leaf2 from folder to root, as a drag-and-drop would
    root->addSubHandler(leaf2);
    folder->delSubHandler(leaf2, true);
    CHECK(names(root->getAllSubHandlers()) == std::vector<std::string>{"folder", "leaf2"});
    CHECK(names(folder->getAllSubHandlers()) == std::vector<std::string>{"leaf1"});

    // The same handler may be referenced from two parents
    folder->addSubHandler(leaf2);
    CHECK(root->getAllSubHandlers()[1] == folder->getAllSubHandlers()[1]);
}

TEST_CASE("Handler::addSubHandler is safe to call from several threads", "[handlers][handler]")
{
    DummyHandler root("root");
    const int threads = 4, per_thread = 50;

    std::vector<std::thread> ts;
    for (int t = 0; t < threads; t++)
        ts.emplace_back(
            [&root, t]()
            {
                for (int i = 0; i < per_thread; i++)
                    root.addSubHandler(dummy(std::to_string(t) + "_" + std::to_string(i)), i % 2);
            });
    for (auto &t : ts)
        t.join();

    auto all = root.getAllSubHandlers();
    CHECK(all.size() == threads * per_thread);
    std::set<std::string> unique;
    for (auto &h : all)
        unique.insert(h->getName());
    CHECK(unique.size() == threads * per_thread);
}
