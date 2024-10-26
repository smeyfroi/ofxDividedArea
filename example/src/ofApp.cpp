#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetFrameRate(50);
}

//--------------------------------------------------------------
void ofApp::update(){
  if (ofGetFrameNum() % 50 == 0) {
    dividedArea.addUnconstrainedDividerLine({ofRandom(300)+100,ofRandom(300)+100},
                                            {ofRandom(300)+100,ofRandom(300)+100});
  }
  dividedArea.addConstrainedDividerLine({ofRandom(600)+10,ofRandom(500)+10},
                                          {ofRandom(600)+10,ofRandom(500)+10});
}

//--------------------------------------------------------------
void ofApp::draw(){
  dividedArea.draw(10, 6, 0.5);
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
