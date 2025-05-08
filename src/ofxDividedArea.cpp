#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "LineGeom.h"

bool DividedArea::addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  const float OCCLUSION_DISTANCE_TOLERANCE = size.x * 1.0/80.0;
  const float OCCLUSION_ANGLE_TOLERANCE = 0.95;
  if (maxUnconstrainedDividerLines >= 0 && unconstrainedDividerLines.size() >= maxUnconstrainedDividerLines) return false;
  if (ref1 == ref2) return false;
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  DividerLine dividerLine {ref1, ref2, lineWithinArea.start, lineWithinArea.end};
  if (dividerLine.isOccludedByAny(unconstrainedDividerLines, OCCLUSION_DISTANCE_TOLERANCE, OCCLUSION_ANGLE_TOLERANCE)) return false;
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

// Update unconstrainedDividerLines to move towards the passed reference
// points (which can be glm::vec4), adding and deleting max one per call
// to maintain the number required.
template<typename PT, typename A>
bool DividedArea::updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints) {
  const float lerpAmount = 0.05; // FIXME: extract somewhere
  const float POINT_DISTANCE_CLOSE = size.x * 1.0/10.0; // FIXME: extract somewhere
  const float OCCLUSION_DISTANCE_TOLERANCE = size.x * 5.0/100.0;
  const float OCCLUSION_ANGLE_TOLERANCE = 0.90;

  bool linesChanged = false;

  // Find an obsolete line with reference points not in majorRefPoints,
  // then replace with a close equivalent or delete it
  for (auto iter = unconstrainedDividerLines.begin(); iter != unconstrainedDividerLines.end(); iter++) {
    auto& line = *iter;
    line.age++;
    
    std::optional<glm::vec2> replacementRef1 = findClosePoint(majorRefPoints, line.ref1, POINT_DISTANCE_CLOSE);
    std::optional<glm::vec2> replacementRef2 = findClosePoint(majorRefPoints, line.ref2, POINT_DISTANCE_CLOSE);
        
    if (replacementRef1.has_value() && replacementRef2.has_value()) {
      // Still valid if ref points close enough
      if (glm::distance2(replacementRef1.value(), line.ref1) < POINT_DISTANCE_CLOSE/20.0 && glm::distance2(replacementRef2.value(), line.ref2) < POINT_DISTANCE_CLOSE/20.0) {
        ofLogNotice() << "similar " << line.ref1.x << ":" << replacementRef1.value().x << " , " << line.ref1.y << ":" << replacementRef1.value().y;
        continue;
      }
      // Move towards updated ref points
//      ofLogNotice() << "move " << line.ref1.x << ":" << replacementRef1.value().x << " , " << line.ref1.y << ":" << replacementRef1.value().y;
      auto newRef1 = glm::mix(line.ref1, replacementRef1.value(), lerpAmount);
      auto newRef2 = glm::mix(line.ref2, replacementRef2.value(), lerpAmount);
      Line updatedLine = DividerLine::findEnclosedLine(newRef1, newRef2, areaConstraints);
      line = DividerLine { newRef1, newRef2, updatedLine.start, updatedLine.end };

      if (line.isOccludedByAny(unconstrainedDividerLines, OCCLUSION_DISTANCE_TOLERANCE, OCCLUSION_ANGLE_TOLERANCE)) {
        unconstrainedDividerLines.erase(iter);
        linesChanged = true;
        break;
      }
      
    } else if (replacementRef1.has_value()) {
      
      // FIXME: refactor all the duplication
      
      // Replace obsolete ref2
      auto iter2 = std::find_if(majorRefPoints.begin(),
                               majorRefPoints.end(),
                               [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      if (iter2 != majorRefPoints.end()) line.ref2 = *iter2;
//      ofLogNotice() << "replace ref2";
    } else if (replacementRef2.has_value()) {
      // Replace obsolete ref1
      auto iter1 = std::find_if(majorRefPoints.begin(),
                               majorRefPoints.end(),
                               [&](const auto& p) { return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p); });
      if (iter1 != majorRefPoints.end()) line.ref1 = *iter1;
//      ofLogNotice() << "replace ref1";
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
//        ofLogNotice() << "replace ref1 and ref2";
      } else {
        // delete max one and break for simplicity
        unconstrainedDividerLines.erase(iter);
        linesChanged = true;
//        ofLogNotice() << "delete";
        break;
      }
    }

    linesChanged = true;
  }
  
  // add max one
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

void DividedArea::deleteEarlyConstrainedDividerLines(size_t count) {
  auto startIter = constrainedDividerLines.begin();
  auto endIter = constrainedDividerLines.begin() + count;
  if (endIter > constrainedDividerLines.end()) endIter = constrainedDividerLines.end();
  constrainedDividerLines.erase(startIter, endIter);
}

DividerLine DividedArea::createConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) const {
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  Line lineWithinUnconstrainedDividerLines = DividerLine::findEnclosedLine(ref1, ref2, unconstrainedDividerLines, lineWithinArea);
  return DividerLine::create(ref1, ref2, constrainedDividerLines, lineWithinUnconstrainedDividerLines);
}

std::optional<DividerLine> DividedArea::addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  const float OCCLUSION_DISTANCE_TOLERANCE = size.x * 1.0/1500.0;
  const float OCCLUSION_ANGLE_TOLERANCE = 0.95;
  if (ref1 == ref2) return std::nullopt;
  DividerLine dividerLine = createConstrainedDividerLine(ref1, ref2);
  if (dividerLine.isOccludedByAny(constrainedDividerLines, OCCLUSION_DISTANCE_TOLERANCE, OCCLUSION_ANGLE_TOLERANCE)) return std::nullopt;
  constrainedDividerLines.push_back(dividerLine);
  return dividerLine;
}

// This doesn't work as is: need a solver, not a replacer like this, which changes things in bulk with bad ripple effects
//void DividedArea::updateConstrainedDividerLines() {
//  auto newDividerLines = constrainedDividerLines;
//  for (auto& dl : newDividerLines) {
//    auto newDl = createConstrainedDividerLine(dl.ref1, dl.ref2);
//    dl = newDl;
//  }
//  constrainedDividerLines = newDividerLines;
//}

void DividedArea::draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth, float scale) const {
  ofPushMatrix();
  ofScale(scale);
  {
    if (constrainedLineWidth > 0) {
      for (const auto& dl : constrainedDividerLines) {
        dl.draw(constrainedLineWidth / scale);
      }
    }
    if (unconstrainedLineWidth > 0) {
      bool isStable = false;
      for (const auto& dl : unconstrainedDividerLines) {
        if (dl.age > 20) {
          isStable = true;
          break;
        }
      }
      if (isStable) {
        for (const auto& dl : unconstrainedDividerLines) {
          if (dl.age > 5) dl.draw(unconstrainedLineWidth / scale);
        }
      }
//      for (const auto& dl : unconstrainedDividerLines) {
//              if (dl.age > 5) dl.draw(unconstrainedLineWidth);
//        dl.draw(unconstrainedLineWidth);
//      }
    }
    if (areaConstraintLineWidth > 0) {
      for (const auto& dl : areaConstraints) {
        dl.draw(areaConstraintLineWidth / scale);
      }
    }
  }
  ofPopMatrix();
}

void DividedArea::draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, LineConfig constrainedLineConfig, float scale) const {
  ofPushMatrix();
  ofScale(scale);
  {
    if (areaConstraintLineConfig.maxWidth > 0.0) {
      for (const auto& dl : areaConstraints) {
        dl.draw(areaConstraintLineConfig);
      }
    }
    if (unconstrainedLineConfig.maxWidth > 0.0) {
      bool isStable = false;
      for (const auto& dl : unconstrainedDividerLines) {
        if (dl.age > 20) {
          isStable = true;
          break;
        }
      }
      if (isStable) {
        for (const auto& dl : unconstrainedDividerLines) {
          if (dl.age > 5) dl.draw(unconstrainedLineConfig);
        }
      }
    }
    if (constrainedLineConfig.maxWidth > 0.0) {
      for (const auto& dl : constrainedDividerLines) {
        dl.draw(constrainedLineConfig);
      }
    }
  }
  ofPopMatrix();
}
