#include "ofApp.h"

DividerLine l1 {{50,50},{600,50},{50,50},{600,50}};
DividerLine l2 {{600,50},{600,500},{600,50},{600,500}};
DividerLine l3 {{600,500},{50,500},{600,500},{50,500}};
DividerLine l4 {{50,500},{50,50},{50,500},{50,50}};
DividerLines lines {l1,l2,l3,l4};

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetFrameRate(50);
}

//--------------------------------------------------------------
void ofApp::update(){
}

//--------------------------------------------------------------
void ofApp::draw(){
  for (const auto& l : lines) {
    l.draw(4);
  }
  auto l = DividerLine::create({ofRandom(300)+100,ofRandom(300)+100}, {ofRandom(300)+100,ofRandom(300)+100}, lines, {400,400});
  lines.push_back(l);
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
