#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetBackgroundAuto(true);
  ofSetBackgroundColor(ofColor::black);
  
  dividedArea.setMaxDividers(4000);
  
  gui.setup(dividedArea.getParameterGroup());
}

//--------------------------------------------------------------
void ofApp::update(){
  // add unconstrained line to join up some established points
  majorRefPoints.insert(majorRefPoints.begin(), {ofRandom(1.0), ofRandom(1.0)});
  majorRefPoints.resize(std::min((int)majorRefPoints.size(), 14));
  dividedArea.updateUnconstrainedDividerLines(majorRefPoints);
    
    // OR add unconstrained line directly
//    dividedArea.addUnconstrainedDividerLine({ofRandom(300)+100, ofRandom(300)+100},
//                                            {ofRandom(300)+100, ofRandom(300)+100});
//  }

  dividedArea.addConstrainedDividerLine({ofRandom(1.0), ofRandom(1.0)},
                                        {ofRandom(1.0), ofRandom(1.0)});
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));
//  dividedArea.draw(10, 6, 0.5);
//  dividedArea.draw({10.0, 10.0, ofColor::white},
//                   {6.0, 6.0, ofColor::white},
//                   {1.0, 1.0, ofColor::white, 6.0*1.0/1000.0});
  const float maxLineWidth = 16.0;
  const float minLineWidth = 12.0;
  const ofFloatColor majorDividerColor { ofColor::white };
  const ofFloatColor minorDividerColor { ofColor::white };
//  // Instanced draw of both existing divider sets for comparison (optional)
//  // Here we mirror constrained lines into instanced buffer just for demo
  dividedArea.clearInstanced();
  for (const auto& dl : dividedArea.constrainedDividerLines) {
    dividedArea.addDividerInstanced(dl.start, dl.end, 12.0f, true, ofFloatColor::white);
  }
  // Only constrained lines are instanced. Unconstrained remain on original path.
  dividedArea.drawInstanced(ofGetWindowWidth());

  dividedArea.draw({},
                   { minLineWidth, maxLineWidth, majorDividerColor },
                   {},
//                   { minLineWidth*0.1f, minLineWidth*0.15f, minorDividerColor, 0.7 },
                   ofGetWindowWidth());

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
