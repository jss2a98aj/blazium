/**************************************************************************/
/*  justamcp_project_tools.cpp                                            */
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

#include "justamcp_project_tools.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_uid.h"
#include "core/math/expression.h"
#include "modules/regex/regex.h"

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

void JustAMCPProjectTools::_bind_methods() {}

JustAMCPProjectTools::JustAMCPProjectTools() {}

JustAMCPProjectTools::~JustAMCPProjectTools() {}

Dictionary JustAMCPProjectTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "get_project_info") {
		return _get_project_info(p_args);
	} else if (p_tool_name == "get_filesystem_tree") {
		return _get_filesystem_tree(p_args);
	} else if (p_tool_name == "search_files") {
		return _search_files(p_args);
	} else if (p_tool_name == "search_in_files") {
		return _search_in_files(p_args);
	} else if (p_tool_name == "get_project_settings") {
		return _get_project_settings(p_args);
	} else if (p_tool_name == "set_project_setting") {
		return _set_project_setting(p_args);
	} else if (p_tool_name == "uid_to_project_path") {
		return _uid_to_project_path(p_args);
	} else if (p_tool_name == "project_path_to_uid") {
		return _project_path_to_uid(p_args);
	} else if (p_tool_name == "add_autoload") {
		return _add_autoload(p_args);
	} else if (p_tool_name == "remove_autoload") {
		return _remove_autoload(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPProjectTools::_get_project_info(const Dictionary &p_params) {
	Dictionary info;

	info["project_name"] = ProjectSettings::get_singleton()->get_setting("application/config/name", "");
	info["godot_version"] = Engine::get_singleton()->get_version_info();
	info["project_path"] = ProjectSettings::get_singleton()->globalize_path("res://");
	info["main_scene"] = ProjectSettings::get_singleton()->get_setting("application/run/main_scene", "");

	info["viewport_width"] = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 0);
	info["viewport_height"] = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 0);
	info["window_width"] = ProjectSettings::get_singleton()->get_setting("display/window/size/window_width_override", 0);
	info["window_height"] = ProjectSettings::get_singleton()->get_setting("display/window/size/window_height_override", 0);

	info["renderer"] = ProjectSettings::get_singleton()->get_setting("rendering/renderer/rendering_method", "");

	Dictionary autoloads;
	List<PropertyInfo> props;
	ProjectSettings::get_singleton()->get_property_list(&props);
	for (const PropertyInfo &E : props) {
		String name = E.name;
		if (name.begins_with("autoload/")) {
			autoloads[name.substr(9)] = ProjectSettings::get_singleton()->get_setting(name);
		}
	}
	info["autoloads"] = autoloads;

	return MCP_SUCCESS(info);
}

Dictionary JustAMCPProjectTools::_get_filesystem_tree(const Dictionary &p_params) {
	String path = p_params.has("path") ? String(p_params["path"]) : "res://";
	String filter = p_params.has("filter") ? String(p_params["filter"]) : "";
	int max_depth = p_params.has("max_depth") ? int(p_params["max_depth"]) : 10;

	Dictionary tree = _scan_directory(path, filter, max_depth, 0);
	Dictionary res;
	res["tree"] = tree;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_scan_directory(const String &p_path, const String &p_filter, int p_max_depth, int p_depth) {
	Dictionary result;
	result["name"] = p_path.get_file();
	result["path"] = p_path;
	result["type"] = "directory";

	if (p_depth >= p_max_depth) {
		return result;
	}

	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		return result;
	}

	Array children;
	dir->list_dir_begin();
	String file_name = dir->get_next();

	while (!file_name.is_empty()) {
		if (file_name.begins_with(".")) {
			file_name = dir->get_next();
			continue;
		}

		String full_path = p_path.path_join(file_name);

		if (dir->current_is_dir()) {
			children.push_back(_scan_directory(full_path, p_filter, p_max_depth, p_depth + 1));
		} else {
			if (p_filter.is_empty() || file_name.match(p_filter)) {
				Dictionary child;
				child["name"] = file_name;
				child["path"] = full_path;
				child["type"] = "file";
				children.push_back(child);
			}
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();

	if (!children.is_empty()) {
		result["children"] = children;
	}

	return result;
}

Dictionary JustAMCPProjectTools::_search_files(const Dictionary &p_params) {
	if (!p_params.has("query") || String(p_params["query"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required parameter: query");
	}
	String query = p_params["query"];
	String path = p_params.has("path") ? String(p_params["path"]) : "res://";
	String file_type = p_params.has("file_type") ? String(p_params["file_type"]) : "";
	int max_results = p_params.has("max_results") ? int(p_params["max_results"]) : 50;

	Array matches;
	_search_recursive(path, query, file_type, matches, max_results);

	Dictionary res;
	res["matches"] = matches;
	res["count"] = matches.size();
	return MCP_SUCCESS(res);
}

void JustAMCPProjectTools::_search_recursive(const String &p_path, const String &p_query, const String &p_file_type, Array &r_matches, int p_max_results) {
	if (r_matches.size() >= p_max_results) {
		return;
	}

	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		return;
	}

	dir->list_dir_begin();
	String file_name = dir->get_next();

	String query_lower = p_query.to_lower();

	while (!file_name.is_empty() && r_matches.size() < p_max_results) {
		if (file_name.begins_with(".")) {
			file_name = dir->get_next();
			continue;
		}

		String full_path = p_path.path_join(file_name);

		if (dir->current_is_dir()) {
			_search_recursive(full_path, p_query, p_file_type, r_matches, p_max_results);
		} else {
			if (!p_file_type.is_empty() && file_name.get_extension() != p_file_type) {
				file_name = dir->get_next();
				continue;
			}

			if (file_name.to_lower().contains(query_lower)) {
				r_matches.push_back(full_path);
			} else if (file_name.match(p_query)) {
				r_matches.push_back(full_path);
			}
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();
}

Dictionary JustAMCPProjectTools::_get_project_settings(const Dictionary &p_params) {
	String section = p_params.has("section") ? String(p_params["section"]) : "";
	String key = p_params.has("key") ? String(p_params["key"]) : "";

	if (!key.is_empty()) {
		if (ProjectSettings::get_singleton()->has_setting(key)) {
			Variant value = ProjectSettings::get_singleton()->get_setting(key);
			Dictionary res;
			res["key"] = key;
			res["value"] = String(value);
			res["type"] = Variant::get_type_name(value.get_type());
			return MCP_SUCCESS(res);
		} else {
			return MCP_NOT_FOUND("Setting '" + key + "'");
		}
	}

	Dictionary settings;
	List<PropertyInfo> props;
	ProjectSettings::get_singleton()->get_property_list(&props);
	for (const PropertyInfo &E : props) {
		String name = E.name;
		if (section.is_empty() || name.begins_with(section)) {
			settings[name] = String(ProjectSettings::get_singleton()->get_setting(name));
		}
	}
	Dictionary res;
	res["settings"] = settings;
	res["count"] = settings.size();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_set_project_setting(const Dictionary &p_params) {
	if (!p_params.has("key") || String(p_params["key"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required parameter: key");
	}
	String key = p_params["key"];
	if (!p_params.has("value")) {
		return MCP_INVALID_PARAMS("Missing required parameter: value");
	}

	Variant value = p_params["value"];

	if (value.get_type() == Variant::STRING) {
		String s = value;
		if (s.begins_with("Vector2(")) {
			Expression expr;
			if (expr.parse(s) == OK) {
				Variant parsed = expr.execute(Array(), nullptr, false, true);
				if (parsed.get_type() == Variant::VECTOR2) {
					value = parsed;
				}
			}
		} else if (s == "true") {
			value = true;
		} else if (s == "false") {
			value = false;
		} else if (s.is_valid_int()) {
			value = s.to_int();
		} else if (s.is_valid_float()) {
			value = s.to_float();
		}
	}

	ProjectSettings::get_singleton()->set_setting(key, value);
	Error err = ProjectSettings::get_singleton()->save();
	if (err != OK) {
		return MCP_INTERNAL("Failed to save project settings.");
	}

	Dictionary res;
	res["key"] = key;
	res["value"] = String(ProjectSettings::get_singleton()->get_setting(key));
	res["saved"] = true;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_uid_to_project_path(const Dictionary &p_params) {
	if (!p_params.has("uid") || String(p_params["uid"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required parameter: uid");
	}
	String uid_str = p_params["uid"];

	ResourceUID::ID uid = ResourceUID::get_singleton()->text_to_id(uid_str);
	if (uid == ResourceUID::INVALID_ID) {
		return MCP_INVALID_PARAMS("Invalid UID format.");
	}

	if (!ResourceUID::get_singleton()->has_id(uid)) {
		return MCP_NOT_FOUND("UID");
	}

	String path = ResourceUID::get_singleton()->get_id_path(uid);
	Dictionary res;
	res["uid"] = uid_str;
	res["path"] = path;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_project_path_to_uid(const Dictionary &p_params) {
	if (!p_params.has("path") || String(p_params["path"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required parameter: path");
	}
	String path = p_params["path"];

	if (!ResourceLoader::exists(path)) {
		return MCP_NOT_FOUND("Resource path");
	}

	ResourceUID::ID uid = ResourceLoader::get_resource_uid(path);
	if (uid == ResourceUID::INVALID_ID) {
		return MCP_ERROR(-32001, "No UID assigned.");
	}

	String uid_str = ResourceUID::get_singleton()->id_to_text(uid);
	Dictionary res;
	res["path"] = path;
	res["uid"] = uid_str;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_search_in_files(const Dictionary &p_params) {
	if (!p_params.has("query") || String(p_params["query"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required parameter: query");
	}
	String query = p_params["query"];

	String path = p_params.has("path") ? String(p_params["path"]) : "res://";
	int max_results = p_params.has("max_results") ? int(p_params["max_results"]) : 50;
	bool use_regex = p_params.has("regex") ? bool(p_params["regex"]) : false;
	String file_type = p_params.has("file_type") ? String(p_params["file_type"]) : "";

	Ref<RegEx> regex;
	if (use_regex) {
		regex.instantiate();
		Error err = regex->compile(query);
		if (err != OK) {
			return MCP_INVALID_PARAMS("Invalid regex pattern");
		}
	}

	Array matches;
	_search_in_files_recursive(path, query, regex, file_type, matches, max_results);

	Dictionary res;
	res["matches"] = matches;
	res["count"] = matches.size();
	res["query"] = query;
	return MCP_SUCCESS(res);
}

void JustAMCPProjectTools::_search_in_files_recursive(const String &p_path, const String &p_query, const Ref<RefCounted> &p_regex, const String &p_file_type, Array &r_matches, int p_max_results) {
	if (r_matches.size() >= p_max_results) {
		return;
	}

	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		return;
	}

	dir->list_dir_begin();
	String file_name = dir->get_next();

	Vector<String> text_extensions = { "gd", "tscn", "tres", "cfg", "godot", "gdshader", "md", "txt", "json" };

	while (!file_name.is_empty() && r_matches.size() < p_max_results) {
		if (file_name.begins_with(".")) {
			file_name = dir->get_next();
			continue;
		}

		String full_path = p_path.path_join(file_name);

		if (dir->current_is_dir()) {
			if (file_name != "addons" && file_name != ".godot") {
				_search_in_files_recursive(full_path, p_query, p_regex, p_file_type, r_matches, p_max_results);
			}
		} else {
			String ext = file_name.get_extension();
			if (!p_file_type.is_empty()) {
				if (ext != p_file_type) {
					file_name = dir->get_next();
					continue;
				}
			} else if (text_extensions.find(ext) == -1) {
				file_name = dir->get_next();
				continue;
			}

			Ref<FileAccess> file = FileAccess::open(full_path, FileAccess::READ);
			if (file.is_valid()) {
				String content = file->get_as_text();
				file->close();
				Vector<String> lines = content.split("\n");
				RegEx *regex = Object::cast_to<RegEx>(p_regex.ptr());

				for (int i = 0; i < lines.size(); i++) {
					if (r_matches.size() >= p_max_results) {
						break;
					}
					String line = lines[i];
					bool matched = false;
					if (regex) {
						matched = regex->search(line).is_valid();
					} else {
						matched = line.contains(p_query);
					}
					if (matched) {
						Dictionary match_dict;
						match_dict["file"] = full_path;
						match_dict["line"] = i + 1;
						match_dict["text"] = line.strip_edges();
						r_matches.push_back(match_dict);
					}
				}
			}
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();
}

Dictionary JustAMCPProjectTools::_add_autoload(const Dictionary &p_params) {
	if (!p_params.has("name") || String(p_params["name"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required param: name");
	}
	if (!p_params.has("path") || String(p_params["path"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required param: path");
	}

	String autoload_name = p_params["name"];
	String autoload_path = p_params["path"];

	if (!FileAccess::exists(autoload_path)) {
		return MCP_NOT_FOUND("File '" + autoload_path + "'");
	}

	String setting_key = "autoload/" + autoload_name;
	if (ProjectSettings::get_singleton()->has_setting(setting_key)) {
		Dictionary error_data;
		error_data["current_value"] = String(ProjectSettings::get_singleton()->get_setting(setting_key));
		error_data["suggestion"] = "Use remove_autoload first to replace it";
		return MCP_ERROR_DATA(-32000, "Autoload already exists", error_data);
	}

	ProjectSettings::get_singleton()->set_setting(setting_key, "*" + autoload_path);
	Error err = ProjectSettings::get_singleton()->save();
	if (err != OK) {
		return MCP_INTERNAL("Failed to save project settings");
	}

	Dictionary res;
	res["name"] = autoload_name;
	res["path"] = autoload_path;
	res["added"] = true;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPProjectTools::_remove_autoload(const Dictionary &p_params) {
	if (!p_params.has("name") || String(p_params["name"]).is_empty()) {
		return MCP_INVALID_PARAMS("Missing required param: name");
	}
	String autoload_name = p_params["name"];
	String setting_key = "autoload/" + autoload_name;

	if (!ProjectSettings::get_singleton()->has_setting(setting_key)) {
		return MCP_NOT_FOUND("Autoload '" + autoload_name + "'");
	}

	String old_value = String(ProjectSettings::get_singleton()->get_setting(setting_key));
	ProjectSettings::get_singleton()->clear(setting_key);
	Error err = ProjectSettings::get_singleton()->save();
	if (err != OK) {
		return MCP_INTERNAL("Failed to save project settings");
	}

	Dictionary res;
	res["name"] = autoload_name;
	res["old_path"] = old_value;
	res["removed"] = true;
	return MCP_SUCCESS(res);
}
