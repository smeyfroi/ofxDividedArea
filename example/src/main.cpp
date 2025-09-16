#include "ofApp.h"

int main(){
  ofGLWindowSettings settings;
  settings.setGLVersion(4,1);
  settings.setSize(1024, 1024);
  auto window = ofCreateWindow(settings);

  ofRunApp(window, std::make_shared<ofApp>());
  ofRunMainLoop();
}
