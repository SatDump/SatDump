#pragma once

#include <cstdint>
#include <arpa/inet.h> // for ntohs() etc.
#include <stdint.h>

#ifdef _WIN32
#pragma pack(push, 1)
#endif
class be_uint16_t {
public:
        be_uint16_t() : be_val_(0) {
        }
        // Transparently cast from uint16_t
        be_uint16_t(const uint16_t &val) : be_val_(htons(val)) {
        }
        // Transparently cast to uint16_t
        operator uint16_t() const {
                return ntohs(be_val_);
        }
private:
        uint16_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif
class be_uint32_t {
public:
        be_uint32_t() : be_val_(0) {
        }
        // Transparently cast from uint32_t
        be_uint32_t(const uint32_t &val) : be_val_((val)) {
        }
        // Transparently cast to uint32_t
        operator uint32_t() const {
                return (be_val_);
        }
private:
        uint32_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
