#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetBackgroundAuto(true);
  ofSetBackgroundColor(ofColor::black);
  
  gui.setup(dividedArea.getParameterGroup());
}

//--------------------------------------------------------------
void ofApp::update(){
  // add unconstrained line to join up some established points
  majorRefPoints.insert(majorRefPoints.begin(), {ofRandom(1.0), ofRandom(1.0)});
  majorRefPoints.resize(std::min((int)majorRefPoints.size(), 14));
  dividedArea.updateUnconstrainedDividerLines(majorRefPoints);
    
  dividedArea.addConstrainedDividerLine({ofRandom(1.0), ofRandom(1.0)},
                                        {ofRandom(1.0), ofRandom(1.0)},
                                        ofFloatColor(1.0, 1.0, 1.0, 0.5));
}

//--------------------------------------------------------------
void ofApp::draw(){
  MajorLineStyle style = dividedArea.getMajorLineStyle();
  std::string styleName = majorLineStyleToString(style);
  ofSetWindowTitle(ofToString(ofGetFrameRate()) + " | Style: " + styleName + " (1-7 to change) | Lines: " + ofToString(dividedArea.constrainedDividerLines.size()));

  // Draw constrained lines
  dividedArea.drawInstanced(ofGetWindowWidth());

  const float maxLineWidth = 26.0;
  const float minLineWidth = 12.0;
  const ofFloatColor majorDividerColor { ofColor::white };
  
  // For styles that need background sampling (Refractive, ChromaticAberration),
  // we need to capture the current frame to an FBO first
  bool needsBackground = (style == MajorLineStyle::Refractive || 
                          style == MajorLineStyle::ChromaticAberration);
  
  // Ensure FBO is allocated (can't do in setup as window may not be ready)
  if (!backgroundFbo.isAllocated() || 
      backgroundFbo.getWidth() != ofGetWidth() || 
      backgroundFbo.getHeight() != ofGetHeight()) {
    ofFbo::Settings settings;
    settings.width = ofGetWidth();
    settings.height = ofGetHeight();
    settings.internalformat = GL_RGBA;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.textureTarget = GL_TEXTURE_2D;  // Ensure non-ARB texture
    backgroundFbo.allocate(settings);
    // Set texture to normalized coordinates (0-1) instead of pixel coordinates
    backgroundFbo.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
  }
  
  if (needsBackground) {
    // Render background content to FBO
    backgroundFbo.begin();
    ofClear(0, 0, 0, 255);
    
    // Test pattern: colored circles to verify FBO sampling
    ofSetColor(255, 0, 0);
    ofDrawCircle(ofGetWidth() * 0.25, ofGetHeight() * 0.5, 80);
    ofSetColor(0, 255, 0);
    ofDrawCircle(ofGetWidth() * 0.5, ofGetHeight() * 0.5, 80);
    ofSetColor(0, 0, 255);
    ofDrawCircle(ofGetWidth() * 0.75, ofGetHeight() * 0.5, 80);
    ofSetColor(255);
    
    dividedArea.drawInstanced(ofGetWindowWidth());
    backgroundFbo.end();
    
    // Draw major lines with background FBO
    dividedArea.draw({}, { minLineWidth, maxLineWidth, majorDividerColor }, ofGetWindowWidth(), backgroundFbo);
  } else {
    // Draw major lines without background (styles that don't need it)
    dividedArea.draw({}, { minLineWidth, maxLineWidth, majorDividerColor }, ofGetWindowWidth(), backgroundFbo);
  }

  gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  // Number keys 1-7 select major line style directly
  if (key >= '1' && key <= '7') {
    int styleIndex = key - '1';
    if (styleIndex < static_cast<int>(MajorLineStyle::Count)) {
      dividedArea.setMajorLineStyle(static_cast<MajorLineStyle>(styleIndex));
    }
  }
  // Left/Right arrows cycle through styles
  else if (key == OF_KEY_LEFT) {
    int current = static_cast<int>(dividedArea.getMajorLineStyle());
    int next = (current - 1 + static_cast<int>(MajorLineStyle::Count)) % static_cast<int>(MajorLineStyle::Count);
    dividedArea.setMajorLineStyle(static_cast<MajorLineStyle>(next));
  }
  else if (key == OF_KEY_RIGHT) {
    int current = static_cast<int>(dividedArea.getMajorLineStyle());
    int next = (current + 1) % static_cast<int>(MajorLineStyle::Count);
    dividedArea.setMajorLineStyle(static_cast<MajorLineStyle>(next));
  }
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
