#include <stdint.h>
#include "rgb_2_rgbw.h"
#include "namespace.h"

FASTLED_NAMESPACE_BEGIN

namespace {
    uint8_t min3(uint8_t a, uint8_t b, uint8_t c) {
        if (a < b) {
            if (a < c) {
                return a;
            } else {
                return c;
            }
        } else {
            if (b < c) {
                return b;
            } else {
                return c;
            }
        }
    }

    #ifdef FAST_LED_FAST_DIVIDE_BY_THREE
    uint8_t s_lookup_table[256] = {0};
    #endif

    uint8_t divide_by_three(uint8_t x) {
        #ifdef FAST_LED_FAST_DIVIDE_BY_THREE
        if (!s_is_initialized) {
            for (int i = 0; i < 256; i++) {
                s_lookup_table[i] = i / 3;
            }
            s_is_initialized = true;
        }
        return s_lookup_table[x];
        #else
        // Divide by three can be done with a simple multiply + bit shift.
        // It's unclear if this is even being done by the compiler and
        // if it's faster than a lookup table. Hopefully someone smart
        // enough will test this and file a pull request to finalize this
        // strategy.
        return x / 3;
        #endif
    }
}



void rgb_2_rgbw(uint8_t r, uint8_t g, uint8_t b, uint16_t color_temperature,
                uint8_t* out_r, uint8_t* out_g, uint8_t* out_b, uint8_t* out_w) {
    uint8_t min_component = min3(r, g, b);
    uint8_t w;
    bool is_min = true;
    if (min_component <= 84) {
        w = 3 * min_component;
    } else {
        w = 255;
        is_min = false;
    }
    uint8_t r_prime;
    uint8_t g_prime;
    uint8_t b_prime;
    if (is_min) {
        //assert(r >= min_component);
        //assert(g >= min_component);
        //assert(b >= min_component);
        r_prime = r - min_component;
        g_prime = g - min_component;
        b_prime = b - min_component;
    } else {
        // Saturation stealing algorithm. Take from the rgb components
        // and give to the white component. However, according to this:
        // https://www.reddit.com/r/FastLED/comments/1f4a0za/comment/lks3l1o
        // the W component is not actually 1/3rd the brightness, it's actually
        // equal to the three components of the same value, as if they were summed
        // together. So consider this algorithm experimental until real world testing
        // finalizes it.
        uint8_t w3 = divide_by_three(w);
        //assert(r >= w3);
        //assert(g >= w3);
        //assert(b >= w3);
        r_prime = r - w3;
        g_prime = g - w3;
        b_prime = b - w3;
    }

    *out_r = r_prime;
    *out_g = g_prime;
    *out_b = b_prime;
    *out_w = w;
}

FASTLED_NAMESPACE_END