//
//  Colors.h
//  NeuralAmpModeler-macOS
//
//  Created by Steven Atkinson on 12/27/22.
//  Redesigned 2026 — Neural DSP inspired dark theme.
//
// Store the defined colors for the plugin in one place.
// The design language here is a deep, near-black monochrome palette with a
// single restrained accent, in the spirit of modern pro-audio plug-ins.

#ifndef Colors_h
#define Colors_h

#include "IGraphicsStructs.h"

namespace PluginColors
{
using iplug::igraphics::IColor;

// HINT: IColor is ARGB.

// ---------------------------------------------------------------------------
// Surfaces
// ---------------------------------------------------------------------------
// Background gradient (subtle top-to-bottom fall-off)
const IColor BG_TOP(255, 22, 22, 25); // Near-black, slightly lifted at the top
const IColor BG_BOTTOM(255, 9, 9, 11); // Pure-ish black at the bottom

// Chrome strips (top toolbar / bottom status bar)
const IColor CHROME(255, 17, 17, 20);
const IColor CHROME_HI(255, 26, 26, 30);

// Cards / panels
const IColor PANEL(255, 24, 24, 28);
const IColor PANEL_HI(255, 42, 43, 48); // Top edge highlight of a card
const IColor PANEL_BORDER(255, 46, 47, 53);
const IColor HAIRLINE(255, 38, 39, 44); // Thin separators

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
const IColor TEXT_HI(255, 235, 237, 240); // Primary / values
const IColor TEXT_MID(255, 156, 159, 166); // Labels
const IColor TEXT_LO(255, 104, 107, 114); // Secondary / captions

// ---------------------------------------------------------------------------
// Knobs
// ---------------------------------------------------------------------------
const IColor KNOB_FACE_TOP(255, 54, 55, 61); // Radial gradient — lit top
const IColor KNOB_FACE_BOTTOM(255, 24, 25, 28); // Radial gradient — shaded bottom
const IColor KNOB_RIM(255, 12, 12, 14); // Dark outer rim
const IColor ARC_TRACK(255, 55, 56, 62); // Unfilled indicator arc

// ---------------------------------------------------------------------------
// Accent
// ---------------------------------------------------------------------------
// A cool, desaturated near-white used for filled arcs, pointers and the LED.
const IColor ACCENT(255, 224, 231, 240);
const IColor ACCENT_DIM(255, 120, 129, 142);
const IColor LED(255, 118, 226, 150); // Small "active" indicator

// ---------------------------------------------------------------------------
// Legacy names (kept so the rest of the codebase keeps compiling).
// They now point at the new palette.
// ---------------------------------------------------------------------------
const IColor OFF_WHITE = TEXT_HI;
const IColor NAM_1 = BG_TOP; // Background base
const IColor NAM_2 = ACCENT; // Foreground / theme
const IColor NAM_3 = TEXT_MID; // Secondary
const IColor NAM_0(0, 22, 22, 25); // Transparent
const IColor NAM_THEMECOLOR = ACCENT; // Main accent
const IColor NAM_THEMEFONTCOLOR = TEXT_HI; // Main font color

// Misc
const IColor MOUSEOVER = NAM_THEMEFONTCOLOR.WithOpacity(0.10f);
const IColor HELP_TEXT = iplug::igraphics::COLOR_WHITE;
const IColor HELP_TEXT_MO = iplug::igraphics::COLOR_WHITE.WithOpacity(0.9f);
const IColor HELP_TEXT_CLICKED = iplug::igraphics::COLOR_WHITE.WithOpacity(0.8f);

}; // namespace PluginColors

#endif /* Colors_h */
