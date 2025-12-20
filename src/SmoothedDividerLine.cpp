#include "SmoothedDividerLine.hpp"
#include "glm/geometric.hpp"
#include <algorithm>

void SmoothedDividerLine::initializeFrom(const DividerLine& dl) {
    ref1 = dl.ref1;
    ref2 = dl.ref2;
    start = dl.start;
    end = dl.end;
    age = dl.age;
    
    // Initialize targets to current position (no initial movement)
    targetStart = start;
    targetEnd = end;
    
    // Zero initial velocity
    startVelocity = glm::vec2(0.0f, 0.0f);
    endVelocity = glm::vec2(0.0f, 0.0f);
    
    // Initialize zone-based hysteresis
    zoneCenter = (start + end) * 0.5f;
    accumStart = glm::vec2(0.0f, 0.0f);
    accumEnd = glm::vec2(0.0f, 0.0f);
    stableFrameCount = 0;
    
    // No deletion pressure
    framesWithoutMatch = 0;
}

void SmoothedDividerLine::proposeTarget(const glm::vec2& newStart, const glm::vec2& newEnd,
                                         float stabilityRadius) {
    // Use midpoint of endpoints as the "zone signature" for simplicity
    glm::vec2 proposedCenter = (newStart + newEnd) * 0.5f;
    
    if (stableFrameCount == 0) {
        // First proposal - initialize zone
        zoneCenter = proposedCenter;
        accumStart = newStart;
        accumEnd = newEnd;
        stableFrameCount = 1;
    } else if (glm::distance(proposedCenter, zoneCenter) <= stabilityRadius) {
        // Within zone - accumulate for centroid
        accumStart += newStart;
        accumEnd += newEnd;
        stableFrameCount++;
    } else {
        // Outside zone - reset with new proposal
        zoneCenter = proposedCenter;
        accumStart = newStart;
        accumEnd = newEnd;
        stableFrameCount = 1;
    }
    
    // Line has a match this frame - reset deletion counter
    framesWithoutMatch = 0;
}

void SmoothedDividerLine::updateSmoothed(float dt, float springStrength, float damping,
                                          int hysteresisFrames, float refPointDistance,
                                          float minRefPointDistance) {
    // Accept target if zone has been stable long enough
    // Use centroid of all accumulated proposals for smooth motion
    if (stableFrameCount >= hysteresisFrames && stableFrameCount > 0) {
        float invCount = 1.0f / static_cast<float>(stableFrameCount);
        targetStart = accumStart * invCount;
        targetEnd = accumEnd * invCount;
        // Reset accumulator for next round of proposals
        stableFrameCount = 0;
        accumStart = glm::vec2(0.0f, 0.0f);
        accumEnd = glm::vec2(0.0f, 0.0f);
    }
    
    // Angular stability: when ref points are close together, small movements
    // cause large angular swings. Reduce spring strength proportionally.
    float angularStabilityFactor = 1.0f;
    if (minRefPointDistance > 0.0f && refPointDistance < minRefPointDistance) {
        // Scale down spring strength as ref points get closer
        angularStabilityFactor = refPointDistance / minRefPointDistance;
        angularStabilityFactor = std::max(0.1f, angularStabilityFactor); // don't go to zero
    }
    
    float effectiveSpring = springStrength * angularStabilityFactor;
    
    // Spring-damper physics for start endpoint
    {
        glm::vec2 displacement = targetStart - start;
        glm::vec2 springForce = displacement * effectiveSpring;
        startVelocity += springForce * dt;
        startVelocity *= damping;
        start += startVelocity * dt;
    }
    
    // Spring-damper physics for end endpoint
    {
        glm::vec2 displacement = targetEnd - end;
        glm::vec2 springForce = displacement * effectiveSpring;
        endVelocity += springForce * dt;
        endVelocity *= damping;
        end += endVelocity * dt;
    }
    
    age++;
}

// Smoothness mappings:
// smoothness 0.0 = instant/responsive: high spring (20), low damping (0.80)
// smoothness 0.5 = balanced: medium spring (10), medium damping (0.90)
// smoothness 1.0 = very dreamy/floaty: low spring (1), high damping (0.985)

float SmoothedDividerLine::smoothnessToSpringStrength(float smoothness) {
    // Lerp from 20.0 (responsive) to 1.0 (very floaty)
    return 20.0f - smoothness * 19.0f;
}

float SmoothedDividerLine::smoothnessToDamping(float smoothness) {
    // Lerp from 0.80 (less damping, more responsive) to 0.985 (heavy damping, very floaty)
    return 0.80f + smoothness * 0.185f;
}

int SmoothedDividerLine::smoothnessToHysteresisFrames(float smoothness) {
    // 1 frame at smoothness 0, up to 12 frames at smoothness 1
    // (minimum 1 so we always get at least one proposal before accepting)
    return 1 + static_cast<int>(smoothness * 11.0f);
}

int SmoothedDividerLine::smoothnessToDeleteHysteresisFrames(float smoothness) {
    // 2 frames at smoothness 0, up to 18 frames at smoothness 1 (~0.3s at 60fps)
    // This prevents lines from flickering during brief cluster instability
    return 2 + static_cast<int>(smoothness * 16.0f);
}
