#pragma once


#include "ofMain.h"
#include "ofxDividedArea.h"
#include <vector>
#include "glm/vec2.hpp"


class ofApp: public ofBaseApp{
public:
  void setup();
  void update();
  void draw();
  
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseMoved(int x, int y);
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);
  
  DividedArea dividedArea { {1024.0, 768.0}, 7 };
  std::vector<glm::vec2> majorRefPoints;
};
