#pragma once

/*
Generic save / restore guards with no SatDump dependency, shared by the whole
suite. Everything is inline and lives in the test_helpers namespace, as every
test file is linked into the same executable.

- CwdGuard changes the current directory, and restores it on destruction.
- ValueGuard<T> saves any variable (a global, a static member...), optionally
  sets a new value, and restores the saved value on destruction:
      test_helpers::ValueGuard g(satdump::RESPATH, dir.str() + "/");
      test_helpers::ValueGuard g2(antenna::GenericAntenna::base_unique_id);
  As a class member the type must be spelled out (no deduction for members):
      test_helpers::ValueGuard<std::string> user_guard;
Restoring never throws, guards are non-copyable.
*/

#include <filesystem>
#include <system_error>
#include <utility>

namespace test_helpers
{
    // Sets the current directory, restores the previous one on destruction
    class CwdGuard
    {
    public:
        explicit CwdGuard(const std::filesystem::path &dir) : saved(std::filesystem::current_path()) { std::filesystem::current_path(dir); }

        ~CwdGuard()
        {
            std::error_code ec;
            std::filesystem::current_path(saved, ec);
        }

        CwdGuard(const CwdGuard &) = delete;
        CwdGuard &operator=(const CwdGuard &) = delete;

    private:
        std::filesystem::path saved;
    };

    // Saves a variable, optionally sets it, and restores it on destruction
    template <typename T>
    class ValueGuard
    {
    public:
        explicit ValueGuard(T &target) : target(target), old(target) {}

        template <typename U>
        ValueGuard(T &target, U &&value) : target(target), old(target)
        {
            target = std::forward<U>(value);
        }

        ~ValueGuard() { target = old; }

        ValueGuard(const ValueGuard &) = delete;
        ValueGuard &operator=(const ValueGuard &) = delete;

        // Value at construction time
        const T &saved() const { return old; }

    private:
        T &target;
        T old;
    };
} // namespace test_helpers
