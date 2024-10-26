#pragma once

#include "glm/vec2.hpp"
#include <vector>

class DividerLine;
using DividerLines = std::vector<DividerLine>;

// A line that divides a DividedPlane, with start and end points contained
// by the plane, originally defined by a pair of reference points somewhere along its length
class DividerLine {
  static float yForLineAtX(float x, glm::vec2 start, glm::vec2 end);
  static float xForLineAtY(float y, glm::vec2 start, glm::vec2 end);
  static std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd);

public:
  static DividerLine create(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, glm::vec2 planeSize);
  void draw(float width) const;
  
  glm::vec2 ref1 {0.0, 0.0}, ref2 {0.0, 0.0};
  glm::vec2 start {0.0, 0.0}, end {0.0, 0.0};

};
