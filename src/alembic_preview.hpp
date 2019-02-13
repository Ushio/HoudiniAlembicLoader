#pragma once

#include "ofMain.h"
#include "houdini_alembic.hpp"

const float kNormalLength = 0.1f;
const ofColor kNormalColor = ofColor(128, 128, 255);

inline void drawAlembicPolygon(const houdini_alembic::PolygonMeshObject *polygon) {
	auto P_Column = polygon->points.column_as_vector3("P");

	bool isTriangleMesh = std::all_of(polygon->faceCounts.begin(), polygon->faceCounts.end(), [](int32_t f) { return f == 3; });

	if (isTriangleMesh) {
		static ofMesh mesh;
		static ofMesh normalMesh;
		mesh.clear();
		mesh.setMode(OF_PRIMITIVE_TRIANGLES);

		normalMesh.clear();
		normalMesh.setMode(OF_PRIMITIVE_LINES);

		int rowCount = P_Column->rowCount();
		for (int i = 0; i < rowCount; ++i) {
			glm::vec3 p;
			P_Column->get(i, glm::value_ptr(p));
			mesh.addVertex(p);
		}

		for (auto index : polygon->indices) {
			mesh.addIndex(index);
		}

		if (auto N = polygon->points.column_as_vector3("N")) {
			for (int i = 0; i < N->rowCount(); ++i) {
				glm::vec3 p;
				P_Column->get(i, glm::value_ptr(p));

				glm::vec3 n;
				N->get(i, glm::value_ptr(n));

				normalMesh.addVertex(p);
				normalMesh.addVertex(p + n * kNormalLength);
			}
		}

		if (auto Cd = polygon->points.column_as_vector3("Cd")) {
			for (int i = 0; i < Cd->rowCount(); ++i) {
				glm::vec3 p;
				Cd->get(i, glm::value_ptr(p));
				mesh.addColor(ofFloatColor(p.x, p.y, p.z));
			}
		}

		ofPushMatrix();
		ofMultMatrix(polygon->combinedXforms.value_ptr());

		//for (auto xform : polygon->xforms) {
		//	ofMultMatrix(xform.value_ptr());
		//}

		// Houdini は CW
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		{
			// 表面を明るく
			ofSetColor(128);
			glCullFace(GL_BACK);
			mesh.draw();

			// 裏面を暗く
			ofSetColor(32);
			glCullFace(GL_FRONT);
			mesh.draw();
		}
		glDisable(GL_CULL_FACE);

		ofSetColor(kNormalColor);
		normalMesh.draw();

		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-0.1f, 1.0f);
		ofSetColor(64);
		mesh.clearColors();
		mesh.drawWireframe();
		glDisable(GL_POLYGON_OFFSET_LINE);

		ofPopMatrix();
	}
}
inline void drawAlembicPoint(const houdini_alembic::PointObject *point) {
	auto P_Column = point->points.column_as_vector3("P");

	static ofMesh mesh;
	static ofMesh normalMesh;
	mesh.clear();
	mesh.setMode(OF_PRIMITIVE_POINTS);

	normalMesh.clear();
	normalMesh.setMode(OF_PRIMITIVE_LINES);

	int rowCount = P_Column->rowCount();
	for (int i = 0; i < rowCount; ++i) {
		glm::vec3 p;
		P_Column->get(i, glm::value_ptr(p));
		mesh.addVertex(p);
	}

	if (auto N = point->points.column_as_vector3("N")) {
		for (int i = 0; i < N->rowCount(); ++i) {
			glm::vec3 p;
			P_Column->get(i, glm::value_ptr(p));

			glm::vec3 n;
			N->get(i, glm::value_ptr(n));

			normalMesh.addVertex(p);
			normalMesh.addVertex(p + n * kNormalLength);
		}
	}

	ofSetColor(200);
	mesh.draw();

	ofSetColor(kNormalColor);
	normalMesh.draw();
}
inline void drawAlembicCamera(const houdini_alembic::CameraObject *camera, ofMesh &camera_model) {
	ofPushMatrix();
	ofMultMatrix(camera->combinedXforms.value_ptr());

	ofSetColor(200);
	camera_model.drawWireframe();
	ofDrawAxis(0.5f);
	ofPopMatrix();

	auto to = [](houdini_alembic::Vector3f p) {
		return glm::vec3(p.x, p.y, p.z);
	};

	glm::vec3 object_center = to(camera->eye) + to(camera->forward) * camera->focusDistance;

	glm::vec3 o = object_center + to(camera->left) * camera->objectPlaneWidth * 0.5f + to(camera->up) * camera->objectPlaneHeight * 0.5f;
	glm::vec3 object_vertices[] = {
		o,
		o + to(camera->right) * camera->objectPlaneWidth,
		o + to(camera->right) * camera->objectPlaneWidth + to(camera->down) * camera->objectPlaneHeight,
		o + to(camera->down) * camera->objectPlaneHeight,
	};

	ofSetColor(200);
	for (int i = 0; i < 4; ++i) {
		ofDrawLine(object_vertices[i], object_vertices[(i + 1) % 4]);
		ofDrawLine(to(camera->eye), to(camera->eye) + (object_vertices[i] - to(camera->eye)) * 3.0f);
	}
}
inline void drawAlembicScene(std::shared_ptr<houdini_alembic::AlembicScene> scene, ofMesh &camera_model, bool draw_camera) {
	for (auto o : scene->objects) {
		if (o->visible == false) {
			continue;
		}
		if (auto camera = o.as_camera()) {
			drawAlembicCamera(camera, camera_model);
		}
		else if (auto point = o.as_point()) {
			drawAlembicPoint(point);
		}
		else if (auto polygon = o.as_polygonMesh()) {
			drawAlembicPolygon(polygon);
		}
	}
}