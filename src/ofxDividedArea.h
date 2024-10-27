#pragma once

#include "glm/vec2.hpp"
#include <vector>


class DividerLine;
using DividerLines = std::vector<DividerLine>;

struct Line {
  glm::vec2 start, end;
};

const Line longestLine {
  {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()},
  {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}
};


// A line that divides a DividedPlane, with start and end points contained
// by the plane, originally defined by a pair of reference points somewhere along its length
class DividerLine {
public:
  glm::vec2 ref1 {0.0, 0.0}, ref2 {0.0, 0.0};
  glm::vec2 start {0.0, 0.0}, end {0.0, 0.0};

  static Line findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, const Line& startLine = longestLine);
  static DividerLine create(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, const Line& startLine = longestLine);
  void draw(float width) const;
  bool isSimilarTo(const DividerLine& dividerLine, float distanceTolerance) const;
  
private:
  static float yForLineAtX(float x, glm::vec2 start, glm::vec2 end);
  static float xForLineAtY(float y, glm::vec2 start, glm::vec2 end);
  static std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd);
};


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
  bool hasSimilarUnconstrainedDividerLine(const DividerLine& dividerLine) const;
  bool addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  bool updateUnconstrainedDividerLines(const std::vector<glm::vec2>& majorRefPoints, const std::vector<size_t>& candidateRefPointIndices);
  bool addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  void draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth) const;
};
