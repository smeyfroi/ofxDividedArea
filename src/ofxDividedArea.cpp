#include "ofxDividedArea.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofPath.h"
#include "ofMain.h"
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
    parameters.add(unconstrainedSmoothnessParameter);
    parameters.add(minRefPointDistanceParameter);
    parameters.add(closePointDistanceParameter);
    parameters.add(unconstrainedOcclusionDistanceParameter);
    parameters.add(constrainedOcclusionDistanceParameter);
    parameters.add(occlusionAngleParameter);
    parameters.add(maxConstrainedLinesParameter);
    parameters.add(maxTaperLengthParameter);
    parameters.add(minWidthFactorStartParameter);
    parameters.add(maxWidthFactorStartParameter);
    parameters.add(minWidthFactorEndParameter);
    parameters.add(maxWidthFactorEndParameter);
    parameters.add(constrainedWidthParameter);
    parameters.add(majorLineStyleParameter);
    
    // Add nested shader parameter groups
    parameters.add(innerGlowLineShader->getParameterGroup());
    parameters.add(bloomedAdditiveLineShader->getParameterGroup());
    parameters.add(glowLineShader->getParameterGroup());
    parameters.add(refractiveLineShader->getParameterGroup());
    parameters.add(blurRefractionLineShader->getParameterGroup());
    parameters.add(chromaticAberrationLineShader->getParameterGroup());
  }
  return parameters;
}

DividedArea::DividedArea(glm::vec2 size_, int maxUnconstrainedDividerLines_) :
size(size_),
maxUnconstrainedDividerLines(maxUnconstrainedDividerLines_)
{
  setupInstancedDraw(maxConstrainedLinesParameter);
  shader.load();
  
  // Create and load all style shaders upfront so their parameters are available
  solidLineShader = std::make_unique<SolidLineShader>();
  solidLineShader->load();
  
  innerGlowLineShader = std::make_unique<InnerGlowLineShader>();
  innerGlowLineShader->load();
  
  bloomedAdditiveLineShader = std::make_unique<BloomedAdditiveLineShader>();
  bloomedAdditiveLineShader->load();
  
  glowLineShader = std::make_unique<GlowLineShader>();
  glowLineShader->load();
  
  refractiveLineShader = std::make_unique<RefractiveLineShader>();
  refractiveLineShader->load();
  
  blurRefractionLineShader = std::make_unique<BlurRefractionLineShader>();
  blurRefractionLineShader->load();
  
  chromaticAberrationLineShader = std::make_unique<ChromaticAberrationLineShader>();
  chromaticAberrationLineShader->load();
}

void DividedArea::setParameterOverrides(const ParameterOverrides& overrides) {
  if (parameterOverrides_ == overrides) return;
  parameterOverrides_ = overrides;
}

void DividedArea::clearParameterOverrides() {
  if (parameterOverrides_ == ParameterOverrides {}) return;
  parameterOverrides_ = {};
}

float DividedArea::getUnconstrainedSmoothnessEffective() const {
  return parameterOverrides_.unconstrainedSmoothness.value_or(unconstrainedSmoothnessParameter.get());
}

bool DividedArea::addUnconstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2) {
  if (maxUnconstrainedDividerLines < 0 || static_cast<int>(unconstrainedDividerLines.size()) >= maxUnconstrainedDividerLines) return false;
  if (ref1 == ref2) return false;
  
  Line lineWithinArea = DividerLine::findEnclosedLine(ref1, ref2, areaConstraints);
  if (lineWithinArea.start == longestLine.start && lineWithinArea.end == longestLine.end) return false;
  
  DividerLine dividerLine { ref1, ref2, lineWithinArea.start, lineWithinArea.end };
  float occlusionDistance = unconstrainedOcclusionDistanceParameter * size.x;
  if (dividerLine.isOccludedByAnyOf(unconstrainedDividerLines, occlusionDistance, occlusionAngleParameter)) return false;
  
  SmoothedDividerLine smoothedLine;
  smoothedLine.initializeFrom(dividerLine);
  unconstrainedDividerLines.push_back(smoothedLine);
  return true;
}

// Update unconstrainedDividerLines to move towards the passed reference
// points (which can be glm::vec4), adding and deleting max one per call
// to maintain the number required.
//
// This algorithm matches existing lines to candidate lines by ENDPOINT proximity
// (not ref point proximity), then uses spring-damper physics with zone-based
// hysteresis for smooth, non-jerky motion even with unstable audio/video clusters.
//
// Zone-based hysteresis: proposals within a stability radius are accumulated,
// and their centroid becomes the target once stable for N frames.
//
// Deletion hysteresis: lines without matches persist for several frames before
// being removed, preventing flicker during brief cluster instability.
template<typename PT, typename A>
bool DividedArea::updateUnconstrainedDividerLines(const std::vector<PT, A>& majorRefPoints) {
  float occlusionDistance = unconstrainedOcclusionDistanceParameter * size.x;
  float closePointDistance = closePointDistanceParameter * size.x;
  float endpointMatchThreshold2 = closePointDistance * closePointDistance * 4.0f; // squared threshold for endpoint matching
  float minRefPointDistance = minRefPointDistanceParameter * size.x;
  
  // Stability radius for zone-based hysteresis: proposals within this distance
  // of the zone center are accumulated for centroid calculation
  float stabilityRadius = closePointDistance * 0.5f;
  
  // Get smoothing parameters from the single smoothness control
  float smoothness = getUnconstrainedSmoothnessEffective();
  float springStrength = SmoothedDividerLine::smoothnessToSpringStrength(smoothness);
  float damping = SmoothedDividerLine::smoothnessToDamping(smoothness);
  int hysteresisFrames = SmoothedDividerLine::smoothnessToHysteresisFrames(smoothness);
  int deleteHysteresisFrames = SmoothedDividerLine::smoothnessToDeleteHysteresisFrames(smoothness);
  
  // Frame-rate independent physics
  float dt = ofGetLastFrameTime();
  if (dt <= 0.0f || dt > 0.1f) dt = 1.0f / 60.0f; // clamp to reasonable range
  
  bool linesChanged = false;
  
  // 1. Build candidate lines from all pairs of ref points
  struct CandidateLine {
    glm::vec2 ref1, ref2;
    glm::vec2 start, end;
    float refPointDistance;
    bool used = false;
  };
  std::vector<CandidateLine> candidates;
  candidates.reserve(majorRefPoints.size() * (majorRefPoints.size() - 1) / 2);
  
  for (size_t i = 0; i < majorRefPoints.size(); ++i) {
    for (size_t j = i + 1; j < majorRefPoints.size(); ++j) {
      glm::vec2 r1 = glm::vec2(majorRefPoints[i]);
      glm::vec2 r2 = glm::vec2(majorRefPoints[j]);
      if (r1 == r2) continue;
      
      Line enclosed = DividerLine::findEnclosedLine(r1, r2, areaConstraints);
      // Skip degenerate lines
      if (enclosed.start == longestLine.start && enclosed.end == longestLine.end) continue;
      
      float refDist = glm::distance(r1, r2);
      candidates.push_back({r1, r2, enclosed.start, enclosed.end, refDist, false});
    }
  }
  
  // 2. For each existing line, find best candidate by endpoint proximity
  int keptCount = 0;
  for (auto iter = unconstrainedDividerLines.begin(); iter != unconstrainedDividerLines.end(); ) {
    // Enforce max count - delete excess lines
    if (maxUnconstrainedDividerLines >= 0 && keptCount >= maxUnconstrainedDividerLines) {
      iter = unconstrainedDividerLines.erase(iter);
      linesChanged = true;
      continue;
    }
    
    auto& line = *iter;
    
    float bestScore = std::numeric_limits<float>::max();
    CandidateLine* bestCandidate = nullptr;
    bool bestFlipped = false;
    
    for (auto& candidate : candidates) {
      if (candidate.used) continue;
      
      // Score by sum of squared endpoint distances; try both orientations
      float score1 = glm::distance2(line.start, candidate.start)
      + glm::distance2(line.end, candidate.end);
      float score2 = glm::distance2(line.start, candidate.end)
      + glm::distance2(line.end, candidate.start);
      
      bool flipped = score2 < score1;
      float score = flipped ? score2 : score1;
      
      if (score < bestScore) {
        bestScore = score;
        bestCandidate = &candidate;
        bestFlipped = flipped;
      }
    }
    
    // If a good match exists, propose target and update with physics
    if (bestCandidate && bestScore < endpointMatchThreshold2) {
      bestCandidate->used = true;
      
      glm::vec2 targetStart = bestFlipped ? bestCandidate->end : bestCandidate->start;
      glm::vec2 targetEnd = bestFlipped ? bestCandidate->start : bestCandidate->end;
      
      // Update ref points to track the new candidate
      line.ref1 = bestCandidate->ref1;
      line.ref2 = bestCandidate->ref2;
      
      // Propose new target (subject to zone-based hysteresis)
      line.proposeTarget(targetStart, targetEnd, stabilityRadius);
      
      // Update with spring-damper physics
      line.updateSmoothed(dt, springStrength, damping, hysteresisFrames,
                          bestCandidate->refPointDistance, minRefPointDistance);
      
      // Check for occlusion after update
      if (line.isOccludedByAnyOf(unconstrainedDividerLines, occlusionDistance, occlusionAngleParameter)) {
        iter = unconstrainedDividerLines.erase(iter);
        linesChanged = true;
        continue;
      }
      
      linesChanged = true;
      ++keptCount;
      ++iter;
    } else {
      // No good match this frame - apply deletion hysteresis
      line.framesWithoutMatch++;
      
      if (line.framesWithoutMatch >= deleteHysteresisFrames) {
        // Line has been without a match for too long - delete it
        iter = unconstrainedDividerLines.erase(iter);
        linesChanged = true;
      } else {
        // Keep the line alive, continue physics toward existing target
        line.updateSmoothed(dt, springStrength, damping, hysteresisFrames,
                            minRefPointDistance, minRefPointDistance);
        ++keptCount;
        ++iter;
      }
    }
  }
  
  // 3. Add one new line from unused candidates (if under max)
  if (maxUnconstrainedDividerLines < 0 || static_cast<int>(unconstrainedDividerLines.size()) < maxUnconstrainedDividerLines) {
    for (auto& candidate : candidates) {
      if (candidate.used) continue;
      
      DividerLine newLine { candidate.ref1, candidate.ref2, candidate.start, candidate.end };
      if (!newLine.isOccludedByAnyOf(unconstrainedDividerLines, occlusionDistance, occlusionAngleParameter)) {
        SmoothedDividerLine smoothedLine;
        smoothedLine.initializeFrom(newLine);
        unconstrainedDividerLines.push_back(smoothedLine);
        linesChanged = true;
        break; // add max one per call
      }
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
  Line lineWithinUnconstrainedDividerLines = DividerLine::findEnclosedLineIn(ref1, ref2, unconstrainedDividerLines, lineWithinArea);
  return DividerLine::create(ref1, ref2, constrainedDividerLines, lineWithinUnconstrainedDividerLines);
}

std::optional<DividerLine> DividedArea::addConstrainedDividerLine(glm::vec2 ref1, glm::vec2 ref2, ofFloatColor color, float overriddenWidth) {
  if (ref1 == ref2) return std::nullopt;
  DividerLine dividerLine = createConstrainedDividerLine(ref1, ref2);
  float occlusionDistance = constrainedOcclusionDistanceParameter * size.x;
  if (dividerLine.isOccludedByAny(constrainedDividerLines, occlusionDistance, occlusionAngleParameter)) return std::nullopt;
  if (constrainedDividerLines.size() > maxConstrainedLinesParameter) deleteEarlyConstrainedDividerLines(maxConstrainedLinesParameter * 0.05);
  constrainedDividerLines.push_back(dividerLine);
  float width = (overriddenWidth > 0.0) ? overriddenWidth : constrainedWidthParameter.get();
  addDividerInstanced(dividerLine.start, dividerLine.end,
                      width, true,
                      color);
  return dividerLine;
}

void DividedArea::setupInstancedDraw(int newInstanceCapacity) {
  // build unit quad only once
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
  }
  
  instanceCapacity = newInstanceCapacity;
  instances.resize(instanceCapacity);
  head = std::min(head, instanceCapacity - 1);
  instanceCount = std::min(instanceCount, instanceCapacity);
  instancesDirty = true;
  
  instanceBO.allocate(instances, GL_DYNAMIC_DRAW);
  
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
}

void DividedArea::addDividerInstanced(const glm::vec2& a, const glm::vec2& b, float width, bool taper, const ofFloatColor& col) {
  if (instanceCapacity != maxConstrainedLinesParameter) {
    setupInstancedDraw(maxConstrainedLinesParameter);
  }
  
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
  if (!bo.isAllocated()) return;
  int tail = (head + count) % capacity;
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
  
  if (instancesDirty) {
    syncInstanceBufferIfNeeded(instances, head, instanceCount, instanceCapacity, instanceBO);
    instancesDirty = false;
  }
  
  ofPushMatrix();
  ofScale(scale);
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofFill(); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); ofDisableDepthTest();
  shader.begin(maxTaperLengthParameter,
               minWidthFactorStartParameter,
               maxWidthFactorStartParameter,
               minWidthFactorEndParameter,
               maxWidthFactorEndParameter);
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

void DividedArea::draw(LineConfig areaConstraintLineConfig, LineConfig unconstrainedLineConfig, float scale, const ofFbo& backgroundFbo) {
  ofPushMatrix();
  ofScale(scale);
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofFill();
  ofDisableDepthTest();
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
      ofFloatColor color = unconstrainedLineConfig.color;
      std::for_each(unconstrainedDividerLines.begin(),
                    unconstrainedDividerLines.end(),
                    [&](const auto& dl) {
        drawMajorLine(dl, unconstrainedLineConfig.maxWidth, scale, color, &backgroundFbo);
      });
    }
  }
  ofPopMatrix();
}

void DividedArea::drawMajorLine(const DividerLine& dl, float width, float scale,
                                const ofFloatColor& color, const ofFbo* backgroundFbo) {
  MajorLineStyle style = getMajorLineStyle();
  float widthNorm = width / scale;
  
  switch (style) {
    case MajorLineStyle::Solid:
      solidLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      break;
      
    case MajorLineStyle::InnerGlow:
      innerGlowLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      break;
      
    case MajorLineStyle::BloomedAdditive:
      bloomedAdditiveLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      break;
      
    case MajorLineStyle::Glow:
      glowLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      break;
      
    case MajorLineStyle::Refractive:
      if (backgroundFbo) {
        if (!refractiveLineShader) {
          refractiveLineShader = std::make_unique<RefractiveLineShader>();
          refractiveLineShader->load();
        }
        refractiveLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      }
      break;
      
    case MajorLineStyle::ChromaticAberration:
      if (backgroundFbo) {
        if (!chromaticAberrationLineShader) {
          chromaticAberrationLineShader = std::make_unique<ChromaticAberrationLineShader>();
          chromaticAberrationLineShader->load();
        }
        chromaticAberrationLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      }
      break;
      
    case MajorLineStyle::BlurRefraction:
      if (backgroundFbo) {
        blurRefractionLineShader->render(dl.start, dl.end, widthNorm, color, backgroundFbo);
      }
      break;
      
    default:
      // Fallback to solid
      dl.draw(widthNorm);
      break;
  }
}

void DividedArea::draw(float areaConstraintLineWidth, float unconstrainedLineWidth, float scale, const ofFbo& backgroundFbo, const ofFloatColor& color) {
  ofPushMatrix();
  ofScale(scale);
  {
    if (unconstrainedLineWidth > 0) {
      std::for_each(unconstrainedDividerLines.begin(),
                    unconstrainedDividerLines.end(),
                    [&](const auto& dl) {
        drawMajorLine(dl, unconstrainedLineWidth, scale, color, &backgroundFbo);
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

void DividedArea::drawMajorLinesWithoutBackground(float unconstrainedLineWidth, float scale, const ofFloatColor& color) {
  if (unconstrainedLineWidth <= 0) return;
  
  MajorLineStyle style = getMajorLineStyle();
  
  // If current style requires background, log warning and fall back to Solid
  if (majorLineStyleRequiresBackground(style)) {
    ofLogWarning("DividedArea") << "Major line style '" << majorLineStyleToString(style)
                                << "' requires background FBO but none provided. "
                                << "Falling back to Solid style for non-overlay layer.";
    style = MajorLineStyle::Solid;
  }
  
  ofPushMatrix();
  ofScale(scale);
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofFill();
  ofDisableDepthTest();
  
  float widthNorm = unconstrainedLineWidth / scale;
  
  for (const auto& dl : unconstrainedDividerLines) {
    switch (style) {
      case MajorLineStyle::Solid:
        solidLineShader->render(dl.start, dl.end, widthNorm, color, nullptr);
        break;
        
      case MajorLineStyle::InnerGlow:
        innerGlowLineShader->render(dl.start, dl.end, widthNorm, color, nullptr);
        break;
        
      case MajorLineStyle::BloomedAdditive:
        bloomedAdditiveLineShader->render(dl.start, dl.end, widthNorm, color, nullptr);
        break;
        
      case MajorLineStyle::Glow:
        glowLineShader->render(dl.start, dl.end, widthNorm, color, nullptr);
        break;
        
      default:
        // Fallback to basic solid line
        dl.draw(widthNorm);
        break;
    }
  }
  
  ofPopMatrix();
}
