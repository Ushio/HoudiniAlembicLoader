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
			r.value[i] = m.getValue()[i];
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
			StringArraySamplePtr values = get_typed_array_property<IStringArrayProperty>(string_compound, ".vals", selector);
			auto values_count = values->size();
			auto values_ptr = values->get();

			UInt32ArraySamplePtr indices = get_typed_array_property<IUInt32ArrayProperty>(string_compound, ".indices", selector);
			auto indices_count = indices->size();
			auto indices_ptr = indices->get();

			std::vector<AttributeString> string_source;
			string_source.reserve(values_count);
			for (int i = 0; i < values_count; ++i) {
				string_source.emplace_back(values_ptr[i]);
			}

			auto attributes = std::shared_ptr<AttributeStringColumn>(new AttributeStringColumn());
			attributes->rows.reserve(indices_count);
			for (int i = 0; i < indices_count; ++i) {
				auto index = indices_ptr[i];
				attributes->rows.emplace_back(string_source[index]);
			}
			attributeColumn = attributes;
			return true;
		} 
		else if (header->isCompound() && metaData.get("podName") == "float32_t" && metaData.get("podExtent") == "2") {
			ICompoundProperty string_compound(parent, key);
			V2fArraySamplePtr values = get_typed_array_property<IV2fArrayProperty>(string_compound, ".vals", selector);
			auto values_ptr = values->get();

			UInt32ArraySamplePtr indices = get_typed_array_property<IUInt32ArrayProperty>(string_compound, ".indices", selector);
			auto indices_count = indices->size();
			auto indices_ptr = indices->get();

			auto attributes = std::shared_ptr<AttributeVector2Column>(new AttributeVector2Column());
			attributes->rows.reserve(indices_count);
			for (int i = 0; i < indices_count; ++i) {
				auto index = indices_ptr[i];
				auto value = values_ptr[index];
				attributes->rows.emplace_back(value.x, value.y);
			}
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
			auto value_size = value->size();
			auto value_ptr = value->get();
			uint8_t extent = header->getDataType().getExtent();

			if (extent == 1) {
				int arrayExtent = getArrayExtent(metaData);
				if (value_size % arrayExtent != 0) {
					throw std::runtime_error("invalid arrayExtent");
				}
				if (arrayExtent == 1) {
					auto attributes = std::shared_ptr<AttributeFloatColumn>(new AttributeFloatColumn());
					attributes->rows.reserve(value_size);
					for (int i = 0; i < value_size; ++i) {
						auto v = value_ptr[i];
						attributes->rows.emplace_back(v);
					}
					attributeColumn = attributes;
					return true;
				}
				else if (arrayExtent == 2) {
					auto attributes = std::shared_ptr<AttributeVector2Column>(new AttributeVector2Column());
					auto n = value_size / arrayExtent;
					attributes->rows.reserve(n);
					for (int i = 0; i < value_size; i += arrayExtent) {
						attributes->rows.emplace_back(value_ptr[i], value_ptr[i + 1]);
					}
					attributeColumn = attributes;
					return true;
				}
				else if (arrayExtent == 3) {
					auto attributes = std::shared_ptr<AttributeVector3Column>(new AttributeVector3Column());
					auto n = value_size / arrayExtent;
					attributes->rows.reserve(n);
					for (int i = 0; i < value_size; i += arrayExtent) {
						attributes->rows.emplace_back(value_ptr[i], value_ptr[i + 1], value_ptr[i + 2]);
					}
					attributeColumn = attributes;
					return true;
				}
				else if (arrayExtent == 4) {
					auto attributes = std::shared_ptr<AttributeVector4Column>(new AttributeVector4Column());
					auto n = value_size / arrayExtent;
					attributes->rows.reserve(n);
					for (int i = 0; i < value_size; i += arrayExtent) {
						attributes->rows.emplace_back(value_ptr[i], value_ptr[i + 1], value_ptr[i + 2], value_ptr[i + 3]);
					}
					attributeColumn = attributes;
					return true;
				}
			}
			else if (extent == 3) {
				auto attributes = std::shared_ptr<AttributeVector3Column>(new AttributeVector3Column());
				attributes->rows.reserve(value_size);
				for (int i = 0; i < value_size; ++i) {
					int index = i * extent;
					auto v0 = value_ptr[index];
					auto v1 = value_ptr[index + 1];
					auto v2 = value_ptr[index + 2];
					attributes->rows.emplace_back(v0, v1, v2);
				}
				attributeColumn = attributes;
				return true;
			}
		}
		else if (IInt32ArrayProperty::matches(*header)) {
			Int32ArraySamplePtr value = get_typed_array_property<IInt32ArrayProperty>(parent, key, selector);
			auto value_size = value->size();
			auto value_ptr = value->get();

			auto attributes = std::shared_ptr<AttributeIntColumn>(new AttributeIntColumn());
			attributes->rows.reserve(value_size);
			for (int i = 0; i < value_size; ++i) {
				auto v = value_ptr[i];
				attributes->rows.emplace_back(v);
			}
			attributeColumn = attributes;
			return true;
		}
		else if (IStringArrayProperty::matches(*header)) {
			StringArraySamplePtr value = get_typed_array_property<IStringArrayProperty>(parent, key, selector);
			auto value_size = value->size();
			auto value_ptr = value->get();

			auto attributes = std::shared_ptr<AttributeStringColumn>(new AttributeStringColumn());
			attributes->rows.reserve(value_size);
			for (int i = 0; i < value_size; ++i) {
				auto v = value_ptr[i];
				attributes->rows.emplace_back(v);
			}
			attributeColumn = attributes;
			return true;
		}
		
		// unsupported type
		return false;
	}

	inline void parse_polymesh(IPolyMesh polyMesh, std::shared_ptr<PolygonMeshObject> polymeshObject, ISampleSelector selector) {
		auto schema = polyMesh.getSchema();
		IPolyMeshSchema::Sample sample;
		schema.get(sample, selector);
		
		Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
		polymeshObject->faceCounts = std::vector<int32_t>(faceCounts->get(), faceCounts->get() + faceCounts->size());

		Int32ArraySamplePtr indices = sample.getFaceIndices();
		polymeshObject->indices = std::vector<int32_t>(indices->get(), indices->get() + indices->size());

		auto process_attributes = [polymeshObject, selector](ICompoundProperty compound_prop) {
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
					if (geoScope == "var" || geoScope == "vtx") {
						polymeshObject->points.sheet[key] = attributes;
					}
					else if (geoScope == "fvr") {
						polymeshObject->vertices.sheet[key] = attributes;
					}
					else if (geoScope == "uni") {
						polymeshObject->primitives.sheet[key] = attributes;
					}
				}
			}
		};

		// 
		process_attributes(ICompoundProperty(polyMesh.getProperties(), ".geom"));
		process_attributes(schema.getArbGeomParams());
	}

	static void parse_object(IObject o, ISampleSelector selector, std::vector<M44d> xforms, std::vector<std::shared_ptr<SceneObject>> &objects) {
		auto header = o.getHeader();
		std::string fullname = header.getFullName();

		if (IPolyMesh::matches(header)) {
			IPolyMesh polyMesh(o);
			std::shared_ptr<PolygonMeshObject> object(new PolygonMeshObject());

			IXform parentXForm(o.getParent());
			object->name = parentXForm.getFullName();

			for (int i = 0; i < xforms.size(); ++i) {
				object->xforms.push_back(to(xforms[i]));
			}
			object->combinedXforms = to(combine_xform(xforms));
			
			parse_polymesh(polyMesh, object, selector);

			// http://www.sidefx.com/ja/docs/houdini/io/alembic.html#%E5%8F%AF%E8%A6%96%E6%80%A7
			auto parentProp = parentXForm.getProperties();
			if (parentProp.getPropertyHeader("visible")) {
				int8_t visible = get_typed_scalar_property<ICharProperty>(parentProp, "visible", selector);
				object->visible = visible == -1;
			}
			else {
				object->visible = true;
			}

			objects.push_back(object);
		}
		else if (ICamera::matches(header)) {
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

			
			object->imageWidth = (int)get_typed_scalar_property<IFloatProperty>(schema.getUserProperties(), "resx", selector);
			object->imageHeight = (int)get_typed_scalar_property<IFloatProperty>(schema.getUserProperties(), "resy", selector);

			M44d inverseTransposed = combined.inverse().transposed();

			V3d eye;
			combined.multVecMatrix(V3d(0, 0, 0), eye);
			V3d front;
			V3d up;
			inverseTransposed.multDirMatrix(V3d(0, 0, -1), front);
			inverseTransposed.multDirMatrix(V3d(0, 1, 0), up);
			
			object->eye = to(eye);
			object->lookat = to(eye + front);
			object->up = to(up);

			objects.push_back(object);

			// to
			//ICamera cameraObj(o);
			//ICompoundProperty props = cameraObj.getProperties();
			//printProperties(props);

			//ICameraSchema schema = cameraObj.getSchema();
			//CameraSample sample;
			//schema.get(sample);
			//printf("-camera-\n");
			//printf("near: %f\n", sample.getNearClippingPlane());
			//printf("far: %f\n", sample.getFarClippingPlane());

			//printf("focal length: %f\n", sample.getFocalLength());
			//printf("horizontal aperture: %f\n", sample.getHorizontalAperture());
			//printf("-camera-\n");
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
	static void parse_object(IObject o, ISampleSelector selector, std::vector<std::shared_ptr<SceneObject>> &objects) {
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