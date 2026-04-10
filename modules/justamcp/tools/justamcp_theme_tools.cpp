/**************************************************************************/
/*  justamcp_theme_tools.cpp                                              */
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

#include "justamcp_theme_tools.h"
#include "core/io/resource_saver.h"
#include "core/math/expression.h"
#include "editor/editor_file_system.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "justamcp_tool_executor.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/gui/margin_container.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

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
#define MCP_SUCCESS(data) return _MCP_SUCCESS(data)
#define MCP_ERROR(code, msg) return _MCP_ERROR_INTERNAL(code, msg)
#define MCP_ERROR_DATA(code, msg, data) return _MCP_ERROR_DATA(code, msg, data)
#define MCP_INVALID_PARAMS(msg) return _MCP_ERROR_INTERNAL(-32602, msg)
#define MCP_NOT_FOUND(msg) return _MCP_ERROR_DATA(-32001, String(msg) + " not found", Dictionary())
#define MCP_INTERNAL(msg) return _MCP_ERROR_INTERNAL(-32603, String("Internal error: ") + msg)

JustAMCPThemeTools::JustAMCPThemeTools() {
}

JustAMCPThemeTools::~JustAMCPThemeTools() {
}

Node *JustAMCPThemeTools::_get_edited_root() {
	if (JustAMCPToolExecutor::get_test_scene_root()) {
		return JustAMCPToolExecutor::get_test_scene_root();
	}
	if (!EditorNode::get_singleton() || !EditorInterface::get_singleton()) {
		return nullptr;
	}
	return EditorInterface::get_singleton()->get_edited_scene_root();
}

Node *JustAMCPThemeTools::_find_node_by_path(const String &p_path) {
	if (p_path == "." || p_path.is_empty()) {
		return _get_edited_root();
	}
	Node *root = _get_edited_root();
	if (!root) {
		return nullptr;
	}
	return root->get_node_or_null(NodePath(p_path));
}

Dictionary JustAMCPThemeTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "create_theme") {
		return create_theme(p_args);
	}
	if (p_tool_name == "set_theme_color") {
		return set_theme_color(p_args);
	}
	if (p_tool_name == "set_theme_constant") {
		return set_theme_constant(p_args);
	}
	if (p_tool_name == "set_theme_font_size") {
		return set_theme_font_size(p_args);
	}
	if (p_tool_name == "set_theme_stylebox") {
		return set_theme_stylebox(p_args);
	}
	if (p_tool_name == "setup_control") {
		return setup_control(p_args);
	}
	if (p_tool_name == "get_theme_info") {
		return get_theme_info(p_args);
	}

	MCP_ERROR(-32601, "Method not found: " + p_tool_name);
}

Dictionary JustAMCPThemeTools::create_theme(const Dictionary &p_params) {
	if (!p_params.has("path")) {
		MCP_INVALID_PARAMS("Missing path");
	}
	String path = p_params["path"];

	Ref<Theme> theme;
	theme.instantiate();

	int font_size = p_params.get("default_font_size", 0);
	if (font_size > 0) {
		theme->set_default_font_size(font_size);
	}

	Error err = ResourceSaver::save(theme, path);
	if (err != OK) {
		MCP_ERROR(-32000, "Failed to save theme");
	}

	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan();
	}

	Dictionary res;
	res["path"] = path;
	res["created"] = true;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::set_theme_color(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	if (!p_params.has("name")) {
		MCP_INVALID_PARAMS("Missing name");
	}
	if (!p_params.has("color")) {
		MCP_INVALID_PARAMS("Missing color");
	}

	String node_path = p_params["node_path"];
	String color_name = p_params["name"];
	String color_str = p_params["color"];

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	Color color = Color(color_str);
	control->add_theme_color_override(color_name, color);

	Dictionary res;
	res["node_path"] = node_path;
	res["name"] = color_name;
	res["color"] = color_str;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::set_theme_constant(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	if (!p_params.has("name")) {
		MCP_INVALID_PARAMS("Missing name");
	}

	String node_path = p_params["node_path"];
	String const_name = p_params["name"];
	int value = p_params.get("value", 0);

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	control->add_theme_constant_override(const_name, value);

	Dictionary res;
	res["node_path"] = node_path;
	res["name"] = const_name;
	res["value"] = value;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::set_theme_font_size(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	if (!p_params.has("name")) {
		MCP_INVALID_PARAMS("Missing name");
	}

	String node_path = p_params["node_path"];
	String font_name = p_params["name"];
	int size = p_params.get("size", 16);

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	control->add_theme_font_size_override(font_name, size);

	Dictionary res;
	res["node_path"] = node_path;
	res["name"] = font_name;
	res["size"] = size;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::set_theme_stylebox(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	if (!p_params.has("name")) {
		MCP_INVALID_PARAMS("Missing name");
	}

	String node_path = p_params["node_path"];
	String style_name = p_params["name"];

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	Ref<StyleBoxFlat> stylebox;
	stylebox.instantiate();

	if (p_params.has("bg_color")) {
		stylebox->set_bg_color(Color(String(p_params["bg_color"])));
	}
	if (p_params.has("border_color")) {
		stylebox->set_border_color(Color(String(p_params["border_color"])));
	}

	int border_width = p_params.get("border_width", 0);
	if (border_width > 0) {
		stylebox->set_border_width_all(border_width);
	}

	int corner_radius = p_params.get("corner_radius", 0);
	if (corner_radius > 0) {
		stylebox->set_corner_radius_all(corner_radius);
	}

	int padding = p_params.get("padding", 0);
	if (padding > 0) {
		stylebox->set_content_margin_all(padding);
	}

	control->add_theme_style_override(style_name, stylebox);

	Dictionary res;
	res["node_path"] = node_path;
	res["name"] = style_name;
	res["type"] = "StyleBoxFlat";
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::setup_control(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	Array applied;

	if (p_params.has("anchor_preset")) {
		String preset = p_params["anchor_preset"];
		int preset_val = -1;
		if (preset == "top_left") {
			preset_val = Control::PRESET_TOP_LEFT;
		} else if (preset == "top_right") {
			preset_val = Control::PRESET_TOP_RIGHT;
		} else if (preset == "bottom_left") {
			preset_val = Control::PRESET_BOTTOM_LEFT;
		} else if (preset == "bottom_right") {
			preset_val = Control::PRESET_BOTTOM_RIGHT;
		} else if (preset == "center_left") {
			preset_val = Control::PRESET_CENTER_LEFT;
		} else if (preset == "center_top") {
			preset_val = Control::PRESET_CENTER_TOP;
		} else if (preset == "center_right") {
			preset_val = Control::PRESET_CENTER_RIGHT;
		} else if (preset == "center_bottom") {
			preset_val = Control::PRESET_CENTER_BOTTOM;
		} else if (preset == "center") {
			preset_val = Control::PRESET_CENTER;
		} else if (preset == "left_wide") {
			preset_val = Control::PRESET_LEFT_WIDE;
		} else if (preset == "top_wide") {
			preset_val = Control::PRESET_TOP_WIDE;
		} else if (preset == "right_wide") {
			preset_val = Control::PRESET_RIGHT_WIDE;
		} else if (preset == "bottom_wide") {
			preset_val = Control::PRESET_BOTTOM_WIDE;
		} else if (preset == "vcenter_wide") {
			preset_val = Control::PRESET_VCENTER_WIDE;
		} else if (preset == "hcenter_wide") {
			preset_val = Control::PRESET_HCENTER_WIDE;
		} else if (preset == "full_rect") {
			preset_val = Control::PRESET_FULL_RECT;
		}

		if (preset_val != -1) {
			control->set_anchors_and_offsets_preset((Control::LayoutPreset)preset_val);
			applied.push_back("anchor_preset=" + preset);
		}
	}

	if (p_params.has("min_size")) {
		String min_size_str = p_params["min_size"];
		Ref<Expression> expr;
		expr.instantiate();
		if (expr->parse(min_size_str) == OK) {
			Variant val = expr->execute(Array(), nullptr, false, true);
			if (val.get_type() == Variant::VECTOR2 && !expr->has_execute_failed()) {
				control->set_custom_minimum_size(val);
				applied.push_back("min_size=" + min_size_str);
			}
		}
	}

	if (p_params.has("size_flags_h")) {
		String sf = p_params["size_flags_h"];
		int flags = -1;
		if (sf == "fill") {
			flags = Control::SIZE_FILL;
		} else if (sf == "expand") {
			flags = Control::SIZE_EXPAND;
		} else if (sf == "fill_expand") {
			flags = Control::SIZE_EXPAND_FILL;
		} else if (sf == "shrink_center") {
			flags = Control::SIZE_SHRINK_CENTER;
		} else if (sf == "shrink_end") {
			flags = Control::SIZE_SHRINK_END;
		}

		if (flags != -1) {
			control->set_h_size_flags(flags);
			applied.push_back("size_flags_h=" + sf);
		}
	}

	if (p_params.has("size_flags_v")) {
		String sf = p_params["size_flags_v"];
		int flags = -1;
		if (sf == "fill") {
			flags = Control::SIZE_FILL;
		} else if (sf == "expand") {
			flags = Control::SIZE_EXPAND;
		} else if (sf == "fill_expand") {
			flags = Control::SIZE_EXPAND_FILL;
		} else if (sf == "shrink_center") {
			flags = Control::SIZE_SHRINK_CENTER;
		} else if (sf == "shrink_end") {
			flags = Control::SIZE_SHRINK_END;
		}

		if (flags != -1) {
			control->set_v_size_flags(flags);
			applied.push_back("size_flags_v=" + sf);
		}
	}

	if (p_params.has("margins") && p_params["margins"].get_type() == Variant::DICTIONARY) {
		Dictionary margins = p_params["margins"];
		MarginContainer *mc = Object::cast_to<MarginContainer>(control);
		if (mc) {
			if (margins.has("left")) {
				mc->add_theme_constant_override("margin_left", margins["left"]);
			}
			if (margins.has("top")) {
				mc->add_theme_constant_override("margin_top", margins["top"]);
			}
			if (margins.has("right")) {
				mc->add_theme_constant_override("margin_right", margins["right"]);
			}
			if (margins.has("bottom")) {
				mc->add_theme_constant_override("margin_bottom", margins["bottom"]);
			}
			applied.push_back("margins=" + String(Variant(margins)));
		}
	}

	if (p_params.has("separation")) {
		int sep = p_params["separation"];
		BoxContainer *bc = Object::cast_to<BoxContainer>(control);
		if (bc) {
			bc->add_theme_constant_override("separation", sep);
			applied.push_back("separation=" + itos(sep));
		}
	}

	if (p_params.has("grow_h")) {
		String g = p_params["grow_h"];
		if (g == "begin") {
			control->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
		} else if (g == "end") {
			control->set_h_grow_direction(Control::GROW_DIRECTION_END);
		} else if (g == "both") {
			control->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
		}
		applied.push_back("grow_h=" + g);
	}

	if (p_params.has("grow_v")) {
		String g = p_params["grow_v"];
		if (g == "begin") {
			control->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
		} else if (g == "end") {
			control->set_v_grow_direction(Control::GROW_DIRECTION_END);
		} else if (g == "both") {
			control->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
		}
		applied.push_back("grow_v=" + g);
	}

	Dictionary res;
	res["node_path"] = node_path;
	res["applied"] = applied;
	res["count"] = applied.size();
	MCP_SUCCESS(res);
}

Dictionary JustAMCPThemeTools::get_theme_info(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	Control *control = Object::cast_to<Control>(node);
	if (!control) {
		MCP_ERROR(-32000, "Node is not a Control");
	}

	Dictionary info;
	info["node_path"] = node_path;
	info["class"] = control->get_class();

	Ref<Theme> theme = control->get_theme();
	if (theme.is_valid()) {
		info["theme_path"] = theme->get_path();
		Array types;
		List<StringName> type_list;
		theme->get_type_list(&type_list);
		for (const StringName &t : type_list) {
			types.push_back(t);
		}
		info["type_list"] = types;
	}

	Dictionary overrides;
	Dictionary colors;
	Dictionary constants;
	Dictionary font_sizes;
	Dictionary styleboxes;

	List<PropertyInfo> plist;
	control->get_property_list(&plist);
	for (const PropertyInfo &prop : plist) {
		String pname = prop.name;
		if (pname.begins_with("theme_override_colors/")) {
			String key = pname.substr(22);
			Color c = control->get(pname);
			colors[key] = "#" + c.to_html();
		} else if (pname.begins_with("theme_override_constants/")) {
			String key = pname.substr(25);
			constants[key] = control->get(pname);
		} else if (pname.begins_with("theme_override_font_sizes/")) {
			String key = pname.substr(26);
			font_sizes[key] = control->get(pname);
		} else if (pname.begins_with("theme_override_styles/")) {
			String key = pname.substr(22);
			Ref<StyleBox> sb = control->get(pname);
			if (sb.is_valid()) {
				styleboxes[key] = sb->get_class();
			} else {
				styleboxes[key] = Variant();
			}
		}
	}

	overrides["colors"] = colors;
	overrides["constants"] = constants;
	overrides["font_sizes"] = font_sizes;
	overrides["styleboxes"] = styleboxes;

	info["overrides"] = overrides;
	MCP_SUCCESS(info);
}

#endif // TOOLS_ENABLED
