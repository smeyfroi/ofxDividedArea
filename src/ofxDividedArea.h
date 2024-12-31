#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include "ofColor.h"

class DividerLine;
using DividerLines = std::vector<DividerLine>;

struct Line {
  glm::vec2 start, end;
};

constexpr Line longestLine {
  {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()},
  {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}
};

struct LineConfig {
  float minWidth { 0.0 }, maxWidth { 0.0 };
  ofColor color;
  float adaptiveWidthMaxLength { 0.0 };
};

// A line that divides a DividedPlane, with start and end points contained
// by the plane, originally defined by a pair of reference points somewhere along its length
class DividerLine {
public:
  glm::vec2 ref1 {0.0, 0.0}, ref2 {0.0, 0.0};
  glm::vec2 start {0.0, 0.0}, end {0.0, 0.0};

  static Line findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine = longestLine);
  static DividerLine create(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine = longestLine);
  void draw(float width) const;
  void draw(const LineConfig& config) const;
  float gradient() const;
  float length() const;
  bool isOccludedBy(const DividerLine& dividerLine, float distanceTolerance, float gradientTolerance) const;
  bool isOccludedByAny(const DividerLines& dividerLines, float distanceTolerance = 10.0, float gradientTolerance = 0.97) const; // gradients close when dot product > gradientTolerance (dot product == 1 when codirectional)

  template<typename PT>
  static bool isRefPointUsed(const DividerLines& dividerLines, const PT refPoint);

private:
  static float gradient(glm::vec2 start, glm::vec2 end);
  static float yForLineAtX(float x, glm::vec2 start, glm::vec2 end);
  static float xForLineAtY(float y, glm::vec2 start, glm::vec2 end);
  static std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd);
  static float pointToLineDistance(glm::vec2 point, const DividerLine& line);
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
  
  bool addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
  
  template<typename PT, typename A>
  bool updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints);
  
  void clearConstrainedDividerLines();
  void deleteEarlyConstrainedDividerLines(size_t count);
  DividerLine createConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) const;
  std::optional<DividerLine> addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2);
//  void updateConstrainedDividerLines();

  void draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth) const;
  void draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, LineConfig constrainedLineConfig) const;
};
