// made by Rayane Tsouli & Yassine Amraoui

#include "ofMain.h"
#include "ofApp.h"

int main() {
	ofGLWindowSettings s;
	s.setSize(1280, 720);
	s.setGLVersion(3, 2);
	s.windowMode = OF_WINDOW;

	ofCreateWindow(s);
	ofRunApp(std::make_shared<ofApp>());
}
