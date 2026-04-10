/**************************************************************************/
/*  justamcp_export_tools.cpp                                             */
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

#include "justamcp_export_tools.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

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

void JustAMCPExportTools::_bind_methods() {}

JustAMCPExportTools::JustAMCPExportTools() {}

JustAMCPExportTools::~JustAMCPExportTools() {}

Dictionary JustAMCPExportTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "list_export_presets") {
		return _list_export_presets(p_args);
	} else if (p_tool_name == "export_project") {
		return _export_project(p_args);
	} else if (p_tool_name == "get_export_info") {
		return _get_export_info(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPExportTools::_list_export_presets(const Dictionary &p_params) {
	String presets_path = "res://export_presets.cfg";
	if (!FileAccess::exists(presets_path)) {
		Dictionary res;
		res["presets"] = Array();
		res["count"] = 0;
		res["message"] = "No export_presets.cfg found";
		return MCP_SUCCESS(res);
	}

	Ref<ConfigFile> cfg;
	cfg.instantiate();
	Error err = cfg->load(presets_path);
	if (err != OK) {
		return MCP_INTERNAL("Failed to read export_presets.cfg");
	}

	Array presets;
	int idx = 0;
	while (cfg->has_section("preset." + itos(idx))) {
		String section = "preset." + itos(idx);
		Dictionary preset;
		preset["index"] = idx;
		preset["name"] = cfg->get_value(section, "name", "");
		preset["platform"] = cfg->get_value(section, "platform", "");
		preset["runnable"] = cfg->get_value(section, "runnable", false);
		preset["export_path"] = cfg->get_value(section, "export_path", "");
		presets.push_back(preset);
		idx++;
	}

	Dictionary res;
	res["presets"] = presets;
	res["count"] = presets.size();
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPExportTools::_export_project(const Dictionary &p_params) {
	int preset_index = p_params.has("preset_index") ? int(p_params["preset_index"]) : -1;
	String preset_name = p_params.has("preset_name") ? String(p_params["preset_name"]) : "";
	bool debug = p_params.has("debug") ? bool(p_params["debug"]) : true;

	String presets_path = "res://export_presets.cfg";
	if (!FileAccess::exists(presets_path)) {
		return MCP_ERROR(-32000, "No export_presets.cfg found. Configure exports in Project > Export first.");
	}

	Ref<ConfigFile> cfg;
	cfg.instantiate();
	Error err = cfg->load(presets_path);
	if (err != OK) {
		return MCP_INTERNAL("Failed to read export_presets.cfg");
	}

	String target_section = "";
	String target_name = "";
	String target_path = "";

	if (!preset_name.is_empty()) {
		int idx = 0;
		while (cfg->has_section("preset." + itos(idx))) {
			String section = "preset." + itos(idx);
			if (String(cfg->get_value(section, "name", "")) == preset_name) {
				target_section = section;
				target_name = preset_name;
				target_path = cfg->get_value(section, "export_path", "");
				break;
			}
			idx++;
		}
	} else if (preset_index >= 0) {
		String section = "preset." + itos(preset_index);
		if (cfg->has_section(section)) {
			target_section = section;
			target_name = cfg->get_value(section, "name", "");
			target_path = cfg->get_value(section, "export_path", "");
		}
	}

	if (target_section.is_empty()) {
		return MCP_NOT_FOUND("Export preset");
	}

	if (target_path.is_empty()) {
		return MCP_ERROR(-32000, "Export path not configured for preset '" + target_name + "'");
	}

	String godot_path = OS::get_singleton()->get_executable_path();
	String project_path = ProjectSettings::get_singleton()->globalize_path("res://");
	String export_path = target_path;
	if (target_path.begins_with("res://")) {
		export_path = ProjectSettings::get_singleton()->globalize_path(target_path);
	}

	String flag = debug ? "--export-debug" : "--export-release";
	String command = vformat("\"%s\" --headless --path \"%s\" %s \"%s\"", godot_path, project_path, flag, target_name);

	Dictionary res;
	res["preset"] = target_name;
	res["export_path"] = export_path;
	res["debug"] = debug;
	res["command"] = command;
	res["message"] = "Run the command above to export. Direct export from editor plugin is not supported in Godot 4 via simple MCP calls yet.";
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPExportTools::_get_export_info(const Dictionary &p_params) {
	Dictionary info;
	info["has_export_presets"] = FileAccess::exists("res://export_presets.cfg");
	info["godot_executable"] = OS::get_singleton()->get_executable_path();
	info["project_path"] = ProjectSettings::get_singleton()->globalize_path("res://");

	String templates_path = OS::get_singleton()->get_data_path().path_join("export_templates");
	info["templates_dir"] = templates_path;
	info["templates_installed"] = DirAccess::dir_exists_absolute(templates_path);

	return MCP_SUCCESS(info);
}
