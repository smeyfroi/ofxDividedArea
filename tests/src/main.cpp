#include "ofMain.h"
#include "ofApp.h"

int main(){
  ofGLFWWindowSettings settings;
  settings.setGLVersion(4,1);
  settings.setSize(800, 600);
  settings.numSamples = 4; // enable MSAA
  auto window = ofCreateWindow(settings);

  ofRunApp(window, std::make_shared<ofApp>());
  ofRunMainLoop();
}
