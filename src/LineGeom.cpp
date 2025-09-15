#include "LineGeom.h"
#include "ofMath.h"
#include "glm/vec2.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp" // to get glm::distance2, which also requires the above define
#include <optional>
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
  inline float cross2(const glm::vec2& a, const glm::vec2& b) { return a.x*b.y - a.y*b.x; }
  constexpr float EPS = 1e-6f;
}

// y = mx + b
float yForLineAtX(float x, glm::vec2 start, glm::vec2 end) {
  float dx = end.x - start.x;
  if (std::fabs(dx) < EPS) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  float m = (end.y - start.y) / dx;
  float b = start.y - (m * start.x);
  return m * x + b;
}

// y = mx + b
float xForLineAtY(float y, glm::vec2 start, glm::vec2 end) {
  float dx = end.x - start.x;
  float dy = end.y - start.y;
  if (std::fabs(dx) < EPS) {
    return start.x;
  }
  float m = dy / dx;
  float b = start.y - (m * start.x);
  return (y - b) / m;
}

static bool pointOnLine(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float eps) {
  glm::vec2 r = b - a;
  glm::vec2 ap = p - a;
  return std::fabs(cross2(r, ap)) <= eps * (1.0f + glm::length(r) + glm::length(ap));
}

static bool pointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float eps) {
  if (!pointOnLine(p, a, b, eps)) return false;
  float minx = std::min(a.x, b.x) - eps, maxx = std::max(a.x, b.x) + eps;
  float miny = std::min(a.y, b.y) - eps, maxy = std::max(a.y, b.y) + eps;
  return (p.x >= minx && p.x <= maxx && p.y >= miny && p.y <= maxy);
}

std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd,
                                                   glm::vec2 lsStart, glm::vec2 lsEnd) {
  glm::vec2 p = lStart;
  glm::vec2 r = lEnd - lStart;
  glm::vec2 q = lsStart;
  glm::vec2 s = lsEnd - lsStart;

  if (glm::length2(r) < EPS*EPS) {
    if (pointOnSegment(p, q, lsEnd, EPS)) return p;
    return std::nullopt;
  }
  if (glm::length2(s) < EPS*EPS) {
    if (pointOnLine(q, p, lEnd, EPS)) return q;
    return std::nullopt;
  }

  float rxs = cross2(r, s);
  glm::vec2 q_p = q - p;
  float qpxr = cross2(q_p, r);

  if (std::fabs(rxs) < EPS) {
    if (std::fabs(qpxr) < EPS) {
      glm::vec2 cand = (glm::distance2(q, p) <= glm::distance2(lsEnd, p)) ? q : lsEnd;
      return cand;
    } else {
      return std::nullopt;
    }
  }

  float t = cross2(q_p, s) / rxs;
  float u = cross2(q_p, r) / rxs;

  if (u >= -EPS && u <= 1.0f + EPS) {
    glm::vec2 inter = p + t * r;
    return inter;
  }
  return std::nullopt;
}

glm::vec2 endPointForSegment(const glm::vec2& startPoint, float angleRadians, float length) {
  float deltaX = length * std::cos(angleRadians);
  float deltaY = length * std::sin(angleRadians);
  return { startPoint.x + deltaX, startPoint.y + deltaY };
}

void shrinkLineToIntersectionAroundReferencePoint(glm::vec2& start, glm::vec2& end,
                                                  const glm::vec2& intersection,
                                                  const glm::vec2& refPoint) {
  float distRefIntersection = glm::distance2(intersection, refPoint);

  float distStartIntersection = glm::distance2(start, intersection);
  float distRefStart = glm::distance2(start, refPoint);
  if (distRefIntersection < distRefStart && distStartIntersection < distRefStart) {
    start = intersection;
    return;
  }

  float distEndIntersection = glm::distance2(end, intersection);
  float distRefEnd = glm::distance2(end, refPoint);
  if (distRefIntersection < distRefEnd && distEndIntersection < distRefEnd) {
    end = intersection;
    return;
  }
}
