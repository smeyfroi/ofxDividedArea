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
  void begin(float maxTaperLength, float minWidthFactorStart, float maxWidthFactorStart, float minWidthFactorEnd, float maxWidthFactorEnd) {
    Shader::begin();
    shader.setUniform1f("maxTaperLength", maxTaperLength);
    shader.setUniform1f("minWidthFactorStart", minWidthFactorStart);
    shader.setUniform1f("maxWidthFactorStart", maxWidthFactorStart);
    shader.setUniform1f("minWidthFactorEnd", minWidthFactorEnd);
    shader.setUniform1f("maxWidthFactorEnd", maxWidthFactorEnd);
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
                uniform float maxTaperLength; // vary widths over this px length, e.g. 1000
                uniform float minWidthFactorStart; // when tapering, minimum width factor at start of taper, e.g. 0.6
                uniform float maxWidthFactorStart; // when tapering, maximum width factor at start of taper, e.g. 1.0
                uniform float minWidthFactorEnd; // when tapering, minimum width factor at end, e.g. 0.4
                uniform float maxWidthFactorEnd; // when tapering, maximum width factor at end, e.g. 0.9

                out vec2 vUv;
                out vec4 vColor;

                void main(){
                  vec2 dir = instP1 - instP0;
                  float len = max(length(dir), 1e-6);
                  vec2 t = dir / len;
                  vec2 n = vec2(-t.y, t.x);
                  
                  vUv = inPos.xy + vec2(0.5);
                  vColor = instColor;
                  
                  float halfW;
                  if (instStyle > 0.5) {
                    float widthFactor = clamp(len, 0.0, maxTaperLength) / maxTaperLength;
                    float startW = instWidth * mix(minWidthFactorStart, maxWidthFactorStart, widthFactor);
                    float endW   = instWidth * mix(minWidthFactorEnd, maxWidthFactorEnd, widthFactor);
                    halfW = mix(startW, endW, vUv.y) * 0.5;
                  } else {
                    halfW = instWidth * 0.5;
                  }
                  
                  vec2 base = mix(instP0, instP1, vUv.y);
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
