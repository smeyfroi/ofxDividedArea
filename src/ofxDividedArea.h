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

struct DividerInstance {
  glm::vec2 p0;
  glm::vec2 p1;
  float width;
  float style;
  ofFloatColor color;
};

class DividedArea {
public:
  glm::vec2 size { 1.0, 1.0 };
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
  std::optional<DividerLine> addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  
  void draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth, float scale = 1.0) const;
  void draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, LineConfig constrainedLineConfig, float scale = 1.0) const;
  
  std::string getParameterGroupName() const { return "Divided Area"; }
  ofParameterGroup parameters;
  ofParameter<float> lerpAmountParameter { "lerpAmount", 0.5, 0.0, 1.0 };
  ofParameter<float> closePointDistanceParameter { "closePoint", 0.03, 0.0, 1.0 };
  ofParameter<float> unconstrainedOcclusionDistanceParameter { "unconstrainedOcclusionDistance", 0.05, 0.0, 0.1 };
  ofParameter<float> constrainedOcclusionDistanceParameter { "constrainedOcclusionDistance", 0.0015, 0.0, 0.01 };
  ofParameter<float> occlusionAngleParameter { "occlusionAngle", 0.97, 0.0, 1.0 }; // 0.0 if perpendicular, 1.0 if coincident
  ofParameter<int> maxConstrainedLinesParameter { "maxConstrainedLines", 1000, 0, 10000 };
  ofParameterGroup& getParameterGroup();

  // Instanced rendering data
  void drawInstanced(float scale = 1.0f) const;
  void setMaxDividers(int max);
  void clearInstanced();
  void addDividerInstanced(const glm::vec2& a, const glm::vec2& b, float width, bool taper, const ofFloatColor& col);

private:
  // instance ring buffer
  std::vector<DividerInstance> instances;
  mutable ofBufferObject instanceBO;
  mutable ofVbo vbo;
  ofMesh quad;
  ofShader shader;
  int instanceCapacity = 0;
  mutable int instanceCount = 0;
  int head = 0;
  mutable bool instancesDirty = false;

};
