#pragma once
#include "ofMain.h"

class ofApp : public ofBaseApp {
public:
  void setup() override;
  void draw() override;
private:
  void runTests();
  std::vector<std::string> failures;
};
