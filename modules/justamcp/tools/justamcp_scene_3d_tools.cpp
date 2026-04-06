/**************************************************************************/
/*  justamcp_scene_3d_tools.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             BLAZIUM ENGINE                             */
/*                          https://blazium.app                           */
/**************************************************************************/
/* Copyright (c) 2024-present Blazium Engine contributors.                */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef TOOLS_ENABLED

#include "justamcp_scene_3d_tools.h"
#include "core/io/resource_loader.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "justamcp_tool_executor.h"
#include "modules/gridmap/grid_map.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/environment.h"
#include "scene/resources/material.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/sky.h"

// Helper macros for returning errors analogous to base_command.gd
static inline Dictionary _MCP_SUCCESS(const Variant &data) {
	Dictionary r;
	r["ok"] = true;
	r["result"] = data;
	return r;
}
static inline Dictionary _MCP_ERROR_INTERNAL(int code, const String &msg) {
	Dictionary e, r;
	e["code"] = code;
	e["message"] = msg;
	r["ok"] = false;
	r["error"] = e;
	return r;
}
[[maybe_unused]] static inline Dictionary _MCP_ERROR_DATA(int code, const String &msg, const Variant &data) {
	Dictionary e, r;
	e["code"] = code;
	e["message"] = msg;
	e["data"] = data;
	r["ok"] = false;
	r["error"] = e;
	return r;
}
#undef MCP_SUCCESS
#undef MCP_ERROR
#undef MCP_ERROR_DATA
#undef MCP_INVALID_PARAMS
#undef MCP_NOT_FOUND
#undef MCP_INTERNAL
#define MCP_SUCCESS(data) _MCP_SUCCESS(data)
#define MCP_ERROR(code, msg) _MCP_ERROR_INTERNAL(code, msg)
#define MCP_ERROR_DATA(code, msg, data) _MCP_ERROR_DATA(code, msg, data)
#define MCP_INVALID_PARAMS(msg) _MCP_ERROR_INTERNAL(-32602, msg)
#define MCP_NOT_FOUND(msg) _MCP_ERROR_DATA(-32001, String(msg) + " not found", Dictionary())
#define MCP_INTERNAL(msg) _MCP_ERROR_INTERNAL(-32603, String("Internal error: ") + msg)

JustAMCPScene3DTools::JustAMCPScene3DTools() {
}

JustAMCPScene3DTools::~JustAMCPScene3DTools() {
}

Node *JustAMCPScene3DTools::_get_edited_root() {
	if (JustAMCPToolExecutor::get_test_scene_root()) {
		return JustAMCPToolExecutor::get_test_scene_root();
	}
	if (!EditorNode::get_singleton() || !EditorInterface::get_singleton()) {
		return nullptr;
	}
	return EditorInterface::get_singleton()->get_edited_scene_root();
}

Node *JustAMCPScene3DTools::_find_node_by_path(const String &p_path) {
	if (p_path == "." || p_path.is_empty()) {
		return _get_edited_root();
	}
	Node *root = _get_edited_root();
	if (!root) {
		return nullptr;
	}
	return root->get_node_or_null(NodePath(p_path));
}

void JustAMCPScene3DTools::_add_child_with_undo(Node *p_node, Node *p_parent, Node *p_root, const String &p_action_name) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();

	undo_redo->create_action(p_action_name);
	undo_redo->add_do_method(p_parent, "add_child", p_node);
	undo_redo->add_do_method(p_node, "set_owner", p_root);
	undo_redo->add_do_reference(p_node);
	undo_redo->add_undo_method(p_parent, "remove_child", p_node);
	undo_redo->commit_action();
}

Color JustAMCPScene3DTools::_parse_color(const Variant &p_val, const Color &p_default) {
	if (p_val.get_type() == Variant::STRING) {
		return Color::html(p_val);
	} else if (p_val.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_val;
		return Color(
				d.has("r") ? (float)d["r"] : p_default.r,
				d.has("g") ? (float)d["g"] : p_default.g,
				d.has("b") ? (float)d["b"] : p_default.b,
				d.has("a") ? (float)d["a"] : p_default.a);
	}
	return p_default;
}

Vector3 JustAMCPScene3DTools::_parse_vector3(const Variant &p_val, const Vector3 &p_default) {
	if (p_val.get_type() == Variant::STRING) {
		String s = p_val;
		s = s.replace("(", "").replace(")", "");
		Vector<String> parts = s.split(",");
		if (parts.size() >= 3) {
			return Vector3(parts[0].to_float(), parts[1].to_float(), parts[2].to_float());
		}
	} else if (p_val.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_val;
		return Vector3(
				d.has("x") ? (float)d["x"] : p_default.x,
				d.has("y") ? (float)d["y"] : p_default.y,
				d.has("z") ? (float)d["z"] : p_default.z);
	} else if (p_val.get_type() == Variant::ARRAY) {
		Array arr = p_val;
		if (arr.size() >= 3) {
			return Vector3(arr[0], arr[1], arr[2]);
		}
	}
	return p_default;
}

float JustAMCPScene3DTools::_optional_float(const Dictionary &p_params, const String &p_key, float p_default) {
	if (p_params.has(p_key)) {
		return p_params[p_key];
	}
	return p_default;
}

Dictionary JustAMCPScene3DTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "add_mesh_instance") {
		return add_mesh_instance(p_args);
	}
	if (p_tool_name == "setup_lighting") {
		return setup_lighting(p_args);
	}
	if (p_tool_name == "set_material_3d") {
		return set_material_3d(p_args);
	}
	if (p_tool_name == "setup_environment") {
		return setup_environment(p_args);
	}
	if (p_tool_name == "setup_camera_3d") {
		return setup_camera_3d(p_args);
	}
	if (p_tool_name == "add_gridmap") {
		return add_gridmap(p_args);
	}

	return MCP_ERROR(-32601, "Method not found: " + p_tool_name);
}

Dictionary JustAMCPScene3DTools::add_mesh_instance(const Dictionary &p_params) {
	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_INVALID_PARAMS("Parent node not found: " + parent_path);
	}

	String node_name = p_params.has("name") ? String(p_params["name"]) : "MeshInstance3D";
	String mesh_type = p_params.has("mesh_type") ? String(p_params["mesh_type"]) : "";
	String mesh_file = p_params.has("mesh_file") ? String(p_params["mesh_file"]) : "";

	if (mesh_type.is_empty() && mesh_file.is_empty()) {
		return MCP_INVALID_PARAMS("Either mesh_type or mesh_file is required.");
	}

	MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
	mesh_instance->set_name(node_name);

	if (!mesh_file.is_empty()) {
		if (!ResourceLoader::exists(mesh_file)) {
			memdelete(mesh_instance);
			return MCP_INVALID_PARAMS("Mesh file not found: " + mesh_file);
		}
		Ref<Resource> loaded = ResourceLoader::load(mesh_file);
		if (loaded.is_valid()) {
			if (loaded->is_class("Mesh")) {
				mesh_instance->set_mesh(loaded);
			} else if (loaded->is_class("PackedScene")) {
				Ref<PackedScene> scene = loaded;
				Node *scene_inst = scene->instantiate();
				if (scene_inst) {
					Ref<Mesh> found_mesh;
					List<Node *> q;
					q.push_back(scene_inst);
					while (q.size()) {
						Node *n = q.front()->get();
						q.pop_front();
						MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(n);
						if (mi && mi->get_mesh().is_valid()) {
							found_mesh = mi->get_mesh();
							break;
						}
						for (int i = 0; i < n->get_child_count(); i++) {
							q.push_back(n->get_child(i));
						}
					}
					memdelete(scene_inst);
					if (found_mesh.is_null()) {
						memdelete(mesh_instance);
						return MCP_INVALID_PARAMS("No mesh found in packed scene: " + mesh_file);
					}
					mesh_instance->set_mesh(found_mesh);
				}
			} else {
				memdelete(mesh_instance);
				return MCP_INVALID_PARAMS("File is not a mesh or packed scene: " + mesh_file);
			}
		} else {
			memdelete(mesh_instance);
			return MCP_INVALID_PARAMS("Failed to load mesh file: " + mesh_file);
		}
	} else {
		Ref<Mesh> m;
		if (mesh_type == "BoxMesh") {
			m.instantiate();
		} else if (mesh_type == "BoxMesh") {
			m = memnew(BoxMesh);
		} else if (mesh_type == "SphereMesh") {
			m = memnew(SphereMesh);
		} else if (mesh_type == "CylinderMesh") {
			m = memnew(CylinderMesh);
		} else if (mesh_type == "CapsuleMesh") {
			m = memnew(CapsuleMesh);
		} else if (mesh_type == "PlaneMesh") {
			m = memnew(PlaneMesh);
		} else if (mesh_type == "PrismMesh") {
			m = memnew(PrismMesh);
		} else if (mesh_type == "TorusMesh") {
			m = memnew(TorusMesh);
		} else if (mesh_type == "QuadMesh") {
			m = memnew(QuadMesh);
		} else {
			memdelete(mesh_instance);
			return MCP_INVALID_PARAMS("Unknown mesh_type: " + mesh_type);
		}

		if (p_params.has("mesh_properties")) {
			Dictionary m_props = p_params["mesh_properties"];
			Array keys = m_props.keys();
			for (int i = 0; i < keys.size(); i++) {
				m->set(keys[i], m_props[keys[i]]);
			}
		}
		mesh_instance->set_mesh(m);
	}

	Vector3 position = _parse_vector3(p_params.get("position", Variant()), Vector3());
	Vector3 rotation = _parse_vector3(p_params.get("rotation", Variant()), Vector3());
	Vector3 scale = _parse_vector3(p_params.get("scale", Variant()), Vector3(1, 1, 1));

	mesh_instance->set_position(position);
	mesh_instance->set_rotation_degrees(rotation);
	mesh_instance->set_scale(scale);

	_add_child_with_undo(mesh_instance, parent, root, "MCP: Add MeshInstance3D");

	Dictionary res;
	res["node_path"] = root->get_path_to(mesh_instance);
	res["name"] = mesh_instance->get_name();
	res["mesh_type"] = mesh_file.is_empty() ? mesh_type : mesh_file;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPScene3DTools::setup_lighting(const Dictionary &p_params) {
	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_INVALID_PARAMS("Parent node not found: " + parent_path);
	}

	String light_type = p_params.has("light_type") ? String(p_params["light_type"]) : "";
	String preset = p_params.has("preset") ? String(p_params["preset"]) : "";
	String node_name = p_params.has("name") ? String(p_params["name"]) : "";

	if (!preset.is_empty()) {
		if (preset == "sun") {
			light_type = "DirectionalLight3D";
			if (node_name.is_empty()) {
				node_name = "SunLight";
			}
		} else if (preset == "indoor") {
			light_type = "OmniLight3D";
			if (node_name.is_empty()) {
				node_name = "IndoorLight";
			}
		} else if (preset == "dramatic") {
			light_type = "SpotLight3D";
			if (node_name.is_empty()) {
				node_name = "DramaticLight";
			}
		} else {
			return MCP_INVALID_PARAMS("Unknown preset: " + preset);
		}
	}

	if (light_type.is_empty()) {
		return MCP_INVALID_PARAMS("Either light_type or preset is required.");
	}

	Light3D *light = nullptr;
	if (light_type == "DirectionalLight3D") {
		light = memnew(DirectionalLight3D);
	} else if (light_type == "OmniLight3D") {
		light = memnew(OmniLight3D);
	} else if (light_type == "SpotLight3D") {
		light = memnew(SpotLight3D);
	} else {
		return MCP_INVALID_PARAMS("Unknown light_type: " + light_type);
	}

	if (node_name.is_empty()) {
		node_name = light_type;
	}
	light->set_name(node_name);

	light->set_color(_parse_color(p_params.get("color", Variant()), Color(1, 1, 1)));
	light->set_param(Light3D::PARAM_ENERGY, _optional_float(p_params, "energy", 1.0));
	light->set_shadow(p_params.get("shadows", false));

	OmniLight3D *omni = Object::cast_to<OmniLight3D>(light);
	SpotLight3D *spot = Object::cast_to<SpotLight3D>(light);

	if (omni) {
		omni->set_param(Light3D::PARAM_RANGE, _optional_float(p_params, "range", 5.0));
		omni->set_param(Light3D::PARAM_ATTENUATION, _optional_float(p_params, "attenuation", 1.0));
	} else if (spot) {
		spot->set_param(Light3D::PARAM_RANGE, _optional_float(p_params, "range", 5.0));
		spot->set_param(Light3D::PARAM_ATTENUATION, _optional_float(p_params, "attenuation", 1.0));
		spot->set_param(Light3D::PARAM_SPOT_ANGLE, _optional_float(p_params, "spot_angle", 45.0));
		spot->set_param(Light3D::PARAM_SPOT_ATTENUATION, _optional_float(p_params, "spot_angle_attenuation", 1.0));
	}

	if (!preset.is_empty()) {
		if (preset == "sun") {
			light->set_param(Light3D::PARAM_ENERGY, _optional_float(p_params, "energy", 1.0));
			light->set_shadow(p_params.get("shadows", true));
			light->set_rotation_degrees(_parse_vector3(p_params.get("rotation", Variant()), Vector3(-45, -30, 0)));
		} else if (preset == "indoor") {
			light->set_param(Light3D::PARAM_ENERGY, _optional_float(p_params, "energy", 0.8));
			light->set_color(_parse_color(p_params.get("color", Variant()), Color(1.0, 0.95, 0.85)));
			if (omni) {
				omni->set_param(Light3D::PARAM_RANGE, _optional_float(p_params, "range", 8.0));
			}
		} else if (preset == "dramatic") {
			light->set_param(Light3D::PARAM_ENERGY, _optional_float(p_params, "energy", 2.0));
			light->set_shadow(p_params.get("shadows", true));
			if (spot) {
				spot->set_param(Light3D::PARAM_SPOT_ANGLE, _optional_float(p_params, "spot_angle", 25.0));
				spot->set_param(Light3D::PARAM_RANGE, _optional_float(p_params, "range", 10.0));
			}
		}
	}

	light->set_position(_parse_vector3(p_params.get("position", Variant()), Vector3()));
	if (p_params.has("rotation")) {
		light->set_rotation_degrees(_parse_vector3(p_params.get("rotation", Variant()), light->get_rotation_degrees()));
	}

	_add_child_with_undo(light, parent, root, "MCP: Add " + light_type);

	Dictionary res;
	res["node_path"] = root->get_path_to(light);
	res["name"] = light->get_name();
	res["light_type"] = light_type;
	res["preset"] = preset;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPScene3DTools::set_material_3d(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_INVALID_PARAMS("Node not found: " + node_path);
	}

	MeshInstance3D *mesh_inst = Object::cast_to<MeshInstance3D>(node);
	if (!mesh_inst) {
		return MCP_INVALID_PARAMS("Node is not a MeshInstance3D");
	}

	int surface_index = p_params.get("surface_index", 0);

	Ref<StandardMaterial3D> mat;
	mat.instantiate();

	mat->set_albedo(_parse_color(p_params.get("albedo_color", Variant()), Color(1, 1, 1)));
	if (p_params.has("albedo_texture")) {
		String tex_path = p_params["albedo_texture"];
		if (ResourceLoader::exists(tex_path)) {
			mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, ResourceLoader::load(tex_path));
		}
	}

	mat->set_metallic(_optional_float(p_params, "metallic", 0.0));
	mat->set_roughness(_optional_float(p_params, "roughness", 1.0));
	if (p_params.has("metallic_texture")) {
		if (ResourceLoader::exists(p_params["metallic_texture"])) {
			mat->set_texture(BaseMaterial3D::TEXTURE_METALLIC, ResourceLoader::load(p_params["metallic_texture"]));
		}
	}
	if (p_params.has("roughness_texture")) {
		if (ResourceLoader::exists(p_params["roughness_texture"])) {
			mat->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, ResourceLoader::load(p_params["roughness_texture"]));
		}
	}
	if (p_params.has("normal_texture")) {
		mat->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
		if (ResourceLoader::exists(p_params["normal_texture"])) {
			mat->set_texture(BaseMaterial3D::TEXTURE_NORMAL, ResourceLoader::load(p_params["normal_texture"]));
		}
	}

	if (p_params.has("emission") || p_params.has("emission_color")) {
		mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		mat->set_emission(_parse_color(p_params.get("emission", p_params.get("emission_color", Variant())), Color()));
		mat->set_emission_energy_multiplier(_optional_float(p_params, "emission_energy", 1.0));
	}
	if (p_params.has("emission_texture")) {
		mat->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		if (ResourceLoader::exists(p_params["emission_texture"])) {
			mat->set_texture(BaseMaterial3D::TEXTURE_EMISSION, ResourceLoader::load(p_params["emission_texture"]));
		}
	}

	if (p_params.has("transparency")) {
		String t = String(p_params["transparency"]).to_upper();
		if (t == "DISABLED" || t == "0") {
			mat->set_transparency(BaseMaterial3D::TRANSPARENCY_DISABLED);
		} else if (t == "ALPHA" || t == "1") {
			mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		} else if (t == "ALPHA_SCISSOR" || t == "2") {
			mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
		} else if (t == "ALPHA_HASH" || t == "3") {
			mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_HASH);
		} else if (t == "ALPHA_DEPTH_PRE_PASS" || t == "4") {
			mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_DEPTH_PRE_PASS);
		}
	}

	if (p_params.has("cull_mode")) {
		String c = String(p_params["cull_mode"]).to_upper();
		if (c == "BACK" || c == "0") {
			mat->set_cull_mode(BaseMaterial3D::CULL_BACK);
		} else if (c == "FRONT" || c == "1") {
			mat->set_cull_mode(BaseMaterial3D::CULL_FRONT);
		} else if (c == "DISABLED" || c == "2") {
			mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		}
	}

	Ref<Material> old_mat = mesh_inst->get_surface_override_material(surface_index);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action("MCP: Set material on " + mesh_inst->get_name());
	undo_redo->add_do_method(mesh_inst, "set_surface_override_material", surface_index, mat);
	undo_redo->add_undo_method(mesh_inst, "set_surface_override_material", surface_index, old_mat);
	undo_redo->commit_action();

	Dictionary res;
	res["node_path"] = root->get_path_to(mesh_inst);
	res["surface_index"] = surface_index;
	res["albedo_color"] = String(mat->get_albedo());
	res["metallic"] = mat->get_metallic();
	res["roughness"] = mat->get_roughness();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPScene3DTools::setup_environment(const Dictionary &p_params) {
	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_INVALID_PARAMS("Parent node not found: " + parent_path);
	}

	String node_path = p_params.has("node_path") ? String(p_params["node_path"]) : "";
	WorldEnvironment *world_env = nullptr;
	bool is_existing = false;

	if (!node_path.is_empty()) {
		Node *ex = _find_node_by_path(node_path);
		if (ex) {
			world_env = Object::cast_to<WorldEnvironment>(ex);
			is_existing = true;
		}
	}

	if (!world_env) {
		world_env = memnew(WorldEnvironment);
		world_env->set_name(p_params.get("name", "WorldEnvironment"));
	}

	Ref<Environment> env = world_env->get_environment();
	if (env.is_null()) {
		env.instantiate();
	}

	String bg_mode = p_params.get("background_mode", "sky");
	if (bg_mode == "sky") {
		env->set_background(Environment::BG_SKY);
	} else if (bg_mode == "color") {
		env->set_background(Environment::BG_COLOR);
		env->set_bg_color(_parse_color(p_params.get("background_color", Variant()), Color(0.3, 0.3, 0.3)));
	} else if (bg_mode == "canvas") {
		env->set_background(Environment::BG_CANVAS);
	} else if (bg_mode == "clear_color") {
		env->set_background(Environment::BG_CLEAR_COLOR);
	}

	if (p_params.has("sky") && p_params["sky"].get_type() == Variant::DICTIONARY) {
		Dictionary sky_params = p_params["sky"];
		Ref<ProceduralSkyMaterial> sky_mat;
		sky_mat.instantiate();
		sky_mat->set_sky_top_color(_parse_color(sky_params.get("sky_top_color", Variant()), Color(0.385, 0.454, 0.55)));
		sky_mat->set_sky_horizon_color(_parse_color(sky_params.get("sky_horizon_color", Variant()), Color(0.646, 0.654, 0.67)));
		sky_mat->set_ground_bottom_color(_parse_color(sky_params.get("ground_bottom_color", Variant()), Color(0.2, 0.169, 0.133)));
		sky_mat->set_ground_horizon_color(_parse_color(sky_params.get("ground_horizon_color", Variant()), Color(0.646, 0.654, 0.67)));
		// ... extra properties could be added based on ProceduralSkyMaterial ...

		Ref<Sky> sky;
		sky.instantiate();
		sky->set_material(sky_mat);
		env->set_sky(sky);
		env->set_background(Environment::BG_SKY);
	}

	if (p_params.has("ambient_light_color")) {
		env->set_ambient_light_color(_parse_color(p_params["ambient_light_color"], Color(1, 1, 1)));
	}
	if (p_params.has("ambient_light_energy")) {
		env->set_ambient_light_energy(p_params["ambient_light_energy"]);
	}

	if (p_params.has("tonemap_mode")) {
		String tm = String(p_params["tonemap_mode"]).to_upper();
		if (tm == "LINEAR" || tm == "0") {
			env->set_tonemapper(Environment::TONE_MAPPER_LINEAR);
		} else if (tm == "REINHARDT" || tm == "1") {
			env->set_tonemapper(Environment::TONE_MAPPER_REINHARDT);
		} else if (tm == "FILMIC" || tm == "2") {
			env->set_tonemapper(Environment::TONE_MAPPER_FILMIC);
		} else if (tm == "ACES" || tm == "3") {
			env->set_tonemapper(Environment::TONE_MAPPER_ACES);
		} else if (tm == "AGX" || tm == "4") {
			env->set_tonemapper((Environment::ToneMapper)4);
		}
	}
	if (p_params.has("tonemap_exposure")) {
		env->set_tonemap_exposure(p_params["tonemap_exposure"]);
	}
	if (p_params.has("tonemap_white")) {
		env->set_tonemap_white(p_params["tonemap_white"]);
	}

	if (p_params.has("fog_enabled")) {
		env->set_fog_enabled(p_params["fog_enabled"]);
	}
	if (env->is_fog_enabled() || p_params.has("fog_light_color")) {
		env->set_fog_light_color(_parse_color(p_params.get("fog_light_color", Variant()), Color(0.518, 0.553, 0.608)));
		if (p_params.has("fog_density")) {
			env->set_fog_density(p_params["fog_density"]);
		}
		if (p_params.has("fog_light_energy")) {
			env->set_fog_light_energy(p_params["fog_light_energy"]);
		}
	}

	if (p_params.has("glow_enabled")) {
		env->set_glow_enabled(p_params["glow_enabled"]);
	}
	if (env->is_glow_enabled()) {
		if (p_params.has("glow_intensity")) {
			env->set_glow_intensity(p_params["glow_intensity"]);
		}
		if (p_params.has("glow_strength")) {
			env->set_glow_strength(p_params["glow_strength"]);
		}
		if (p_params.has("glow_bloom")) {
			env->set_glow_bloom(p_params["glow_bloom"]);
		}
	}

	if (p_params.has("ssao_enabled")) {
		env->set_ssao_enabled(p_params["ssao_enabled"]);
	}
	if (env->is_ssao_enabled()) {
		if (p_params.has("ssao_radius")) {
			env->set_ssao_radius(p_params["ssao_radius"]);
		}
		if (p_params.has("ssao_intensity")) {
			env->set_ssao_intensity(p_params["ssao_intensity"]);
		}
	}

	if (p_params.has("ssr_enabled")) {
		env->set_ssr_enabled(p_params["ssr_enabled"]);
	}
	if (env->is_ssr_enabled()) {
		if (p_params.has("ssr_max_steps")) {
			env->set_ssr_max_steps(p_params["ssr_max_steps"]);
		}
		if (p_params.has("ssr_fade_in")) {
			env->set_ssr_fade_in(p_params["ssr_fade_in"]);
		}
		if (p_params.has("ssr_fade_out")) {
			env->set_ssr_fade_out(p_params["ssr_fade_out"]);
		}
	}

	if (p_params.has("sdfgi_enabled")) {
		env->set_sdfgi_enabled(p_params["sdfgi_enabled"]);
	}

	world_env->set_environment(env);

	if (!is_existing) {
		_add_child_with_undo(world_env, parent, root, "MCP: Add WorldEnvironment");
	}

	Array features;
	if (env->is_fog_enabled()) {
		features.push_back("fog");
	}
	if (env->is_glow_enabled()) {
		features.push_back("glow");
	}
	if (env->is_ssao_enabled()) {
		features.push_back("ssao");
	}
	if (env->is_ssr_enabled()) {
		features.push_back("ssr");
	}
	if (env->is_sdfgi_enabled()) {
		features.push_back("sdfgi");
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(world_env);
	res["name"] = world_env->get_name();
	res["background_mode"] = bg_mode;
	res["features"] = features;
	res["is_existing"] = is_existing;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPScene3DTools::setup_camera_3d(const Dictionary &p_params) {
	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_INVALID_PARAMS("Parent node not found: " + parent_path);
	}

	String node_path = p_params.has("node_path") ? String(p_params["node_path"]) : "";
	Camera3D *camera = nullptr;
	bool is_existing = false;

	if (!node_path.is_empty()) {
		Node *ex = _find_node_by_path(node_path);
		if (ex) {
			camera = Object::cast_to<Camera3D>(ex);
			if (!camera) {
				return MCP_INVALID_PARAMS("Node is not a Camera3D: " + node_path);
			}
			is_existing = true;
		}
	}

	if (!camera) {
		camera = memnew(Camera3D);
		camera->set_name(p_params.get("name", "Camera3D"));
	}

	String proj = p_params.has("projection") ? String(p_params["projection"]).to_lower() : "";
	if (proj == "perspective" || proj == "0") {
		camera->set_projection(Camera3D::PROJECTION_PERSPECTIVE);
	} else if (proj == "orthogonal" || proj == "orthographic" || proj == "1") {
		camera->set_projection(Camera3D::PROJECTION_ORTHOGONAL);
	} else if (proj == "frustum" || proj == "2") {
		camera->set_projection(Camera3D::PROJECTION_FRUSTUM);
	}

	if (p_params.has("fov")) {
		camera->set_fov(p_params["fov"]);
	}
	if (p_params.has("size")) {
		camera->set_size(p_params["size"]);
	}
	if (p_params.has("near")) {
		camera->set_near(p_params["near"]);
	}
	if (p_params.has("far")) {
		camera->set_far(p_params["far"]);
	}
	if (p_params.has("cull_mask")) {
		camera->set_cull_mask(p_params["cull_mask"]);
	}

	if (p_params.has("current")) {
		camera->set_current(p_params["current"]);
	}

	camera->set_position(_parse_vector3(p_params.get("position", Variant()), camera->get_position()));
	if (p_params.has("rotation")) {
		camera->set_rotation_degrees(_parse_vector3(p_params["rotation"], camera->get_rotation_degrees()));
	}
	if (p_params.has("look_at")) {
		camera->look_at(_parse_vector3(p_params["look_at"], Vector3()), Vector3(0, 1, 0));
	}

	if (p_params.has("environment_path")) {
		String ep = p_params["environment_path"];
		if (ResourceLoader::exists(ep)) {
			Ref<Environment> e = ResourceLoader::load(ep);
			if (e.is_valid()) {
				camera->set_environment(e);
			}
		}
	}

	if (!is_existing) {
		_add_child_with_undo(camera, parent, root, "MCP: Add Camera3D");
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(camera);
	res["name"] = camera->get_name();
	res["projection"] = camera->get_projection() == Camera3D::PROJECTION_PERSPECTIVE ? "perspective" : "orthogonal";
	res["fov"] = camera->get_fov();
	res["position"] = String(camera->get_position());
	res["is_existing"] = is_existing;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPScene3DTools::add_gridmap(const Dictionary &p_params) {
	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No active scene");
	}

	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_INVALID_PARAMS("Parent node not found: " + parent_path);
	}

	String node_path = p_params.has("node_path") ? String(p_params["node_path"]) : "";
	GridMap *gridmap = nullptr;
	bool is_existing = false;

	if (!node_path.is_empty()) {
		Node *ex = _find_node_by_path(node_path);
		if (ex) {
			gridmap = Object::cast_to<GridMap>(ex);
			if (!gridmap) {
				return MCP_INVALID_PARAMS("Node is not a GridMap: " + node_path);
			}
			is_existing = true;
		}
	}

	if (!gridmap) {
		gridmap = memnew(GridMap);
		gridmap->set_name(p_params.get("name", "GridMap"));
	}

	if (p_params.has("mesh_library_path")) {
		String mlp = p_params["mesh_library_path"];
		if (!ResourceLoader::exists(mlp)) {
			if (!is_existing) {
				memdelete(gridmap);
			}
			return MCP_INVALID_PARAMS("Mesh library not found: " + mlp);
		}
		Ref<MeshLibrary> ml = ResourceLoader::load(mlp);
		if (ml.is_valid()) {
			gridmap->set_mesh_library(ml);
		} else {
			if (!is_existing) {
				memdelete(gridmap);
			}
			return MCP_INVALID_PARAMS("Not a mesh library: " + mlp);
		}
	}

	if (p_params.has("cell_size")) {
		gridmap->set_cell_size(_parse_vector3(p_params["cell_size"], Vector3(2, 2, 2)));
	}

	gridmap->set_position(_parse_vector3(p_params.get("position", Variant()), gridmap->get_position()));

	if (!is_existing) {
		_add_child_with_undo(gridmap, parent, root, "MCP: Add GridMap");
	}

	int cells_set = 0;
	if (p_params.has("cells") && p_params["cells"].get_type() == Variant::ARRAY) {
		Array cells = p_params["cells"];
		for (int i = 0; i < cells.size(); i++) {
			if (cells[i].get_type() == Variant::DICTIONARY) {
				Dictionary cell = cells[i];
				int x = cell.get("x", 0);
				int y = cell.get("y", 0);
				int z = cell.get("z", 0);
				int item = cell.get("item", 0);
				int orientation = cell.get("orientation", 0);
				gridmap->set_cell_item(Vector3i(x, y, z), item, orientation);
				cells_set++;
			}
		}
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(gridmap);
	res["name"] = gridmap->get_name();
	res["cells_set"] = cells_set;
	res["is_existing"] = is_existing;
	res["has_mesh_library"] = gridmap->get_mesh_library().is_valid();
	return MCP_SUCCESS(res);
}

#endif // TOOLS_ENABLED
