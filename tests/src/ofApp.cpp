#include "ofApp.h"
#include "LineGeom.h"
#include "DividerLine.hpp"

static void expect(bool cond, std::vector<std::string>& failures, const std::string& msg){ if(!cond) failures.push_back(msg); }

void ofApp::setup(){
  runTests();
  ofLogNotice() << "Tests complete: " << (failures.empty()?"PASS":"FAIL") << " (" << failures.size() << " failures)";
  for(const auto& f: failures) ofLogError() << f;
  std::exit(failures.empty()?0:1);
}

void ofApp::draw(){}

void ofApp::runTests(){
  // gradient vertical undefined: just ensure it produces inf or nan
  {
    glm::vec2 a{0,0}, b{0,1};
    float m = gradient(a,b);
    expect(!std::isfinite(m), failures, "gradient vertical should be non-finite");
  }
  // xForLineAtY handles vertical
  {
    glm::vec2 a{2,0}, b{2,5};
    float x = xForLineAtY(3,a,b);
    expect(x==2.0f, failures, "xForLineAtY vertical should return constant x");
  }
  // yForLineAtX on vertical line yields non-finite (current impl will NaN); assert non-finite to capture bug
  {
    glm::vec2 a{2,0}, b{2,5};
    float y = yForLineAtX(2,a,b);
    expect(!std::isfinite(y), failures, "yForLineAtX vertical should not return finite y");
  }
  // Intersection: crossing
  {
    auto p = lineToSegmentIntersection({0,0},{10,10},{0,10},{10,10});
    expect(p.has_value(), failures, "intersection crossing exists");
    if(p) expect(glm::all(glm::epsilonEqual(*p, glm::vec2{10,10}, 1e-5f)), failures, "intersection at (10,10)");
  }
  // Intersection: vertical line vs segment
  {
    auto p = lineToSegmentIntersection({5,-10},{5,10},{0,0},{10,0});
    expect(p.has_value(), failures, "vertical line with horizontal segment should intersect");
    if(p) expect(glm::all(glm::epsilonEqual(*p, glm::vec2{5,0}, 1e-5f)), failures, "intersection at (5,0)");
  }
  // Parallel disjoint: expect no intersection
  {
    auto p = lineToSegmentIntersection({0,0},{10,0},{0,1},{10,1});
    expect(!p.has_value(), failures, "parallel disjoint should have no intersection");
  }
  // Collinear overlapping: expect nearest endpoint to lStart
  {
    auto p = lineToSegmentIntersection({0,0},{10,0},{2,0},{8,0});
    expect(p.has_value(), failures, "collinear overlapping should return a deterministic endpoint");
    if (p) expect(glm::all(glm::epsilonEqual(*p, glm::vec2{2,0}, 1e-5f)), failures, "collinear: nearest endpoint to lStart");
  }
  // Degenerate line (point) on segment: expect point
  {
    auto p = lineToSegmentIntersection({1,1},{1,1},{0,0},{2,2});
    expect(p.has_value(), failures, "degenerate line point on segment should intersect");
    if (p) expect(glm::all(glm::epsilonEqual(*p, glm::vec2{1,1}, 1e-5f)), failures, "degenerate line returned point");
  }
  // Degenerate segment (point) on line: expect point
  {
    auto p = lineToSegmentIntersection({0,0},{10,0},{5,0},{5,0});
    expect(p.has_value(), failures, "degenerate segment point on line should intersect");
    if (p) expect(glm::all(glm::epsilonEqual(*p, glm::vec2{5,0}, 1e-5f)), failures, "degenerate segment returned point");
  }
  // Zero-length distance equals point distance
  {
    DividerLine dl; dl.start = {1,1}; dl.end = {1,1};
    float d = DividerLine::pointToLineDistance({2,2}, dl);
    expect(std::isfinite(d), failures, "pointToLineDistance finite for zero-length");
    expect(std::fabs(d - glm::distance(glm::vec2{2,2}, glm::vec2{1,1})) < 1e-6f, failures, "distance equals point distance for zero-length");
  }
  // Zero-length occlusion is false
  {
    DividerLine a; a.start={0,0}; a.end={0,0};
    DividerLine b; b.start={0,0}; b.end={10,0};
    bool occ = a.isOccludedBy(b, 0.1f, 0.99f);
    expect(occ==false, failures, "zero-length line should not be considered occluded");
  }
}
