#include "houdini_alembic.hpp"

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>

namespace houdini_alembic {
	using namespace Alembic::Abc;
	using namespace Alembic::AbcGeom;

	inline IObject top_of_archive(std::shared_ptr<void> p) {
		IArchive *archive = static_cast<IArchive *>(p.get());
		return IObject(*archive, kTop);
	}

	inline Matrix4x4f to(M44d m) {
		Matrix4x4f r;
		for (int i = 0; i < 16; ++i) {
			r.value[i] = (float)m.getValue()[i];
		}
		return r;
	}
	inline Vector3f to(V3d v) {
		return Vector3f(v.x, v.y, v.z);
	}

	/*
		M44dの仕様で行列の作用する順序は左から右のようだ。
		したがって、
		local to world = xforms[3] * xforms[2] * xforms[1] * xforms[0]
		という順序で掛ける
	*/
	inline M44d combine_xform(const std::vector<M44d> &xforms) {
		M44d m;
		for (int i = 0; i < xforms.size(); ++i) {
			m = xforms[i] * m;
		}
		return m;
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

	inline int getArrayExtent(const MetaData &meta)
	{
		std::string extent_s = meta.get("arrayExtent");
		return (extent_s == "") ? 1 : atoi(extent_s.c_str());
	}

	class AttributeStraightforwardVector2Column : public AttributeVector2Column {
	public:
		enum {
			DIMENSIONS = 2
		};
		void get(uint32_t index, float *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = p[src_index + i];
			}
		}
		virtual void get(uint32_t index, double *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = (double)p[src_index + i];
			}
		}
		uint32_t rowCount() const override {
			return _size;
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			float xs[DIMENSIONS];
			get(index, xs);
			return snprintf(buffer, buffersize, "(%f, %f)", xs[0], xs[1]);
		}
		uint32_t _size = 0;
		FloatArraySamplePtr _floats;
	};
	class AttributeStraightforwardVector3Column : public AttributeVector3Column {
	public:
		enum {
			DIMENSIONS = 3
		};
		void get(uint32_t index, float *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = p[src_index + i];
			}
		}
		virtual void get(uint32_t index, double *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = (double)p[src_index + i];
			}
		}
		uint32_t rowCount() const override {
			return _size;
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			float xs[DIMENSIONS];
			get(index, xs);
			return snprintf(buffer, buffersize, "(%f, %f, %f)", xs[0], xs[1], xs[2]);
		}
		uint32_t _size = 0;
		FloatArraySamplePtr _floats;
	};
	class AttributeStraightforwardVector4Column : public AttributeVector4Column {
	public:
		enum {
			DIMENSIONS = 4
		};
		void get(uint32_t index, float *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = p[src_index + i];
			}
		}
		virtual void get(uint32_t index, double *xs) const override {
			const float *p = _floats->get();
			uint32_t src_index = index * DIMENSIONS;
			for (int i = 0; i < DIMENSIONS; ++i) {
				xs[i] = (double)p[src_index + i];
			}
		}
		uint32_t rowCount() const override {
			return _size;
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			float xs[DIMENSIONS];
			get(index, xs);
			return snprintf(buffer, buffersize, "(%f, %f, %f, %f)", xs[0], xs[1], xs[2], xs[3]);
		}
		uint32_t _size = 0;
		FloatArraySamplePtr _floats;
	};

	class AttributeIndexedVector2Column : public AttributeVector2Column {
	public:
		void get(uint32_t index, float *xs) const override {
			const V2f *vector2 = _indexed_vector2->get();
			const uint32_t *indices = _indices->get();
			const V2f &value = vector2[indices[index]];
			xs[0] = value.x;
			xs[1] = value.y;
		}
		virtual void get(uint32_t index, double *xs) const override {
			const V2f *vector2 = _indexed_vector2->get();
			const uint32_t *indices = _indices->get();
			const V2f &value = vector2[indices[index]];
			xs[0] = value.x;
			xs[1] = value.y;
		}
		uint32_t rowCount() const override {
			return (uint32_t)_indices->size();
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			float xs[2];
			get(index, xs);
			return snprintf(buffer, buffersize, "(%f, %f)", xs[0], xs[1]);
		}
		UInt32ArraySamplePtr _indices;
		V2fArraySamplePtr _indexed_vector2;
	};
	class AttributeStraightforwardFloatColumn : public AttributeFloatColumn {
	public:
		float get(uint32_t index) const override {
			return _floats->get()[index];
		}
		uint32_t rowCount() const override {
			return (uint32_t)_floats->size();
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			return snprintf(buffer, buffersize, "%f", get(index));
		}
		FloatArraySamplePtr _floats;
	};
	class AttributeStraightforwardIntColumn : public AttributeIntColumn {
	public:
		int32_t get(uint32_t index) const override {
			return _ints->get()[index];
		}
		uint32_t rowCount() const override {
			return (uint32_t)_ints->size();
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			return snprintf(buffer, buffersize, "%d", get(index));
		}
		Int32ArraySamplePtr _ints;
	};
	class AttributeStraightforwardStringColumn : public AttributeStringColumn {
	public:
		const std::string &get(uint32_t index) const override {
			const std::string *strings = _strings->get();
			return strings[index];
		}
		uint32_t rowCount() const override {
			return (uint32_t)_strings->size();
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			const std::string &value = get(index);
			return snprintf(buffer, buffersize, "%s", value.c_str());
		}
		StringArraySamplePtr _strings;
	};

	class AttributeIndexedStringColumn : public AttributeStringColumn {
	public:
		const std::string &get(uint32_t index) const override {
			const std::string *strings = _indexed_strings->get();
			const uint32_t *indices = _indices->get();
			return strings[indices[index]];
		}
		uint32_t rowCount() const override {
			return (uint32_t)_indices->size();
		}
		int snprint(uint32_t index, char *buffer, uint32_t buffersize) const override {
			const std::string &value = get(index);
			return snprintf(buffer, buffersize, "%s", value.c_str());
		}
		UInt32ArraySamplePtr _indices;
		StringArraySamplePtr _indexed_strings;
	};

	inline bool parse_attributes(ICompoundProperty parent, const std::string &key, ISampleSelector selector, std::shared_ptr<AttributeColumn> &attributeColumn, std::string &geoScope) {
		auto header = parent.getPropertyHeader(key);
		auto metaData = header->getMetaData();
		for (auto meta : metaData) {
			if (meta.first == "geoScope") {
				geoScope = meta.second;
			}
		}

		if (header->isCompound() && metaData.get("podName") == "string") {
			ICompoundProperty string_compound(parent, key);

			auto attributes = std::shared_ptr<AttributeIndexedStringColumn>(new AttributeIndexedStringColumn());
			attributes->_indexed_strings = get_typed_array_property<IStringArrayProperty>(string_compound, ".vals", selector);
			attributes->_indices = get_typed_array_property<IUInt32ArrayProperty>(string_compound, ".indices", selector);
			attributeColumn = attributes;

			return true;
		} 
		else if (header->isCompound() && metaData.get("podName") == "float32_t" && metaData.get("podExtent") == "2") {
			ICompoundProperty string_compound(parent, key);

			auto attributes = std::shared_ptr<AttributeIndexedVector2Column>(new AttributeIndexedVector2Column());
			attributes->_indices = get_typed_array_property<IUInt32ArrayProperty>(string_compound, ".indices", selector);
			attributes->_indexed_vector2 = get_typed_array_property<IV2fArrayProperty>(string_compound, ".vals", selector);
			
			attributeColumn = attributes;
			return true;
		}
		else if (IV2fArrayProperty::matches(*header)) {
			V2fArraySamplePtr value = get_typed_array_property<IV2fArrayProperty>(parent, key, selector);
			auto value_ptr = value->get();
		}
		else if (IFloatArrayProperty::matches(*header, kNoMatching)) {
			// float, vector2, vector3, vector4 handling
			FloatArraySamplePtr value = get_typed_array_property<IFloatArrayProperty>(parent, key, selector);
			uint8_t extent = header->getDataType().getExtent();
			int arrayExtent = getArrayExtent(metaData);

			int compornent_size = extent * arrayExtent;
			int size = 0;

			if (1 < arrayExtent) {
				size = value->size() / arrayExtent;
			}
			else {
				size = value->size();
			}
			switch (compornent_size)
			{
			case 1: {
				auto attributes = std::shared_ptr<AttributeStraightforwardFloatColumn>(new AttributeStraightforwardFloatColumn());
				attributes->_floats = value;
				attributeColumn = attributes;
				return true;
			}
			case 2: {
				auto attributes = std::shared_ptr<AttributeStraightforwardVector2Column>(new AttributeStraightforwardVector2Column());
				attributes->_floats = value;
				attributes->_size = size;
				attributeColumn = attributes;
				return true;
			}
			case 3: {
				auto attributes = std::shared_ptr<AttributeStraightforwardVector3Column>(new AttributeStraightforwardVector3Column());
				attributes->_floats = value;
				attributes->_size = size;
				attributeColumn = attributes;
				return true;
			}
			case 4: {
				auto attributes = std::shared_ptr<AttributeStraightforwardVector4Column>(new AttributeStraightforwardVector4Column());
				attributes->_floats = value;
				attributes->_size = size;
				attributeColumn = attributes;
				return true;
			}
			default:
				break;
			}
		}
		else if (IInt32ArrayProperty::matches(*header)) {
			auto attributes = std::shared_ptr<AttributeStraightforwardIntColumn>(new AttributeStraightforwardIntColumn());
			attributes->_ints = get_typed_array_property<IInt32ArrayProperty>(parent, key, selector);
			attributeColumn = attributes;
			return true;
		}
		else if (IStringArrayProperty::matches(*header)) {
			auto attributes = std::shared_ptr<AttributeStraightforwardStringColumn>(new AttributeStraightforwardStringColumn());
			attributes->_strings = get_typed_array_property<IStringArrayProperty>(parent, key, selector);
			attributeColumn = attributes;
			return true;
		}
		
		// unsupported type
		return false;
	}

	static void parse_attributes(
		AttributeSpreadSheet *points, AttributeSpreadSheet *vertices, AttributeSpreadSheet *primitives,
		ICompoundProperty compound_prop, ISampleSelector selector
	) {
		for (int i = 0; i < compound_prop.getNumProperties(); ++i) {
			auto child_header = compound_prop.getPropertyHeader(i);
			auto key = child_header.getName();
			if (key.size() == 0) {
				continue;
			}
			if (key[0] == '.') {
				continue;
			}

			std::shared_ptr<AttributeColumn> attributes;
			std::string geoScope;
			if (parse_attributes(compound_prop, key, selector, attributes, geoScope)) {
				if (points && geoScope == "var" || geoScope == "vtx") {
					points->sheet.emplace_back(key, attributes);
				}
				else if (vertices && geoScope == "fvr") {
					vertices->sheet.emplace_back(key, attributes);
				}
				else if (primitives && geoScope == "uni") {
					primitives->sheet.emplace_back(key, attributes);
				}
			}
		}
		
		if (points) {
			std::sort(points->sheet.begin(), points->sheet.end());
		}
		if (vertices) {
			std::sort(vertices->sheet.begin(), vertices->sheet.end());
		}
		if (primitives) {
			std::sort(primitives->sheet.begin(), primitives->sheet.end());
		}
	}

	inline void parse_polymesh(IPolyMesh polyMesh, std::shared_ptr<PolygonMeshObject> polymeshObject, ISampleSelector selector) {
		auto schema = polyMesh.getSchema();
		IPolyMeshSchema::Sample sample;
		schema.get(sample, selector);
		
		Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
		polymeshObject->faceCounts = std::vector<uint32_t>(faceCounts->get(), faceCounts->get() + faceCounts->size());

		Int32ArraySamplePtr indices = sample.getFaceIndices();
		polymeshObject->indices = std::vector<uint32_t>(indices->get(), indices->get() + indices->size());

		parse_attributes(
			&polymeshObject->points, &polymeshObject->vertices, &polymeshObject->primitives,
			schema.getArbGeomParams(), selector
		);
		parse_attributes(
			&polymeshObject->points, &polymeshObject->vertices, &polymeshObject->primitives,
			ICompoundProperty(polyMesh.getProperties(), ".geom"), selector
		);

		// Pは流石に登場頻度が高いので予め入れておく
		auto p = polymeshObject->points.column_as_vector3("P");
		polymeshObject->P.resize(p->rowCount());
		for (int i = 0; i < polymeshObject->P.size() ; ++i) {
			float *xyz = (float *)&polymeshObject->P[i];
			p->get(i, xyz);
		}
	}

	inline void parse_points(IPoints points, std::shared_ptr<PointObject> pointObject, ISampleSelector selector) {
		auto schema = points.getSchema();
		IPointsSchema::Sample sample;
		schema.get(sample, selector);

		auto pointIds = sample.getIds();
		pointObject->pointIds = std::vector<uint64_t>(pointIds->get(), pointIds->get() + pointIds->size());
		
		parse_attributes(
			&pointObject->points, nullptr, nullptr,
			schema.getArbGeomParams(), selector
		);
		parse_attributes(
			&pointObject->points, nullptr, nullptr,
			ICompoundProperty(points.getProperties(), ".geom"), selector
		);

		// Pは流石に登場頻度が高いので予め入れておく
		auto p = pointObject->points.column_as_vector3("P");
		pointObject->P.resize(p->rowCount());
		for (int i = 0; i < pointObject->P.size(); ++i) {
			float *xyz = (float *)&pointObject->P[i];
			p->get(i, xyz);
		}
	}

	static void parse_common_property(IObject o, SceneObject *object, const std::vector<M44d> &xforms, ISampleSelector selector) {
		IXform parentXForm(o.getParent());
		object->name = parentXForm.getFullName();

		for (int i = 0; i < xforms.size(); ++i) {
			object->xforms.push_back(to(xforms[i]));
		}
		object->combinedXforms = to(combine_xform(xforms));

		// http://www.sidefx.com/ja/docs/houdini/io/alembic.html#%E5%8F%AF%E8%A6%96%E6%80%A7
		auto parentProp = parentXForm.getProperties();
		if (parentProp.getPropertyHeader("visible")) {
			int8_t visible = get_typed_scalar_property<ICharProperty>(parentProp, "visible", selector);
			object->visible = visible == -1;
		}
		else {
			object->visible = true;
		}
	}

	static void parse_object(IObject o, ISampleSelector selector, std::vector<M44d> xforms, std::vector<SceneObjectPointer> &objects) {
		auto header = o.getHeader();
		std::string fullname = header.getFullName();

		if (IPolyMesh::matches(header)) {
			IPolyMesh polyMesh(o);
			std::shared_ptr<PolygonMeshObject> object(new PolygonMeshObject());

			parse_common_property(o, object.get(), xforms, selector);
			parse_polymesh(polyMesh, object, selector);

			objects.emplace_back(object);
		}
		else if (IPoints::matches(header)) {
			IPoints points(o);
			std::shared_ptr<PointObject> object(new PointObject());

			parse_common_property(o, object.get(), xforms, selector);
			parse_points(points, object, selector);

			objects.emplace_back(object);
		}
		else if (ICamera::matches(header)) {
			// Implementation Notes
			https://docs.google.com/presentation/d/1f5EVQTul15x4Q30IbeA7hP9_Xc0AgDnWsOacSQmnNT8/edit?usp=sharing

			ICamera camera(o);
			auto schema = camera.getSchema();

			std::shared_ptr<CameraObject> object(new CameraObject());

			IXform parentXForm(o.getParent());
			object->name = parentXForm.getFullName();

			for (int i = 0; i < xforms.size(); ++i) {
				object->xforms.push_back(to(xforms[i]));
			}
			M44d combined = combine_xform(xforms);
			object->combinedXforms = to(combined);

			// http://www.sidefx.com/ja/docs/houdini/io/alembic.html#%E5%8F%AF%E8%A6%96%E6%80%A7
			auto parentProp = parentXForm.getProperties();
			if (parentProp.getPropertyHeader("visible")) {
				int8_t visible = get_typed_scalar_property<ICharProperty>(parentProp, "visible", selector);
				object->visible = visible == -1;
			}
			else {
				object->visible = true;
			}

			M44d inverseTransposed = combined.inverse().transposed();

			V3d eye;
			combined.multVecMatrix(V3d(0, 0, 0), eye);
			V3d forward;
			V3d up;
			inverseTransposed.multDirMatrix(V3d(0, 0, -1), forward);
			inverseTransposed.multDirMatrix(V3d(0, 1, 0), up);

			V3d right;
			inverseTransposed.multDirMatrix(V3d(1, 0, 0), right);

			object->eye = to(eye);
			object->lookat = to(eye + forward);
			object->up = to(up);
			object->down = to(-up);
			object->forward = to(forward);
			object->back = to(-forward);
			object->left = to(-right);
			object->right = to(right);

			CameraSample sample;
			schema.get(sample, selector);

			// Houdini Parameters [ View ]
			object->resolution_x = (int)get_typed_scalar_property<IFloatProperty>(schema.getUserProperties(), "resx", selector);
			object->resolution_y = (int)get_typed_scalar_property<IFloatProperty>(schema.getUserProperties(), "resy", selector);
			object->focalLength_mm = sample.getFocalLength();
			object->aperture_horizontal_mm = sample.getHorizontalAperture() * 10.0f;
			object->aperture_vertical_mm = sample.getVerticalAperture() * 10.0f;
			object->nearClip = sample.getNearClippingPlane();
			object->farClip = sample.getFarClippingPlane();

			// Houdini Parameters [ Sampling ]
			object->focusDistance = sample.getFocusDistance();
			object->f_stop = sample.getFStop();

			// Calculated by Parameters
			object->fov_horizontal_degree = sample.getFieldOfView();
			float fov_vertical_radian = std::atan(object->aperture_vertical_mm * 0.5f / object->focalLength_mm) * 2.0f;
			object->fov_vertical_degree = fov_vertical_radian / (2.0 * M_PI) * 360.0f;
			
			float A = object->focalLength_mm / 1000.0f;
			float B = object->focusDistance;
			float F = A * B / (A + B);
			object->lensRadius = F / (2.0f * object->f_stop);

			object->objectPlaneWidth  = 2.0f * object->focusDistance * std::tan(0.5f * object->fov_horizontal_degree / 360.0f * 2.0 * M_PI);
			object->objectPlaneHeight = 2.0f * object->focusDistance * std::tan(0.5f * object->fov_vertical_degree   / 360.0f * 2.0 * M_PI);

			objects.emplace_back(object);
		}
		else if (IXform::matches(header)) {
			IXform xform(o);
			auto schema = xform.getSchema();
			auto schemaValue = schema.getValue(selector);
			M44d matrix = schemaValue.getMatrix();
			xforms.push_back(matrix);

			for (int i = 0; i < o.getNumChildren(); ++i) {
				IObject child = o.getChild(i);
				parse_object(child, selector, xforms, objects);
			}
		}
		else {
			for (int i = 0; i < o.getNumChildren(); ++i) {
				IObject child = o.getChild(i);
				parse_object(child, selector, xforms, objects);
			}
		}
	}
	static void parse_object(IObject o, ISampleSelector selector, std::vector<SceneObjectPointer> &objects) {
		parse_object(o, selector, std::vector<M44d>(), objects);
	}

	bool AlembicStorage::open(const std::string &filePath, std::string &error_message) {
		try {
			_alembicArchive = std::shared_ptr<void>();
			_alembicArchive = std::shared_ptr<void>(new IArchive(Alembic::AbcCoreOgawa::ReadArchive(), filePath), [](void *p) {
				IArchive *archive = static_cast<IArchive *>(p);
				delete archive;
			});

			auto top = top_of_archive(_alembicArchive);
			auto prop = top.getProperties();
			const char *kSample = "1.samples";
			if (prop.getPropertyHeader(kSample)) {
				IUInt32Property samples(top.getProperties(), kSample);
				samples.get(_frameCount);
			}
			else {
				_frameCount = 1;
			}
		}
		catch (std::exception &e) {
			error_message = e.what();
			return false;
		}
		return true;
	}
	bool AlembicStorage::isOpened() const {
		return (bool)_alembicArchive;
	}
	std::shared_ptr<AlembicScene> AlembicStorage::read(uint32_t index, std::string &error_message) const {
		if (!_alembicArchive) {
			return std::shared_ptr<AlembicScene>();
		}
		try {
			ISampleSelector selector((index_t)index);
			std::shared_ptr<AlembicScene> scene(new AlembicScene());
			parse_object(top_of_archive(_alembicArchive), selector, scene->objects);
			return scene;
		}
		catch (std::exception &e) {
			error_message = e.what();
			return std::shared_ptr<AlembicScene>();
		}
		return std::shared_ptr<AlembicScene>();
	}
}