#pragma once

#include "ofMath.h"
#include "glm/vec2.hpp"

float gradient(glm::vec2 start, glm::vec2 end);
float yForLineAtX(float x, glm::vec2 start, glm::vec2 end);
float xForLineAtY(float y, glm::vec2 start, glm::vec2 end) ;
std::optional<glm::vec2> lineToSegmentIntersection(glm::vec2 lStart, glm::vec2 lEnd, glm::vec2 lsStart, glm::vec2 lsEnd);
glm::vec2 endPointForSegment(const glm::vec2& startPoint, float angleRadians, float length);
