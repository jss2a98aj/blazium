/**************************************************************************/
/*  justamcp_physics_tools.cpp                                            */
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

#include "justamcp_physics_tools.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#endif

#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/2d/physics/ray_cast_2d.h"
#include "scene/2d/physics/rigid_body_2d.h"
#include "scene/2d/physics/static_body_2d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/3d/physics/ray_cast_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/gui/control.h"

#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/convex_polygon_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/2d/segment_shape_2d.h"

#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"

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

void JustAMCPPhysicsTools::_bind_methods() {}

JustAMCPPhysicsTools::JustAMCPPhysicsTools() {}
JustAMCPPhysicsTools::~JustAMCPPhysicsTools() {}

#include "justamcp_tool_executor.h"

Node *JustAMCPPhysicsTools::_get_edited_root() {
#ifdef TOOLS_ENABLED
	if (JustAMCPToolExecutor::get_test_scene_root()) {
		return JustAMCPToolExecutor::get_test_scene_root();
	}
	if (EditorNode::get_singleton() && EditorInterface::get_singleton()) {
		return EditorInterface::get_singleton()->get_edited_scene_root();
	}
#endif
	return nullptr;
}

Node *JustAMCPPhysicsTools::_find_node_by_path(const String &p_path) {
	Node *root = _get_edited_root();
	if (!root) {
		return nullptr;
	}

	if (p_path == "." || p_path == root->get_name()) {
		return root;
	}
	if (root->has_node(p_path)) {
		return root->get_node(p_path);
	}

	if (p_path.begins_with(String(root->get_name()) + "/")) {
		String rel = p_path.substr(String(root->get_name()).length() + 1);
		if (root->has_node(rel)) {
			return root->get_node(rel);
		}
	}
	return nullptr;
}

String JustAMCPPhysicsTools::_detect_dimension(Node *p_node) {
	Node *current = p_node;
	while (current) {
		if (current->is_class("Node2D") || current->is_class("Control")) {
			return "2d";
		}
		if (current->is_class("Node3D")) {
			return "3d";
		}
		current = current->get_parent();
	}
	return "";
}

Dictionary JustAMCPPhysicsTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "setup_collision") {
		return _setup_collision(p_args);
	}
	if (p_tool_name == "set_physics_layers") {
		return _set_physics_layers(p_args);
	}
	if (p_tool_name == "get_physics_layers") {
		return _get_physics_layers(p_args);
	}
	if (p_tool_name == "add_raycast") {
		return _add_raycast(p_args);
	}
	if (p_tool_name == "setup_physics_body") {
		return _setup_physics_body(p_args);
	}
	if (p_tool_name == "get_collision_info") {
		return _get_collision_info(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPPhysicsTools::_setup_collision(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];

	if (!p_params.has("shape")) {
		return MCP_INVALID_PARAMS("Missing param: shape");
	}
	String shape_name = p_params["shape"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	String dim = _detect_dimension(node);
	if (dim.is_empty()) {
		dim = p_params.has("dimension") ? String(p_params["dimension"]) : "2d";
	}

	bool is_valid_parent = false;
	if (dim == "2d") {
		if (node->is_class("PhysicsBody2D") || node->is_class("Area2D") || node->is_class("StaticBody2D") ||
				node->is_class("CharacterBody2D") || node->is_class("RigidBody2D") || node->is_class("AnimatableBody2D")) {
			is_valid_parent = true;
		}
	} else {
		if (node->is_class("PhysicsBody3D") || node->is_class("Area3D") || node->is_class("StaticBody3D") ||
				node->is_class("CharacterBody3D") || node->is_class("RigidBody3D") || node->is_class("AnimatableBody3D")) {
			is_valid_parent = true;
		}
	}

	if (!is_valid_parent) {
		return MCP_INVALID_PARAMS("Node '" + node_path + "' (" + node->get_class() + ") is not a physics body or area.");
	}

	Node *collision_node = nullptr;
	String shape_class_name;

	if (dim == "2d") {
		CollisionShape2D *c2d = memnew(CollisionShape2D);
		Ref<Shape2D> shape;
		if (shape_name == "rectangle" || shape_name == "rect") {
			Ref<RectangleShape2D> r;
			r.instantiate();
			r->set_size(Vector2(float(p_params.get("width", 32.0)), float(p_params.get("height", 32.0))));
			shape = r;
		} else if (shape_name == "circle") {
			Ref<CircleShape2D> c;
			c.instantiate();
			c->set_radius(float(p_params.get("radius", 16.0)));
			shape = c;
		} else if (shape_name == "capsule") {
			Ref<CapsuleShape2D> c;
			c.instantiate();
			c->set_radius(float(p_params.get("radius", 16.0)));
			c->set_height(float(p_params.get("height", 40.0)));
			shape = c;
		} else {
			memdelete(c2d);
			return MCP_INVALID_PARAMS("Unknown 2D shape or not supported yet: " + shape_name);
		}
		c2d->set_shape(shape);
		c2d->set_disabled(bool(p_params.get("disabled", false)));
		c2d->set_one_way_collision(bool(p_params.get("one_way_collision", false)));
		collision_node = c2d;
		shape_class_name = shape->get_class();
	} else {
		CollisionShape3D *c3d = memnew(CollisionShape3D);
		Ref<Shape3D> shape;
		if (shape_name == "box" || shape_name == "rectangle" || shape_name == "rect") {
			Ref<BoxShape3D> b;
			b.instantiate();
			b->set_size(Vector3(float(p_params.get("width", 1.0)), float(p_params.get("height", 1.0)), float(p_params.get("depth", 1.0))));
			shape = b;
		} else if (shape_name == "sphere" || shape_name == "circle") {
			Ref<SphereShape3D> s;
			s.instantiate();
			s->set_radius(float(p_params.get("radius", 0.5)));
			shape = s;
		} else if (shape_name == "capsule") {
			Ref<CapsuleShape3D> c;
			c.instantiate();
			c->set_radius(float(p_params.get("radius", 0.5)));
			c->set_height(float(p_params.get("height", 2.0)));
			shape = c;
		} else {
			memdelete(c3d);
			return MCP_INVALID_PARAMS("Unknown 3D shape or not supported yet: " + shape_name);
		}
		c3d->set_shape(shape);
		c3d->set_disabled(bool(p_params.get("disabled", false)));
		collision_node = c3d;
		shape_class_name = shape->get_class();
	}

	collision_node->set_name("CollisionShape");

#ifdef TOOLS_ENABLED
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	if (ur) {
		ur->create_action("MCP: Add CollisionShape to " + node->get_name());
		ur->add_do_method(node, "add_child", collision_node);
		ur->add_do_method(collision_node, "set_owner", root);
		ur->add_do_reference(collision_node);
		ur->add_undo_method(node, "remove_child", collision_node);
		ur->commit_action();
	} else {
#endif
		node->add_child(collision_node);
		collision_node->set_owner(root);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["node_path"] = root->get_path_to(collision_node);
	res["shape_type"] = shape_class_name;
	res["dimension"] = dim.to_upper();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPPhysicsTools::_set_physics_layers(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	uint32_t current_layer = 1;
	uint32_t current_mask = 1;

	// Read current using reflection
	bool valid_layer = false;
	bool valid_mask = false;
	Variant vlayer = node->get("collision_layer", &valid_layer);
	Variant vmask = node->get("collision_mask", &valid_mask);
	if (valid_layer) {
		current_layer = vlayer;
	}
	if (valid_mask) {
		current_mask = vmask;
	}

	if (!valid_layer && !valid_mask) {
		return MCP_INVALID_PARAMS("Node does not support collision_layer/mask properties.");
	}

	if (p_params.has("layer")) {
		current_layer = int(p_params["layer"]);
	}
	if (p_params.has("mask")) {
		current_mask = int(p_params["mask"]);
	}

	// Apply via reflection
	if (valid_layer) {
		node->set("collision_layer", current_layer);
	}
	if (valid_mask) {
		node->set("collision_mask", current_mask);
	}

	Dictionary res;
	res["node_path"] = node_path;
	res["layer"] = current_layer;
	res["mask"] = current_mask;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPPhysicsTools::_get_physics_layers(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	uint32_t current_layer = 0;
	uint32_t current_mask = 0;

	bool valid_layer = false;
	bool valid_mask = false;
	Variant vlayer = node->get("collision_layer", &valid_layer);
	Variant vmask = node->get("collision_mask", &valid_mask);
	if (valid_layer) {
		current_layer = vlayer;
	}
	if (valid_mask) {
		current_mask = vmask;
	}

	Dictionary res;
	res["node_path"] = node_path;
	res["layer"] = current_layer;
	res["mask"] = current_mask;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPPhysicsTools::_add_raycast(const Dictionary &p_params) {
	if (!p_params.has("parent_path")) {
		return MCP_INVALID_PARAMS("Missing param: parent_path");
	}
	String parent_path = p_params["parent_path"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_NOT_FOUND("Node '" + parent_path + "'");
	}

	String dim = _detect_dimension(parent);
	if (dim.is_empty()) {
		dim = p_params.has("dimension") ? String(p_params["dimension"]) : "2d";
	}

	Node *rc = nullptr;
	if (dim == "2d") {
		RayCast2D *r = memnew(RayCast2D);
		if (p_params.has("target_position")) {
			Dictionary tp = p_params["target_position"];
			r->set_target_position(Vector2(float(tp.get("x", 0)), float(tp.get("y", 50.0))));
		}
		if (p_params.has("enabled")) {
			r->set_enabled(bool(p_params["enabled"]));
		}
		if (p_params.has("collision_mask")) {
			r->set_collision_mask(int(p_params["collision_mask"]));
		}
		rc = r;
	} else {
		RayCast3D *r = memnew(RayCast3D);
		if (p_params.has("target_position")) {
			Dictionary tp = p_params["target_position"];
			r->set_target_position(Vector3(float(tp.get("x", 0)), float(tp.get("y", -50.0)), float(tp.get("z", 0))));
		}
		if (p_params.has("enabled")) {
			r->set_enabled(bool(p_params["enabled"]));
		}
		if (p_params.has("collision_mask")) {
			r->set_collision_mask(int(p_params["collision_mask"]));
		}
		rc = r;
	}

	rc->set_name(p_params.get("name", "RayCast"));
	parent->add_child(rc, true);
	rc->set_owner(root);

	Dictionary res;
	res["node_path"] = root->get_path_to(rc);
	res["dimension"] = dim.to_upper();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPPhysicsTools::_setup_physics_body(const Dictionary &p_params) {
	if (!p_params.has("parent_path")) {
		return MCP_INVALID_PARAMS("Missing param: parent_path");
	}
	String parent_path = p_params["parent_path"];
	if (!p_params.has("body_type")) {
		return MCP_INVALID_PARAMS("Missing param: body_type");
	}
	String body_type = p_params["body_type"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_NOT_FOUND("Node '" + parent_path + "'");
	}

	String dim = _detect_dimension(parent);
	if (dim.is_empty()) {
		dim = p_params.has("dimension") ? String(p_params["dimension"]) : "2d";
	}

	Node *body = nullptr;
	String t = body_type.to_lower();
	if (dim == "2d") {
		if (t == "static") {
			body = memnew(StaticBody2D);
		} else if (t == "character" || t == "kinematic") {
			body = memnew(CharacterBody2D);
		} else if (t == "rigid") {
			body = memnew(RigidBody2D);
		} else if (t == "area") {
			body = memnew(Area2D);
		} else {
			return MCP_INVALID_PARAMS("Unknown 2D body type: " + body_type);
		}
	} else {
		if (t == "static") {
			body = memnew(StaticBody3D);
		} else if (t == "character" || t == "kinematic") {
			body = memnew(CharacterBody3D);
		} else if (t == "rigid") {
			body = memnew(RigidBody3D);
		} else if (t == "area") {
			body = memnew(Area3D);
		} else {
			return MCP_INVALID_PARAMS("Unknown 3D body type: " + body_type);
		}
	}

	body->set_name(p_params.get("name", String(body->get_class())));

	// Apply layers if provided
	if (p_params.has("collision_layer")) {
		body->set("collision_layer", int(p_params["collision_layer"]));
	}
	if (p_params.has("collision_mask")) {
		body->set("collision_mask", int(p_params["collision_mask"]));
	}

	parent->add_child(body, true);
	body->set_owner(root);

	Dictionary res;
	res["node_path"] = root->get_path_to(body);
	res["class"] = body->get_class();
	res["dimension"] = dim.to_upper();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPPhysicsTools::_get_collision_info(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	bool valid_layer = false, valid_mask = false;
	Variant vlayer = node->get("collision_layer", &valid_layer);
	Variant vmask = node->get("collision_mask", &valid_mask);

	Dictionary info;
	info["node_path"] = node_path;
	info["type"] = node->get_class();
	if (valid_layer) {
		info["collision_layer"] = vlayer;
	}
	if (valid_mask) {
		info["collision_mask"] = vmask;
	}

	Array shapes;
	for (int i = 0; i < node->get_child_count(); i++) {
		Node *c = node->get_child(i);
		if (c->is_class("CollisionShape2D") || c->is_class("CollisionShape3D") ||
				c->is_class("CollisionPolygon2D") || c->is_class("CollisionPolygon3D")) {
			Dictionary shape_info;
			shape_info["name"] = c->get_name();
			shape_info["type"] = c->get_class();
			shape_info["disabled"] = c->get("disabled");
			shapes.push_back(shape_info);
		}
	}
	info["shapes"] = shapes;

	return MCP_SUCCESS(info);
}
