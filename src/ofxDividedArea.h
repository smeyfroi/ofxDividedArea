#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include "ofColor.h"
#include "DividerLine.hpp"

class DividedArea {
public:
  glm::vec2 size {1.0, 1.0};
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
};
