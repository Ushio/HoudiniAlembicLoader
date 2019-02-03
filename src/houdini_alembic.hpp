#pragma once

#include <vector>
#include <array>
#include <memory>
#include <map>
#include <sstream>

namespace houdini_alembic {
	class IStringConvertible {
	public:
		virtual ~IStringConvertible() {}
		virtual void to_string(char *buffer, std::size_t buffersize) const = 0;
	};

	class Vector2f : public IStringConvertible {
	public:
		Vector2f() {}
		Vector2f(float x_, float y_) : x(x_), y(y_) {
		}
		float x = 0.0f;
		float y = 0.0f;

		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "(%f, %f)", x, y);
		}
	};

	class Vector3f : public IStringConvertible {
	public:
		Vector3f() {}
		Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {
		}
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "(%f, %f, %f)", x, y, z);
		}
	};
	class Vector4f : public IStringConvertible {
	public:
		Vector4f() {}
		Vector4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {
		}
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;

		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "(%f, %f, %f, %f)", x, y, z, w);
		}
	};

	class Matrix4x4f {
	public:
		float *value_ptr() {
			return value.data();
		}
		const float *value_ptr() const {
			return value.data();
		}

		/*
		colurm majar order
			0, 4, 8,  12,
			1, 5, 9,  13,
			2, 6, 10, 14,
			3, 7, 11, 15
		*/
		std::array<float, 16> value;
	};

	class AttributeFloat : public IStringConvertible {
	public:
		AttributeFloat() {

		}
		AttributeFloat(float v) :_value(v) {

		}
		operator float() const {
			return _value;
		}
		float value() const {
			return _value;
		}
		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "%f", _value);
		}
	private:
		float _value = 0.0f;
	};
	class AttributeInt : public IStringConvertible {
	public:
		AttributeInt() {

		}
		AttributeInt(int v) :_value(v) {

		}
		operator int() const {
			return _value;
		}
		int value() const {
			return _value;
		}
		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "%d", _value);
		}
	private:
		int _value = 0;
	};

	using AttributeVector2 = Vector2f;
	using AttributeVector3 = Vector3f;
	using AttributeVector4 = Vector4f;

	class AttributeString : public IStringConvertible {
	public:
		AttributeString() {

		}
		AttributeString(const char *v) :_value(new std::string(v)) {

		}
		AttributeString(const std::string &v) :_value(new std::string(v)) {

		}
		operator std::string() const {
			return *_value;
		}
		std::string value() const {
			return *_value;
		}
		void to_string(char *buffer, std::size_t buffersize) const override {
			snprintf(buffer, buffersize, "%s", _value->c_str());
		}
	private:
		std::shared_ptr<std::string> _value = std::shared_ptr<std::string>(new std::string());
	};

	class AttributeColumn {
	public:
		virtual ~AttributeColumn() {}
		virtual std::size_t rowCount() const = 0;

		template <class T>
		bool is() const {
			return typeid(*this) == typeid(T);
		}

		virtual void get_as_string(std::size_t index, char *buffer, std::size_t buffersize) const = 0;
	};
	template <class T>
	class AttributeColumnT : public AttributeColumn {
	public:
		T get(std::size_t index) const {
			return rows[index];
		}
		std::size_t rowCount() const override {
			return rows.size();
		}
		void get_as_string(std::size_t index, char *buffer, std::size_t buffersize) const {
			const IStringConvertible &convertible = rows[index];
			convertible.to_string(buffer, buffersize);
		}
		std::vector<T> rows;
	};

	using AttributeFloatColumn = AttributeColumnT<AttributeFloat>;
	using AttributeIntColumn = AttributeColumnT<AttributeInt>;
	using AttributeVector2Column = AttributeColumnT<AttributeVector2>;
	using AttributeVector3Column = AttributeColumnT<AttributeVector3>;
	using AttributeVector4Column = AttributeColumnT<AttributeVector4>;
	using AttributeStringColumn = AttributeColumnT<AttributeString>;

	class AttributeSpreadSheet {
	public:
		template <class TColumn>
		std::shared_ptr<TColumn> get_as(const char *key) const {
			auto it = sheet.find(key);
			if (it == sheet.end()) {
				throw std::runtime_error("[SpreadSheet] key not found.");
			}
			auto col = std::dynamic_pointer_cast<TColumn>(it->second);
			if (!col) {
				throw std::runtime_error("[SpreadSheet] value type mismatch");
			}
			return col;
		}
		/*
		get column by key. if the key don't exist, a exception will be thrown.
		*/
		std::shared_ptr<AttributeStringColumn> get_as_string(const char *key) const {
			return get_as<AttributeStringColumn>(key);
		}
		std::shared_ptr<AttributeFloatColumn> get_as_float(const char *key) const {
			return get_as<AttributeFloatColumn>(key);
		}
		std::shared_ptr<AttributeIntColumn> get_as_int(const char *key) const {
			return get_as<AttributeIntColumn>(key);
		}
		std::shared_ptr<AttributeVector2Column> get_as_vector2(const char *key) const {
			return get_as<AttributeVector2Column>(key);
		}
		std::shared_ptr<AttributeVector3Column> get_as_vector3(const char *key) const {
			return get_as<AttributeVector3Column>(key);
		}
		std::shared_ptr<AttributeVector4Column> get_as_vector4(const char *key) const {
			return get_as<AttributeVector4Column>(key);
		}

		/*
		get value by key and column. if the key don't exist, a exception will be thrown. No Range Check.
		*/
		AttributeString get_as_string(size_t index, const char *key) const {
			return get_as_string(key)->get(index);
		}
		AttributeFloat get_as_float(size_t index, const char *key) const {
			return get_as_float(key)->get(index);
		}
		AttributeInt get_as_int(size_t index, const char *key) const {
			return get_as_int(key)->get(index);
		}
		AttributeVector2 get_as_vector2(size_t index, const char *key) const {
			return get_as_vector2(key)->get(index);
		}
		AttributeVector3 get_as_vector3(size_t index, const char *key) const {
			return get_as_vector3(key)->get(index);
		}
		AttributeVector4 get_as_vector4(size_t index, const char *key) const {
			return get_as_vector4(key)->get(index);
		}

		std::size_t rowCount() const {
			if (sheet.empty()) {
				return 0;
			}
			return sheet.begin()->second->rowCount();
		}
		std::size_t columnCount() const {
			return sheet.size();
		}

		bool contains_key(const char *key) {
			return sheet.count(key) != 0;
		}
		std::map<std::string, std::shared_ptr<AttributeColumn>> sheet;
	};

	enum SceneObjectType {
		SceneObjectType_PolygonMesh,
		SceneObjectType_Camera,
	};
	class SceneObject {
	public:
		virtual ~SceneObject() {}
		virtual SceneObjectType type() const = 0;

		std::string name;

		bool visible = false;

		/*
		 local to world matrix
		*/
		Matrix4x4f combinedXforms;

		/*
		 local to world matrix

		 例）
		 (world point) == xforms[0] * xforms[1] * xforms[2] * (local point)
		 combinedXforms == xforms[0] * xforms[1] * xforms[2]
		*/
		std::vector<Matrix4x4f> xforms;
	};
	class PolygonMeshObject : public SceneObject {
	public:
		SceneObjectType type() const override {
			return SceneObjectType_PolygonMesh;
		}

		std::vector<int32_t> faceCounts;
		std::vector<int32_t> indices;

		AttributeSpreadSheet points;
		AttributeSpreadSheet vertices;
		AttributeSpreadSheet primitives;
	};
	class CameraObject : public SceneObject {
	public:
		SceneObjectType type() const override {
			return SceneObjectType_Camera;
		}
		Vector3f eye;
		Vector3f lookat;
		Vector3f up;
		int imageWidth = 0;
		int imageHeight = 0;


	};
	class AlembicScene {
	public:
		std::vector<std::shared_ptr<SceneObject>> objects;
	};

	class AlembicStorage {
	public:
		bool open(const std::string &filePath, std::string &error_message);
		bool isOpened() const;

		// return null if failed to sample.
		std::shared_ptr<AlembicScene> read(uint32_t index, std::string &error_message) const;

		uint32_t frameCount() const {
			return _frameCount;
		}
	private:
		uint32_t _frameCount = 0;
		std::shared_ptr<void> _alembicArchive;
	};
}