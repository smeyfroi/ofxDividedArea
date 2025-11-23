//
//  MajorLineShaders.h
//  ofxDividedArea
//
//  Shader implementations for major (unconstrained) divider line styles.
//

#pragma once

#include "Shader.h"
#include "UnitQuadMesh.h"

// Base class for major line shaders that render oriented rectangles
class MajorLineShaderBase : public Shader {
public:
  virtual void render(const glm::vec2& p0, const glm::vec2& p1, float width,
                      const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) {
    glm::vec2 center = (p0 + p1) / 2.0f;
    float length = glm::distance(p0, p1) + width; // add width as end caps
    float angle = std::atan2(p1.y - p0.y, p1.x - p0.x);
    
    shader.begin();
    setUniforms(color, width, length, backgroundFbo);
    quadMesh.draw(center, { length, width }, angle);
    shader.end();
  }
  
  virtual ~MajorLineShaderBase() = default;
  
protected:
  virtual void setUniforms(const ofFloatColor& color, float width, float length,
                           const ofFbo* backgroundFbo) {
    shader.setUniform4f("lineColor", color);
    shader.setUniform1f("lineWidth", width);
    shader.setUniform1f("lineLength", length);
  }
  
  // Shared vertex shader that provides localPos in normalized coords
  std::string getVertexShader() override {
    return GLSL(
      in vec4 position;
      uniform mat4 modelViewProjectionMatrix;
      out vec2 fragTexCoord;
      out vec2 localPos;

      void main() {
        vec4 screenPos = modelViewProjectionMatrix * position;
        gl_Position = screenPos;
        
        // Convert NDC (-1 to 1) to texture coordinates (0 to 1)
        vec2 ndcPos = screenPos.xy / screenPos.w;
        fragTexCoord = ndcPos * 0.5 + 0.5;
        // Flip Y to match OF screen/FBO orientation consistently
        fragTexCoord.y = 1.0 - fragTexCoord.y;
        
        localPos = position.xy; // -0.5 to 0.5
      }
    );
  }
  
  UnitQuadMesh quadMesh;
};


// Solid color line shader
class SolidLineShader : public MajorLineShaderBase {
protected:
  std::string getFragmentShader() override {
    return GLSL(
      in vec2 localPos;
      out vec4 fragColor;
      uniform vec4 lineColor;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) {
          discard;
        }
        fragColor = lineColor;
      }
    );
  }
};


// Metallic shader - anisotropic specular across width
class MetallicLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> lightAngleParameter { "lightAngle", 0.0, -3.14159f, 3.14159f };
  ofParameter<float> highlightSharpnessParameter { "highlightSharpness", 20.0, 1.0, 100.0 };
  ofParameter<float> highlightIntensityParameter { "highlightIntensity", 1.0, 0.0, 3.0 };
  ofParameter<float> anisotropyFrequencyParameter { "anisotropyFreq", 30.0, 0.0, 100.0 };
  ofParameter<ofFloatColor> metalTintParameter { "metalTint", ofFloatColor(0.85, 0.86, 0.88, 1.0) };

  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Metallic Line");
      parameters.add(lightAngleParameter);
      parameters.add(highlightSharpnessParameter);
      parameters.add(highlightIntensityParameter);
      parameters.add(anisotropyFrequencyParameter);
      parameters.add(metalTintParameter);
    }
    return parameters;
  }

protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("lightAngle", lightAngleParameter);
    shader.setUniform1f("highlightSharpness", highlightSharpnessParameter);
    shader.setUniform1f("highlightIntensity", highlightIntensityParameter);
    shader.setUniform1f("anisotropyFreq", anisotropyFrequencyParameter);
    shader.setUniform4f("metalTint", metalTintParameter.get());
  }

  std::string getFragmentShader() override {
    return GLSL(
      in vec2 localPos;
      out vec4 fragColor;
      uniform vec4 lineColor;
      uniform float lightAngle;
      uniform float highlightSharpness;
      uniform float highlightIntensity;
      uniform float anisotropyFreq;
      uniform vec4 metalTint;

      void main() {
        vec2 p = localPos; // -0.5..0.5
        vec2 absLocal = abs(p);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) discard;

        // Normal across width (Y is width)
        vec2 n = normalize(vec2(0.0, sign(p.y) + 1e-5));
        // Light direction in local space
        vec2 L = vec2(cos(lightAngle), sin(lightAngle));
        // Specular-like term focused at center (y=0)
        float viewDotHalf = 1.0 - abs(p.y) * 2.0; // 1 at center, 0 at edge
        viewDotHalf = clamp(viewDotHalf, 0.0, 1.0);
        float spec = pow(viewDotHalf, highlightSharpness) * highlightIntensity;

        // Anisotropy streaks along length (x)
        float streaks = 0.5 + 0.5 * sin((p.x + 0.5) * anisotropyFreq * 6.28318);
        streaks *= 0.35; // subtle

        vec3 base = lineColor.rgb * metalTint.rgb;
        vec3 color = base + (spec + streaks) * vec3(1.0);
        fragColor = vec4(color, lineColor.a);
      }
    );
  }

private:
  ofParameterGroup parameters;
};

// Inner glow shader - bright edges, darker core
class InnerGlowLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> edgeBoostParameter { "edgeBoost", 1.0, 0.0, 3.0 };
  ofParameter<float> coreDarknessParameter { "coreDarkness", 0.5, 0.0, 1.0 };
  ofParameter<float> softnessParameter { "softness", 0.4, 0.0, 1.0 };

  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Inner Glow Line");
      parameters.add(edgeBoostParameter);
      parameters.add(coreDarknessParameter);
      parameters.add(softnessParameter);
    }
    return parameters;
  }

protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("edgeBoost", edgeBoostParameter);
    shader.setUniform1f("coreDarkness", coreDarknessParameter);
    shader.setUniform1f("softness", softnessParameter);
  }

  std::string getFragmentShader() override {
    return GLSL(
      in vec2 localPos;
      out vec4 fragColor;
      uniform vec4 lineColor;
      uniform float edgeBoost;
      uniform float coreDarkness;
      uniform float softness;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) discard;

        float t = absLocal.y * 2.0; // 0 at center, 1 at edge
        float edge = smoothstep(1.0 - softness, 1.0, t);
        float core = 1.0 - smoothstep(0.0, softness, t);

        float brightness = edge * edgeBoost + core * (1.0 - coreDarkness);
        vec3 col = lineColor.rgb * brightness;
        fragColor = vec4(col, lineColor.a);
      }
    );
  }

private:
  ofParameterGroup parameters;
};

// Bloomed additive shader - neon tube look
class BloomedAdditiveLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> coreIntensityParameter { "coreIntensity", 1.2, 0.0, 4.0 };
  ofParameter<float> haloRadiusParameter { "haloRadius", 0.5, 0.0, 1.0 };
  ofParameter<float> haloFalloffParameter { "haloFalloff", 6.0, 0.5, 20.0 };

  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Bloomed Additive Line");
      parameters.add(coreIntensityParameter);
      parameters.add(haloRadiusParameter);
      parameters.add(haloFalloffParameter);
    }
    return parameters;
  }

  void render(const glm::vec2& p0, const glm::vec2& p1, float width,
              const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) override {
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    MajorLineShaderBase::render(p0, p1, width * 2.0f, color, backgroundFbo);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  }

protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("coreIntensity", coreIntensityParameter);
    shader.setUniform1f("haloRadius", haloRadiusParameter);
    shader.setUniform1f("haloFalloff", haloFalloffParameter);
  }

  std::string getFragmentShader() override {
    return GLSL(
      in vec2 localPos;
      out vec4 fragColor;
      uniform vec4 lineColor;
      uniform float coreIntensity;
      uniform float haloRadius;
      uniform float haloFalloff;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) discard;

        float t = absLocal.y * 2.0; // 0 center, 1 edge
        float core = exp(-pow(t / max(0.001, 1.0 - haloRadius), 2.0) * haloFalloff) * coreIntensity;
        float halo = exp(-pow(max(0.0, t - (1.0 - haloRadius)) / max(0.001, haloRadius), 2.0) * haloFalloff);

        float intensity = core + halo;
        fragColor = vec4(lineColor.rgb * intensity, intensity * lineColor.a);
      }
    );
  }

private:
  ofParameterGroup parameters;
};


// Glow shader - additive blend with gaussian falloff (across width)
class GlowLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> glowFalloffParameter { "glowFalloff", 4.0, 0.5, 20.0 };
  ofParameter<float> glowIntensityParameter { "glowIntensity", 1.5, 0.0, 5.0 };
  ofParameter<float> coreWidthParameter { "coreWidth", 0.3, 0.0, 1.0 };
  
  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Glow Line");
      parameters.add(glowFalloffParameter);
      parameters.add(glowIntensityParameter);
      parameters.add(coreWidthParameter);
    }
    return parameters;
  }
  
  void render(const glm::vec2& p0, const glm::vec2& p1, float width,
              const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) override {
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    MajorLineShaderBase::render(p0, p1, width * 2.0f, color, backgroundFbo); // wider for glow
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  }
  
protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("glowFalloff", glowFalloffParameter);
    shader.setUniform1f("glowIntensity", glowIntensityParameter);
    shader.setUniform1f("coreWidth", coreWidthParameter);
  }
  
  std::string getFragmentShader() override {
    return GLSL(
      in vec2 localPos;
      out vec4 fragColor;
      uniform vec4 lineColor;
      uniform float glowFalloff;
      uniform float glowIntensity;
      uniform float coreWidth;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) {
          discard;
        }
        
        // Distance from center line across width (Y is width direction)
        // localPos.y goes from -0.5 to 0.5 across the line width
        float distFromCenter = absLocal.y * 2.0; // 0 at center, 1 at edge
        
        // Gaussian falloff across width
        float t = max(0.0, distFromCenter - coreWidth) / max(0.001, 1.0 - coreWidth);
        float intensity = exp(-t * t * glowFalloff) * glowIntensity;
        
        fragColor = vec4(lineColor.rgb * intensity, intensity * lineColor.a);
      }
    );
  }
  
private:
  ofParameterGroup parameters;
};


// Refractive line shader - glass-like distortion effect
class RefractiveLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> edgeThicknessParameter { "edgeThickness", 0.15, 0.0, 1.0 };
  ofParameter<float> refractionStrengthParameter { "refractionStrength", 0.06, 0.0, 0.2 };
  ofParameter<float> reflectionStrengthParameter { "reflectionStrength", 0.8, 0.0, 4.0 };
  ofParameter<float> reflectionFalloffParameter { "reflectionFalloff", 1.2, 0.0, 4.0 };
  ofParameter<float> reflectionOffsetParameter { "reflectionOffset", 0.05, 0.0, 1.0 };
  ofParameter<float> fresnelStrengthParameter { "fresnelStrength", 0.05, 0.0, 1.0 };
  ofParameter<float> fresnelFalloffParameter { "fresnelFalloff", 10.0, 0.0, 20.0 };
  
  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Refractive Line");
      parameters.add(edgeThicknessParameter);
      parameters.add(refractionStrengthParameter);
      parameters.add(reflectionStrengthParameter);
      parameters.add(reflectionFalloffParameter);
      parameters.add(reflectionOffsetParameter);
      parameters.add(fresnelStrengthParameter);
      parameters.add(fresnelFalloffParameter);
    }
    return parameters;
  }
  
  void render(const glm::vec2& p0, const glm::vec2& p1, float width,
              const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) override {
    if (!backgroundFbo) return; // requires background
    
    glm::vec2 center = (p0 + p1) / 2.0f;
    float length = glm::distance(p0, p1) + width;
    float angle = std::atan2(p1.y - p0.y, p1.x - p0.x);
    
    shader.begin();
    shader.setUniformTexture("backgroundTex", backgroundFbo->getTexture(), 0);
    setUniforms(color, width, length, backgroundFbo);
    quadMesh.draw(center, { length, width }, angle);
    shader.end();
  }
  
protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("edgeThicknessNorm", edgeThicknessParameter);
    shader.setUniform1f("refractionStrength", refractionStrengthParameter);
    shader.setUniform1f("reflectionStrength", reflectionStrengthParameter);
    shader.setUniform1f("reflectionFalloff", reflectionFalloffParameter);
    shader.setUniform1f("reflectionOffset", reflectionOffsetParameter);
    shader.setUniform1f("fresnelStrength", fresnelStrengthParameter);
    shader.setUniform1f("fresnelFalloff", fresnelFalloffParameter);
  }
  
  std::string getFragmentShader() override {
    return GLSL(
      in vec2 fragTexCoord;
      in vec2 localPos;
      out vec4 fragColor;

      uniform sampler2D backgroundTex;
      uniform float edgeThicknessNorm;
      uniform float refractionStrength;
      uniform float reflectionStrength;
      uniform float reflectionFalloff;
      uniform float reflectionOffset;
      uniform float fresnelStrength;
      uniform float fresnelFalloff;

      void main() {
        vec2 absLocal = abs(localPos);
        const float rectHalfSize = 0.5;
        
        if (absLocal.x > rectHalfSize || absLocal.y > rectHalfSize) {
          discard;
        }
        
        vec2 sampleUV = fragTexCoord;
        
        // Apply refraction at edges
        float distLeft = rectHalfSize - absLocal.x;
        float distTop = rectHalfSize - absLocal.y;
        float minDist = min(distLeft, distTop);
        
        if (minDist < edgeThicknessNorm) {
          float edgePercent = 1.0 - (minDist / edgeThicknessNorm);
          float falloff = pow(edgePercent, 3.0);
          
          vec2 normalLocal = vec2(0.0);
          
          if (minDist == distLeft) {
            normalLocal = vec2(sign(localPos.x), 0.0);
          } else {
            normalLocal = vec2(0.0, sign(localPos.y));
          }
          
          // Corner handling
          if (distLeft < edgeThicknessNorm && distTop < edgeThicknessNorm) {
            normalLocal = normalize(vec2(sign(localPos.x), sign(localPos.y)));
            float cornerFalloff = min(distLeft / edgeThicknessNorm, distTop / edgeThicknessNorm);
            falloff = pow(1.0 - cornerFalloff, 3.0);
          }
          
          float distortionAmount = falloff * refractionStrength;
          vec2 distortionUV = normalLocal * distortionAmount;
          
          sampleUV = fragTexCoord + distortionUV;
          sampleUV = clamp(sampleUV, vec2(0.0), vec2(1.0));
        }
        
        vec4 backgroundColor = texture(backgroundTex, sampleUV);
        
        // Reflection effect
        vec2 centerDist = absLocal / rectHalfSize;
        float distFromCenter = length(centerDist);
        
        float reflectionMask = (1.0 - distFromCenter) * reflectionStrength;
        reflectionMask = pow(reflectionMask, reflectionFalloff);
        reflectionMask = clamp(reflectionMask, 0.0, 1.0);
        
        if (reflectionMask > 0.01 && length(localPos) > 0.01) {
          vec2 reflectOff = normalize(localPos) * reflectionOffset;
          vec2 reflectUV = fragTexCoord - reflectOff;
          
          if (reflectUV.x >= 0.0 && reflectUV.x <= 1.0 && reflectUV.y >= 0.0 && reflectUV.y <= 1.0) {
            vec4 reflection = texture(backgroundTex, reflectUV);
            backgroundColor.rgb = mix(backgroundColor.rgb, reflection.rgb, reflectionMask);
          }
        }
        
        // Fresnel edge brightness
        float edgeOnly = min(absLocal.x, absLocal.y) / rectHalfSize;
        edgeOnly = 1.0 - edgeOnly;
        float edgeBrightness = pow(distFromCenter, fresnelFalloff) * fresnelStrength * edgeOnly;
        backgroundColor.rgb += vec3(edgeBrightness);
        
        fragColor = backgroundColor;
      }
    );
  }
  
private:
  ofParameterGroup parameters;
};


// Chromatic aberration shader - RGB channel split at edges
class ChromaticAberrationLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> aberrationStrengthParameter { "aberrationStrength", 0.02, 0.0, 0.1 };
  ofParameter<float> edgeThicknessParameter { "edgeThickness", 0.3, 0.0, 1.0 };
  
  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Chromatic Aberration Line");
      parameters.add(aberrationStrengthParameter);
      parameters.add(edgeThicknessParameter);
    }
    return parameters;
  }
  
  void render(const glm::vec2& p0, const glm::vec2& p1, float width,
              const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) override {
    if (!backgroundFbo) return; // requires background
    
    glm::vec2 center = (p0 + p1) / 2.0f;
    float length = glm::distance(p0, p1) + width;
    float angle = std::atan2(p1.y - p0.y, p1.x - p0.x);
    
    shader.begin();
    shader.setUniformTexture("backgroundTex", backgroundFbo->getTexture(), 0);
    setUniforms(color, width, length, backgroundFbo);
    quadMesh.draw(center, { length, width }, angle);
    shader.end();
  }
  
protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("aberrationStrength", aberrationStrengthParameter);
    shader.setUniform1f("edgeThickness", edgeThicknessParameter);
  }
  
  std::string getFragmentShader() override {
    return GLSL(
      in vec2 fragTexCoord;
      in vec2 localPos;
      out vec4 fragColor;
      
      uniform sampler2D backgroundTex;
      uniform float aberrationStrength;
      uniform float edgeThickness;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) {
          discard;
        }
        
        // Distance from edge
        float distFromEdgeX = 0.5 - absLocal.x;
        float distFromEdgeY = 0.5 - absLocal.y;
        float minDistFromEdge = min(distFromEdgeX, distFromEdgeY);
        
        // Edge factor (1 at edge, 0 at center)
        float edgeFactor = 1.0 - smoothstep(0.0, edgeThickness * 0.5, minDistFromEdge);
        edgeFactor = pow(edgeFactor, 2.0);
        
        // Direction from center
        vec2 dir = normalize(localPos + vec2(0.001));
        vec2 offset = dir * aberrationStrength * edgeFactor;
        
        // Sample RGB with offset
        float r = texture(backgroundTex, fragTexCoord + offset).r;
        float g = texture(backgroundTex, fragTexCoord).g;
        float b = texture(backgroundTex, fragTexCoord - offset).b;
        
        fragColor = vec4(r, g, b, 1.0);
      }
    );
  }
  
private:
  ofParameterGroup parameters;
};

// Blur/refraction shader - screen-space blur with mild refraction near edges
class BlurRefractionLineShader : public MajorLineShaderBase {
public:
  ofParameter<float> blurRadiusParameter { "blurRadius", 1.5, 0.0, 8.0 }; // in pixels
  ofParameter<float> refractionStrengthParameter { "refractionStrength", 0.015, 0.0, 0.1 };

  ofParameterGroup& getParameterGroup() {
    if (parameters.size() == 0) {
      parameters.setName("Blur/Refraction Line");
      parameters.add(blurRadiusParameter);
      parameters.add(refractionStrengthParameter);
    }
    return parameters;
  }

  void render(const glm::vec2& p0, const glm::vec2& p1, float width,
              const ofFloatColor& color, const ofFbo* backgroundFbo = nullptr) override {
    if (!backgroundFbo) return;
    glm::vec2 center = (p0 + p1) / 2.0f;
    float length = glm::distance(p0, p1) + width;
    float angle = std::atan2(p1.y - p0.y, p1.x - p0.x);

    shader.begin();
    shader.setUniformTexture("backgroundTex", backgroundFbo->getTexture(), 0);
    shader.setUniform2f("invResolution", 1.0f / backgroundFbo->getWidth(), 1.0f / backgroundFbo->getHeight());
    setUniforms(color, width, length, backgroundFbo);
    quadMesh.draw(center, { length, width }, angle);
    shader.end();
  }

protected:
  void setUniforms(const ofFloatColor& color, float width, float length,
                   const ofFbo* backgroundFbo) override {
    MajorLineShaderBase::setUniforms(color, width, length, backgroundFbo);
    shader.setUniform1f("blurRadius", blurRadiusParameter);
    shader.setUniform1f("refractStrength", refractionStrengthParameter);
  }

  std::string getFragmentShader() override {
    return GLSL(
      in vec2 fragTexCoord;
      in vec2 localPos;
      out vec4 fragColor;
      
      uniform sampler2D backgroundTex;
      uniform vec2 invResolution;
      uniform float blurRadius;
      uniform float refractStrength;

      void main() {
        vec2 absLocal = abs(localPos);
        if (absLocal.x > 0.5 || absLocal.y > 0.5) discard;

        // Mild refraction near edges based on distance to edge
        float edgeDist = min(0.5 - absLocal.x, 0.5 - absLocal.y);
        float edgeFactor = 1.0 - smoothstep(0.0, 0.1, edgeDist);
        vec2 normalLocal = (absLocal.x < absLocal.y) ? vec2(sign(localPos.x), 0.0) : vec2(0.0, sign(localPos.y));
        vec2 refractUV = fragTexCoord + normalLocal * refractStrength * edgeFactor;

        // 9-tap box blur around refracted UV
        vec2 texel = invResolution * blurRadius;
        vec4 sum = vec4(0.0);
        for (int dx = -1; dx <= 1; ++dx) {
          for (int dy = -1; dy <= 1; ++dy) {
            sum += texture(backgroundTex, refractUV + vec2(dx, dy) * texel);
          }
        }
        vec4 blurred = sum / 9.0;
        fragColor = blurred;
      }
    );
  }

private:
  ofParameterGroup parameters;
};
