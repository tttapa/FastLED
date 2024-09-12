#pragma once

#include "FastLED.h"
#include "pixel_controller.h"
#include "pixel_iterator.h"

FASTLED_NAMESPACE_BEGIN

template <typename PixelControllerT>
class PixelIteratorT : protected PixelIterator {
    // New chipsets/drivers should use as_iterator() to process led output.
    // Accessing PixelController directly from user code deprecated, and should
    // be minimized.
    //
    // Create an adapter class to allow PixelController to be used as a
    // PixelIterator. Caller must save the fully templatized object to the
    // stack, then call base() in order to get the non-templatized base class
    // reference, which is then used to call the get the ordering of pixels.
    // PixelIterator should be used for more powerful micro controllers which
    // can easily handle the virtual function overhead.

  public:
    static const EOrder RGB_ORDER = PixelControllerT::RGB_ORDER_VALUE;

    PixelIteratorT(PixelControllerT &pc, RgbwArg rgbw) : mPixelController(pc) {
        set_rgbw(rgbw);
    }
    virtual bool has(int n) { return mPixelController.has(n); }
    virtual void loadAndScaleRGBW(uint8_t *b0_out, uint8_t *b1_out,
                                  uint8_t *b2_out, uint8_t *b3_out) {

#ifdef __AVR__
        // Don't do RGBW conversion for AVR, just set the W pixel to black.
        uint8_t out[4] = {
            // Get the pixels in native order.
            mPixelController.loadAndScale0(),
            mPixelController.loadAndScale1(),
            mPixelController.loadAndScale2(),
            0,
        };
        EOrderW w_placement = get_rgbw().w_placement;
        // Apply w-component insertion.
        rgbw_reorder(w_placement, out[0], out[1], out[2],
                     0, // Pre-ordered RGB data with a 0 white component.
                     b0_out, b1_out, b2_out, b3_out);
#else
        const uint8_t b0_index = RGB_BYTE0(RGB_ORDER);
        const uint8_t b1_index = RGB_BYTE1(RGB_ORDER);
        const uint8_t b2_index = RGB_BYTE2(RGB_ORDER);
        // Get the naive RGB data order in r,g,b order.
        CRGB rgb(mPixelController.mData[0], mPixelController.mData[1],
                 mPixelController.mData[2]);
        CRGB scale = mPixelController.mScale;
        uint8_t w = 0;
        RgbwArg rgbw_arg = get_rgbw();
        uint16_t white_color_temp = rgbw_arg.white_color_temp;
        rgb_2_rgbw(rgbw_arg.rgbw_mode, white_color_temp, rgb.r, rgb.b, rgb.g, scale.r,
                   scale.g, scale.b, &rgb.r, &rgb.g, &rgb.b, &w);
        rgbw_reorder(rgbw_arg.w_placement, rgb.raw[b0_index], rgb.raw[b1_index],
                     rgb.raw[b2_index], w, b0_out, b1_out, b2_out, b3_out);
#endif
    }
    virtual void loadAndScaleRGB(uint8_t *b0, uint8_t *b1, uint8_t *b2) {
        *b0 = mPixelController.loadAndScale0();
        *b1 = mPixelController.loadAndScale1();
        *b2 = mPixelController.loadAndScale2();
    }
    virtual void loadAndScale_APA102_HD(uint8_t *b0_out, uint8_t *b1_out,
                                        uint8_t *b2_out,
                                        uint8_t *brightness_out) {
        CRGB rgb = CRGB(mPixelController.mData[0], mPixelController.mData[1],
                        mPixelController.mData[2]);
        uint8_t brightness = 0;
        if (rgb) {
            five_bit_hd_gamma_bitshift(
                rgb.r, rgb.g, rgb.b,
                // Note this mScale has the global brightness scale mixed in
                // with the color correction scale.
                mPixelController.mScale.r, mPixelController.mScale.g,
                mPixelController.mScale.b, &rgb.r, &rgb.g, &rgb.b, &brightness);
        }
        const uint8_t b0_index = RGB_BYTE0(RGB_ORDER);
        const uint8_t b1_index = RGB_BYTE1(RGB_ORDER);
        const uint8_t b2_index = RGB_BYTE2(RGB_ORDER);
        *b0_out = rgb.raw[b0_index];
        *b1_out = rgb.raw[b1_index];
        *b2_out = rgb.raw[b2_index];
        *brightness_out = brightness;
    }
    virtual void stepDithering() { mPixelController.stepDithering(); }
    virtual void advanceData() { mPixelController.advanceData(); }
    virtual int size() { return mPixelController.size(); }

    // Get the base class reference for the PixelIterator, which is used by the
    // driver system.
    PixelIterator &base() { return *this; }

    PixelControllerT &mPixelController;
};

FASTLED_NAMESPACE_END