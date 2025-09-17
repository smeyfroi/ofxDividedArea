#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetBackgroundAuto(true);
  ofSetBackgroundColor(ofColor::black);
  
  dividedArea.setMaxConstrainedDividers(4000);
  
  gui.setup(dividedArea.getParameterGroup());
}

//--------------------------------------------------------------
void ofApp::update(){
  // add unconstrained line to join up some established points
  majorRefPoints.insert(majorRefPoints.begin(), {ofRandom(1.0), ofRandom(1.0)});
  majorRefPoints.resize(std::min((int)majorRefPoints.size(), 14));
  dividedArea.updateUnconstrainedDividerLines(majorRefPoints);
    
  auto dl = dividedArea.addConstrainedDividerLine({ofRandom(1.0), ofRandom(1.0)},
                                                  {ofRandom(1.0), ofRandom(1.0)});
  if (dl.has_value()) {
    const ofFloatColor minorDividerColor { ofColor::white };
    dividedArea.addDividerInstanced(dl->start, dl->end, 1.0/200.0f, true, ofFloatColor::white);
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));

  dividedArea.drawInstanced(ofGetWindowWidth());

  const float maxLineWidth = 16.0;
  const float minLineWidth = 12.0;
  const ofFloatColor majorDividerColor { ofColor::white };
  dividedArea.draw({}, { minLineWidth, maxLineWidth, majorDividerColor }, ofGetWindowWidth());

  gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
