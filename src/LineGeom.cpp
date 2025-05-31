#include "LineGeom.h"
#include "ofMath.h"
#include "glm/vec2.hpp"

float gradient(glm::vec2 start, glm::vec2 end) {
  return (end.y - start.y) / (end.x - start.x);
}

// y = mx + b
float yForLineAtX(float x, glm::vec2 start, glm::vec2 end) {
  float m = gradient(start, end);
  float b = start.y - (m * start.x);
  return m * x + b;
}

// y = mx + b
float xForLineAtY(float y, glm::vec2 start, glm::vec2 end) {
  float m = gradient(start, end);
  if (std::isinf(m)) return start.x; // vertical so x is a constant
  float b = start.y - (m * start.x);
  return (y - b) / m;
}

std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd) {
  float x, y;
  if (lsEnd.x == lsStart.x) {
    x = lsStart.x;
    y = yForLineAtX(lsStart.x, lStart, lEnd);
  } else if (lsEnd.y == lsStart.y) {
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
  
  if (std::isnan(x) || std::isnan(y)
      || x < std::min({lsStart.x, lsEnd.x}) || x > std::max({lsStart.x, lsEnd.x})
      || y < std::min({lsStart.y, lsEnd.y}) || y > std::max({lsStart.y, lsEnd.y})) {
    return std::nullopt;
  }
  return glm::vec2 { x, y };
}

glm::vec2 endPointForSegment(const glm::vec2& startPoint, float angleRadians, float length) {
    float deltaX = length * std::cos(angleRadians);
    float deltaY = length * std::sin(angleRadians);
  return { startPoint.x + deltaX, startPoint.y + deltaY };
}
