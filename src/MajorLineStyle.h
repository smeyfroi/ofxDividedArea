//
//  MajorLineStyle.h
//  ofxDividedArea
//
//  Rendering styles for major (unconstrained) divider lines.
//

#pragma once

#include <string>
#include <vector>

enum class MajorLineStyle {
  Solid = 0,          // Simple flat-colored line
  Metallic,           // Anisotropic metallic highlight
  InnerGlow,          // Light edges, darker core
  BloomedAdditive,    // Neon tube: core + halo (additive)
  Glow,               // Additive gaussian falloff
  Refractive,         // Glass-like distortion (background FBO)
  BlurRefraction,     // Screen-space blur + mild refraction (background FBO)
  ChromaticAberration,// RGB split at edges (background FBO)
  Count               // Number of styles (for iteration)
};

inline std::string majorLineStyleToString(MajorLineStyle style) {
  switch (style) {
    case MajorLineStyle::Solid: return "Solid";
    case MajorLineStyle::Metallic: return "Metallic";
    case MajorLineStyle::InnerGlow: return "Inner Glow";
    case MajorLineStyle::BloomedAdditive: return "Bloomed Additive";
    case MajorLineStyle::Glow: return "Glow";
    case MajorLineStyle::Refractive: return "Refractive";
    case MajorLineStyle::BlurRefraction: return "Blur/Refraction";
    case MajorLineStyle::ChromaticAberration: return "Chromatic Aberration";
    default: return "Unknown";
  }
}

inline std::vector<std::string> getMajorLineStyleNames() {
  std::vector<std::string> names;
  for (int i = 0; i < static_cast<int>(MajorLineStyle::Count); ++i) {
    names.push_back(majorLineStyleToString(static_cast<MajorLineStyle>(i)));
  }
  return names;
}
