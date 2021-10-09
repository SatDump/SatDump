#pragma once

#include "detail/function_traits.hpp"

#include <unordered_map>
#include <typeindex>
#include <functional>
#include <any>
#include <mutex>

namespace dp
{
    struct handler_registration
    {
        const void* handle{ nullptr };
    };
    
    class event_bus 
    {
    public:

        event_bus() = default;		
        template<typename EventType, typename EventHandler>
        /*[[nodiscard]]*/ handler_registration register_handler(EventHandler &&handler)
        {
            using traits = detail::function_traits<EventHandler>;
            const auto type_idx = std::type_index(typeid(EventType));
            handler_registration registration;
            if constexpr (traits::arity == 0)
            {
                safe_registrations_access([&]() {
                    auto it = handler_registrations_.emplace(type_idx, [handler = std::forward<EventHandler>(handler)](auto) {
                        handler();
                    });

                    registration.handle = static_cast<const void *>(&(it->second));
                });
            }
            else
            {
                safe_registrations_access([&]() {
                    auto it = handler_registrations_.emplace(type_idx, [func = std::forward<EventHandler>(handler)](auto value) {
                        func(std::any_cast<EventType>(value));
                    });

                    registration.handle = static_cast<const void *>(&(it->second));
                });
            }
            return registration;
        }

        template<typename EventType, typename ClassType, typename MemberFunction>
        [[nodiscard]] handler_registration register_handler(ClassType* class_instance, MemberFunction&& function) noexcept
        {
            using traits = detail::function_traits<MemberFunction>;
            static_assert(std::is_same_v<ClassType, std::decay_t<typename traits::owner_type>>, "Member function pointer must match instance type.");
            
            const auto type_idx = std::type_index(typeid(EventType));
            handler_registration registration;

            if constexpr (traits::arity == 0)
            {
                safe_registrations_access([&]() {
                    auto it = handler_registrations_.emplace(type_idx, [class_instance, function](auto) {
                        (class_instance->*function)();
                    });

                    registration.handle = static_cast<const void *>(&(it->second));
                });
            }
            else
            {
                safe_registrations_access([&]() {
                    auto it = handler_registrations_.emplace(type_idx, [class_instance, function](auto value) {
                        (class_instance->*function)(std::any_cast<EventType>(value));
                    });

                    registration.handle = static_cast<const void *>(&(it->second));
                });
            }
            return registration;
        }

        template<typename EventType>
        void fire_event(const EventType& evt) noexcept
        {
            safe_registrations_access([&]() {
                // only call the functions we need to
                auto [begin_evt_id, end_evt_id] = handler_registrations_.equal_range(std::type_index(typeid(EventType)));
                for (; begin_evt_id != end_evt_id; ++begin_evt_id)
                {
                    try
                    {
                        begin_evt_id->second(std::any_cast<EventType>(evt));
                    }
                    catch (std::bad_any_cast &)
                    {
                        // Ignore for now
                    } 
                }
            });
        }

        bool remove_handler(const handler_registration &registration) noexcept
        {
            if (!registration.handle) { return false; }

            std::lock_guard<std::mutex> lock(registration_mutex_);
            for(auto it = handler_registrations_.begin(); it != handler_registrations_.end(); ++it)
            {
                if(static_cast<const void*>(&(it->second)) == registration.handle)
                {
                    handler_registrations_.erase(it);
                    return true;
                }
            }

            return false;
        }

        void remove_handlers() noexcept
        {
            std::lock_guard<std::mutex> lock(registration_mutex_);
            handler_registrations_.clear();
        }

        [[nodiscard]] std::size_t handler_count() noexcept
        {
            std::lock_guard<std::mutex> lock(registration_mutex_);
            return handler_registrations_.size();
        }
        
    private:
        std::mutex registration_mutex_;
        std::unordered_multimap<std::type_index, std::function<void(std::any)>> handler_registrations_;

        template<typename Callable>
        void safe_registrations_access(Callable&& callable) {
            try {
                // if this fails, an exception will be thrown.
                std::lock_guard<std::mutex> lock(registration_mutex_);
                callable();
            }
            catch(std::system_error&) {
                // do nothing
            }
        }
    };
}
