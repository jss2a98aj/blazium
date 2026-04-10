/**************************************************************************/
/*  justamcp_scene_tools.cpp                                              */
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

#include "justamcp_scene_tools.h"
#include "../justamcp_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_file_system.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "scene/2d/sprite_2d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/resources/packed_scene.h"

void JustAMCPSceneTools::_bind_methods() {
	// Bindings for tool methods
}

JustAMCPSceneTools::JustAMCPSceneTools() {
}

JustAMCPSceneTools::~JustAMCPSceneTools() {
}

void JustAMCPSceneTools::_refresh_and_reload(const String &p_scene_path) {
	_refresh_filesystem();
	_reload_scene_in_editor(p_scene_path);
}

void JustAMCPSceneTools::_refresh_filesystem() {
	if (editor_plugin) {
		EditorFileSystem::get_singleton()->scan();
	}
}

void JustAMCPSceneTools::_reload_scene_in_editor(const String &p_scene_path) {
	if (!editor_plugin) {
		return;
	}
	Node *edited = EditorInterface::get_singleton()->get_edited_scene_root();
	if (edited && edited->get_scene_file_path() == p_scene_path) {
		EditorInterface::get_singleton()->reload_scene_from_path(p_scene_path);
	}
}

String JustAMCPSceneTools::_ensure_res_path(const String &p_path) {
	if (!p_path.begins_with("res://")) {
		return "res://" + p_path;
	}
	return p_path;
}

String JustAMCPSceneTools::_to_scene_res_path(const String &p_project_path, const String &p_scene_path) {
	String p = p_scene_path.strip_edges();
	if (p.begins_with("res://")) {
		return p;
	}

	if (!p_project_path.strip_edges().is_empty()) {
		String normalized_project = p_project_path.replace("\\", "/");
		String normalized_scene = p.replace("\\", "/");
		if (normalized_scene.begins_with(normalized_project)) {
			String rel = normalized_scene.substr(normalized_project.length());
			if (rel.begins_with("/")) {
				rel = rel.substr(1);
			}
			return _ensure_res_path(rel);
		}
	}

	return _ensure_res_path(p);
}

Array JustAMCPSceneTools::_load_scene(const String &p_scene_path) {
	Array ret;
	ret.resize(2);
	ret[0] = (Object *)nullptr;
	Dictionary err;

	if (!FileAccess::exists(p_scene_path)) {
		err["ok"] = false;
		err["error"] = "Scene not found: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	Ref<PackedScene> packed = ResourceLoader::load(p_scene_path);
	if (packed.is_null()) {
		err["ok"] = false;
		err["error"] = "Failed to load: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	Node *root = packed->instantiate();
	if (!root) {
		err["ok"] = false;
		err["error"] = "Failed to instantiate: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	ret[0] = root;
	ret[1] = Dictionary();
	return ret;
}

Dictionary JustAMCPSceneTools::_save_scene(Node *p_scene_root, const String &p_scene_path) {
	Dictionary ret;
	Ref<PackedScene> packed;
	packed.instantiate();
	if (packed->pack(p_scene_root) != OK) {
		memdelete(p_scene_root);
		ret["ok"] = false;
		ret["error"] = "Failed to pack scene";
		return ret;
	}
	if (ResourceSaver::save(packed, p_scene_path) != OK) {
		memdelete(p_scene_root);
		ret["ok"] = false;
		ret["error"] = "Failed to save scene";
		return ret;
	}
	memdelete(p_scene_root);
	_refresh_and_reload(p_scene_path);
	return Dictionary(); // empty dict meaning okay
}

Node *JustAMCPSceneTools::_find_node(Node *p_root, const String &p_path) {
	if (p_path == "." || p_path.is_empty()) {
		return p_root;
	}
	return p_root->get_node_or_null(p_path);
}

Variant JustAMCPSceneTools::_parse_value(const Variant &p_value) {
	if (p_value.get_type() == Variant::DICTIONARY) {
		Dictionary value = p_value;
		if (value.has("type")) {
			String t = value["type"];
			if (t == "Vector2") {
				return Vector2(value.get("x", 0), value.get("y", 0));
			}
			if (t == "Vector3") {
				return Vector3(value.get("x", 0), value.get("y", 0), value.get("z", 0));
			}
			if (t == "Color") {
				return Color(value.get("r", 1), value.get("g", 1), value.get("b", 1), value.get("a", 1));
			}
			if (t == "Vector2i") {
				return Vector2i(value.get("x", 0), value.get("y", 0));
			}
			if (t == "Vector3i") {
				return Vector3i(value.get("x", 0), value.get("y", 0), value.get("z", 0));
			}
			if (t == "Rect2") {
				return Rect2(value.get("x", 0), value.get("y", 0), value.get("width", 0), value.get("height", 0));
			}
			if (t == "Transform2D") {
				if (value.has("x") && value.has("y") && value.has("origin")) {
					Dictionary xx = value["x"];
					Dictionary yy = value["y"];
					Dictionary oo = value["origin"];
					return Transform2D(
							Vector2(xx.get("x", 1), xx.get("y", 0)),
							Vector2(yy.get("x", 0), yy.get("y", 1)),
							Vector2(oo.get("x", 0), oo.get("y", 0)));
				}
			}
			if (t == "Transform3D") {
				if (value.has("basis") && value.has("origin")) {
					Dictionary b = value["basis"];
					Dictionary o = value["origin"];
					Dictionary bx = b.get("x", Dictionary());
					Dictionary by = b.get("y", Dictionary());
					Dictionary bz = b.get("z", Dictionary());
					Basis basis = Basis(
							Vector3(bx.get("x", 1), bx.get("y", 0), bx.get("z", 0)),
							Vector3(by.get("x", 0), by.get("y", 1), by.get("z", 0)),
							Vector3(bz.get("x", 0), bz.get("y", 0), bz.get("z", 1)));
					return Transform3D(basis, Vector3(o.get("x", 0), o.get("y", 0), o.get("z", 0)));
				}
			}
			if (t == "NodePath") {
				return NodePath(String(value.get("path", "")));
			}
			if (t == "Resource") {
				String resource_path = value.get("path", "");
				if (resource_path.is_empty()) {
					return Variant();
				}
				return ResourceLoader::load(resource_path);
			}
		}
	} else if (p_value.get_type() == Variant::ARRAY) {
		Array arr = p_value;
		Array result;
		for (int i = 0; i < arr.size(); i++) {
			result.push_back(_parse_value(arr[i]));
		}
		return result;
	}
	return p_value;
}

Variant JustAMCPSceneTools::_serialize_value(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::VECTOR2: {
			Vector2 v = p_value;
			Dictionary d;
			d["type"] = "Vector2";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR3: {
			Vector3 v = p_value;
			Dictionary d;
			d["type"] = "Vector3";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::COLOR: {
			Color c = p_value;
			Dictionary d;
			d["type"] = "Color";
			d["r"] = c.r;
			d["g"] = c.g;
			d["b"] = c.b;
			d["a"] = c.a;
			return d;
		}
		case Variant::VECTOR2I: {
			Vector2i v = p_value;
			Dictionary d;
			d["type"] = "Vector2i";
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR3I: {
			Vector3i v = p_value;
			Dictionary d;
			d["type"] = "Vector3i";
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::RECT2: {
			Rect2 r = p_value;
			Dictionary d;
			d["type"] = "Rect2";
			d["x"] = r.position.x;
			d["y"] = r.position.y;
			d["width"] = r.size.width;
			d["height"] = r.size.height;
			return d;
		}
		case Variant::NODE_PATH: {
			NodePath p = p_value;
			Dictionary d;
			d["type"] = "NodePath";
			d["path"] = String(p);
			return d;
		}
		case Variant::TRANSFORM2D: {
			Transform2D t = p_value;
			Dictionary d;
			d["type"] = "Transform2D";
			Dictionary xx;
			xx["x"] = t.columns[0].x;
			xx["y"] = t.columns[0].y;
			Dictionary yy;
			yy["x"] = t.columns[1].x;
			yy["y"] = t.columns[1].y;
			Dictionary oo;
			oo["x"] = t.get_origin().x;
			oo["y"] = t.get_origin().y;
			d["x"] = xx;
			d["y"] = yy;
			d["origin"] = oo;
			return d;
		}
		case Variant::TRANSFORM3D: {
			Transform3D t = p_value;
			Dictionary d;
			d["type"] = "Transform3D";
			Dictionary bx;
			bx["x"] = t.basis.get_column(0).x;
			bx["y"] = t.basis.get_column(0).y;
			bx["z"] = t.basis.get_column(0).z;
			Dictionary by;
			by["x"] = t.basis.get_column(1).x;
			by["y"] = t.basis.get_column(1).y;
			by["z"] = t.basis.get_column(1).z;
			Dictionary bz;
			bz["x"] = t.basis.get_column(2).x;
			bz["y"] = t.basis.get_column(2).y;
			bz["z"] = t.basis.get_column(2).z;
			Dictionary basis;
			basis["x"] = bx;
			basis["y"] = by;
			basis["z"] = bz;
			Dictionary origin;
			origin["x"] = t.origin.x;
			origin["y"] = t.origin.y;
			origin["z"] = t.origin.z;
			d["basis"] = basis;
			d["origin"] = origin;
			return d;
		}
		case Variant::OBJECT: {
			Ref<Resource> res = p_value;
			if (res.is_valid() && !res->get_path().is_empty()) {
				Dictionary d;
				d["type"] = "Resource";
				d["path"] = res->get_path();
				return d;
			}
			return Variant();
		}
		default:
			return p_value;
	}
}

void JustAMCPSceneTools::_set_node_properties(Node *p_node, const Dictionary &p_properties) {
	Array keys = p_properties.keys();
	for (int i = 0; i < keys.size(); i++) {
		String prop_name = keys[i];
		Variant val = _parse_value(p_properties[prop_name]);
		p_node->set(prop_name, val);
	}
}

Dictionary JustAMCPSceneTools::_parse_properties_arg(const Variant &p_raw_properties) {
	if (p_raw_properties.get_type() == Variant::DICTIONARY) {
		return p_raw_properties;
	}
	if (p_raw_properties.get_type() == Variant::STRING) {
		String text = p_raw_properties;
		if (text.strip_edges().is_empty()) {
			return Dictionary();
		}
		Ref<JSON> json;
		json.instantiate();
		if (json->parse(text) == OK) {
			Variant parsed = json->get_data();
			if (parsed.get_type() == Variant::DICTIONARY) {
				return parsed;
			}
		}
	}
	return Dictionary();
}

void JustAMCPSceneTools::_ensure_parent_dir_for_scene(const String &p_scene_path) {
	String base_dir = p_scene_path.get_base_dir();
	Ref<DirAccess> dir = DirAccess::open(base_dir);
	if (dir.is_null()) {
		DirAccess::make_dir_recursive_absolute(base_dir);
	}
}

void JustAMCPSceneTools::_set_owner_recursive(Node *p_node, Node *p_scene_owner) {
	p_node->set_owner(p_scene_owner);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		_set_owner_recursive(child, p_scene_owner);
	}
}

Dictionary JustAMCPSceneTools::_build_node_tree(Node *p_node, bool p_include_properties, int p_depth, int p_current_depth, const String &p_node_path) {
	Dictionary tree_data;
	tree_data["name"] = String(p_node->get_name());
	tree_data["type"] = p_node->get_class();
	tree_data["path"] = p_node_path;
	Array children;
	tree_data["children"] = children;

	if (p_include_properties) {
		Dictionary props;
		List<PropertyInfo> plist;
		p_node->get_property_list(&plist);
		for (const PropertyInfo &E : plist) {
			if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
				continue;
			}
			String pn = E.name;
			if (pn.is_empty()) {
				continue;
			}
			props[pn] = _serialize_value(p_node->get(pn));
		}
		tree_data["properties"] = props;
	}

	if (p_depth >= 0 && p_current_depth >= p_depth) {
		return tree_data;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		String child_path = (p_node_path == ".") ? String(child->get_name()) : p_node_path + "/" + String(child->get_name());
		Dictionary child_tree = _build_node_tree(child, p_include_properties, p_depth, p_current_depth + 1, child_path);
		Array c = tree_data["children"];
		c.push_back(child_tree);
		tree_data["children"] = c;
	}

	return tree_data;
}

void JustAMCPSceneTools::_collect_nodes_recursive(Node *p_node, const String &p_path, Array &r_out_nodes) {
	Dictionary entry;
	entry["path"] = p_path;
	entry["node"] = p_node;
	r_out_nodes.push_back(entry);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		String child_path = (p_path == ".") ? String(child->get_name()) : p_path + "/" + String(child->get_name());
		_collect_nodes_recursive(child, child_path, r_out_nodes);
	}
}

Dictionary JustAMCPSceneTools::create_scene(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String root_node_type = p_args.get("rootNodeType", "Node");
	String script_path = p_args.get("scriptPath", "");

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (!scene_path.ends_with(".tscn")) {
		scene_path += ".tscn";
	}
	if (!ClassDB::class_exists(StringName(root_node_type))) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Invalid rootNodeType: " + root_node_type;
		return ret;
	}

	_ensure_parent_dir_for_scene(scene_path);

	Object *_root_obj = ClassDB::instantiate(StringName(root_node_type));
	Node *root = Object::cast_to<Node>(_root_obj);
	if (!root) {
		if (_root_obj) {
			memdelete(_root_obj);
		}
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to instantiate root node: " + root_node_type;
		return ret;
	}
	root->set_name(root_node_type);

	if (!script_path.is_empty()) {
		String full_script_path = _to_scene_res_path(project_path, script_path);
		Ref<Script> p_script_res = ResourceLoader::load(full_script_path);
		if (p_script_res.is_null()) {
			memdelete(root);
			Dictionary ret;
			ret["ok"] = false;
			ret["error"] = "Failed to load script: " + full_script_path;
			return ret;
		}
		root->set_script(p_script_res);
	}

	Dictionary err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["scenePath"] = scene_path;
	ret["rootNodeType"] = root_node_type;
	return ret;
}

Dictionary JustAMCPSceneTools::list_scene_nodes(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	int depth = p_args.get("depth", -1);
	bool include_properties = p_args.get("includeProperties", false);

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Dictionary tree = _build_node_tree(root, include_properties, depth, 0, ".");
	memdelete(root);

	Dictionary ret;
	ret["ok"] = true;
	ret["tree"] = tree;
	return ret;
}

Dictionary JustAMCPSceneTools::add_node(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_type = p_args.get("nodeType", "");
	String node_name = p_args.get("nodeName", "");
	String parent_node_path = p_args.get("parentNodePath", ".");
	Dictionary properties = _parse_properties_arg(p_args.get("properties", Dictionary()));

	if (node_type.is_empty() || node_name.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing nodeType or nodeName";
		return ret;
	}
	if (!ClassDB::class_exists(StringName(node_type))) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Invalid nodeType: " + node_type;
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *parent = _find_node(root, parent_node_path);
	if (!parent) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Parent node not found: " + parent_node_path;
		return ret;
	}

	Object *_new_node_obj = ClassDB::instantiate(StringName(node_type));
	Node *new_node = Object::cast_to<Node>(_new_node_obj);
	if (!new_node) {
		if (_new_node_obj) {
			memdelete(_new_node_obj);
		}
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to instantiate nodeType: " + node_type;
		return ret;
	}

	new_node->set_name(node_name);
	_set_node_properties(new_node, properties);
	parent->add_child(new_node);
	_set_owner_recursive(new_node, root);

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodeName"] = node_name;
	ret["nodeType"] = node_type;
	return ret;
}

Dictionary JustAMCPSceneTools::delete_node(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", "");

	if (node_path.is_empty() || node_path == ".") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Cannot delete root node";
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *node = _find_node(root, node_path);
	if (!node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}

	Node *parent = node->get_parent();
	if (!parent) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Cannot delete root node";
		return ret;
	}

	parent->remove_child(node);
	memdelete(node);

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["deletedNodePath"] = node_path;
	return ret;
}

Dictionary JustAMCPSceneTools::duplicate_node(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", "");
	String new_name = p_args.get("newName", "");
	String parent_path = p_args.get("parentPath", "");

	if (node_path.is_empty() || new_name.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing nodePath or newName";
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *source = _find_node(root, node_path);
	if (!source) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}

	Node *target_parent = source->get_parent();
	if (!parent_path.is_empty()) {
		target_parent = _find_node(root, parent_path);
	}
	if (!target_parent) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Parent not found: " + parent_path;
		return ret;
	}

	Node *duplicated_node = source->duplicate();
	if (!duplicated_node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to duplicate node: " + node_path;
		return ret;
	}

	duplicated_node->set_name(new_name);
	target_parent->add_child(duplicated_node);
	_set_owner_recursive(duplicated_node, root);

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodePath"] = node_path;
	ret["newName"] = new_name;
	return ret;
}

Dictionary JustAMCPSceneTools::reparent_node(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", "");
	String new_parent_path = p_args.get("newParentPath", "");

	if (node_path.is_empty() || node_path == ".") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Cannot reparent root node";
		return ret;
	}
	if (new_parent_path.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing newParentPath";
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *node = _find_node(root, node_path);
	Node *new_parent = _find_node(root, new_parent_path);
	if (!node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}
	if (!new_parent) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "New parent not found: " + new_parent_path;
		return ret;
	}

	Node *old_parent = node->get_parent();
	if (!old_parent) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Cannot reparent root node";
		return ret;
	}

	old_parent->remove_child(node);
	new_parent->add_child(node);
	_set_owner_recursive(node, root);

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodePath"] = node_path;
	ret["newParentPath"] = new_parent_path;
	return ret;
}

Dictionary JustAMCPSceneTools::set_node_properties(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", ".");
	Dictionary properties = _parse_properties_arg(p_args.get("properties", Dictionary()));

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *node = _find_node(root, node_path);
	if (!node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}

	_set_node_properties(node, properties);

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodePath"] = node_path;
	return ret;
}

Dictionary JustAMCPSceneTools::get_node_properties(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", ".");
	bool include_defaults = p_args.get("includeDefaults", false);

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *node = _find_node(root, node_path);
	if (!node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}

	Node *defaults = nullptr;
	if (!include_defaults && ClassDB::class_exists(node->get_class_name())) {
		Object *_defaults_obj = ClassDB::instantiate(node->get_class_name());
		defaults = Object::cast_to<Node>(_defaults_obj);
		if (!defaults && _defaults_obj) {
			memdelete(_defaults_obj);
		}
	}

	Dictionary props;
	List<PropertyInfo> plist;
	node->get_property_list(&plist);
	for (const PropertyInfo &E : plist) {
		if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}
		String prop_name = E.name;
		if (prop_name.is_empty()) {
			continue;
		}

		Variant current_val = node->get(prop_name);
		if (!include_defaults && defaults) {
			Variant default_val = defaults->get(prop_name);
			if (current_val == default_val) {
				continue;
			}
		}
		props[prop_name] = _serialize_value(current_val);
	}

	if (defaults) {
		memdelete(defaults);
	}
	memdelete(root);

	Dictionary ret;
	ret["ok"] = true;
	ret["nodePath"] = node_path;
	ret["properties"] = props;
	return ret;
}

Dictionary JustAMCPSceneTools::load_sprite(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String node_path = p_args.get("nodePath", ".");
	String texture_path = _to_scene_res_path(project_path, p_args.get("texturePath", ""));

	if (texture_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing texturePath";
		return ret;
	}

	Ref<Texture2D> texture = ResourceLoader::load(texture_path);
	if (texture.is_null()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to load texture: " + texture_path;
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *node = _find_node(root, node_path);
	if (!node) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node not found: " + node_path;
		return ret;
	}

	if (Sprite2D *s2d = Object::cast_to<Sprite2D>(node)) {
		s2d->set_texture(texture);
	} else if (Sprite3D *s3d = Object::cast_to<Sprite3D>(node)) {
		s3d->set_texture(texture);
	} else {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Node is not Sprite2D or Sprite3D: " + node_path;
		return ret;
	}

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodePath"] = node_path;
	ret["texturePath"] = texture_path;
	return ret;
}

Dictionary JustAMCPSceneTools::save_scene(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String new_path_raw = p_args.get("newPath", "");
	String target_path = scene_path;
	if (!new_path_raw.is_empty()) {
		target_path = _to_scene_res_path(project_path, new_path_raw);
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	_ensure_parent_dir_for_scene(target_path);

	Node *root = Object::cast_to<Node>(result[0]);
	err = _save_scene(root, target_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["scenePath"] = scene_path;
	ret["savedPath"] = target_path;
	return ret;
}

Dictionary JustAMCPSceneTools::connect_signal(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String source_node_path = p_args.get("sourceNodePath", "");
	String signal_name = p_args.get("signalName", "");
	String target_node_path = p_args.get("targetNodePath", "");
	String method_name = p_args.get("methodName", "");
	int flags = p_args.get("flags", 0);

	if (source_node_path.is_empty() || signal_name.is_empty() || target_node_path.is_empty() || method_name.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing required signal connection arguments";
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *source = _find_node(root, source_node_path);
	Node *target = _find_node(root, target_node_path);
	if (!source) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Source node not found: " + source_node_path;
		return ret;
	}
	if (!target) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Target node not found: " + target_node_path;
		return ret;
	}
	if (!source->has_signal(signal_name)) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Signal not found on source: " + signal_name;
		return ret;
	}

	Callable callable(target, StringName(method_name));
	if (!source->is_connected(signal_name, callable)) {
		Error connect_result = source->connect(signal_name, callable, flags);
		if (connect_result != OK) {
			memdelete(root);
			Dictionary ret;
			ret["ok"] = false;
			ret["error"] = "Failed to connect signal: " + itos(connect_result);
			return ret;
		}
	}

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["sourceNodePath"] = source_node_path;
	ret["signalName"] = signal_name;
	ret["targetNodePath"] = target_node_path;
	ret["methodName"] = method_name;
	ret["flags"] = flags;
	return ret;
}

Dictionary JustAMCPSceneTools::disconnect_signal(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String source_node_path = p_args.get("sourceNodePath", "");
	String signal_name = p_args.get("signalName", "");
	String target_node_path = p_args.get("targetNodePath", "");
	String method_name = p_args.get("methodName", "");

	if (source_node_path.is_empty() || signal_name.is_empty() || target_node_path.is_empty() || method_name.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing required signal disconnection arguments";
		return ret;
	}

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Node *source = _find_node(root, source_node_path);
	Node *target = _find_node(root, target_node_path);
	if (!source) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Source node not found: " + source_node_path;
		return ret;
	}
	if (!target) {
		memdelete(root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Target node not found: " + target_node_path;
		return ret;
	}

	Callable callable(target, StringName(method_name));
	if (source->is_connected(signal_name, callable)) {
		source->disconnect(signal_name, callable);
	}

	err = _save_scene(root, scene_path);
	if (!err.is_empty()) {
		return err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["sourceNodePath"] = source_node_path;
	ret["signalName"] = signal_name;
	ret["targetNodePath"] = target_node_path;
	ret["methodName"] = method_name;
	return ret;
}

Dictionary JustAMCPSceneTools::list_connections(const Dictionary &p_args) {
	String project_path = p_args.get("projectPath", "");
	String scene_path = _to_scene_res_path(project_path, p_args.get("scenePath", ""));
	String filter_path = p_args.get("nodePath", "");

	Array result = _load_scene(scene_path);
	Dictionary err = result[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *root = Object::cast_to<Node>(result[0]);
	Array nodes;
	_collect_nodes_recursive(root, ".", nodes);

	Array conn_arr;
	for (int i = 0; i < nodes.size(); i++) {
		Dictionary entry = nodes[i];
		String path = entry["path"];
		if (!filter_path.is_empty() && filter_path != path) {
			continue;
		}
		Node *node = Object::cast_to<Node>(entry["node"]);
		List<MethodInfo> signals;
		node->get_signal_list(&signals);
		for (const MethodInfo &si : signals) {
			String signal_name = si.name;
			if (signal_name.is_empty()) {
				continue;
			}

			List<Connection> conns;
			node->get_signal_connection_list(signal_name, &conns);
			for (const Connection &conn : conns) {
				Callable callable = conn.callable;
				Object *target_obj = callable.get_object();
				String target_path = "";
				if (target_obj && Object::cast_to<Node>(target_obj)) {
					target_path = root->get_path_to(Object::cast_to<Node>(target_obj));
				}
				Dictionary c_info;
				c_info["sourceNodePath"] = path;
				c_info["signalName"] = signal_name;
				c_info["targetNodePath"] = target_path;
				c_info["methodName"] = callable.get_method();
				c_info["flags"] = conn.flags;
				conn_arr.push_back(c_info);
			}
		}
	}

	memdelete(root);

	Dictionary ret;
	ret["ok"] = true;
	ret["connections"] = conn_arr;
	return ret;
}

#endif // TOOLS_ENABLED
