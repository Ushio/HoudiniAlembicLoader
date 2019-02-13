#include "ofApp.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "unit_test.hpp"
#include "alembic_preview.hpp"

template <class T>
void imgui_tree(const char *name, bool isOpen, T f) {
	if (isOpen) {
		ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	}
	if (ImGui::TreeNode(name)) {
		f();
		ImGui::TreePop();
	}
}

inline void show_meta(MetaData metaData) {
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.5, 1, 1));
	for (auto meta : metaData) {
		ImGui::Text("meta %s : %s", meta.first.c_str(), meta.second.c_str());
	}
	ImGui::PopStyleColor();
}

template <class ITypedProperty>
typename ITypedProperty::value_type get_typed_scalar_property(ICompoundProperty parent, const std::string &key, ISampleSelector selector) {
	static_assert(std::is_base_of<IScalarProperty, ITypedProperty>::value, "ITypedProperty is must be derived IScalarProperty");

	ITypedProperty typed_property(parent, key);
	typename ITypedProperty::value_type value;
	typed_property.get(value, selector);
	return value;
}

template <class ITypedProperty>
typename ITypedProperty::sample_ptr_type get_typed_array_property(ICompoundProperty parent, const std::string &key, ISampleSelector selector) {
	static_assert(std::is_base_of<IArrayProperty, ITypedProperty>::value, "ITypedProperty is must be derived IArrayProperty");

	ITypedProperty typed_property(parent, key);
	typename ITypedProperty::sample_ptr_type value;
	typed_property.get(value, selector);
	return value;
}

template <class ITypedProperty>
void show_typed_array_property(ITypedProperty prop, const char *label, ISampleSelector selector) {
	static_assert(std::is_base_of<IArrayProperty, ITypedProperty>::value, "ITypedProperty is must be derived IArrayProperty");
	typename ITypedProperty::sample_ptr_type value;
	prop.get(value, selector);

	std::stringstream ss;
	for (int i = 0; i < value->size(); ++i) {
		ss << (*value)[i];
		ss << "\n";
	}

	ImGui::Text(label);
	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(0, 100), ImGuiWindowFlags_NoTitleBar);
	ImGui::TextWrapped("%s", ss.str().c_str());
	ImGui::EndChild();
}

inline void show_alembic_property_array(IArrayProperty prop, ISampleSelector selector) {
	std::string label = prop.getName() + "[IArrayProperty]";
	imgui_tree(label.c_str(), true, [&]() {
		show_meta(prop.getHeader().getMetaData());

		imgui_tree("data", false, [&]() {
#define SHOW_TYPED_ARRAY(property_type) show_typed_array_property(property_type(prop.getParent(), prop.getName()), #property_type, selector);
			if (IP3fArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IP3fArrayProperty);
			}
			else if (IV2fArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IV2fArrayProperty);
			}
			else if (IV3fArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IV3fArrayProperty);
			}
			else if (IN3fArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IN3fArrayProperty);
			}
			else if (IFloatArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IFloatArrayProperty);
			}
			else if (IInt32ArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IInt32ArrayProperty);
			}
			else if (IUInt32ArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IUInt32ArrayProperty);
			}
			else if (IUInt64ArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IUInt64ArrayProperty);
			}
			else if (IStringArrayProperty::matches(prop.getHeader())) {
				SHOW_TYPED_ARRAY(IStringArrayProperty);
			}
			else {
				auto dataType = prop.getHeader().getDataType();
				ImGui::Text("%s, extent(%d)", PODName(dataType.getPod()), dataType.getExtent());
			}
#undef SHOW_TYPED_ARRAY
		});
	});
}

inline void show_alembic_property_scalar(IScalarProperty prop, ISampleSelector selector) {
	std::string label = prop.getName() + "[IScalarProperty]";
	imgui_tree(label.c_str(), true, [&]() {
		show_meta(prop.getHeader().getMetaData());
		auto header = prop.getHeader();

		if (IBoolProperty::matches(header)) {
			auto value = get_typed_scalar_property<IBoolProperty>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%s [IBoolProperty]", value ? "true" : "false");
		}
		else if (IUcharProperty::matches(header)) {
			auto value = get_typed_scalar_property<IUcharProperty>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%d [IUcharProperty]", (int)value);
		}
		else if (ICharProperty::matches(header)) {
			auto value = get_typed_scalar_property<ICharProperty>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%d [ICharProperty]", (int)value);
		}
		else if (IUInt32Property::matches(header)) {
			auto value = get_typed_scalar_property<IUInt32Property>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%d [IUInt32Property]", value);
		}
		else if (IInt32Property::matches(header)) {
			auto value = get_typed_scalar_property<IInt32Property>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%d [IInt32Property]", value);
		}
		else if (IFloatProperty::matches(header)) {
			auto value = get_typed_scalar_property<IFloatProperty>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%f [IFloatProperty]", value);
		}
		else if (IStringProperty::matches(header)) {
			auto value = get_typed_scalar_property<IStringProperty>(prop.getParent(), prop.getName(), selector);
			ImGui::Text("%s [IStringProperty]", value.c_str());
		}
		else if (IM44dProperty::matches(header, kNoMatching)) {
			auto value = get_typed_scalar_property<IM44dProperty>(prop.getParent(), prop.getName(), selector);
			for (int row = 0; row < 4; ++row) {
				for (int col = 0; col < 4; ++col) {
					ImGui::Text("%.4f ", value.x[col][row]);

					if (col < 3) {
						ImGui::SameLine();
					}
				}
			}
		}
		else if (IBox3dProperty::matches(header)) {
			IBox3dProperty typed_property(prop.getParent(), prop.getName());
			Box3d sample;
			typed_property.get(sample, selector);
			ImGui::Text(
				"Box (%f, %f, %f), (%f, %f, %f)",
				sample.min.x, sample.min.y, sample.min.z,
				sample.max.x, sample.max.y, sample.max.z
			);
		}
		else {
			auto dataType = header.getDataType();
			ImGui::Text("%s, extent (%d)", PODName(dataType.getPod()), dataType.getExtent());
		}
	});
}

inline void show_alembic_property_compound(ICompoundProperty props, ISampleSelector selector) {
	std::string label = props.getName() + "[ICompoundProperty]";
	imgui_tree(label.c_str(), true, [&]() {
		show_meta(props.getHeader().getMetaData());

		for (int i = 0; i < props.getNumProperties(); ++i) {
			auto childHeader = props.getPropertyHeader(i);
			if (childHeader.isCompound()) {
				ICompoundProperty compoundProperty(props, childHeader.getName());
				show_alembic_property_compound(compoundProperty, selector);
			}
			else if (childHeader.isArray()) {
				show_alembic_property_array(IArrayProperty(props, childHeader.getName()), selector);
			}
			else if (childHeader.isScalar()) {
				show_alembic_property_scalar(IScalarProperty(props, childHeader.getName()), selector);
			}
			else if (childHeader.isSimple()) {
				ImGui::Text("isSimple");
			}
		}
	});
}


inline void show_alembic_hierarchy(IObject o, ISampleSelector selector) {
	auto header = o.getHeader();
	std::string fullname = header.getFullName();

	if (IPolyMesh::matches(header)) {
		std::string label = fullname + "(IPolyMesh)";
		IPolyMesh polyMesh(o);

		imgui_tree(label.c_str(), true, [&]() {
			show_alembic_property_compound(o.getProperties(), selector);
		});
	}
	else if (ICamera::matches(header)) {
		std::string label = fullname + "(ICamera)";
		imgui_tree(label.c_str(), true, [&]() {
			show_alembic_property_compound(o.getProperties(), selector);
		});
	}
	else if (IXform::matches(header)) {
		std::string label = fullname + "(IXform)";

		imgui_tree(label.c_str(), true, [&]() {
			show_alembic_property_compound(o.getProperties(), selector);

			for (int i = 0; i < o.getNumChildren(); ++i) {
				IObject child = o.getChild(i);
				show_alembic_hierarchy(child, selector);
			}
		});
	}
	else {
		std::string label = fullname + "(IObject)";
		imgui_tree(label.c_str(), true, [&]() {

			show_alembic_property_compound(o.getProperties(), selector);

			for (int i = 0; i < o.getNumChildren(); ++i) {
				IObject child = o.getChild(i);
				show_alembic_hierarchy(child, selector);
			}
		});
	}
}
inline void show_alembic_hierarchy(IObject o, int64_t sample_index) {
	show_alembic_hierarchy(o, ISampleSelector(sample_index));
}

inline void show_sheet(const houdini_alembic::AttributeSpreadSheet &sheet) {
	if (sheet.rowCount() == 0) {
		return;
	}

	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(0, 500), ImGuiWindowFlags_NoTitleBar);

	ImGui::Columns(sheet.columnCount(), "sheet");
	ImGui::Separator();
	for (auto col : sheet.sheet) {
		ImGui::Text("%s (%s)", col.key.c_str(), houdini_alembic::attributeTypeString(col.column->attributeType())); ImGui::NextColumn();
	}
	ImGui::Separator();

	char buffer[64];
	bool breaked = false;
	for (int i = 0; i < sheet.rowCount(); i++) {
		for (auto col : sheet.sheet) {
			col.column->snprint(i, buffer, sizeof(buffer));
			ImGui::Text("%s", buffer);
			ImGui::NextColumn();
		}

		if (10000 < i) {
			breaked = true;
			break;
		}
	}

	ImGui::EndChild();

	if (breaked) {
		ImGui::Text("skipped 10000 over points...");
	}
}
inline void show_polygon_sheet(houdini_alembic::PolygonMeshObject *object) {
	ImGui::Text("[PolygonMeshObject]");

	if (ImGui::BeginTabBar("Geometry SpreadSheet", ImGuiTabBarFlags_None)) {
		ImGui::Spacing();
		if (ImGui::BeginTabItem("Points"))
		{
			show_sheet(object->points);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Vertices"))
		{
			show_sheet(object->vertices);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Primitives"))
		{
			show_sheet(object->primitives);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}
inline void show_point_sheet(houdini_alembic::PointObject *object) {
	ImGui::Text("[PointObject]");

	if (ImGui::BeginTabBar("Geometry SpreadSheet", ImGuiTabBarFlags_None)) {
		ImGui::Spacing();
		if (ImGui::BeginTabItem("Points"))
		{
			show_sheet(object->points);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}
inline void show_camera(houdini_alembic::CameraObject *camera) {
	ImGui::Text("[CameraObject]");

	ImGui::Text("resolution_x : %d", camera->resolution_x);
	ImGui::Text("resolution_y : %d", camera->resolution_y);
	ImGui::Separator();
	ImGui::Text("Focal Length : %.3f mm", camera->focalLength_mm);
	ImGui::Text("Aperture H : %.3f mm", camera->aperture_horizontal_mm);
	ImGui::Text("Aperture V : %.3f mm", camera->aperture_vertical_mm);
	ImGui::Text("Near Clip : %.3f", camera->nearClip);
	ImGui::Text("Far  Clip : %.3f", camera->farClip);
	ImGui::Separator();
	ImGui::Text("Focus Distance : %.3f", camera->focusDistance);
	ImGui::Text("F-Stop : %.3f", camera->f_stop);
	ImGui::Separator();
	ImGui::Text("fov horizontal : %.3f deg", camera->fov_horizontal_degree);
	ImGui::Text("fov vertical   : %.3f deg", camera->fov_vertical_degree);
	ImGui::Text("Lens Radius : %.3f", camera->lensRadius);
	ImGui::Text("Object Plane Width  : %.3f", camera->objectPlaneWidth);
	ImGui::Text("Object Plane Height : %.3f", camera->objectPlaneHeight);
}
inline void show_houdini_alembic(std::shared_ptr<houdini_alembic::AlembicScene> scene) {
	if (!scene) {
		ImGui::Text("scene load failed");
		return;
	}
	for (auto object : scene->objects) {
		imgui_tree(object->name.c_str(), true, [&]() {
			ImGui::Text("visible : %s", object->visible ? "True" : "False");
			if (auto o = object.as_camera()) {
				show_camera(o);
			}
			else if (auto o = object.as_point()) {
				show_point_sheet(o);
			}
			else if (auto o = object.as_polygonMesh()) {
				show_polygon_sheet(o);
			}
		});
	}
}

//--------------------------------------------------------------
void ofApp::setup() {
	// run_unit_test();

	// setup ImGui
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	ofAppGLFWWindow *window = dynamic_cast<ofAppGLFWWindow *>(ofGetWindowPtr());
	GLFWwindow *glfwWindow = window->getGLFWWindow();
	ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
	ImGui_ImplOpenGL2_Init();


	_camera.setNearClip(0.1f);
	_camera.setFarClip(100.0f);
	_camera.setDistance(5.0f);

	_camera_model.load("camera_model.ply");

	open_alembic(ofToDataPath("points.abc"));

	ofSetVerticalSync(false);
}
void ofApp::exit() {
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ofApp::open_alembic(std::string filePath) {
	try {
		_archive = IArchive(Alembic::AbcCoreOgawa::ReadArchive(), filePath);
	}
	catch (std::exception &e) {
		printf("open alembic error: %s\n", e.what());
	}

	std::string error_message;
	if (_storage.open(filePath, error_message) == false) {
		printf("AlembicStorage::open error: %s\n", error_message.c_str());
	}
}

//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::draw() {
	static int sample_index = 0;
	static bool hide_ui = false;
	static bool hide_model = false;

	static bool camera_sync = false;

	if (camera_sync && _scene)
	{
		for (auto o : _scene->objects) {
			if (o->visible == false) {
				continue;
			}

			if (auto camera = o.as_camera()) {
				ofSetWindowShape(camera->resolution_x, camera->resolution_y);
				_camera.setPosition(camera->eye.x, camera->eye.y, camera->eye.z);
				_camera.lookAt(
					glm::vec3(camera->lookat.x, camera->lookat.y, camera->lookat.z),
					glm::vec3(camera->up.x, camera->up.y, camera->up.z)
				);
				_camera.setFov(camera->fov_vertical_degree);
				break;
			}
		}
	}

	ofEnableDepthTest();

	ofClear(0);

	_camera.begin();
	ofPushMatrix();
	ofRotateZDeg(90.0f);
	ofSetColor(64);
	ofDrawGridPlane(1.0f);
	ofPopMatrix();

	ofDrawAxis(50);

	ofSetColor(255);

	if (_storage.isOpened()) {
		std::string error_message;

		_scene = _storage.read(sample_index, error_message);
		if (!_scene) {
			printf("sample error_message: %s\n", error_message.c_str());
		}
	}

	if (_scene && hide_model == false) {
		drawAlembicScene(_scene.get(), _camera_model, camera_sync == false /*draw camera*/);
	}

	_camera.end();

	ofDisableDepthTest();
	ofSetColor(255);

	// Begin ImGui
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// camera control                                          for control clicked problem
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && ImGui::IsAnyMouseDown())) {
		_camera.disableMouseInput();
	}
	else {
		_camera.enableMouseInput();
	}

	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Appearing);
	ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Appearing);
	ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
	ImGui::SetNextWindowBgAlpha(0.5f);

	ImGui::Begin("settings", nullptr);
	ImGui::Text("fps: %.2f", ofGetFrameRate());
	ImGui::InputInt("Sample Index", &sample_index, 1);
	ImGui::SliderInt("Sample Index Slider", &sample_index, 0, 255);

	ImGui::Checkbox("hide ui", &hide_ui);
	ImGui::Checkbox("hide model", &hide_model);
	
	ImGui::Spacing();
	ImGui::Checkbox("camera sync", &camera_sync);

	if (hide_ui == false) {
		if (ImGui::BeginTabBar("Alembic", ImGuiTabBarFlags_None))
		{
			if (_archive) {
				if (ImGui::BeginTabItem("Alembic Raw Hierarchy"))
				{
					show_alembic_hierarchy(_archive.getTop(), sample_index);
					ImGui::EndTabItem();
				}
			}
			if (_scene) {
				if (ImGui::BeginTabItem("Geometry Spread Sheet"))
				{
					ImGui::Text("Frame Count: %d", _storage.frameCount());
					show_houdini_alembic(_scene);
					ImGui::EndTabItem();
				}
			}
			if (ImGui::BeginTabItem("Unit Test"))
			{
				if (ImGui::Button("Run", ImVec2(100, 30))) {
					run_unit_test();
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}

	ImGui::End();

	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

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
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
	if (dragInfo.files.empty()) {
		return;
	}

	open_alembic(dragInfo.files[0]);
}
