/**************************************************************************/
/*  justamcp_node_tools.cpp                                               */
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

#include "justamcp_node_tools.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#endif

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "scene/gui/control.h"

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

void JustAMCPNodeTools::_bind_methods() {}

JustAMCPNodeTools::JustAMCPNodeTools() {}
JustAMCPNodeTools::~JustAMCPNodeTools() {}

#include "justamcp_tool_executor.h"

Node *JustAMCPNodeTools::_get_edited_root() {
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

Node *JustAMCPNodeTools::_find_node_by_path(const String &p_path) {
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

void JustAMCPNodeTools::_set_owner_recursive(Node *p_node, Node *p_owner) {
	p_node->set_owner(p_owner);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_set_owner_recursive(p_node->get_child(i), p_owner);
	}
}

Dictionary JustAMCPNodeTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "add_node") {
		return _add_node(p_args);
	}
	if (p_tool_name == "delete_node") {
		return _delete_node(p_args);
	}
	if (p_tool_name == "duplicate_node") {
		return _duplicate_node(p_args);
	}
	if (p_tool_name == "move_node") {
		return _move_node(p_args);
	}
	if (p_tool_name == "update_property") {
		return _update_property(p_args);
	}
	if (p_tool_name == "get_node_properties") {
		return _get_node_properties(p_args);
	}
	if (p_tool_name == "add_resource") {
		return _add_resource(p_args);
	}
	if (p_tool_name == "set_anchor_preset") {
		return _set_anchor_preset(p_args);
	}
	if (p_tool_name == "rename_node") {
		return _rename_node(p_args);
	}
	if (p_tool_name == "connect_signal") {
		return _connect_signal(p_args);
	}
	if (p_tool_name == "disconnect_signal") {
		return _disconnect_signal(p_args);
	}
	if (p_tool_name == "get_node_groups") {
		return _get_node_groups(p_args);
	}
	if (p_tool_name == "set_node_groups") {
		return _set_node_groups(p_args);
	}
	if (p_tool_name == "find_nodes_in_group") {
		return _find_nodes_in_group(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPNodeTools::_add_node(const Dictionary &p_params) {
	if (!p_params.has("type")) {
		return MCP_INVALID_PARAMS("Missing param: type");
	}
	String type = p_params["type"];
	String parent_path = p_params.has("parent_path") ? String(p_params["parent_path"]) : ".";
	String node_name = p_params.has("name") ? String(p_params["name"]) : "";
	Dictionary properties = p_params.has("properties") ? Dictionary(p_params["properties"]) : Dictionary();

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *parent = _find_node_by_path(parent_path);
	if (!parent) {
		return MCP_NOT_FOUND("Parent node '" + parent_path + "'");
	}

	Node *node = nullptr;
	Ref<Script> custom_script;

	if (ClassDB::class_exists(type)) {
		Object *_node_obj = ClassDB::instantiate(type);
		node = Object::cast_to<Node>(_node_obj);
		if (!node && _node_obj) {
			memdelete(_node_obj);
		}
	} else {
		TypedArray<Dictionary> global_classes = ProjectSettings::get_singleton()->get_global_class_list();
		String script_path = "";
		for (int i = 0; i < global_classes.size(); i++) {
			Dictionary entry = global_classes[i];
			if (String(entry.get("class", "")) == type) {
				script_path = entry.get("path", "");
				break;
			}
		}
		if (script_path.is_empty()) {
			return MCP_INVALID_PARAMS("Unknown node type: '" + type + "'");
		}
		custom_script = ResourceLoader::load(script_path);
		if (custom_script.is_null()) {
			return MCP_INVALID_PARAMS("Unknown node type: loading script failed");
		}
		String base_type = custom_script->get_instance_base_type();
		if (!ClassDB::class_exists(base_type)) {
			return MCP_INVALID_PARAMS("Script extends invalid type");
		}
		Object *_base_node_obj = ClassDB::instantiate(base_type);
		node = Object::cast_to<Node>(_base_node_obj);
		if (!node && _base_node_obj) {
			memdelete(_base_node_obj);
		}
		node->set_script(custom_script);
	}

	if (!node_name.is_empty()) {
		node->set_name(node_name);
	} else {
		node->set_name(type);
	}

	Array keys = properties.keys();
	for (int i = 0; i < keys.size(); i++) {
		String prop_name = keys[i];
		bool valid = false;
		Variant val = properties[prop_name];
		node->set(prop_name, val, &valid);
	}

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Add " + type);
		ur->add_do_method(parent, "add_child", node);
		ur->add_do_method(node, "set_owner", root);
		ur->add_do_reference(node);
		ur->add_undo_method(parent, "remove_child", node);
		ur->commit_action();
	} else {
#endif
		parent->add_child(node);
		node->set_owner(root);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["node_path"] = root->get_path_to(node);
	res["type"] = type;
	res["name"] = node->get_name();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_delete_node(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	if (node == root) {
		return MCP_INVALID_PARAMS("Cannot delete the root node");
	}

	Node *parent = node->get_parent();
	String node_name = node->get_name();

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Delete " + node_name);
		ur->add_do_method(parent, "remove_child", node);
		ur->add_undo_method(parent, "add_child", node);
		ur->add_undo_method(node, "set_owner", root);
		ur->add_undo_reference(node);
		ur->commit_action();
	} else {
#endif
		parent->remove_child(node);
		node->queue_free();
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["deleted"] = node_name;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_duplicate_node(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];
	String new_name = p_params.has("name") ? String(p_params["name"]) : "";

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	if (new_name.is_empty()) {
		new_name = String(node->get_name()) + "_copy";
	}

	Node *dup = node->duplicate();
	dup->set_name(new_name);
	Node *parent = node->get_parent();

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Duplicate " + node->get_name());
		ur->add_do_method(parent, "add_child", dup);
		ur->add_do_method(dup, "set_owner", root);
		ur->add_do_reference(dup);
		ur->add_undo_method(parent, "remove_child", dup);
		ur->commit_action();
	} else {
#endif
		parent->add_child(dup);
		dup->set_owner(root);
#ifdef TOOLS_ENABLED
	}
#endif

	_set_owner_recursive(dup, root);

	Dictionary res;
	res["original"] = root->get_path_to(node);
	res["duplicate"] = root->get_path_to(dup);
	res["name"] = dup->get_name();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_move_node(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("new_parent_path")) {
		return MCP_INVALID_PARAMS("Missing param: new_parent_path");
	}
	String node_path = p_params["node_path"];
	String new_parent_path = p_params["new_parent_path"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}
	if (node == root) {
		return MCP_INVALID_PARAMS("Cannot move the root node");
	}

	Node *new_parent = _find_node_by_path(new_parent_path);
	if (!new_parent) {
		return MCP_NOT_FOUND("Target parent '" + new_parent_path + "'");
	}

	if (new_parent == node || node->is_ancestor_of(new_parent)) {
		return MCP_INVALID_PARAMS("Cannot move a node into its own subtree");
	}

	Node *old_parent = node->get_parent();

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Move " + node->get_name());
		ur->add_do_method(old_parent, "remove_child", node);
		ur->add_do_method(new_parent, "add_child", node);
		ur->add_do_method(node, "set_owner", root);
		ur->add_undo_method(new_parent, "remove_child", node);
		ur->add_undo_method(old_parent, "add_child", node);
		ur->add_undo_method(node, "set_owner", root);
		ur->commit_action();
	} else {
#endif
		old_parent->remove_child(node);
		new_parent->add_child(node);
		node->set_owner(root);
#ifdef TOOLS_ENABLED
	}
#endif

	_set_owner_recursive(node, root);

	Dictionary res;
	res["node"] = node->get_name();
	res["old_parent"] = root->get_path_to(old_parent);
	res["new_parent"] = root->get_path_to(new_parent);
	res["new_path"] = root->get_path_to(node);
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_update_property(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("property")) {
		return MCP_INVALID_PARAMS("Missing param: property");
	}
	if (!p_params.has("value")) {
		return MCP_INVALID_PARAMS("Missing param: value");
	}

	String node_path = p_params["node_path"];
	String property = p_params["property"];
	Variant value = p_params["value"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	bool valid = false;
	Variant old_value = node->get(property, &valid);
	if (!valid) {
		return MCP_NOT_FOUND("Property '" + property + "'");
	}

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Set " + String(node->get_name()) + "." + property);
		ur->add_do_property(node, property, value);
		ur->add_undo_property(node, property, old_value);
		ur->commit_action();
	} else {
#endif
		node->set(property, value);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["node"] = root->get_path_to(node);
	res["property"] = property;
	res["old_value"] = old_value;
	res["new_value"] = node->get(property);
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_get_node_properties(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	String node_path = p_params["node_path"];
	String category = p_params.has("category") ? String(p_params["category"]) : "";

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	Dictionary props;
	List<PropertyInfo> prop_list;
	node->get_property_list(&prop_list);

	for (const PropertyInfo &pi : prop_list) {
		if (pi.usage & PROPERTY_USAGE_EDITOR) {
			if (!category.is_empty() && !String(pi.name).begins_with(category)) {
				continue;
			}
			props[pi.name] = node->get(pi.name);
		}
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(node);
	res["type"] = node->get_class();
	res["properties"] = props;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_add_resource(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("property")) {
		return MCP_INVALID_PARAMS("Missing param: property");
	}
	if (!p_params.has("resource_type")) {
		return MCP_INVALID_PARAMS("Missing param: resource_type");
	}

	String node_path = p_params["node_path"];
	String property = p_params["property"];
	String resource_type = p_params["resource_type"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	if (!ClassDB::class_exists(resource_type) || !ClassDB::is_parent_class(resource_type, "Resource")) {
		return MCP_INVALID_PARAMS("Unknown resource type: " + resource_type);
	}

	Object *_resource_obj = ClassDB::instantiate(resource_type);
	Ref<Resource> resource = Ref<Resource>(Object::cast_to<Resource>(_resource_obj));
	if (resource.is_null()) {
		if (_resource_obj) {
			memdelete(_resource_obj);
		}
		return MCP_INTERNAL("Failed to create resource");
	}

	if (p_params.has("resource_properties")) {
		Dictionary rp = p_params["resource_properties"];
		Array keys = rp.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			resource->set(k, rp[k]);
		}
	}

	Variant old_value = node->get(property);

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Add " + resource_type + " to " + String(node->get_name()));
		ur->add_do_property(node, property, resource);
		ur->add_undo_property(node, property, old_value);
		ur->commit_action();
	} else {
#endif
		node->set(property, resource);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["node_path"] = root->get_path_to(node);
	res["property"] = property;
	res["resource_type"] = resource_type;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_set_anchor_preset(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("preset")) {
		return MCP_INVALID_PARAMS("Missing param: preset");
	}

	String node_path = p_params["node_path"];
	String preset_name = p_params["preset"];
	bool keep_offsets = p_params.has("keep_offsets") ? bool(p_params["keep_offsets"]) : false;

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node || !Object::cast_to<Control>(node)) {
		return MCP_NOT_FOUND("Control node '" + node_path + "'");
	}
	Control *control = Object::cast_to<Control>(node);

	Control::LayoutPreset preset = Control::PRESET_TOP_LEFT;
	if (preset_name == "top_left") {
		preset = Control::PRESET_TOP_LEFT;
	} else if (preset_name == "top_right") {
		preset = Control::PRESET_TOP_RIGHT;
	} else if (preset_name == "bottom_left") {
		preset = Control::PRESET_BOTTOM_LEFT;
	} else if (preset_name == "bottom_right") {
		preset = Control::PRESET_BOTTOM_RIGHT;
	} else if (preset_name == "center_left") {
		preset = Control::PRESET_CENTER_LEFT;
	} else if (preset_name == "center_top") {
		preset = Control::PRESET_CENTER_TOP;
	} else if (preset_name == "center_right") {
		preset = Control::PRESET_CENTER_RIGHT;
	} else if (preset_name == "center_bottom") {
		preset = Control::PRESET_CENTER_BOTTOM;
	} else if (preset_name == "center") {
		preset = Control::PRESET_CENTER;
	} else if (preset_name == "left_wide") {
		preset = Control::PRESET_LEFT_WIDE;
	} else if (preset_name == "top_wide") {
		preset = Control::PRESET_TOP_WIDE;
	} else if (preset_name == "right_wide") {
		preset = Control::PRESET_RIGHT_WIDE;
	} else if (preset_name == "bottom_wide") {
		preset = Control::PRESET_BOTTOM_WIDE;
	} else if (preset_name == "vcenter_wide") {
		preset = Control::PRESET_VCENTER_WIDE;
	} else if (preset_name == "hcenter_wide") {
		preset = Control::PRESET_HCENTER_WIDE;
	} else if (preset_name == "full_rect") {
		preset = Control::PRESET_FULL_RECT;
	} else {
		return MCP_INVALID_PARAMS("Unknown preset");
	}

	Control::LayoutPresetMode mode = keep_offsets ? Control::PRESET_MODE_KEEP_SIZE : Control::PRESET_MODE_MINSIZE;

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Set anchor preset");
		// Add logic for saving undo parameters manually since `set_anchors_and_offsets_preset` mutates multiple
		ur->add_do_method(control, "set_anchors_and_offsets_preset", preset, mode);

		ur->add_undo_property(control, "anchor_left", control->get_anchor(SIDE_LEFT));
		ur->add_undo_property(control, "anchor_top", control->get_anchor(SIDE_TOP));
		ur->add_undo_property(control, "anchor_right", control->get_anchor(SIDE_RIGHT));
		ur->add_undo_property(control, "anchor_bottom", control->get_anchor(SIDE_BOTTOM));
		ur->add_undo_property(control, "offset_left", control->get_offset(SIDE_LEFT));
		ur->add_undo_property(control, "offset_top", control->get_offset(SIDE_TOP));
		ur->add_undo_property(control, "offset_right", control->get_offset(SIDE_RIGHT));
		ur->add_undo_property(control, "offset_bottom", control->get_offset(SIDE_BOTTOM));
		ur->commit_action();
	} else {
#endif
		control->set_anchors_and_offsets_preset(preset, mode);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["node_path"] = root->get_path_to(control);
	res["preset"] = preset_name;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_rename_node(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("new_name")) {
		return MCP_INVALID_PARAMS("Missing param: new_name");
	}

	String node_path = p_params["node_path"];
	String new_name = p_params["new_name"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(node_path);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + node_path + "'");
	}

	String old_name = node->get_name();

#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		ur->create_action("MCP: Rename " + old_name + " to " + new_name);
		ur->add_do_property(node, "name", new_name);
		ur->add_undo_property(node, "name", old_name);
		ur->commit_action();
	} else {
#endif
		node->set_name(new_name);
#ifdef TOOLS_ENABLED
	}
#endif

	Dictionary res;
	res["old_name"] = old_name;
	res["new_name"] = node->get_name();
	res["node_path"] = root->get_path_to(node);
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_connect_signal(const Dictionary &p_params) {
	if (!p_params.has("source_path")) {
		return MCP_INVALID_PARAMS("Missing source");
	}
	if (!p_params.has("signal_name")) {
		return MCP_INVALID_PARAMS("Missing signal");
	}
	if (!p_params.has("target_path")) {
		return MCP_INVALID_PARAMS("Missing target");
	}
	if (!p_params.has("method_name")) {
		return MCP_INVALID_PARAMS("Missing method");
	}

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *source = _find_node_by_path(p_params["source_path"]);
	if (!source) {
		return MCP_NOT_FOUND("Source node");
	}

	Node *target = _find_node_by_path(p_params["target_path"]);
	if (!target) {
		return MCP_NOT_FOUND("Target node");
	}

	String signal_name = p_params["signal_name"];
	String method_name = p_params["method_name"];

	if (!source->has_signal(signal_name)) {
		return MCP_INVALID_PARAMS("Signal not found");
	}

	Callable conn = Callable(target, StringName(method_name));
	if (source->is_connected(signal_name, conn)) {
		Dictionary res;
		res["already_connected"] = true;
		res["signal"] = signal_name;
		return MCP_SUCCESS(res);
	}

	source->connect(signal_name, conn);

	Dictionary res;
	res["source"] = root->get_path_to(source);
	res["signal"] = signal_name;
	res["target"] = root->get_path_to(target);
	res["method"] = method_name;
	res["connected"] = true;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_disconnect_signal(const Dictionary &p_params) {
	if (!p_params.has("source_path")) {
		return MCP_INVALID_PARAMS("Missing source");
	}
	if (!p_params.has("signal_name")) {
		return MCP_INVALID_PARAMS("Missing signal");
	}
	if (!p_params.has("target_path")) {
		return MCP_INVALID_PARAMS("Missing target");
	}
	if (!p_params.has("method_name")) {
		return MCP_INVALID_PARAMS("Missing method");
	}

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *source = _find_node_by_path(p_params["source_path"]);
	if (!source) {
		return MCP_NOT_FOUND("Source node");
	}

	Node *target = _find_node_by_path(p_params["target_path"]);
	if (!target) {
		return MCP_NOT_FOUND("Target node");
	}

	String signal_name = p_params["signal_name"];
	String method_name = p_params["method_name"];

	Callable conn = Callable(target, StringName(method_name));
	if (!source->is_connected(signal_name, conn)) {
		Dictionary res;
		res["was_connected"] = false;
		return MCP_SUCCESS(res);
	}

	source->disconnect(signal_name, conn);

	Dictionary res;
	res["source"] = root->get_path_to(source);
	res["signal"] = signal_name;
	res["target"] = root->get_path_to(target);
	res["method"] = method_name;
	res["disconnected"] = true;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_get_node_groups(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(p_params["node_path"]);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + String(p_params["node_path"]) + "'");
	}

	Array groups;
	List<Node::GroupInfo> gi;
	node->get_groups(&gi);
	for (const Node::GroupInfo &g : gi) {
		String gs = g.name;
		if (!gs.begins_with("_")) {
			groups.push_back(gs);
		}
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(node);
	res["groups"] = groups;
	res["count"] = groups.size();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_set_node_groups(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		return MCP_INVALID_PARAMS("Missing param: node_path");
	}
	if (!p_params.has("groups")) {
		return MCP_INVALID_PARAMS("Missing param: groups");
	}

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Node *node = _find_node_by_path(p_params["node_path"]);
	if (!node) {
		return MCP_NOT_FOUND("Node '" + String(p_params["node_path"]) + "'");
	}

	Array desired_groups = p_params["groups"];
	Array current_groups;
	List<Node::GroupInfo> gi;
	node->get_groups(&gi);
	for (const Node::GroupInfo &g : gi) {
		String gs = g.name;
		if (!gs.begins_with("_")) {
			current_groups.push_back(gs);
		}
	}

	Array added;
	Array removed;

	for (int i = 0; i < current_groups.size(); i++) {
		if (!desired_groups.has(current_groups[i])) {
			node->remove_from_group(current_groups[i]);
			removed.push_back(current_groups[i]);
		}
	}

	for (int i = 0; i < desired_groups.size(); i++) {
		if (!current_groups.has(desired_groups[i])) {
			node->add_to_group(desired_groups[i], true);
			added.push_back(desired_groups[i]);
		}
	}

	Dictionary res;
	res["node_path"] = root->get_path_to(node);
	res["groups"] = desired_groups;
	res["added"] = added;
	res["removed"] = removed;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPNodeTools::_find_nodes_in_group(const Dictionary &p_params) {
	if (!p_params.has("group")) {
		return MCP_INVALID_PARAMS("Missing param: group");
	}
	String group_name = p_params["group"];

	Node *root = _get_edited_root();
	if (!root) {
		return MCP_ERROR(-32000, "No scene is currently open");
	}

	Array matches;
	_find_in_group_recursive(root, root, group_name, matches);

	Dictionary res;
	res["group"] = group_name;
	res["nodes"] = matches;
	res["count"] = matches.size();
	return MCP_SUCCESS(res);
}

void JustAMCPNodeTools::_find_in_group_recursive(Node *p_node, Node *p_root, const String &p_group_name, Array &r_matches) {
	if (p_node->is_in_group(p_group_name)) {
		Dictionary m;
		m["name"] = p_node->get_name();
		m["path"] = p_root->get_path_to(p_node);
		m["type"] = p_node->get_class();
		r_matches.push_back(m);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_in_group_recursive(p_node->get_child(i), p_root, p_group_name, r_matches);
	}
}
