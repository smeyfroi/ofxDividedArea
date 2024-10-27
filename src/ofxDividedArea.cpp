#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"

// y = mx + b
float DividerLine::yForLineAtX(float x, glm::vec2 start, glm::vec2 end) {
  float m = (end.y - start.y) / (end.x - start.x);
  float b = start.y - (m * start.x);
  return m * x + b;
}

// y = mx + b
float DividerLine::xForLineAtY(float y, glm::vec2 start, glm::vec2 end) {
  float m = (end.y - start.y) / (end.x - start.x);
  float b = start.y - (m * start.x);
  return (y - b) / m;
}

std::optional<glm::vec2> DividerLine::lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd) {
  float x, y;
  if (lsEnd.x - lsStart.x == 0.0) {
    x = lsStart.x;
    y = yForLineAtX(lsStart.x, lStart, lEnd);
  } else if (lsEnd.y - lsStart.y == 0.0) {
    y = lsStart.y;
    x = xForLineAtY(lsStart.y, lStart, lEnd);
  } else {
    // y=ax+c and y=bx+d (https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_line_equations)
    float a = (lEnd.y - lStart.y) / (lEnd.x - lStart.x);
    float b = (lsEnd.y - lsStart.y) / (lsEnd.x - lsStart.x);
    float c = lStart.y - (a * lStart.x);
    float d = lsStart.y - (b * lsStart.x);
    if (a == b) return std::nullopt;
    x = (d - c) / (a - b);
    y = a * x + c;
  }
  
  if (x < std::min({lsStart.x, lsEnd.x}) || x > std::max({lsStart.x, lsEnd.x})
      || y < std::min({lsStart.y, lsEnd.y}) || y > std::max({lsStart.y, lsEnd.y})) {
    return std::nullopt;
  }
  return glm::vec2 { x, y };
}

Line DividerLine::findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, const Line& startLine) {
  // Sort ref1 to be lower x than ref2
  if (ref1.x > ref2.x) std::swap(ref1, ref2);

  glm::vec2 start = startLine.start;
  glm::vec2 end = startLine.end;

  for (const auto& constraint : constraints) {
    if (auto intersectionResult = lineToSegmentIntersection(ref1, ref2, constraint.start, constraint.end)) {
      glm::vec2 intersection = intersectionResult.value();
      float distRef1New = glm::distance(intersection, ref1);
      if (intersection.x < ref1.x) {
        float distRef1Start = glm::distance(start, ref1);
        if (distRef1New < distRef1Start) start = intersection;
      } else {
        float distRef1End = glm::distance(end, ref1);
        if (distRef1New < distRef1End) end = intersection;
      }
    }
  }
  
  return Line { start, end };
}

// Look for the shortest constrained line segment centred on ref1, optionally starting with a line segment to be constrained
DividerLine DividerLine::create(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, const Line& startLine) {
  Line constrainedLine = findEnclosedLine(ref1, ref2, constraints, startLine);
  return DividerLine {ref1, ref2, constrainedLine.start, constrainedLine.end};
}

void DividerLine::draw(float width) const {
  if (width <= 0.0) return;
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));

//  ofPath path;
//  path.moveTo(0.0, 0.0);
//  path.lineTo(glm::distance(start, end), -width/2.0);
//  path.lineTo(glm::distance(start, end), width/2.0);
//  path.lineTo(0.0, 0.0);
//  path.setFilled(true);
//  path.draw();
  
  ofDrawRectangle(0.0, -width/2.0, glm::length(end-start), width);
  ofPopMatrix();
}

bool DividerLine::isSimilarTo(const DividerLine& dividerLine, float distanceTolerance) const {
  return ((glm::distance(dividerLine.start, start) < distanceTolerance) &&
          (glm::distance(dividerLine.end, end) < distanceTolerance));
}

bool DividedArea::hasSimilarUnconstrainedDividerLine(const DividerLine& dividerLine) const {
  const float distanceTolerance = size.x * 10.0/100.0;
  return std::any_of(unconstrainedDividerLines.begin(),
                     unconstrainedDividerLines.end(),
                     [&](const auto& dl) { return dl.isSimilarTo(dividerLine, distanceTolerance); });
}

bool DividedArea::addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  if (maxUnconstrainedDividerLines >= 0 && unconstrainedDividerLines.size() >= maxUnconstrainedDividerLines) return false;
  if (ref1 == ref2) return false;
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  DividerLine dividerLine {ref1, ref2, lineWithinArea.start, lineWithinArea.end};
  unconstrainedDividerLines.push_back(dividerLine);
  return true;
}

bool containsPoint(const std::vector<glm::vec2>& points, glm::vec2 point) {
  return std::find(points.begin(),
                   points.end(),
                   point) != points.end();
}

std::optional<glm::vec2> findClosePoint(const std::vector<glm::vec2>& points, glm::vec2 point, float tolerance) {
  auto iter = std::find_if(points.begin(),
                           points.end(),
                           [&](glm::vec2 p) { return glm::distance(p, point) < tolerance; });
  if (iter != points.end()) {
    return *iter;
  } else {
    return std::nullopt;
  }
}

// Update unconstrainedDividerLines to run through the passed reference points, adding one extra to top up towards the max
bool DividedArea::updateUnconstrainedDividerLines(const std::vector<glm::vec2>& majorRefPoints, const std::vector<size_t>& candidateRefPointIndices) {
  bool linesChanged = false;
  const float POINT_DISTANCE_CLOSE = size.x * 1.0/10.0;

  // Find an obsolete line with reference points not in majorRefPoints,
  // then replace with a close equivalent or delete it
  for (auto iter = unconstrainedDividerLines.cbegin(); iter != unconstrainedDividerLines.cend(); iter++) {
    const auto& line = *iter;
    bool obsoleteRef1 = false, obsoleteRef2 = false;
    std::optional<glm::vec2> replacementRef1, replacementRef2;
    if (!containsPoint(majorRefPoints, line.ref1)) {
      obsoleteRef1 = true;
      replacementRef1 = findClosePoint(majorRefPoints, line.ref1, POINT_DISTANCE_CLOSE);
    }
    if (!containsPoint(majorRefPoints, line.ref2)) {
      obsoleteRef2 = true;
      replacementRef2 = findClosePoint(majorRefPoints, line.ref2, POINT_DISTANCE_CLOSE);
    }
    if (!obsoleteRef1 || !obsoleteRef2) continue;
    linesChanged = true;
    unconstrainedDividerLines.erase(iter);
    if ((obsoleteRef1 && !replacementRef1.has_value()) || (obsoleteRef2 && !replacementRef2.has_value())) break;
    glm::vec2 newRef1 = replacementRef1.has_value() ? replacementRef1.value() : line.ref1;
    glm::vec2 newRef2 = replacementRef2.has_value() ? replacementRef2.value() : line.ref2;
    addUnconstrainedDividerLine(newRef1, newRef2);
    break;
  }
  
  // add new lines from candidate ref points
  if (candidateRefPointIndices.size() > 1) {
    for (auto iter = candidateRefPointIndices.cbegin(); iter != candidateRefPointIndices.cend() - 1; iter++) {
      auto p1 = majorRefPoints.at(*iter);
      auto p2 = majorRefPoints.at(*(iter+1));
      linesChanged |= addUnconstrainedDividerLine(p1, p2);
    }
  }
  
  return linesChanged;
}

bool DividedArea::addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  if (ref1 == ref2) return false;
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  Line lineWithinUnconstrainedDividerLines = DividerLine::findEnclosedLine(ref1, ref2, unconstrainedDividerLines, lineWithinArea);
  DividerLine dividerLine = DividerLine::create(ref1, ref2, constrainedDividerLines, lineWithinUnconstrainedDividerLines);
  // TODO: check for validity here
  constrainedDividerLines.push_back(dividerLine);
  return true;
}

void DividedArea::draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth) const {
  if (areaConstraintLineWidth > 0) {
    for (const auto& dl : areaConstraints) {
      dl.draw(areaConstraintLineWidth);
    }
  }
  if (unconstrainedLineWidth > 0) {
    for (const auto& dl : unconstrainedDividerLines) {
      dl.draw(unconstrainedLineWidth);
    }
  }
  if (constrainedLineWidth > 0) {
    for (const auto& dl : constrainedDividerLines) {
      dl.draw(constrainedLineWidth);
    }
  }
}
