#pragma once

#include <vector>
#include <array>
#include <memory>
#include <map>
#include <sstream>
#include <functional>

namespace houdini_alembic {
	//class Vector2f : public IStringConvertible {
	//public:
	//	Vector2f() {}
	//	Vector2f(float x_, float y_) : x(x_), y(y_) {
	//	}
	//	float x = 0.0f;
	//	float y = 0.0f;

	//	int snprint(char *buffer, uint32_t buffersize) const override {
	//		return snprintf(buffer, buffersize, "(%f, %f)", x, y);
	//	}
	//};

	class Vector3f {
	public:
		Vector3f() {}
		Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {

		}
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};
	//class Vector4f : public IStringConvertible {
	//public:
	//	Vector4f() {}
	//	Vector4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {
	//	}
	//	float x = 0.0f;
	//	float y = 0.0f;
	//	float z = 0.0f;
	//	float w = 0.0f;

	//	int snprint(char *buffer, uint32_t buffersize) const override {
	//		return snprintf(buffer, buffersize, "(%f, %f, %f, %f)", x, y, z, w);
	//	}
	//};

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

	enum AttributeType {
		AttributeType_Int = 0,
		AttributeType_Float,
		AttributeType_Vector2,
		AttributeType_Vector3,
		AttributeType_Vector4,
		AttributeType_String,
	};
	inline const char *attributeTypeString(AttributeType type) {
		static const char *types[] = {
			"Int",
			"Float",
			"Vector2",
			"Vector3",
			"Vector4",
			"String",
		};
		return types[type];
	}

	class AttributeColumn {
	public:
		AttributeColumn() {}
		AttributeColumn(const AttributeColumn &) = delete;
		void operator=(const AttributeColumn &) = delete;

		virtual ~AttributeColumn() {}
		virtual AttributeType attributeType() const = 0;
		virtual uint32_t rowCount() const = 0;

		virtual int snprint(uint32_t index, char *buffer, uint32_t buffersize) const = 0;
	};

	class AttributeFloatColumn : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_Float;
		}
		virtual float get(uint32_t index) const = 0;
	};
	class AttributeIntColumn : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_Int;
		}
		virtual int32_t get(uint32_t index) const = 0;
	};

	class AttributeVector2Column : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_Vector2;
		}
		virtual void get(uint32_t index, float *xy) const = 0;
		virtual void get(uint32_t index, double *xy) const = 0;
	};
	class AttributeVector3Column : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_Vector3;
		}
		virtual void get(uint32_t index, float *xyz) const = 0;
		virtual void get(uint32_t index, double *xyz) const = 0;
	};
	class AttributeVector4Column : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_Vector4;
		}
		virtual void get(uint32_t index, float *xyzw) const = 0;
		virtual void get(uint32_t index, double *xyzw) const = 0;
	};

	class AttributeStringColumn : public AttributeColumn {
	public:
		AttributeType attributeType() const override {
			return AttributeType_String;
		}
		virtual const std::string &get(uint32_t index) const = 0;
	};

	class AttributeSpreadSheet {
		template <class T>
		const T *column_as(const char *key) const {
			return dynamic_cast<const T *>(column(key));
		}
	public:
		const AttributeStringColumn *column_as_string(const char *key) const {
			return column_as<AttributeStringColumn>(key);
		}
		const AttributeFloatColumn *column_as_float(const char *key) const {
			return column_as<AttributeFloatColumn>(key);
		}
		const AttributeIntColumn *column_as_int(const char *key) const {
			return column_as<AttributeIntColumn>(key);
		}
		const AttributeVector2Column *column_as_vector2(const char *key) const {
			return column_as<AttributeVector2Column>(key);
		}
		const AttributeVector3Column *column_as_vector3(const char *key) const {
			return column_as<AttributeVector3Column>(key);
		}
		const AttributeVector4Column *column_as_vector4(const char *key) const {
			return column_as<AttributeVector4Column>(key);
		}

		uint32_t rowCount() const {
			if (sheet.empty()) {
				return 0;
			}
			return sheet[0].column->rowCount();
		}
		uint32_t columnCount() const {
			return (uint32_t)sheet.size();
		}

		const AttributeColumn *column(const char *key) const {
			auto it = std::lower_bound(sheet.begin(), sheet.end(), key, [](const char *a, const char *b) { 
				return strcmp(a, b) < 0; 
			});
			if (it == sheet.end()) {
				return nullptr;
			}
			if (strcmp(it->key.c_str(), key) != 0) {
				return nullptr;
			}
			return it->column.get();
		}

		struct Attribute {
			Attribute() {}
			Attribute(std::string k, std::shared_ptr<AttributeColumn> c) : key(k), column(c) {}
			std::string key;
			std::shared_ptr<AttributeColumn> column;

			operator const char *() const {
				return key.c_str();
			}
			bool operator<(const Attribute &rhs) const {
				return key < rhs.key;
			}
		};
		std::vector<Attribute> sheet;
	};

	enum SceneObjectType {
		SceneObjectType_PolygonMesh,
		SceneObjectType_Point,
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

		std::vector<uint32_t> faceCounts;
		std::vector<uint32_t> indices;
		std::vector<Vector3f> P;

		AttributeSpreadSheet points;
		AttributeSpreadSheet vertices;
		AttributeSpreadSheet primitives;
	};
	class PointObject : public SceneObject {
	public:
		SceneObjectType type() const override {
			return SceneObjectType_Point;
		}

		std::vector<uint64_t> pointIds;
		std::vector<Vector3f> P;

		AttributeSpreadSheet points;
	};
	
	/*
	Parameter descriptions
	https://docs.google.com/presentation/d/1f5EVQTul15x4Q30IbeA7hP9_Xc0AgDnWsOacSQmnNT8/edit?usp=sharing
	*/
	class CameraObject : public SceneObject {
	public:
		SceneObjectType type() const override {
			return SceneObjectType_Camera;
		}
		Vector3f eye;
		Vector3f lookat;
		Vector3f up;
		Vector3f down;
		Vector3f forward;
		Vector3f back;
		Vector3f left;
		Vector3f right;

		/*
		 Houdini Parameters [ View ]
		*/
		int resolution_x = 0; /* Resolution x (in pixels) */
		int resolution_y = 0; /* Resolution y (in pixels) */
		float focalLength_mm = 50.0f; /* Focal Length (in millimeter) */
		float aperture_horizontal_mm = 41.4214; /* Aperture (in millimeter) */
		float aperture_vertical_mm = 0;         /* Aperture (in millimeter) */
		float nearClip = 0.001f;  /* Near Clipping (in meter) */
		float farClip = 10000.0f; /* Far  Clipping (in meter) */

		/*
		 Houdini Parameters [ Sampling ]
		*/
		float focusDistance = 0.0f; /* Focus Distance (in meter) */
		float f_stop = 5.6f;        /* F-Stop */

		/*
		 Calculated by Parameters
		*/
		float fov_horizontal_degree = 45.0f; /* fov (in degree) */
		float fov_vertical_degree = 45.0f;   /* fov (in degree) */
		float lensRadius = 0.0f; /* fov (in meter) */
		float objectPlaneWidth = 0.0f;  /* object plane width  (in meter) */
		float objectPlaneHeight = 0.0f; /* object plane height (in meter) */
	};

	class SceneObjectPointer {
	public:
		SceneObjectPointer(std::shared_ptr<SceneObject> p) {
			_pointer = p;
		}
		PolygonMeshObject *as_polygonMesh() const {
			return dynamic_cast<PolygonMeshObject *>(_pointer.get());
		}
		PointObject *as_point() const {
			return dynamic_cast<PointObject *>(_pointer.get());
		}
		CameraObject *as_camera() const {
			return dynamic_cast<CameraObject *>(_pointer.get());
		}
		SceneObject *operator->() const {
			return _pointer.get();
		}
	private:
		std::shared_ptr<SceneObject> _pointer;
	};

	class AlembicScene {
	public:
		std::vector<SceneObjectPointer> objects;
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