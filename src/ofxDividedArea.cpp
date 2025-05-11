#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "LineGeom.h"

ofParameterGroup& DividedArea::getParameterGroup() {
  if (parameters.size() == 0) {
    parameters.setName(getParameterGroupName());
    parameters.add(lerpAmountParameter);
    parameters.add(closePointDistanceParameter);
    parameters.add(unconstrainedOcclusionDistanceParameter);
    parameters.add(occlusionAngleParameter);
    parameters.add(maxConstrainedLinesParameter);
  }
  return parameters;
}

bool DividedArea::addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  if (maxUnconstrainedDividerLines < 0 || unconstrainedDividerLines.size() >= maxUnconstrainedDividerLines) return false;
  if (ref1 == ref2) return false;
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  DividerLine dividerLine { ref1, ref2, lineWithinArea.start, lineWithinArea.end };
  float occlusionDistance = unconstrainedOcclusionDistanceParameter * size.x;
  if (dividerLine.isOccludedByAny(unconstrainedDividerLines, occlusionDistance, occlusionAngleParameter)) return false;
  unconstrainedDividerLines.push_back(dividerLine);
  return true;
}

// Update unconstrainedDividerLines to move towards the passed reference
// points (which can be glm::vec4), adding and deleting max one per call
// to maintain the number required.
template<typename PT, typename A>
bool DividedArea::updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints) {
  float closePointDistance = closePointDistanceParameter * size.x;
  float closePointDistance2 = closePointDistance * closePointDistance;
  float occlusionDistance = unconstrainedOcclusionDistanceParameter * size.x;

  bool linesChanged = false;

  // Find an obsolete line with reference points not in majorRefPoints,
  // then replace with a close equivalent or delete it
  for (auto iter = unconstrainedDividerLines.begin(); iter != unconstrainedDividerLines.end(); iter++) {
    auto& line = *iter;
//    line.age = std::min(10, line.age + 1);
    
    // find close points that might be replacements
    std::optional<glm::vec2> replacementRef1 = findClosePoint(majorRefPoints, line.ref1, closePointDistance);
    std::optional<glm::vec2> replacementRef2 = findClosePoint(majorRefPoints, line.ref2, closePointDistance);
    
    if (replacementRef1.has_value() && replacementRef2.has_value()) {
      
      // continue if no change
      if (replacementRef1.value() == line.ref1 && replacementRef2.value() == line.ref2) continue;
      
      // Move towards updated ref points
//      ofLogNotice() << "move " << line.ref1.x << ":" << replacementRef1.value().x << " , " << line.ref1.y << ":" << replacementRef1.value().y;
      float lerp = lerpAmountParameter;
      auto newRef1 = glm::mix(line.ref1, replacementRef1.value(), lerp);
      auto newRef2 = glm::mix(line.ref2, replacementRef2.value(), lerp);

      Line updatedLine = DividerLine::findEnclosedLine(newRef1, newRef2, areaConstraints);
      line = DividerLine { newRef1, newRef2, updatedLine.start, updatedLine.end };

      if (line.isOccludedByAny(unconstrainedDividerLines, occlusionDistance, occlusionAngleParameter)) {
//        ofLogNotice() << "occluded " << newRef1.x << "," << newRef1.y << " : " <<  newRef2.x << "," << newRef2.y;
        unconstrainedDividerLines.erase(iter);
        linesChanged = true;
        break;
      }

    } else {
      // Obsolete line
      auto iter1 = std::find_if(majorRefPoints.begin(),
                                majorRefPoints.end(),
                                [&](const auto& p) {
        return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p, closePointDistanceParameter);
      });
      auto iter2 = std::find_if(iter1 + 1,
                                majorRefPoints.end(),
                                [&](const auto& p) {
        return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p, closePointDistanceParameter);
      });
      if (iter1 != majorRefPoints.end() && iter2 != majorRefPoints.end()) {
        line.ref1 = *iter1;
        line.ref2 = *iter2;
//        ofLogNotice() << "replace ref1 and ref2";
      } else {
//        line.age -= 2;
//        if (line.age <= 0) {
          // delete max one and break for simplicity
          unconstrainedDividerLines.erase(iter);
          linesChanged = true;
//          ofLogNotice() << "delete";
//        }
        break; // for simplicity break here after finding one divider line that is obsolete
      }
    }

    linesChanged = true;
  }
  
  // add max one
  auto iter1 = std::find_if(majorRefPoints.begin(),
                            majorRefPoints.end(),
                            [&](const auto& p) {
    return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p, closePointDistanceParameter);
  });
  if (iter1 < majorRefPoints.end()) {
    auto iter2 = std::find_if(iter1 + 1,
                              majorRefPoints.end(),
                              [&](const auto& p) {
      return !DividerLine::isRefPointUsed(unconstrainedDividerLines, p, closePointDistanceParameter);
    });
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
  float occlusionDistance = constrainedOcclusionDistanceParameter * size.x;
  if (ref1 == ref2) return std::nullopt;
  DividerLine dividerLine = createConstrainedDividerLine(ref1, ref2);
  if (dividerLine.isOccludedByAny(constrainedDividerLines, occlusionDistance, occlusionAngleParameter)) return std::nullopt;
  if (constrainedDividerLines.size() > maxConstrainedLinesParameter) deleteEarlyConstrainedDividerLines(maxConstrainedLinesParameter * 0.05);
  constrainedDividerLines.push_back(dividerLine);
  return dividerLine;
}

void DividedArea::draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float constrainedLineWidth, float scale) const {
  ofPushMatrix();
  ofScale(scale);
  {
    if (constrainedLineWidth > 0) {
      std::for_each(constrainedDividerLines.begin(),
                    constrainedDividerLines.end(),
                    [&](const auto& dl) {
        dl.draw(constrainedLineWidth / scale);
      });
    }
    if (unconstrainedLineWidth > 0) {
      std::for_each(unconstrainedDividerLines.begin(),
                    unconstrainedDividerLines.end(),
                    [&](const auto& dl) {
        dl.draw(unconstrainedLineWidth / scale);
      });
    }
    if (areaConstraintLineWidth > 0) {
      std::for_each(areaConstraints.begin(),
                    areaConstraints.end(),
                    [&](const auto& dl) {
        dl.draw(areaConstraintLineWidth / scale);
      });
    }
  }
  ofPopMatrix();
}

void DividedArea::draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, LineConfig constrainedLineConfig, float scale) const {
  ofPushMatrix();
  ofScale(scale);
  {
    if (areaConstraintLineConfig.maxWidth > 0.0) {
      areaConstraintLineConfig.scale(scale);
      std::for_each(areaConstraints.begin(),
                    areaConstraints.end(),
                    [&](const auto& dl) {
        dl.draw(areaConstraintLineConfig);
      });
    }
    if (unconstrainedLineConfig.maxWidth > 0.0) {
      unconstrainedLineConfig.scale(scale);
      std::for_each(unconstrainedDividerLines.begin(),
                    unconstrainedDividerLines.end(),
                    [&](const auto& dl) {
        dl.draw(unconstrainedLineConfig);
      });
    }
    if (constrainedLineConfig.maxWidth > 0.0) {
      constrainedLineConfig.scale(scale);
      std::for_each(constrainedDividerLines.begin(),
                    constrainedDividerLines.end(),
                    [&](const auto& dl) {
        dl.draw(constrainedLineConfig);
      });
    }
  }
  ofPopMatrix();
}
