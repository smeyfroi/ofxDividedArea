#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include "ofColor.h"
#include "DividerLine.hpp"
#include "ofxGui.h"

class DividedArea {
public:
  glm::vec2 size { 1.0, 1.0 };
  int maxUnconstrainedDividerLines { -1 };
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
  ofParameter<float> constrainedOcclusionDistanceParameter { "constrainedOcclusionDistance", 0.003, 0.0, 0.01 };
  ofParameter<float> occlusionAngleParameter { "occlusionAngle", 0.90, 0.0, 1.0 }; // 0.0 if perpendicular, 1.0 if coincident
  ofParameter<int> maxConstrainedLinesParameter { "maxConstrainedLines", 500, 0, 10000 }; // TODO: 1000 is too slow until the mesh refactor
  ofParameterGroup& getParameterGroup();
};



template<typename PT, typename A>
bool containsPoint(const std::vector<PT, A>& points, glm::vec2 point) {
  return std::any_of(points.begin(),
                     points.end(),
                     [&](const auto& p) {
    return (glm::vec2(p) == point);
  });
}

template<typename PT, typename A>
std::optional<glm::vec2> findClosePoint(const std::vector<PT, A>& points, glm::vec2 point, float tolerance) {
  float tolerance2 = tolerance * tolerance;
  auto iter = std::find_if(points.begin(),
                           points.end(),
                           [&](const auto& p) {
    return glm::distance2(glm::vec2(p), point) < tolerance2;
  });
  if (iter != points.end()) {
    return *iter;
  } else {
    return std::nullopt;
  }
}
