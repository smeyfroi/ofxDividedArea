#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "glm/vec2.hpp"
#include "ofColor.h"
#include "DividerLine.hpp"
#include "SmoothedDividerLine.hpp"
#include "ofxGui.h"
#include "ofVbo.h"
#include "ofBufferObject.h"
#include "ofShader.h"
#include "ofMesh.h"
#include "LineGeom.h"
#include "DividerLineShader.h"
#include "MajorLineStyle.h"
#include "MajorLineShaders.h"

struct DividerInstance {
  glm::vec2 p0;
  glm::vec2 p1;
  float width;
  float style;
  ofFloatColor color;
};

class DividedArea {
public:
  DividedArea(glm::vec2 size = {1.0, 1.0}, int maxUnconstrainedDividerLines = 3);

  struct ParameterOverrides {
    std::optional<float> unconstrainedSmoothness;

    bool operator==(const ParameterOverrides& other) const {
      return unconstrainedSmoothness == other.unconstrainedSmoothness;
    }
    bool operator!=(const ParameterOverrides& other) const { return !(*this == other); }
  };

  void setParameterOverrides(const ParameterOverrides& overrides);
  void clearParameterOverrides();

  glm::vec2 size;
  int maxUnconstrainedDividerLines;
  DividerLines areaConstraints {
    {{0.0, 0.0}, {size.x, 0.0}, {0.0, 0.0}, {size.x, 0.0}},
    {{size.x, 0.0}, size, {size.x, 0.0}, size},
    {size, {0.0, size.y}, size, {0.0, size.y}},
    {{0.0, size.y}, {0.0, 0.0}, {0.0, size.y}, {0.0, 0.0}}
  };
  std::vector<SmoothedDividerLine> unconstrainedDividerLines; // unconstrained, across the entire area, with velocity-based smoothing
  DividerLines constrainedDividerLines; // constrained by all other divider lines
  
  bool addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  template<typename PT, typename A>
  bool updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints);
  
  void clearConstrainedDividerLines();
  void deleteEarlyConstrainedDividerLines(size_t count);
  DividerLine createConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) const;
  // taper = false (default) renders uniform-width rectangles. taper = true renders
  // rhomboids using the maxTaperLength + minWidthFactorStart/End + maxWidthFactorStart/End
  // parameters (see DividerLineShader). Callers opt in explicitly per-call.
  std::optional<DividerLine> addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2, ofFloatColor color, float overriddenWidth = -1.0, bool taper = false);
  
  void draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float scale, const ofFbo& backgroundFbo, const ofFloatColor& color = ofFloatColor(1.0f));
  
  /// Draw major (unconstrained) lines without a background FBO.
  /// Only works with background-free styles (Solid, InnerGlow, BloomedAdditive, Glow).
  /// If the current style requires a background FBO, falls back to Solid.
  void drawMajorLinesWithoutBackground(float unconstrainedLineWidth, float scale, const ofFloatColor& color = ofFloatColor(1.0f));
  void draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, float scale = 1.0) const;
  void draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, float scale, const ofFbo& backgroundFbo);
  
  std::string getParameterGroupName() const { return "Divided Area"; }
  ofParameterGroup parameters;
  ofParameter<float> lerpAmountParameter { "unconstrainedLerpAmount", 0.5, 0.0, 1.0 }; // DEPRECATED: use unconstrainedSmoothness instead
  ofParameter<float> unconstrainedSmoothnessParameter { "unconstrainedSmoothness", 0.5, 0.0, 1.0 }; // 0=responsive, 1=dreamy
  ofParameter<float> minRefPointDistanceParameter { "minRefPointDistance", 0.08, 0.0, 0.3 }; // below this, damping increases to prevent angular jitter
  ofParameter<float> closePointDistanceParameter { "unconstrainedClosePoint", 0.03, 0.0, 1.0 };
  ofParameter<float> unconstrainedOcclusionDistanceParameter { "unconstrainedOcclusionDistance", 0.05, 0.0, 0.1 };
  ofParameter<float> constrainedOcclusionDistanceParameter { "constrainedOcclusionDistance", 0.0015, 0.0, 0.01 };
  ofParameter<float> occlusionAngleParameter { "occlusionAngle", 0.97, 0.0, 1.0 }; // 0.0 if perpendicular, 1.0 if coincident
  ofParameter<int> maxConstrainedLinesParameter { "maxConstrainedLines", 800, 50, 10000 };
  ofParameter<float> maxTaperLengthParameter { "maxTaperLength", 0.5, 0.01, 2.0 }; // vary widths over this NORMALISED length. DividerLineShader computes line length from normalised instance positions, so this comparison must also be in normalised units. Was 1000 in pixels which mismatched the shader's normalised len (always ~0..1.4) and forced widthFactor to ~0, collapsing every line to the MIN end of its taper width-factor range (~40-60% of configured PathWidth). Default 0.5 means lines half-screen or longer render at full width; shorter lines taper narrower as designed.
  ofParameter<float> minWidthFactorStartParameter { "minWidthFactorStart", 0.6, 0.0, 1.0 }; // when tapering, minimum width factor at start of taper
  ofParameter<float> maxWidthFactorStartParameter { "maxWidthFactorStart", 1.0, 0.0, 1.0 }; // when tapering, maximum width factor at start of taper
  ofParameter<float> minWidthFactorEndParameter { "minWidthFactorEnd", 0.4, 0.0, 1.0 }; // when tapering, minimum width factor at end
  ofParameter<float> maxWidthFactorEndParameter { "maxWidthFactorEnd", 0.9, 0.0, 1.0 }; // when tapering, maximum width factor at end
  // Edge-fade taper (per-endpoint, independent of the length-based MinorLineTaper).
  // 0 = disabled. >0 = each endpoint's width is interpolated within this normalised
  // distance from any screen edge, from `edgeWidthFactor` at the edge toward 1.0
  // at the band boundary (full width outside the band).
  ofParameter<float> edgeFadeWidthParameter { "edgeFadeWidth", 0.0, 0.0, 0.5 };
  // Width factor AT the screen edge (within the fade band). The line endpoint's
  // width is `instWidth × mix(edgeWidthFactor, centerWidthFactor, smoothstep(0, edgeFadeWidth, edgeDist))`.
  // Values < 1.0 fade the line thinner toward the edge (0 = vanish at edge).
  // Values > 1.0 bulge the line thicker toward the edge (e.g. 2.0 = double width at edge).
  // Default 0.0 preserves the original "fade to zero at edges" behaviour.
  ofParameter<float> edgeWidthFactorParameter { "edgeWidthFactor", 0.0, 0.0, 4.0 };
  // Width factor FAR from any screen edge (outside the fade band, i.e. toward the
  // centre). Default 1.0 = normal full width in the middle (existing behaviour).
  // Set < 1.0 to thin lines in the middle (e.g. 0 = invisible in middle, paired with
  // edgeWidthFactor > 0 gives "thick at edges, fade to zero in middle"). Set > 1.0
  // to bulge in the middle relative to edges. Combined with `edgeFadeWidth`, the
  // fade band becomes a smooth gradient between the two factor values.
  ofParameter<float> centerWidthFactorParameter { "centerWidthFactor", 1.0, 0.0, 4.0 };
  // Extend each line's drawn geometry past its endpoints by this distance (in
  // normalised line-direction units, so 0.05 extends each end of every line by
  // an additional 5% of the canvas-width-equivalent in the line's direction).
  // Default 0 = draw exactly between instP0 and instP1. Used to push line
  // geometry past the [0, 1] visible canvas bounds so fluid advection at edges
  // doesn't leave missing patches — the line gets re-painted into the boundary
  // every frame from the extended geometry, even as advection pulls content
  // off-screen. Width interpolation along the extension uses the original
  // endpoint widths (the extension is treated as a continuation of the line).
  ofParameter<float> extendBeyondCanvasParameter { "extendBeyondCanvas", 0.0, 0.0, 0.2 };
  // Per-line length-based width scaling. Short lines (length=0) get this factor;
  // long lines (length>=maxTaperLength) get 1.0. Default 1.0 = disabled.
  // Use values <1 to make short lines thinner (sense of depth). Independent of
  // the per-endpoint length taper (MinorLineTaper).
  ofParameter<float> lineLengthMinFactorParameter { "lineLengthMinFactor", 1.0, 0.0, 4.0 };
  // Per-line position-based width modulation (independent of per-endpoint
  // edge fade). `linePositionFadeWidth` defines a normalised band measured
  // from canvas edges inward; the line's midpoint distance to the nearest
  // edge selects between `linePositionEdgeFactor` (line midpoint near edge)
  // and `linePositionCenterFactor` (line midpoint far from edges, i.e.
  // toward centre). Default disabled (linePositionFadeWidth = 0).
  ofParameter<float> linePositionFadeWidthParameter { "linePositionFadeWidth", 0.0, 0.0, 0.5 };
  // Width multiplier applied to lines whose midpoint sits AT a canvas edge.
  // Default 1.0 = neutral. <1 thins lines near edges; >1 bulges them.
  ofParameter<float> linePositionEdgeFactorParameter { "linePositionEdgeFactor", 1.0, 0.0, 4.0 };
  // Width multiplier applied to lines whose midpoint sits FAR from any canvas
  // edge (i.e. toward centre). Default 1.0 = neutral. Combined with
  // `linePositionEdgeFactor`, gives a smooth midpoint-position gradient:
  //   edge=1, center=0.3 → thinner in the middle of canvas (depth cue)
  //   edge=0.3, center=1 → thinner toward edges (frame-focus)
  ofParameter<float> linePositionCenterFactorParameter { "linePositionCenterFactor", 1.0, 0.0, 4.0 };
  ofParameter<float> constrainedWidthParameter { "constrainedWidth", 1.0/500.0f, 0.0, 0.01 };
  ofParameter<int> majorLineStyleParameter { "majorLineStyle", static_cast<int>(MajorLineStyle::Refractive), 0, static_cast<int>(MajorLineStyle::Count) - 1 };

  ofParameterGroup& getParameterGroup();
  MajorLineStyle getMajorLineStyle() const { return static_cast<MajorLineStyle>(majorLineStyleParameter.get()); }
  void setMajorLineStyle(MajorLineStyle style) { majorLineStyleParameter = static_cast<int>(style); }

  // Instanced rendering data
  void drawInstanced(float scale = 1.0f);
  void addDividerInstanced(const glm::vec2& a, const glm::vec2& b, float width, bool taper, const ofFloatColor& col);

private:
  float getUnconstrainedSmoothnessEffective() const;

  void setupInstancedDraw(int instanceNumber);
  std::vector<DividerInstance> instances; // ring buffer
  mutable ofBufferObject instanceBO; // GPU buffer for instances
  mutable ofVbo vbo; // instance vertices
  ofMesh quad; // for each instance
  DividerLineShader shader; // instanced render
  int instanceCapacity = 0;
  mutable int instanceCount = 0;
  int head = 0;
  mutable bool instancesDirty = false;

  // Major line style shaders (lazy-loaded)
  std::unique_ptr<SolidLineShader> solidLineShader;
  std::unique_ptr<InnerGlowLineShader> innerGlowLineShader;
  std::unique_ptr<BloomedAdditiveLineShader> bloomedAdditiveLineShader;
  std::unique_ptr<GlowLineShader> glowLineShader;
  std::unique_ptr<RefractiveLineShader> refractiveLineShader;
  std::unique_ptr<BlurRefractionLineShader> blurRefractionLineShader;
  std::unique_ptr<ChromaticAberrationLineShader> chromaticAberrationLineShader;
  
  void drawMajorLine(const DividerLine& dl, float width, float scale, 
                     const ofFloatColor& color, const ofFbo* backgroundFbo);

  ParameterOverrides parameterOverrides_;
};
