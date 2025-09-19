#include "ofMain.h"
#include "ofApp.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4,1);
  settings.setSize(1024, 1024);
  settings.numSamples = 8; // enable MSAA
  auto window = ofCreateWindow(settings);

  ofRunApp(window, std::make_shared<ofApp>());
  ofRunMainLoop();
}
