#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"

float DividerLine::gradient(glm::vec2 start, glm::vec2 end) {
  return (end.y - start.y) / (end.x - start.x);
}

// y = mx + b
float DividerLine::yForLineAtX(float x, glm::vec2 start, glm::vec2 end) {
  float m = gradient(start, end);
  float b = start.y - (m * start.x);
  return m * x + b;
}

// y = mx + b
float DividerLine::xForLineAtY(float y, glm::vec2 start, glm::vec2 end) {
  float m = gradient(start, end);
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

// https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
float DividerLine::pointToLineDistance(glm::vec2 point, const DividerLine& line) {
  return std::abs( (line.end.y-line.start.y)*point.x - (line.end.x-line.start.x)*point.y + (line.end.x*line.start.y) - (line.end.y*line.start.x) ) / std::sqrt( std::pow(line.end.y-line.start.y,2) + std::pow(line.end.x-line.start.x,2) );
}

// endpoints of one line close to other line and gradients similar
bool DividerLine::isOccludedBy(DividerLine dividerLine, float distanceTolerance, float gradientTolerance) const {
  float dot = glm::dot(glm::normalize(end-start), glm::normalize(dividerLine.end-dividerLine.start));
  if (std::abs(dot) < gradientTolerance) return false;
  if ((pointToLineDistance(start, dividerLine) < distanceTolerance &&
       pointToLineDistance(end, dividerLine) < distanceTolerance)) return true;
  if ((pointToLineDistance(dividerLine.start, *this) < distanceTolerance &&
       pointToLineDistance(dividerLine.end, *this) < distanceTolerance)) return true;
  return false;
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

float DividerLine::gradient() const {
  return (end.y - start.y) / (end.x - start.x);
}

bool DividerLine::isSimilarTo(const DividerLine& dividerLine, float distanceTolerance) const {
  return ((glm::distance2(dividerLine.start, start) < distanceTolerance) &&
          (glm::distance2(dividerLine.end, end) < distanceTolerance));
}

template<typename PT>
bool DividerLine::isRefPointUsed(const DividerLines& dividerLines, const PT refPoint) {
  for (const auto& dl : dividerLines) {
    if (glm::distance(dl.ref1, glm::vec2(refPoint)) < 0.05 || glm::distance(dl.ref2, glm::vec2(refPoint)) < 0.05) return true;
  }
  return false;
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
  if (hasSimilarUnconstrainedDividerLine(dividerLine)) return false;
  unconstrainedDividerLines.push_back(dividerLine);
  return true;
}

template<typename PT, typename A>
bool containsPoint(const std::vector<PT, A>& points, glm::vec2 point) {
  return std::find_if(points.begin(),
                      points.end(),
                      [&](const auto& p) { return (glm::vec2(p) == point); }) != points.end();
}

template<typename PT, typename A>
std::optional<glm::vec2> findClosePoint(const std::vector<PT, A>& points, glm::vec2 point, float tolerance) {
  auto iter = std::find_if(points.begin(),
                           points.end(),
                           [&](const auto& p) { return glm::distance(glm::vec2(p), point) < tolerance; });
  if (iter != points.end()) {
    return *iter;
  } else {
    return std::nullopt;
  }
}

// Update unconstrainedDividerLines to run through the passed reference points (which can be glm::vec4), adding one extra to top up towards the max
template<typename PT, typename A>
bool DividedArea::updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints) {
  const float tolerance = 0.05; // FIXME: extract somewhere
  const float POINT_DISTANCE_CLOSE = size.x * 1.0/10.0; // FIXME: extract somewhere

  bool linesChanged = false;

  // Find an obsolete line with reference points not in majorRefPoints,
  // then replace with a close equivalent or delete it
  for (auto iter = unconstrainedDividerLines.begin(); iter != unconstrainedDividerLines.end(); iter++) {
    auto& line = *iter;
    
    std::optional<glm::vec2> replacementRef1 = findClosePoint(majorRefPoints, line.ref1, POINT_DISTANCE_CLOSE);
    std::optional<glm::vec2> replacementRef2 = findClosePoint(majorRefPoints, line.ref2, POINT_DISTANCE_CLOSE);
        
    if (replacementRef1.has_value() && replacementRef2.has_value()) {
      // Still valid
      if (replacementRef1.value() == line.ref1 && replacementRef2.value() == line.ref2) continue;
      // Move towards updated ref points
      Line updatedLine = DividerLine::findEnclosedLine(replacementRef1.value(), replacementRef2.value(), areaConstraints);
      line.ref1 = glm::mix(line.ref1, replacementRef1.value(), tolerance);
      line.ref2 = glm::mix(line.ref2, replacementRef2.value(), tolerance);
    } else if (replacementRef1.has_value()) {
      
      // FIXME: refactor all the duplication
      
      // Replace obsolete ref2
      auto iter2 = std::find_if(majorRefPoints.begin(),
                               majorRefPoints.end(),
                               [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      if (iter2 != majorRefPoints.end()) line.ref2 = *iter2;
    } else if (replacementRef2.has_value()) {
      // Replace obsolete ref1
      auto iter1 = std::find_if(majorRefPoints.begin(),
                               majorRefPoints.end(),
                               [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      if (iter1 != majorRefPoints.end()) line.ref1 = *iter1;
    } else {
      // Obsolete ref1 and ref2
      auto iter1 = std::find_if(majorRefPoints.begin(),
                               majorRefPoints.end(),
                                [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      auto iter2 = std::find_if(iter1 + 1,
                               majorRefPoints.end(),
                                [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      if (iter1 != majorRefPoints.end() && iter2 != majorRefPoints.end()) {
        line.ref1 = *iter1;
        line.ref2 = *iter2;
      } else {
        // delete max one and break for simplicity
        unconstrainedDividerLines.erase(iter);
        linesChanged = true;
        break;
      }
    }

    linesChanged = true;
  }
  
  auto iter1 = std::find_if(majorRefPoints.begin(),
                           majorRefPoints.end(),
                            [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
  if (iter1 < majorRefPoints.end()) {
    auto iter2 = std::find_if(iter1 + 1,
                              majorRefPoints.end(),
                              [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
    if (iter1 != majorRefPoints.end() && iter2 != majorRefPoints.end()) {
      linesChanged |= addUnconstrainedDividerLine(*iter1, *iter2);
    }
  }
  
  return linesChanged;
}

template bool DividedArea::updateUnconstrainedDividerLines<glm::vec2>(const std::vector<glm::vec2>& majorRefPoints);
template bool DividedArea::updateUnconstrainedDividerLines<glm::vec3>(const std::vector<glm::vec3>& majorRefPoints);
template bool DividedArea::updateUnconstrainedDividerLines<glm::vec4>(const std::vector<glm::vec4>& majorRefPoints);

void DividedArea::clearConstrainedDividerLines() {
  constrainedDividerLines.clear();
}

DividerLine DividedArea::createConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) const {
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  Line lineWithinUnconstrainedDividerLines = DividerLine::findEnclosedLine(ref1, ref2, unconstrainedDividerLines, lineWithinArea);
  return DividerLine::create(ref1, ref2, constrainedDividerLines, lineWithinUnconstrainedDividerLines);
}

bool DividedArea::addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  const float POINT_DISTANCE_CLOSE = size.x * 1.0/200.0;
  const float GRADIENT_CLOSE = 0.4; // gradients close when dot product < this constant
  if (ref1 == ref2) return false;
  DividerLine dividerLine = createConstrainedDividerLine(ref1, ref2);
  for (const auto& dl : constrainedDividerLines) {
    if (dividerLine.isOccludedBy(dl, POINT_DISTANCE_CLOSE, GRADIENT_CLOSE)) return false;
  }
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
