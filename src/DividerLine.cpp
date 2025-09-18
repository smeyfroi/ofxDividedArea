#include "DividerLine.hpp"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "LineGeom.h"
#include "GeomUtils.h"

using namespace geom;

// https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
float DividerLine::pointToLineDistance(glm::vec2 point, const DividerLine& line) {
  glm::vec2 d = line.end - line.start;
  auto norm = safeNormalize(d);
  if (norm.length < EPS) {
    return glm::distance(point, line.start);
  }
  // Distance to infinite line via cross/|d|
  return std::fabs(cross2(d, point - line.start)) / norm.length;
}

// Occluded IF spans are close in perpendicular direction AND directions similar AND spans overlap along tangent
bool DividerLine::isOccludedBy(const DividerLine& dividerLine, float distanceTolerance, float gradientTolerance) const {
  if (&dividerLine == this) return false;

  glm::vec2 d1 = end - start;
  glm::vec2 d2 = dividerLine.end - dividerLine.start;
  auto n1 = safeNormalize(d1);
  auto n2 = safeNormalize(d2);
  if (n1.length < EPS || n2.length < EPS) return false;

  float dot = glm::dot(n1.unit, n2.unit);
  if (std::abs(dot) < gradientTolerance) return false;

  // Normal vector (perp to direction)
  glm::vec2 n = glm::vec2(-n1.unit.y, n1.unit.x);

  // Compute perpendicular distances of all endpoints to the other's supporting line; require small on both ends (both spans near)
  float dA0 = std::fabs(cross2(d2, start - dividerLine.start)) / n2.length;
  float dA1 = std::fabs(cross2(d2, end   - dividerLine.start)) / n2.length;
  float dB0 = std::fabs(cross2(d1, dividerLine.start - start)) / n1.length;
  float dB1 = std::fabs(cross2(d1, dividerLine.end   - start)) / n1.length;

  if (!((dA0 < distanceTolerance && dA1 < distanceTolerance) ||
        (dB0 < distanceTolerance && dB1 < distanceTolerance))) {
    return false;
  }

  // Project spans onto the tangent (direction) to ensure overlap along the line, not just proximity
  auto projRange = [&](const glm::vec2& a, const glm::vec2& b, const glm::vec2& origin, const glm::vec2& dirUnit){
    float ta = glm::dot(a - origin, dirUnit);
    float tb = glm::dot(b - origin, dirUnit);
    return std::pair<float,float>{std::min(ta, tb), std::max(ta, tb)};
  };

  auto [a0, a1] = projRange(start, end, start, n1.unit); // self range along own tangent
  // project other span into same frame
  float b0 = glm::dot(dividerLine.start - start, n1.unit);
  float b1 = glm::dot(dividerLine.end   - start, n1.unit);

  if (!rangesOverlap(a0, a1, b0, b1, distanceTolerance)) {
    return false;
  }

  return true;
}

bool DividerLine::isOccludedByAny(const DividerLines& dividerLines, float distanceTolerance, float gradientTolerance) const {
  return std::any_of(dividerLines.cbegin(),
                     dividerLines.cend(),
                     [&](const auto& dl) {
    return (isOccludedBy(dl, distanceTolerance, gradientTolerance));
  });
}

// Quantised hash to ensure random but stable results
static inline uint32_t mix32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352d;
  x ^= x >> 15; x *= 0x846ca68b;
  x ^= x >> 16; return x;
}

static inline uint32_t hashVec2(const glm::vec2& v) {
  // Quantize to reduce floating noise; adjust scale as needed
  int xi = static_cast<int>(std::floor(v.x * 1e6f));
  int yi = static_cast<int>(std::floor(v.y * 1e6f));
  uint32_t h = 0x9e3779b9u;
  h ^= mix32(static_cast<uint32_t>(xi)) + 0x9e3779b9u + (h<<6) + (h>>2);
  h ^= mix32(static_cast<uint32_t>(yi)) + 0x9e3779b9u + (h<<6) + (h>>2);
  return h;
}

// Shrink the startLine towards a reference point to fit inside the constraints
Line DividerLine::findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine) {
  glm::vec2 start = startLine.start, end = startLine.end;

  // Order-independent pair hash for randomised but stable choice of shrinkTowards point
  uint32_t h1 = hashVec2(ref1), h2 = hashVec2(ref2);
  uint32_t pairHash = (h1 < h2) ? (h1 * 0x85ebca6bu ^ h2) : (h2 * 0x85ebca6bu ^ h1);

  const glm::vec2& shrinkTowards = (pairHash & 1u) ? ref1 : ref2;

  for (const auto& constraint : constraints) {
    if ((ref1 == constraint.ref1 && ref2 == constraint.ref2) || (ref2 == constraint.ref1 && ref1 == constraint.ref2)) {
      continue; // don't constrain by self
    }
    if (auto intersection = lineToSegmentIntersection(ref1, ref2, constraint.start, constraint.end)) {
      shrinkLineToIntersectionAroundReferencePoint(start, end, *intersection, shrinkTowards);
    }
  }
  
  return Line { start, end };
}

// Look for the shortest constrained line segment passing through (ref1, ref2), optionally starting with a line segment to be constrained
DividerLine DividerLine::create(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine) {
  Line constrainedLine = findEnclosedLine(ref1, ref2, constraints, startLine);
  return DividerLine { ref1, ref2, constrainedLine.start, constrainedLine.end };
}

void DividerLine::draw(float width) const {
  if (mesh.getNumVertices() == 0) {
    mesh = ofMesh::plane(width, glm::distance(start, end), 2, 2, OF_PRIMITIVE_TRIANGLES);
  }
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));
  ofTranslate(0.0, -width / 2.0);
  mesh.draw();
  ofPopMatrix();
}

void DividerLine::draw(const LineConfig& config) const {
  if (mesh.getNumVertices() == 0) {
    float widthFactor = 1.0;
    if (config.adaptiveWidthMaxLength > 0.0) {
      widthFactor = std::fminf(1.0, glm::distance(start, end) / config.adaptiveWidthMaxLength);
    }
    ofPath path;
    path.moveTo(0.0, -widthFactor*config.minWidth/2.0);
    path.lineTo(glm::distance(start, end), -widthFactor*config.maxWidth/2.0);
    path.lineTo(glm::distance(start, end), widthFactor*config.maxWidth/2.0);
    path.lineTo(0.0, widthFactor*config.minWidth/2.0);
    mesh = path.getTessellation();
  }
  
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));
  ofSetColor(config.color);
  mesh.draw();
  ofPopMatrix();
}

template<typename PT>
bool DividerLine::isRefPointUsed(const DividerLines& dividerLines, const PT refPoint, const float closePointDistance) {
  float tolerance = closePointDistance * closePointDistance;
  return std::any_of(dividerLines.begin(), dividerLines.end(), [&](const auto& dl) {
    return (glm::distance2(dl.ref1, glm::vec2(refPoint)) < tolerance
            || glm::distance2(dl.ref2, glm::vec2(refPoint)) < tolerance);
  });
}

template bool DividerLine::isRefPointUsed<glm::vec2>(const DividerLines& dividerLines, const glm::vec2 refPoint, const float closePointDistance);
template bool DividerLine::isRefPointUsed<glm::vec3>(const DividerLines& dividerLines, const glm::vec3 refPoint, const float closePointDistance);
template bool DividerLine::isRefPointUsed<glm::vec4>(const DividerLines& dividerLines, const glm::vec4 refPoint, const float closePointDistance);
