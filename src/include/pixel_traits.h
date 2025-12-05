#ifndef PIXEL_TRAITS_H
#define PIXEL_TRAITS_H

#include "pixel_t.h"
#include <cstdint>

// Generic pixel traits for simple pixel types (8-bit, 16-bit, 32-bit)
template<typename PixelT, typename PixelDataT, typename PPixelT, int BytesPerPixel>
struct PixelTraits {
    using pixel_t = PixelT;
    using pixeldata_t = PixelDataT;
    using ppixel_t = PPixelT;
    static constexpr int bpp = BytesPerPixel;

    // Generic pixel operations (used for 8, 16, 32 bpp)
    static inline void set(pixel_t* ptr, pixeldata_t val) {
        *ptr = val;
    }

    static inline pixeldata_t get(const pixel_t* ptr) {
        return *ptr;
    }

    static inline void setp(ppixel_t ptr, int pos, pixeldata_t val) {
        ptr[pos] = val;
    }

    static inline pixeldata_t getp(const ppixel_t ptr, int pos) {
        return ptr[pos];
    }

    static inline pixel_t* add(pixel_t* ptr, int val) {
        return ptr + val;
    }

    static inline void inc(pixel_t*& ptr, int val) {
        ptr += val;
    }

    static inline void copy(ppixel_t ptr1, int val1, ppixel_t ptr2, int val2) {
        ptr1[val1] = ptr2[val2];
    }
};

// Specialized traits for 8-bit pixels
using Pixel8Traits = PixelTraits<pixel8_t, pixel8_t, ppixel8_t, 1>;

// Specialized traits for 16-bit pixels
using Pixel16Traits = PixelTraits<pixel16_t, pixel16_t, ppixel16_t, 2>;

// Specialized traits for 32-bit pixels
using Pixel32Traits = PixelTraits<pixel32_t, pixel32_t, ppixel32_t, 4>;

// Specialized traits for 24-bit pixels (requires custom operations)
struct Pixel24Traits {
    using pixel_t = pixel8_t;
    using pixeldata_t = pixel32_t;
    using ppixel_t = ppixel24_t;
    static constexpr int bpp = 3;

    // 24-bit pixel operations (from true24.h)
    static inline void set(pixel_t* ptr, pixeldata_t val) {
        *(pixel16_t*)(ptr) = (pixel16_t)(val);
        ptr[2] = (pixel8_t)((val) >> 16);
    }

    static inline pixeldata_t get(const pixel_t* ptr) {
        return ((pixeldata_t)*(pixel16_t*)(ptr) + (pixeldata_t)(ptr[2] << 16));
    }

    static inline void setp(ppixel_t ptr, int pos, pixeldata_t val) {
        set(ptr + pos * 3, val);
    }

    static inline pixeldata_t getp(const ppixel_t ptr, int pos) {
        return get(ptr + pos * 3);
    }

    static inline pixel_t* add(pixel_t* ptr, int pos) {
        return ptr + pos * 3;
    }

    static inline void inc(pixel_t*& ptr, int pos) {
        ptr += pos * 3;
    }

    static inline void copy(ppixel_t ptr1, int pos1, ppixel_t ptr2, int pos2) {
        *((pixel16_t*)(ptr1 + pos1 * 3)) = *(pixel16_t*)(ptr2 + pos2 * 3);
        ptr1[pos1 * 3 + 2] = ptr2[pos2 * 3 + 2];
    }
};

#endif // PIXEL_TRAITS_H
