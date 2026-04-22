// hashN.cpp
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// --- 32-bit helpers ---

static inline uint32_t rotl32(uint32_t x, uint32_t n) {
    return (x << n) | (x >> (32 - n));
}

// Simple xorshift32 to expand seeds into more constants
static inline uint32_t xorshift32(uint32_t &s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

// --- UTF-8 encoding (like your JS) ---

static std::vector<uint8_t> stringToUtf8Bytes(const std::string &str) {
    // Assume input is UTF-8 already (Emscripten/JS side will pass UTF-8)
    // If you want full UTF-16 → UTF-8 like JS, you’d handle code points.
    // For wasm interop, this is usually enough.
    return std::vector<uint8_t>(str.begin(), str.end());
}

// --- State init: lane-count-parametric ---

template <size_t LANES>
static std::vector<uint32_t> initState() {
    // Start from your 32 Blowfish P-array constants, then expand with xorshift.
    static const uint32_t base[32] = {
        0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
        0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
        0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
        0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917,
        0x9216d5d9, 0x8979fb1b, 0xd1310ba6, 0x98dfb5ac,
        0x2ffd72db, 0xd01adfb7, 0xb8e1afed, 0x6a267e96,
        0xba7c9045, 0xf12c7f99, 0x24a19947, 0xb3916cf7,
        0x0801f2e2, 0x858efc16, 0x636920d8, 0x71574e69
    };

    std::vector<uint32_t> state(LANES);
    uint32_t s = 0x9e3779b1u; // seed for expansion

    for (size_t i = 0; i < LANES; ++i) {
        if (i < 32) {
            state[i] = base[i];
        } else {
            state[i] = xorshift32(s);
        }
    }
    return state;
}

// --- Core hashN (LANES * 32 bits) ---

template <size_t LANES>
static std::string hashN(const std::string &input) {
    auto bytes = stringToUtf8Bytes(input);
    auto state = initState<LANES>(); // LANES x 32-bit

    // Main mixing loop
    for (size_t i = 0; i < bytes.size(); ++i) {
        uint8_t b = bytes[i];
        uint32_t t = static_cast<uint32_t>(b) * 0x9e3779b1u;

        for (size_t j = 0; j < LANES; ++j) {
            uint32_t v = state[j];

            v ^= (t + static_cast<uint32_t>(j) * 0x85ebca6bu);

            uint32_t rot = static_cast<uint32_t>(j * 7 + i) & 31u;
            v = rotl32(v, rot);

            v = v + ((v ^ (v >> 16)) * 0x27d4eb2du);

            state[j] = v;
        }
    }

    // Finalization rounds
    for (int r = 0; r < 4; ++r) {
        for (size_t j = 0; j < LANES; ++j) {
            uint32_t v = state[j];
            v ^= v >> 15;
            v *= 0x85ebca6bu;
            v ^= v >> 13;
            v *= 0xc2b2ae35u;
            v ^= v >> 16;
            state[j] = v;
        }
    }

    // State → hex
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < LANES; ++i) {
        oss << std::setw(8) << state[i];
    }
    return oss.str();
}

// Convenience typedefs:
// 32 lanes  = 1024 bits
// 64 lanes  = 2048 bits
// 128 lanes = 4096 bits

static std::string hash1024(const std::string &input) {
    return hashN<32>(input);
}

static std::string hash2048(const std::string &input) {
    return hashN<64>(input);
}

static std::string hash4096(const std::string &input) {
    return hashN<128>(input);
}

#ifdef __EMSCRIPTEN__
// Simple C-style exports for JS FFI

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* hash1024_c(const char* input) {
    static std::string out;
    out = hash1024(input);
    return out.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* hash2048_c(const char* input) {
    static std::string out;
    out = hash2048(input);
    return out.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* hash4096_c(const char* input) {
    static std::string out;
    out = hash4096(input);
    return out.c_str();
}

}

#endif
