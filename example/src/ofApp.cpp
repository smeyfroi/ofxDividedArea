#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
//  ofSetFrameRate(50);
}

//--------------------------------------------------------------
void ofApp::update(){
//  if (ofGetFrameNum() % 50 == 0) {
    
    // add unconstrained line to join some established points
    majorRefPoints.insert(majorRefPoints.begin(), {ofRandom(ofGetWindowWidth()-100)+50, ofRandom(ofGetWindowHeight()-100)+50});
    majorRefPoints.resize(std::min((int)majorRefPoints.size(), 14));
    dividedArea.updateUnconstrainedDividerLines(majorRefPoints);
    
    // OR add unconstrained line directly
//    dividedArea.addUnconstrainedDividerLine({ofRandom(300)+100, ofRandom(300)+100},
//                                            {ofRandom(300)+100, ofRandom(300)+100});
//  }

  dividedArea.addConstrainedDividerLine({ofRandom(ofGetWindowWidth()-100)+50, ofRandom(ofGetWindowHeight()-100)+50},
                                        {ofRandom(ofGetWindowWidth()-100)+50, ofRandom(ofGetWindowHeight()-100)+50});
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));
//  dividedArea.draw(10, 6, 0.5);
  dividedArea.draw({10.0, 10.0, ofColor::white},
                   {6.0, 6.0, ofColor::white},
                   {1.0, 1.0, ofColor::white, 6.0*1.0/1000.0});
//  std::for_each(majorRefPoints.begin(),
//                majorRefPoints.end(),
//                [](const auto& p) { return ofDrawCircle(p.x, p.y, 8); });
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
