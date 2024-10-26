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

// Look for the shortest constrained line segment centred on ref1
DividerLine DividerLine::create(glm::vec2 ref1, glm::vec2 ref2, DividerLines constraints, glm::vec2 planeSize) {
  // Sort ref1 to be lower x than ref2
  if (ref1.x > ref2.x) std::swap(ref1, ref2);
  glm::vec2 start {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
  glm::vec2 end {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

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
  return DividerLine {ref1, ref2, start, end};
}

void DividerLine::draw(float width) const {
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));

  ofPath path;
  path.moveTo(0.0, 0.0);
  path.lineTo(glm::distance(start, end), -width/2.0);
  path.lineTo(glm::distance(start, end), width/2.0);
  path.lineTo(0.0, 0.0);
  path.setFilled(true);
  path.draw();
  
//  ofDrawRectangle(-1.0, -width/2.0, glm::length(end-start)+width*2.0, width);
  ofPopMatrix();
}
