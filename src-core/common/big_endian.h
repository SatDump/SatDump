#pragma once

#include <cstdint>
#if defined(_WIN32)
#include <WS2tcpip.h>
#include <winsock2.h>
#else
#include <arpa/inet.h> // for ntohs() etc.
#endif
#include <stdint.h>

#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Creates an unsigned 2-byte integer from big-endian bytes
 *
 */
class be_uint16_t
{
public:
    be_uint16_t() : be_val_(0) {}
    // Transparently cast from uint16_t
    be_uint16_t(const uint16_t &val) : be_val_(htons(val)) {}
    // Transparently cast to uint16_t
    operator uint16_t() const { return ntohs(be_val_); }

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
///////////////////////////////////
#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Creates an unsigned 4-byte integer from big-endian bytes
 *
 */
class be_uint32_t
{
public:
    be_uint32_t() : be_val_(0) {}
    // Transparently cast from uint32_t
    be_uint32_t(const uint32_t &val) : be_val_(htonl(val)) {}
    // Transparently cast to uint32_t
    operator uint32_t() const { return ntohl(be_val_); }

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
///////////////////////////////////
#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Takes 6 bytes, creates an unsigned 48-bit integer in big endian. Stored in a 64 bit signed integer
 *
 */
struct be_uint48_t
{
    uint8_t bytes[6];

    uint64_t value() const
    {
        return ((uint64_t)bytes[0] << 40) | ((uint64_t)bytes[1] << 32) | ((uint64_t)bytes[2] << 24) | ((uint64_t)bytes[3] << 16) | ((uint64_t)bytes[4] << 8) | ((uint64_t)bytes[5]);
    }

    operator uint64_t() const { return value(); }
}

#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif
//////////////////////////////
#ifdef _WIN32
#pragma pack(push, 1)
#endif
/**
 * @brief Creates an unsigned 8-byte integer from big-endian bytes
 *
 */
class be_uint64_t
{
public:
    be_uint64_t() : be_val_(0) {}
    // Transparently cast from uint32_t
    be_uint64_t(const uint64_t &val) : be_val_(htonl(val)) {}
    // Transparently cast to uint32_t
    operator uint64_t() const { return ntohl(be_val_); }

private:
    uint64_t be_val_;
}
#ifdef _WIN32
;
#else
__attribute__((packed));
#endif
#ifdef _WIN32
#pragma pack(pop)
#endif