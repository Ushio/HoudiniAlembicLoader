#pragma once

#include "ofMain.h"

#include "houdini_alembic.hpp"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>

using namespace Alembic::Abc;
using namespace Alembic::AbcGeom;

class ofApp : public ofBaseApp{
public:
	void setup();
	void exit();

	void update();
	void draw();

	void open_alembic(std::string filePath);

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	ofEasyCam _camera;

	IArchive _archive;
	houdini_alembic::AlembicStorage _storage;
	std::shared_ptr<houdini_alembic::AlembicScene> _scene;
};
