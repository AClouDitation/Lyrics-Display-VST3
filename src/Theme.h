#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{

struct Palette
{
    juce::Colour bg;
    juce::Colour surface;
    juce::Colour surfaceAlt;
    juce::Colour surfaceHover;
    juce::Colour border;
    juce::Colour text;
    juce::Colour textMuted;
    juce::Colour textDim;
    juce::Colour accent;
    juce::Colour accentSoft;
    juce::Colour accentGlow;
    juce::Colour success;
    juce::Colour warning;
    juce::Colour danger;
};

inline Palette darkPalette()
{
    return {
        juce::Colour(0xff0f1117), // bg
        juce::Colour(0xff171a23), // surface
        juce::Colour(0xff1f2430), // surfaceAlt
        juce::Colour(0xff272d3d), // surfaceHover
        juce::Colour(0xff2d3446), // border
        juce::Colour(0xfff1f5f9), // text
        juce::Colour(0xffa8b3cf), // textMuted
        juce::Colour(0xff6b7694), // textDim
        juce::Colour(0xff8b5cf6), // accent (studio violet)
        juce::Colour(0x288b5cf6), // accentSoft
        juce::Colour(0x448b5cf6), // accentGlow
        juce::Colour(0xff10b981), // success
        juce::Colour(0xfff59e0b), // warning
        juce::Colour(0xffef4444)  // danger
    };
}

inline Palette lightPalette()
{
    return {
        juce::Colour(0xfff4f6fb), // bg
        juce::Colour(0xffffffff), // surface
        juce::Colour(0xffeef1f7), // surfaceAlt
        juce::Colour(0xffe2e7f2), // surfaceHover
        juce::Colour(0xffd5dce9), // border
        juce::Colour(0xff111827), // text
        juce::Colour(0xff4b5563), // textMuted
        juce::Colour(0xff8a94a6), // textDim
        juce::Colour(0xff7c3aed), // accent
        juce::Colour(0x1f7c3aed), // accentSoft
        juce::Colour(0x367c3aed), // accentGlow
        juce::Colour(0xff059669), // success
        juce::Colour(0xffd97706), // warning
        juce::Colour(0xffdc2626)  // danger
    };
}

inline juce::Font makeUiFont(float height, bool bold = false)
{
    // Microsoft YaHei UI renders crisp Latin + CJK (Chinese/Japanese/Korean) via Windows DirectWrite
    auto options = juce::FontOptions("Microsoft YaHei UI", height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

inline juce::Font makeMonoFont(float height, bool bold = false)
{
    auto options = juce::FontOptions("Consolas", height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

} // namespace theme
