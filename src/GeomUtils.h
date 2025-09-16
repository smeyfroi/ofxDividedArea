//
//  GeomUtils.h
//  tests
//
//  Created by Steve Meyfroidt on 15/09/2025.
//

#pragma once

#include "glm/vec2.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"
#include <cmath>
#include <limits>

namespace geom {
  constexpr float EPS = 1e-6f;

  inline float cross2(const glm::vec2& a, const glm::vec2& b) { return a.x*b.y - a.y*b.x; }
  inline float dot2(const glm::vec2& a, const glm::vec2& b) { return a.x*b.x + a.y*b.y; }

  inline float len2(const glm::vec2& v) { return glm::length2(v); }
  inline float len(const glm::vec2& v) { return std::sqrt(glm::length2(v)); }

  inline bool nearZero(float v, float eps = EPS) { return std::fabs(v) < eps; }
  inline bool near(float a, float b, float eps = EPS) { return std::fabs(a-b) < eps; }

  struct SafeNorm {
    glm::vec2 unit;
    float length;
  };

  inline SafeNorm safeNormalize(const glm::vec2& v, float eps = EPS) {
    float l2 = len2(v);
    if (l2 < eps*eps) return { glm::vec2(0), 0.0f };
    float l = std::sqrt(l2);
    return { v / l, l };
  }

  // Project point p to infinite line AB; returns t parameter and projected point.
  inline std::pair<float, glm::vec2> projectPointOntoLine(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    glm::vec2 ab = b - a;
    float ab2 = glm::length2(ab);
    if (ab2 == 0.0f) return { 0.0f, a };
    float t = dot2(p - a, ab) / ab2;
    return { t, a + t * ab };
  }

  inline float pointToSegmentDistance(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    auto [t, proj] = projectPointOntoLine(p, a, b);
    if (t < 0.0f) return std::sqrt(glm::distance2(p, a));
    if (t > 1.0f) return std::sqrt(glm::distance2(p, b));
    return std::sqrt(glm::distance2(p, proj));
  }

  inline bool rangesOverlap(float a0, float a1, float b0, float b1, float eps = EPS) {
    if (a0 > a1) std::swap(a0, a1);
    if (b0 > b1) std::swap(b0, b1);
    return !(a1 < b0 - eps || b1 < a0 - eps);
  }

  template<typename PT, typename A>
  bool containsPoint(const std::vector<PT, A>& points, glm::vec2 point) {
    return std::any_of(points.begin(),
                       points.end(),
                       [&](const auto& p) {
      return (glm::vec2(p) == point);
    });
  }

  template<typename PT, typename A>
  std::optional<glm::vec2> findClosePoint(const std::vector<PT, A>& points, glm::vec2 point, float tolerance) {
    float tolerance2 = tolerance * tolerance;
    auto iter = std::find_if(points.begin(),
                             points.end(),
                             [&](const auto& p) {
      return glm::distance2(glm::vec2(p), point) < tolerance2;
    });
    if (iter != points.end()) {
      return *iter;
    } else {
      return std::nullopt;
    }
  }

}
