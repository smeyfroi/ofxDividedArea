#pragma once

#include "glm/vec2.hpp"
#include <vector>
#include "ofColor.h"
#include "ofVboMesh.h"

// Notes:
// - pointToLineDistance: For zero-length lines (start≈end), returns distance to start point.
// - isOccludedBy: Zero-length lines never occlude/are occluded. Occlusion requires
//   * similar direction (|dot| >= gradientTolerance),
//   * small perpendicular distance between spans (< distanceTolerance),
//   * and overlap along the tangent direction (EPS-aware).

class DividerLine;
using DividerLines = std::vector<DividerLine>;

struct Line {
  glm::vec2 start, end;
};

// NOTE: if this is too large then precision issues cause trouble
constexpr Line longestLine {
  { -1e4, -1e4 },
  { 1e4, 1e4 }
};

struct LineConfig {
  float minWidth { 0.0 }, maxWidth { 0.0 };
  ofColor color;
  float adaptiveWidthMaxLength { 0.0 };
  void scale(float scale) {
    minWidth /= scale;
    maxWidth /= scale;
  }
};

// A line with start and end points contained by constraining lines,
// originally defined by a pair of reference points somewhere along its length
class DividerLine {
public:
  glm::vec2 ref1 {0.0, 0.0}, ref2 {0.0, 0.0};
  glm::vec2 start {0.0, 0.0}, end {0.0, 0.0};
  int age = 0;
  mutable ofVboMesh mesh;

  static Line findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine = longestLine);
  
  // Templated version for containers of DividerLine subclasses (e.g., SmoothedDividerLine)
  template<typename Container>
  static Line findEnclosedLineIn(glm::vec2 ref1, glm::vec2 ref2, const Container& constraints, const Line& startLine = longestLine);
  
  static DividerLine create(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine = longestLine);
  void draw(float width) const;
  void draw(const LineConfig& config) const;
  bool isOccludedBy(const DividerLine& dividerLine, float distanceTolerance, float gradientTolerance) const;
  bool isOccludedByAny(const DividerLines& dividerLines, float distanceTolerance, float gradientTolerance) const; // gradients close when dot product > gradientTolerance (dot product == 1 when codirectional)
  
  // Templated version for containers of DividerLine subclasses (e.g., SmoothedDividerLine)
  template<typename Container>
  bool isOccludedByAnyOf(const Container& dividerLines, float distanceTolerance, float gradientTolerance) const;

  template<typename PT>
  static bool isRefPointUsed(const DividerLines& dividerLines, const PT refPoint, const float closePointDistance);

//private:
  static float pointToLineDistance(glm::vec2 point, const DividerLine& line);
};
