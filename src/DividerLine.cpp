#include "DividerLine.hpp"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "LineGeom.h"

// https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
float DividerLine::pointToLineDistance(glm::vec2 point, const DividerLine& line) {
  return std::abs( (line.end.y-line.start.y)*point.x - (line.end.x-line.start.x)*point.y + (line.end.x*line.start.y) - (line.end.y*line.start.x) ) / std::sqrt( std::pow(line.end.y-line.start.y,2) + std::pow(line.end.x-line.start.x,2) );
}

// Occluded IF endpoints of one line close to other line AND gradients similar
bool DividerLine::isOccludedBy(const DividerLine& dividerLine, float distanceTolerance, float gradientTolerance) const {
  if (&dividerLine == this) return false;
  float dot = glm::dot(glm::normalize(end - start), glm::normalize(dividerLine.end - dividerLine.start));
//  ofLogNotice() << "dot " << dot << ", tolerance " << gradientTolerance;
  if (std::abs(dot) < gradientTolerance) return false;
//  ofLogNotice() << "-- " << pointToLineDistance(start, dividerLine) << ", " << pointToLineDistance(end, dividerLine) << ", " << distanceTolerance;
  if ((pointToLineDistance(start, dividerLine) < distanceTolerance &&
       pointToLineDistance(end, dividerLine) < distanceTolerance)) return true;
//  ofLogNotice() << "   " << pointToLineDistance(dividerLine.start, *this) << ", " << pointToLineDistance(dividerLine.end, *this) << ", " << distanceTolerance;
  if ((pointToLineDistance(dividerLine.start, *this) < distanceTolerance &&
       pointToLineDistance(dividerLine.end, *this) < distanceTolerance)) return true;
  return false;
}

bool DividerLine::isOccludedByAny(const DividerLines& dividerLines, float distanceTolerance, float gradientTolerance) const {
  return std::any_of(dividerLines.cbegin(),
                     dividerLines.cend(),
                     [&](const auto& dl) {
    return (isOccludedBy(dl, distanceTolerance, gradientTolerance));
  });
}

// ref1 and ref2 are points on the constraining line that the intersection is with
void shrinkLineToIntersectionTopLeft(glm::vec2& start, glm::vec2& end, const glm::vec2& intersection, const glm::vec2& ref1, const glm::vec2& ref2) {
  float distRef1New = glm::distance2(intersection, ref1);
  if ((intersection.x < ref1.x) || (intersection.x == ref1.x && intersection.y < ref1.y)) { // handle intersections with horizontal constraints
    float distRef1Start = glm::distance2(start, ref1);
    if (distRef1New < distRef1Start) start = intersection;
  } else {
    float distRef1End = glm::distance2(end, ref1);
    if (distRef1New < distRef1End) end = intersection;
  }
}

// ref1 and ref2 are points on the constraining line that the intersection is with
void shrinkLineToIntersectionBotRight(glm::vec2& start, glm::vec2& end, const glm::vec2& intersection, const glm::vec2& ref1, const glm::vec2& ref2) {
  float distRef2New = glm::distance2(intersection, ref2);
  if ((intersection.x > ref2.x) || (intersection.x == ref2.x && intersection.y > ref2.y)) { // handle intersections with horizontal constraints
    float distRef2End = glm::distance2(end, ref2);
    if (distRef2New < distRef2End) end = intersection;
  } else {
    float distRef2Start = glm::distance2(start, ref2);
    if (distRef2New < distRef2Start) start = intersection;
  }
}

Line DividerLine::findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine) {
  bool intersectTopLeft = true;
  if (ref1.x > ref2.x) {
    // Sort ref1 to be always left of ref2
    std::swap(ref1, ref2);
    intersectTopLeft = false;
  }

  glm::vec2 start = startLine.start;
  glm::vec2 end = startLine.end;
  
  for (const auto& constraint : constraints) {
    if ((ref1 == constraint.ref1 && ref2 == constraint.ref2) || (ref2 == constraint.ref1 && ref1 == constraint.ref2)) {
      // don't constrain by self
      continue;
    }
    if (auto intersectionResult = lineToSegmentIntersection(ref1, ref2, constraint.start, constraint.end)) {
      glm::vec2 intersection = intersectionResult.value();
      if (intersectTopLeft) {
        shrinkLineToIntersectionTopLeft(start, end, intersection, ref1, ref2);
      } else {
        shrinkLineToIntersectionBotRight(start, end, intersection, ref1, ref2);
      }
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
