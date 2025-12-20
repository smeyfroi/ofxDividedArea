#pragma once

#include "DividerLine.hpp"
#include "glm/vec2.hpp"

// A DividerLine with velocity-based smoothing for unconstrained (major) lines.
// Uses spring-damper physics with zone-based hysteresis to provide smooth,
// non-jerky motion even when input reference points (e.g., cluster centres)
// jump around unpredictably (common with audio/video analysis).
//
// Zone-based hysteresis: instead of requiring the exact same target for N frames,
// targets within a "stability radius" are accumulated and their centroid is used.
// This handles jittery input where each frame proposes a slightly different target.
//
// Deletion hysteresis: lines aren't removed immediately when they lose their match.
// They persist for several frames, allowing temporary cluster instability to pass.
//
// The "smoothness" parameter (0.0-1.0) controls overall feel:
//   0.0 = instant/responsive (nearly direct tracking)
//   0.5 = balanced (smooth but responsive)
//   1.0 = very dreamy/floaty (heavy damping, slow response)

class SmoothedDividerLine : public DividerLine {
public:
    // Target position (where we want endpoints to be)
    glm::vec2 targetStart {0.0f, 0.0f};
    glm::vec2 targetEnd {0.0f, 0.0f};
    
    // Velocities for spring-damper physics
    glm::vec2 startVelocity {0.0f, 0.0f};
    glm::vec2 endVelocity {0.0f, 0.0f};
    
    // Zone-based hysteresis: proposals within stabilityRadius of zone center
    // are accumulated, and their centroid becomes the target once stable
    glm::vec2 zoneCenter {0.0f, 0.0f};    // center of current stability zone (midpoint of start+end)
    glm::vec2 accumStart {0.0f, 0.0f};    // accumulated start proposals for centroid
    glm::vec2 accumEnd {0.0f, 0.0f};      // accumulated end proposals for centroid
    int stableFrameCount = 0;              // frames the zone has been stable
    
    // Deletion hysteresis: line must have no match for N frames before removal
    int framesWithoutMatch = 0;
    
    // Initialize from a regular DividerLine (for new lines)
    void initializeFrom(const DividerLine& dl);
    
    // Propose a new target position (subject to zone-based hysteresis)
    // stabilityRadius: proposals within this distance of zone center count as "stable"
    // Resets framesWithoutMatch to 0 (line has a match this frame)
    void proposeTarget(const glm::vec2& newStart, const glm::vec2& newEnd,
                       float stabilityRadius);
    
    // Update endpoints using spring-damper physics
    // dt: frame delta time in seconds
    // springStrength: how quickly lines accelerate toward target
    // damping: velocity decay factor (0.0-1.0, higher = less damping)
    // hysteresisFrames: frames before accumulated centroid is accepted as target
    // refPointDistance: distance between ref points (for angular stability)
    // minRefPointDistance: below this, damping increases to prevent angular jitter
    void updateSmoothed(float dt, float springStrength, float damping,
                        int hysteresisFrames, float refPointDistance,
                        float minRefPointDistance);
    
    // Convert smoothness (0.0-1.0) to physics parameters
    static float smoothnessToSpringStrength(float smoothness);
    static float smoothnessToDamping(float smoothness);
    static int smoothnessToHysteresisFrames(float smoothness);
    static int smoothnessToDeleteHysteresisFrames(float smoothness);
};
