#include "LineGeom.h"
#include "GeomUtils.h"
#include <optional>
#include <cmath>
#include <algorithm>
#include <limits>

// Notes:
// - EPS is the global geometric tolerance used across all computations.
// - Use nearZero/near for comparisons instead of raw == to avoid floating-point pitfalls.
// - safeNormalize returns zero-length when vector magnitude < EPS to prevent NaNs.
// - All distances/overlaps are EPS-tolerant.

// - gradient(start,end): Undefined (infinite) for vertical lines (|dx| ~ 0). Callers should not assume finiteness.
// - yForLineAtX(x,start,end): Returns NaN for vertical lines to signal non-unique y at given x.
// - xForLineAtY(y,start,end): For vertical lines, returns start.x (x is constant).
// - lineToSegmentIntersection:
//   * Intersects the infinite line (lStart-lEnd) with the finite segment (lsStart-lsEnd).
//   * Handles degenerate inputs (point-line, point-segment).
//   * Parallel disjoint → no intersection.
//   * Collinear overlap → returns the segment endpoint nearest to lStart (deterministic policy).
//   * All comparisons use a small EPS tolerance; see GeomUtils.h.

using namespace geom;

float gradient(glm::vec2 start, glm::vec2 end) {
  return (end.y - start.y) / (end.x - start.x);
}

float yForLineAtX(float x, glm::vec2 start, glm::vec2 end) {
  float dx = end.x - start.x;
  if (nearZero(dx)) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  float m = (end.y - start.y) / dx;
  float b = start.y - (m * start.x);
  return m * x + b;
}

float xForLineAtY(float y, glm::vec2 start, glm::vec2 end) {
  float dx = end.x - start.x;
  float dy = end.y - start.y;
  if (nearZero(dx)) {
    return start.x;
  }
  float m = dy / dx;
  float b = start.y - (m * start.x);
  return (y - b) / m;
}

static bool pointOnLine(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float eps) {
  glm::vec2 r = b - a;
  glm::vec2 ap = p - a;
  return std::fabs(cross2(r, ap)) <= eps * (1.0f + len(r) + len(ap));
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

  if (len2(r) < EPS*EPS) {
    if (pointOnSegment(p, q, lsEnd, EPS)) return p;
    return std::nullopt;
  }
  if (len2(s) < EPS*EPS) {
    if (pointOnLine(q, p, lEnd, EPS)) return q;
    return std::nullopt;
  }

  // Vertical/horizontal fast-paths for the segment bounds (cheap)
  if (near(lsStart.x, lsEnd.x)) {
    float x = lsStart.x;
    float y = yForLineAtX(x, lStart, lEnd);
    if (std::isfinite(y) &&
        y >= std::min(lsStart.y, lsEnd.y) - EPS &&
        y <= std::max(lsStart.y, lsEnd.y) + EPS) {
      return glm::vec2{x, y};
    }
    // fallthrough to parametric for edge numerics or NaN y
  } else if (near(lsStart.y, lsEnd.y)) {
    float y = lsStart.y;
    float x = xForLineAtY(y, lStart, lEnd);
    if (std::isfinite(x) &&
        x >= std::min(lsStart.x, lsEnd.x) - EPS &&
        x <= std::max(lsStart.x, lsEnd.x) + EPS) {
      return glm::vec2{x, y};
    }
    // fallthrough to parametric
  }

  float rxs = cross2(r, s);
  glm::vec2 q_p = q - p;
  float qpxr = cross2(q_p, r);

  if (nearZero(rxs)) {
    if (nearZero(qpxr)) {
      glm::vec2 cand = (glm::distance2(q, p) <= glm::distance2(lsEnd, p)) ? q : lsEnd;
      return cand;
    } else {
      return std::nullopt;
    }
  }

  // Use double for borderline stability
  double t = static_cast<double>(cross2(q_p, s)) / static_cast<double>(rxs);
  double u = static_cast<double>(cross2(q_p, r)) / static_cast<double>(rxs);

  if (u >= -EPS && u <= 1.0 + EPS) {
    glm::vec2 inter = p + static_cast<float>(t) * r;
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
