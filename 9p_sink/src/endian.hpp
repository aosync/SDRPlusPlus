#pragma once

#include <cstring>
#include <cstdint>

namespace Endian {
    constexpr bool isLittleEndian() { return (const uint8_t &)0x01020304 == 0x04; }
    
    inline void revmcpy(void* dst, void* buf, size_t n) {
        for(size_t i = 0; i < n; i++) {
            ((char*)dst)[i] = ((char*)buf)[n - 1 - i];
        }
    }

    /* functions to output host->le or parse le->host */
    namespace LittleEndian {
        template <typename T>
        inline size_t put(void* dst, T buf) {
            if constexpr (isLittleEndian()) {
                memcpy(dst, (void*)&buf, sizeof(T));
            }
            else {
                revmcpy(dst, (void*)&buf, sizeof(T));
            }
            return sizeof(T);
        }
        template <typename T>
        inline T get(void* buf) {
            T res;
            if constexpr (isLittleEndian()) {
                memcpy((void*)&res, buf, sizeof(T));
            }
            else {
                revmcpy((void*)&res, buf, sizeof(T));
            }
            return res;
        }
    }

    /* functions to output host->be or parse be->host */
    namespace BigEndian {
        template <typename T>
        inline size_t put(void* dst, T buf) {
            if constexpr (!isLittleEndian()) {
                memcpy(dst, (void*)&buf, sizeof(T));
            }
            else {
                revmcpy(dst, (void*)&buf, sizeof(T));
            }
            return sizeof(T);
        }
        template <typename T>
        inline T get(void* buf) {
            T res;
            if constexpr (!isLittleEndian()) {
                memcpy((void*)&res, buf, sizeof(T));
            }
            else {
                revmcpy((void*)&res, buf, sizeof(T));
            }
            return res;
        }
    }
}