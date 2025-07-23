#pragma once

/**
 * @file event_bus.h
 * @brief Event bus implementation
 */

#include <functional>
#include <string>
#include <typeinfo>
#include <vector>

namespace satdump
{
    /**
     * @brief Very simple event bus implementation using
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
        /**
         * @brief Register a handler function to be called
         * when a specific event is fired.
         *
         * @param handler_fun function to register
         */
        template <typename T>
        void register_handler(std::function<void(T)> handler_fun)
        {
            all_handlers.push_back({std::string(typeid(T).name()), [handler_fun](void *raw)
                                    {
                                        T evt = *((T *)raw); // Cast struct to original type
                                        handler_fun(evt);    // Call real handler
                                    }});
        }

        /**
         * @brief Trigger an event, called every registered
         * handler
         *
         * @param evt event struct
         */
        template <typename T>
        void fire_event(T evt)
        {
            for (std::pair<std::string, std::function<void(void *)>> h : all_handlers) // Iterate through all registered functions
                if (std::string(typeid(T).name()) == h.first)                          // Check struct type is the same
                    h.second((void *)&evt);                                            // Fire handler up
        }

        /**
         * @brief Trigger an event, called every registered
         * handler. Allows specifying the event name.
         * Used by task scheduler.
         *
         * @param evt event struct
         * @param evt_name event name to trigger
         */
        void fire_event(void *evt, std::string evt_name)
        {
            for (std::pair<std::string, std::function<void(void *)>> h : all_handlers) // Iterate through all registered functions
                if (evt_name == h.first)                                               // Check struct type is the same
                    h.second(evt);                                                     // Fire handler up
        }
    };
} // namespace satdump