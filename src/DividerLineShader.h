//
//  DividerLineShader.h
//  example
//
//  Created by Steve Meyfroidt on 17/09/2025.
//

#pragma once

#include "Shader.h"

class DividerLineShader : public Shader {

public:
  void begin(float maxTaperLength, float minWidthFactorStart, float maxWidthFactorStart, float minWidthFactorEnd, float maxWidthFactorEnd, float edgeFadeWidth, float edgeWidthFactor, float centerWidthFactor, float extendBeyondCanvas, float lineLengthMinFactor, float linePositionFadeWidth, float linePositionEdgeFactor, float linePositionCenterFactor) {
    Shader::begin();
    shader.setUniform1f("maxTaperLength", maxTaperLength);
    shader.setUniform1f("minWidthFactorStart", minWidthFactorStart);
    shader.setUniform1f("maxWidthFactorStart", maxWidthFactorStart);
    shader.setUniform1f("minWidthFactorEnd", minWidthFactorEnd);
    shader.setUniform1f("maxWidthFactorEnd", maxWidthFactorEnd);
    shader.setUniform1f("edgeFadeWidth", edgeFadeWidth);
    shader.setUniform1f("edgeWidthFactor", edgeWidthFactor);
    shader.setUniform1f("centerWidthFactor", centerWidthFactor);
    shader.setUniform1f("extendBeyondCanvas", extendBeyondCanvas);
    shader.setUniform1f("lineLengthMinFactor", lineLengthMinFactor);
    shader.setUniform1f("linePositionFadeWidth", linePositionFadeWidth);
    shader.setUniform1f("linePositionEdgeFactor", linePositionEdgeFactor);
    shader.setUniform1f("linePositionCenterFactor", linePositionCenterFactor);
  }

protected:
  std::string getVertexShader() override {
    return GLSL(
                layout(location = 0) in vec3 inPos;
                layout(location = 1) in vec2 instP0;
                layout(location = 2) in vec2 instP1;
                layout(location = 3) in float instWidth;
                layout(location = 4) in float instStyle;
                layout(location = 5) in vec4 instColor;

                uniform mat4 modelViewProjectionMatrix;
                uniform float maxTaperLength; // vary widths over this normalised length (e.g. 0.5)
                uniform float minWidthFactorStart; // when tapering, minimum width factor at start of taper, e.g. 0.6
                uniform float maxWidthFactorStart; // when tapering, maximum width factor at start of taper, e.g. 1.0
                uniform float minWidthFactorEnd; // when tapering, minimum width factor at end, e.g. 0.4
                uniform float maxWidthFactorEnd; // when tapering, maximum width factor at end, e.g. 0.9
                uniform float edgeFadeWidth; // 0 = disabled; >0 = per-endpoint width is interpolated within this normalised distance of any screen edge
                uniform float edgeWidthFactor; // width factor AT the edge (within the fade band). 0 = vanish, 1 = neutral, >1 = bulge.
                uniform float centerWidthFactor; // width factor FAR from edges (outside the band). 1 = full width (default), 0 = invisible in middle, >1 = bulge in middle.
                uniform float extendBeyondCanvas; // extend each line's drawn geometry past instP0/instP1 by this distance in normalised units along the line direction.
                uniform float lineLengthMinFactor; // 1 = disabled; <1 = short lines thinner (multiplier at len=0, approaches 1 at len>=maxTaperLength).
                uniform float linePositionFadeWidth; // 0 = disabled; >0 = band width (normalised) from canvas edges within which midpoint-based width modulation applies.
                uniform float linePositionEdgeFactor; // width factor for lines whose MIDPOINT sits at a canvas edge. 1 = neutral.
                uniform float linePositionCenterFactor; // width factor for lines whose MIDPOINT sits far from any canvas edge (toward centre). 1 = neutral.

                out vec2 vUv;
                out vec4 vColor;

                void main(){
                  vec2 dir = instP1 - instP0;
                  float len = max(length(dir), 1e-6);
                  vec2 t = dir / len;
                  vec2 n = vec2(-t.y, t.x);

                  vUv = inPos.xy + vec2(0.5);
                  vColor = instColor;

                  // Per-endpoint length-based width (with optional length taper).
                  // (Declared on separate lines because the GLSL() macro
                  // interprets top-level commas as argument separators.)
                  float baseStartW;
                  float baseEndW;
                  if (instStyle > 0.5) {
                    float widthFactor = clamp(len, 0.0, maxTaperLength) / maxTaperLength;
                    baseStartW = instWidth * mix(minWidthFactorStart, maxWidthFactorStart, widthFactor);
                    baseEndW   = instWidth * mix(minWidthFactorEnd, maxWidthFactorEnd, widthFactor);
                  } else {
                    baseStartW = instWidth;
                    baseEndW   = instWidth;
                  }

                  // Per-endpoint edge modulation. Each endpoint's width is interpolated
                  // within edgeFadeWidth of any screen edge, from edgeWidthFactor (at the
                  // edge) toward centerWidthFactor (at the band boundary and outside).
                  // Combinations:
                  //   edge=0, center=1 → original "fade to zero at edges"
                  //   edge=2, center=1 → bulge at edges, normal in middle
                  //   edge=1, center=0 → fade to zero IN THE MIDDLE (lines visible at edges only)
                  //   edge=2, center=0 → bulge at edges + fade to zero in middle
                  // Disabled (factor 1 everywhere) when edgeFadeWidth <= 0.
                  float edgeFactorP0 = 1.0;
                  float edgeFactorP1 = 1.0;
                  if (edgeFadeWidth > 0.0001) {
                    float edgeDistP0 = min(min(instP0.x, 1.0 - instP0.x), min(instP0.y, 1.0 - instP0.y));
                    float edgeDistP1 = min(min(instP1.x, 1.0 - instP1.x), min(instP1.y, 1.0 - instP1.y));
                    float ssP0 = smoothstep(0.0, edgeFadeWidth, edgeDistP0);
                    float ssP1 = smoothstep(0.0, edgeFadeWidth, edgeDistP1);
                    edgeFactorP0 = mix(edgeWidthFactor, centerWidthFactor, ssP0);
                    edgeFactorP1 = mix(edgeWidthFactor, centerWidthFactor, ssP1);
                  }

                  float startW = baseStartW * edgeFactorP0;
                  float endW   = baseEndW   * edgeFactorP1;

                  // Per-line length-based multiplier (uniform across the line).
                  // Short lines get lineLengthMinFactor; long lines (>=maxTaperLength) get 1.0.
                  float lengthFactor = 1.0;
                  if (lineLengthMinFactor < 0.9999 || lineLengthMinFactor > 1.0001) {
                    float lengthT = clamp(len, 0.0, maxTaperLength) / maxTaperLength;
                    lengthFactor = mix(lineLengthMinFactor, 1.0, lengthT);
                  }
                  startW *= lengthFactor;
                  endW   *= lengthFactor;

                  // STEP 4: per-line position-based multiplier. Computes midpoint
                  // distance to nearest canvas edge and mixes between two factors
                  // within a normalised band. Hardcoded 1.0/1.0 here = no visual
                  // effect; later steps will expose edge/center factors as uniforms.
                  float positionFactor = 1.0;
                  if (linePositionFadeWidth > 0.0001) {
                    vec2 lineMid = (instP0 + instP1) * 0.5;
                    float midDist = min(min(lineMid.x, 1.0 - lineMid.x), min(lineMid.y, 1.0 - lineMid.y));
                    float ssMid = smoothstep(0.0, linePositionFadeWidth, midDist);
                    positionFactor = mix(linePositionEdgeFactor, linePositionCenterFactor, ssMid);
                  }
                  startW *= positionFactor;
                  endW   *= positionFactor;

                  float halfW = mix(startW, endW, vUv.y) * 0.5;

                  // Extend the drawn line geometry past its original endpoints by
                  // extendBeyondCanvas units along the line direction — BUT ONLY
                  // for endpoints that actually sit on a canvas edge. Interior
                  // endpoints (away from any edge) are left alone, otherwise two
                  // lines that don't cross in their original instP0–instP1 form
                  // can visibly cross once both are extended, breaking
                  // DividedArea's non-intersection guarantee (which is enforced on
                  // the un-extended segments only). The original instP0/instP1
                  // still define the line's identity (edge-fade math above uses
                  // them); the drawn quad spans from drawP0 to drawP1. Width
                  // interpolation along the quad still uses startW->endW.
                  vec2 drawP0 = instP0;
                  vec2 drawP1 = instP1;
                  if (extendBeyondCanvas > 0.0001) {
                    float edgeDistP0_ext = min(min(instP0.x, 1.0 - instP0.x), min(instP0.y, 1.0 - instP0.y));
                    float edgeDistP1_ext = min(min(instP1.x, 1.0 - instP1.x), min(instP1.y, 1.0 - instP1.y));
                    if (edgeDistP0_ext < 0.001) drawP0 = instP0 - t * extendBeyondCanvas;
                    if (edgeDistP1_ext < 0.001) drawP1 = instP1 + t * extendBeyondCanvas;
                  }
                  vec2 base = mix(drawP0, drawP1, vUv.y);
                  float side = inPos.x; // -0.5..0.5
                  vec2 offset = n * side * (2.0 * halfW);
                  vec2 worldPos = base + offset;
                  gl_Position = modelViewProjectionMatrix * vec4(worldPos, 0.0, 1.0);
                }
                );
  }
  
  std::string getFragmentShader() override {
    return GLSL(
                in vec2 vUv;
                in vec4 vColor;
                out vec4 fragColor;
                
                void main(){
                    fragColor = vColor;
                }
                );
  }

};
