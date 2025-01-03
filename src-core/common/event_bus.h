#pragma once

#include <vector>
#include <string>
#include <functional>
#include <typeinfo>

/*
 * Very simple event bus implementation using
 * std::function and typeid.
 * All this does is fire any registered handler
 * when called.
 *
 * std::any could not be used as it can mess up
 * over several .so.
 * (typeinfo does NOT allow casting between
 * several interpretations of the exact same
 * struct)
 */
class EventBus
{
private:
    std::vector<std::pair<std::string, std::function<void(void *)>>> all_handlers;

public:
    template <typename T>
    void register_handler(std::function<void(T)> handler_fun)
    {
        all_handlers.push_back({std::string(typeid(T).name()),
                                [handler_fun](void *raw)
                                {
                                    T evt = *((T *)raw); // Cast struct to original type
                                    handler_fun(evt);    // Call real handler
                                }});
    }

    template <typename T>
    void fire_event(T evt)
    {
        for (std::pair<std::string, std::function<void(void *)>> h : all_handlers) // Iterate through all registered functions
            if (std::string(typeid(T).name()) == h.first)                          // Check struct type is the same
                h.second((void *)&evt);                                            // Fire handler up
    }

    // Used by task scheduler
    void fire_event(void *evt, std::string evt_name)
    {
        for (std::pair<std::string, std::function<void(void *)>> h : all_handlers) // Iterate through all registered functions
            if (evt_name == h.first)                                               // Check struct type is the same
                h.second(evt);                                                     // Fire handler up
    }
};