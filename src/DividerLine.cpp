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

Line DividerLine::findEnclosedLine(glm::vec2 ref1, glm::vec2 ref2, const DividerLines& constraints, const Line& startLine) {
  // Sort ref1 to be left of ref2
  if (ref1.x > ref2.x) std::swap(ref1, ref2);

  glm::vec2 start = startLine.start;
  glm::vec2 end = startLine.end;
  
  // Start from somewhere random along the line else we always bias towards the left
  // TODO: this still biases towards the left: need to be able to search left and right
  start = start + ofRandom(1.0) * (end - start);

  for (const auto& constraint : constraints) {
    if ((ref1 == constraint.ref1 && ref2 == constraint.ref2) || (ref2 == constraint.ref1 && ref1 == constraint.ref2)) {
      // don't constrain by self
      continue;
    }
    if (auto intersectionResult = lineToSegmentIntersection(ref1, ref2, constraint.start, constraint.end)) {
      glm::vec2 intersection = intersectionResult.value();
      float distRef1New = glm::distance2(intersection, ref1);
      if ((intersection.x < ref1.x) || (intersection.x == ref1.x && intersection.y < ref1.y)) { // handle intersections with horizontal constraints
        float distRef1Start = glm::distance2(start, ref1);
        if (distRef1New < distRef1Start) start = intersection;
      } else {
        float distRef1End = glm::distance2(end, ref1);
        if (distRef1New < distRef1End) end = intersection;
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
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));
  // why no ofSetFill()?
  ofDrawRectangle(0.0, -width/2.0, glm::length(end-start), width);
  ofPopMatrix();
}

void DividerLine::draw(const LineConfig& config) const {
  ofPushMatrix();
  ofTranslate(start);
  ofRotateRad(std::atan2((end.y - start.y), (end.x - start.x)));
  
  float widthFactor = 1.0;
  if (config.adaptiveWidthMaxLength > 0.0) {
    widthFactor = std::fminf(1.0, glm::distance(start, end) / config.adaptiveWidthMaxLength);
  }

  ofPath path;
//  ofLogNotice() << start << " | " << end;
//  ofLogNotice() << glm::vec2{0.0, -widthFactor*config.minWidth/2.0} << " | " << glm::vec2{glm::distance(start, end), -widthFactor*config.minWidth/2.0} << " | " << glm::vec2{glm::distance(start, end), widthFactor*config.maxWidth/2.0} << " | " << glm::vec2{0.0, widthFactor*config.maxWidth/2.0};
  path.moveTo(0.0, -widthFactor*config.minWidth/2.0);
  path.lineTo(glm::distance(start, end), -widthFactor*config.minWidth/2.0);
  path.lineTo(glm::distance(start, end), widthFactor*config.maxWidth/2.0);
  path.lineTo(0.0, widthFactor*config.maxWidth/2.0);
  path.setFilled(true);
  path.setFillColor(config.color);
  path.draw();
  
  ofPopMatrix();
}

template<typename PT>
bool DividerLine::isRefPointUsed(const DividerLines& dividerLines, const PT refPoint, const float closePointDistance) {
  float tolerance = closePointDistance * closePointDistance;
  return std::any_of(dividerLines.begin(),
              dividerLines.end(),
              [&](const auto& dl) {
    return (glm::distance2(dl.ref1, glm::vec2(refPoint)) < tolerance
            || glm::distance2(dl.ref2, glm::vec2(refPoint)) < tolerance);
  });
}

template bool DividerLine::isRefPointUsed<glm::vec2>(const DividerLines& dividerLines, const glm::vec2 refPoint, const float closePointDistance);
template bool DividerLine::isRefPointUsed<glm::vec3>(const DividerLines& dividerLines, const glm::vec3 refPoint, const float closePointDistance);
template bool DividerLine::isRefPointUsed<glm::vec4>(const DividerLines& dividerLines, const glm::vec4 refPoint, const float closePointDistance);
