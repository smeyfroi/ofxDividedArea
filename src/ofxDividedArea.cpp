#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "LineGeom.h"
#include "GeomUtils.h"

static constexpr int ATTR_LOC_POS = 0;
static constexpr int ATTR_LOC_P0 = 1;
static constexpr int ATTR_LOC_P1 = 2;
static constexpr int ATTR_LOC_WIDTH = 3;
static constexpr int ATTR_LOC_STYLE = 4;
static constexpr int ATTR_LOC_COLOR = 5;

ofParameterGroup& DividedArea::getParameterGroup() {
  if (parameters.size() == 0) {
    parameters.setName(getParameterGroupName());
    parameters.add(lerpAmountParameter);
    parameters.add(closePointDistanceParameter);
    parameters.add(unconstrainedOcclusionDistanceParameter);
    parameters.add(constrainedOcclusionDistanceParameter);
    parameters.add(occlusionAngleParameter);
    parameters.add(maxConstrainedLinesParameter);
  }
  return parameters;
}

DividedArea::DividedArea(glm::vec2 size_, int maxUnconstrainedDividerLines_) :
size(size_),
maxUnconstrainedDividerLines(maxUnconstrainedDividerLines_)
{}

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
    std::optional<glm::vec2> replacementRef1 = geom::findClosePoint(majorRefPoints, line.ref1, closePointDistance);
    std::optional<glm::vec2> replacementRef2 = geom::findClosePoint(majorRefPoints, line.ref2, closePointDistance);
    
    if (replacementRef1.has_value() && replacementRef2.has_value()) {
      
      // continue if no change
      if (replacementRef1.value() == line.ref1 && replacementRef2.value() == line.ref2) continue;
      
      // Move towards updated ref points
//      ofLogNotice() << "move " << line.ref1.x << ":" << replacementRef1.value().x << " , " << line.ref1.y << ":" << replacementRef1.value().y;
      float lerp = lerpAmountParameter;
      auto newRef1 = glm::mix(line.ref1, replacementRef1.value(), lerp);
      auto newRef2 = glm::mix(line.ref2, replacementRef2.value(), lerp);
      
      if (newRef1 == newRef2) continue;

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
  if (ref1 == ref2) return std::nullopt;
  DividerLine dividerLine = createConstrainedDividerLine(ref1, ref2);
  float occlusionDistance = constrainedOcclusionDistanceParameter * size.x;
  if (dividerLine.isOccludedByAny(constrainedDividerLines, occlusionDistance, occlusionAngleParameter)) return std::nullopt;
  if (constrainedDividerLines.size() > maxConstrainedLinesParameter) deleteEarlyConstrainedDividerLines(maxConstrainedLinesParameter * 0.05);
  constrainedDividerLines.push_back(dividerLine);
  return dividerLine;
}

void DividedArea::setMaxConstrainedDividers(int max) {
  instanceCapacity = std::max(0, max);
  instances.resize(instanceCapacity);
  head = 0;
  instanceCount = 0;
  if (instanceCapacity > 0 && !instanceBO.isAllocated()) {
    instanceBO.allocate(instances, GL_DYNAMIC_DRAW);
  } else if (instanceCapacity > 0) {
    instanceBO.updateData(0, instanceCapacity * (int)sizeof(DividerInstance), instances.data());
  }
  instancesDirty = true;

  // build unit quad and vbo only once
  if (quad.getNumVertices() == 0) {
    quad.setMode(OF_PRIMITIVE_TRIANGLES);
    quad.addVertex({-0.5f, -0.5f, 0.0f}); // 0 bottom-left
    quad.addVertex({ 0.5f, -0.5f, 0.0f}); // 1 bottom-right
    quad.addVertex({ 0.5f,  0.5f, 0.0f}); // 2 top-right
    quad.addVertex({-0.5f,  0.5f, 0.0f}); // 3 top-left
    // Optional per-vertex attributes if you want (uvs/colors) — not required for instancing
//    quad.addTexCoord({0.0f, 0.0f});
//    quad.addTexCoord({1.0f, 0.0f});
//    quad.addTexCoord({1.0f, 1.0f});
//    quad.addTexCoord({0.0f, 1.0f});
    // Indices: two CCW triangles (0,1,2) and (0,2,3)
    quad.addIndex(0); quad.addIndex(1); quad.addIndex(2);
    quad.addIndex(2); quad.addIndex(3); quad.addIndex(0);
    vbo.setMesh(quad, GL_STATIC_DRAW);

    // bind per-instance attributes
    vbo.bind();
    GLsizei stride = sizeof(DividerInstance);
    std::size_t offP0    = offsetof(DividerInstance, p0);
    std::size_t offP1    = offsetof(DividerInstance, p1);
    std::size_t offWidth = offsetof(DividerInstance, width);
    std::size_t offStyle = offsetof(DividerInstance, style);
    std::size_t offColor = offsetof(DividerInstance, color);

    vbo.setAttributeBuffer(1, instanceBO, 2, stride, offP0);
    vbo.setAttributeDivisor(1, 1);
    vbo.setAttributeBuffer(2, instanceBO, 2, stride, offP1);
    vbo.setAttributeDivisor(2, 1);
    vbo.setAttributeBuffer(3, instanceBO, 1, stride, offWidth);
    vbo.setAttributeDivisor(3, 1);
    vbo.setAttributeBuffer(4, instanceBO, 1, stride, offStyle);
    vbo.setAttributeDivisor(4, 1);
    vbo.setAttributeBuffer(5, instanceBO, 4, stride, offColor);
    vbo.setAttributeDivisor(5, 1);
    vbo.unbind();
    
    shader.load();
  }
}

void DividedArea::clearInstanced() {
  head = 0;
  instanceCount = 0;
  instancesDirty = true;
}

void DividedArea::addDividerInstanced(const glm::vec2& a, const glm::vec2& b, float width, bool taper, const ofFloatColor& col) {
  if (instanceCapacity == 0) return;
  if (instanceCount == instanceCapacity) {
    head = (head + 1) % instanceCapacity;
    instanceCount--;
  }
  int idx = (head + instanceCount) % instanceCapacity;
  instances[idx].p0 = a;
  instances[idx].p1 = b;
  instances[idx].width = width;
  instances[idx].style = taper ? 1.0f : 0.0f;
  instances[idx].color = col;
  instanceCount++;
  instancesDirty = true;
}

static void syncInstanceBufferIfNeeded(const std::vector<DividerInstance>& instances, int head, int count, int capacity, ofBufferObject& bo) {
  if (count <= 0) return;
  int tail = (head + count) % capacity;
  if (!bo.isAllocated()) return;
  if (head < tail) {
    bo.updateData(0, count * (int)sizeof(DividerInstance), &instances[head]);
  } else {
    int firstCount = capacity - head;
    bo.updateData(0, firstCount * (int)sizeof(DividerInstance), &instances[head]);
    bo.updateData(firstCount * (int)sizeof(DividerInstance), tail * (int)sizeof(DividerInstance), &instances[0]);
  }
}

void DividedArea::drawInstanced(float scale) {
  if (instanceCount == 0) return;

  ofPushMatrix();
  ofScale(scale);

  if (instancesDirty) {
    syncInstanceBufferIfNeeded(instances, head, instanceCount, instanceCapacity, instanceBO);
    instancesDirty = false;
  }

  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofFill(); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); ofDisableDepthTest();
  shader.begin();
  vbo.bind();
  vbo.drawElementsInstanced(GL_TRIANGLES, quad.getNumIndices(), instanceCount);
  vbo.unbind();
  shader.end();

  ofPopMatrix();
}

void DividedArea::draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, float scale) const {
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
  }
  ofPopMatrix();
}

void DividedArea::draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float scale) const {
  ofPushMatrix();
  ofScale(scale);
  {
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
