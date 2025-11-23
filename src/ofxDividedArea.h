#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include "ofColor.h"
#include "DividerLine.hpp"
#include "ofxGui.h"
#include "ofVbo.h"
#include "ofBufferObject.h"
#include "ofShader.h"
#include "ofMesh.h"
#include "LineGeom.h"
#include "DividerLineShader.h"
#include "RefractiveRectangleShader.h"

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

  glm::vec2 size;
  int maxUnconstrainedDividerLines;
  DividerLines areaConstraints {
    {{0.0, 0.0}, {size.x, 0.0}, {0.0, 0.0}, {size.x, 0.0}},
    {{size.x, 0.0}, size, {size.x, 0.0}, size},
    {size, {0.0, size.y}, size, {0.0, size.y}},
    {{0.0, size.y}, {0.0, 0.0}, {0.0, size.y}, {0.0, 0.0}}
  };
  DividerLines unconstrainedDividerLines; // unconstrained, across the entire area
  DividerLines constrainedDividerLines; // constrained by all other divider lines
  
  bool addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  template<typename PT, typename A>
  bool updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints);
  
  void clearConstrainedDividerLines();
  void deleteEarlyConstrainedDividerLines(size_t count);
  DividerLine createConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) const;
  std::optional<DividerLine> addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2, ofFloatColor color, float overriddenWidth = -1.0);
  
  void draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float scale, const ofFbo& backgroundFbo);
  void draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, float scale = 1.0) const;
  
  std::string getParameterGroupName() const { return "Divided Area"; }
  ofParameterGroup parameters;
  ofParameter<float> lerpAmountParameter { "unconstrainedLerpAmount", 0.5, 0.0, 1.0 };
  ofParameter<float> closePointDistanceParameter { "unconstrainedClosePoint", 0.03, 0.0, 1.0 };
  ofParameter<float> unconstrainedOcclusionDistanceParameter { "unconstrainedOcclusionDistance", 0.05, 0.0, 0.1 };
  ofParameter<float> constrainedOcclusionDistanceParameter { "constrainedOcclusionDistance", 0.0015, 0.0, 0.01 };
  ofParameter<float> occlusionAngleParameter { "occlusionAngle", 0.97, 0.0, 1.0 }; // 0.0 if perpendicular, 1.0 if coincident
  ofParameter<int> maxConstrainedLinesParameter { "maxConstrainedLines", 800, 100, 10000 };
  ofParameter<float> maxTaperLengthParameter { "maxTaperLength", 1000.0, 100.0, 6000.0 }; // vary widths over this px length
  ofParameter<float> minWidthFactorStartParameter { "minWidthFactorStart", 0.6, 0.0, 1.0 }; // when tapering, minimum width factor at start of taper
  ofParameter<float> maxWidthFactorStartParameter { "maxWidthFactorStart", 1.0, 0.0, 1.0 }; // when tapering, maximum width factor at start of taper
  ofParameter<float> minWidthFactorEndParameter { "minWidthFactorEnd", 0.4, 0.0, 1.0 }; // when tapering, minimum width factor at end
  ofParameter<float> maxWidthFactorEndParameter { "maxWidthFactorEnd", 0.9, 0.0, 1.0 }; // when tapering, maximum width factor at end
  ofParameter<float> constrainedWidthParameter { "constrainedWidth", 1.0/500.0f, 0.0, 0.01 };

  ofParameterGroup& getParameterGroup();

  // Instanced rendering data
  void drawInstanced(float scale = 1.0f);
  void addDividerInstanced(const glm::vec2& a, const glm::vec2& b, float width, bool taper, const ofFloatColor& col);

private:
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

  RefractiveRectangleShader refractiveRectangleShader;
};
