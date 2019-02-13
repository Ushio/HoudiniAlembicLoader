#pragma once

#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"
#include "ofMain.h"

#include "houdini_alembic.hpp"

void run_unit_test() {
	static Catch::Session session;
	session.run();
}

TEST_CASE("points_attributes.abc", "[points_attributes]") {
	using namespace houdini_alembic;

	AlembicStorage storage;
	std::string error_message;
	REQUIRE(storage.open(ofToDataPath("test_case/points_attributes.abc"), error_message));
	auto scene = storage.read(0, error_message);
	REQUIRE(scene);
	REQUIRE(scene->objects.size() == 1);

	auto point = scene->objects[0].as_point();
	REQUIRE(point->visible);

	const AttributeVector3Column *P = point->points.column_as_vector3("P");
	const AttributeVector3Column *N = point->points.column_as_vector3("N");
	REQUIRE(P);
	REQUIRE(N);
	REQUIRE(P->rowCount() == N->rowCount());

	for (int i = 0; i < P->rowCount(); ++i) {
		glm::vec3 p;
		P->get(i, glm::value_ptr(p));
		glm::vec3 n;
		N->get(i, glm::value_ptr(n));

		p = glm::normalize(p);

		float margin = 1.0e-4f;
		auto x = Approx(p.x).margin(margin);
		auto y = Approx(p.y).margin(margin);
		auto z = Approx(p.z).margin(margin);
		REQUIRE(n.x == x);
		REQUIRE(n.y == y);
		REQUIRE(n.z == z);
	}
}

TEST_CASE("polymesh_attributes.abc", "[polymesh_attributes]") {
	using namespace houdini_alembic;

	AlembicStorage storage;
	std::string error_message;
	REQUIRE(storage.open(ofToDataPath("test_case/polymesh_attributes.abc"), error_message));
	auto scene = storage.read(0, error_message);
	REQUIRE(scene);
	REQUIRE(scene->objects.size() == 1);

	auto polymesh = scene->objects[0].as_polygonMesh();
	REQUIRE(polymesh->visible);

	bool isTriangleMesh = std::all_of(polymesh->faceCounts.begin(), polymesh->faceCounts.end(), [](int32_t f) { return f == 3; });
	REQUIRE(isTriangleMesh);

	SECTION("Points") {
		const AttributeIntColumn *int_values = polymesh->points.column_as_int("int_points");
		const AttributeFloatColumn *float_values = polymesh->points.column_as_float("float_points");
		const AttributeVector2Column *vec2_values = polymesh->points.column_as_vector2("vec2_points");
		const AttributeVector3Column *vec3_values = polymesh->points.column_as_vector3("vec3_points");
		const AttributeVector4Column *vec4_values = polymesh->points.column_as_vector4("vec4_points");
		const AttributeStringColumn *string_values = polymesh->points.column_as_string("string_points");

		REQUIRE(int_values);
		REQUIRE(float_values);
		REQUIRE(vec2_values);
		REQUIRE(vec3_values);
		REQUIRE(vec4_values);
		REQUIRE(string_values);

		REQUIRE(float_values->rowCount() == int_values->rowCount());
		REQUIRE(vec2_values->rowCount() == int_values->rowCount());
		REQUIRE(vec3_values->rowCount() == int_values->rowCount());
		REQUIRE(vec4_values->rowCount() == int_values->rowCount());
		REQUIRE(string_values->rowCount() == int_values->rowCount());

		for (int i = 0, n = polymesh->points.rowCount(); i < n; ++i) {
			// e.g. REQUIRE(a == Approx(3.0).margin(0.1))
			//   2.9 < a < 3.1
			float margin = 1.0e-4f;
			auto x = Approx(i * 0.1f).margin(margin);
			auto y = Approx(i * 0.5f).margin(margin);
			auto z = Approx(i * 0.75f).margin(margin);
			auto w = Approx(i * 0.25f).margin(margin);

			REQUIRE(int_values->get(i) == i);
			REQUIRE(float_values->get(i) == x);

			{
				float vec2[2];
				vec2_values->get(i, vec2);
				REQUIRE(vec2[0] == x);
				REQUIRE(vec2[1] == y);
			}
			{
				float vec3[3];
				vec3_values->get(i, vec3);
				REQUIRE(vec3[0] == x);
				REQUIRE(vec3[1] == y);
				REQUIRE(vec3[2] == z);
			}
			{
				float vec4[4];
				vec4_values->get(i, vec4);
				REQUIRE(vec4[0] == x);
				REQUIRE(vec4[1] == y);
				REQUIRE(vec4[2] == z);
				REQUIRE(vec4[3] == w);
			}

			REQUIRE(string_values->get(i) == ("index = " + std::to_string(i)));
		}
	}

	SECTION("Vertices") {
		REQUIRE(polymesh->indices.size() == polymesh->vertices.rowCount());

		const AttributeIntColumn *int_values = polymesh->vertices.column_as_int("int_vertices");
		const AttributeFloatColumn *float_values = polymesh->vertices.column_as_float("float_vertices");
		const AttributeVector2Column *vec2_values = polymesh->vertices.column_as_vector2("vec2_vertices");
		const AttributeVector3Column *vec3_values = polymesh->vertices.column_as_vector3("vec3_vertices");
		const AttributeVector4Column *vec4_values = polymesh->vertices.column_as_vector4("vec4_vertices");
		const AttributeStringColumn *string_values = polymesh->vertices.column_as_string("string_vertices");

		REQUIRE(int_values);
		REQUIRE(float_values);
		REQUIRE(vec2_values);
		REQUIRE(vec3_values);
		REQUIRE(vec4_values);
		REQUIRE(string_values);

		REQUIRE(float_values->rowCount() == int_values->rowCount());
		REQUIRE(vec2_values->rowCount() == int_values->rowCount());
		REQUIRE(vec3_values->rowCount() == int_values->rowCount());
		REQUIRE(vec4_values->rowCount() == int_values->rowCount());
		REQUIRE(string_values->rowCount() == int_values->rowCount());

		for (int i = 0, n = polymesh->points.rowCount(); i < n; ++i) {
			// e.g. REQUIRE(a == Approx(3.0).margin(0.1))
			//   2.9 < a < 3.1
			float margin = 1.0e-4f;
			auto x = Approx(i * 0.1f).margin(margin);
			auto y = Approx(i * 0.5f).margin(margin);
			auto z = Approx(i * 0.75f).margin(margin);
			auto w = Approx(i * 0.25f).margin(margin);

			REQUIRE(int_values->get(i) == i);
			REQUIRE(float_values->get(i) == x);

			{
				float vec2[2];
				vec2_values->get(i, vec2);
				REQUIRE(vec2[0] == x);
				REQUIRE(vec2[1] == y);
			}
			{
				float vec3[3];
				vec3_values->get(i, vec3);
				REQUIRE(vec3[0] == x);
				REQUIRE(vec3[1] == y);
				REQUIRE(vec3[2] == z);
			}
			{
				float vec4[4];
				vec4_values->get(i, vec4);
				REQUIRE(vec4[0] == x);
				REQUIRE(vec4[1] == y);
				REQUIRE(vec4[2] == z);
				REQUIRE(vec4[3] == w);
			}

			REQUIRE(string_values->get(i) == ("index = " + std::to_string(i)));
		}
	}

	SECTION("Primitives") {
		REQUIRE(polymesh->faceCounts.size() == polymesh->primitives.rowCount());

		const AttributeIntColumn *int_values = polymesh->primitives.column_as_int("int_primitives");
		const AttributeFloatColumn *float_values = polymesh->primitives.column_as_float("float_primitives");
		const AttributeVector2Column *vec2_values = polymesh->primitives.column_as_vector2("vec2_primitives");
		const AttributeVector3Column *vec3_values = polymesh->primitives.column_as_vector3("vec3_primitives");
		const AttributeVector4Column *vec4_values = polymesh->primitives.column_as_vector4("vec4_primitives");
		const AttributeStringColumn *string_values = polymesh->primitives.column_as_string("string_primitives");

		REQUIRE(int_values);
		REQUIRE(float_values);
		REQUIRE(vec2_values);
		REQUIRE(vec3_values);
		REQUIRE(vec4_values);
		REQUIRE(string_values);

		REQUIRE(float_values->rowCount() == int_values->rowCount());
		REQUIRE(vec2_values->rowCount() == int_values->rowCount());
		REQUIRE(vec3_values->rowCount() == int_values->rowCount());
		REQUIRE(vec4_values->rowCount() == int_values->rowCount());
		REQUIRE(string_values->rowCount() == int_values->rowCount());

		for (int i = 0, n = polymesh->points.rowCount(); i < n; ++i) {
			// e.g. REQUIRE(a == Approx(3.0).margin(0.1))
			//   2.9 < a < 3.1
			float margin = 1.0e-4f;
			auto x = Approx(i * 0.1f).margin(margin);
			auto y = Approx(i * 0.5f).margin(margin);
			auto z = Approx(i * 0.75f).margin(margin);
			auto w = Approx(i * 0.25f).margin(margin);

			REQUIRE(int_values->get(i) == i);
			REQUIRE(float_values->get(i) == x);

			{
				float vec2[2];
				vec2_values->get(i, vec2);
				REQUIRE(vec2[0] == x);
				REQUIRE(vec2[1] == y);
			}
			{
				float vec3[3];
				vec3_values->get(i, vec3);
				REQUIRE(vec3[0] == x);
				REQUIRE(vec3[1] == y);
				REQUIRE(vec3[2] == z);
			}
			{
				float vec4[4];
				vec4_values->get(i, vec4);
				REQUIRE(vec4[0] == x);
				REQUIRE(vec4[1] == y);
				REQUIRE(vec4[2] == z);
				REQUIRE(vec4[3] == w);
			}

			REQUIRE(string_values->get(i) == ("index = " + std::to_string(i)));
		}
	}
}