/**************************************************************************/
/*  justamcp_tool_executor.cpp                                            */
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

#include "justamcp_tool_executor.h"
#include "../justamcp_editor_plugin.h"
#include "core/config/project_settings.h"
#include "editor/editor_settings.h"
#include "justamcp_analysis_tools.h"
#include "justamcp_animation_tools.h"
#include "justamcp_audio_tools.h"
#include "justamcp_batch_tools.h"
#include "justamcp_export_tools.h"
#include "justamcp_input_tools.h"
#include "justamcp_node_tools.h"
#include "justamcp_particle_tools.h"
#include "justamcp_physics_tools.h"
#include "justamcp_profiling_tools.h"
#include "justamcp_project_tools.h"
#include "justamcp_resource_tools.h"
#include "justamcp_scene_3d_tools.h"
#include "justamcp_scene_tools.h"
#include "justamcp_script_tools.h"
#include "justamcp_shader_tools.h"
#include "justamcp_theme_tools.h"
#include "justamcp_tilemap_tools.h"

#ifdef MODULE_AUTOWORK_ENABLED
#include "justamcp_autowork_tools.h"
#endif

void JustAMCPToolExecutor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("execute_tool", "tool_name", "args"), &JustAMCPToolExecutor::execute_tool);
	ClassDB::bind_static_method("JustAMCPToolExecutor", D_METHOD("get_tool_schemas", "register_only", "ignore_settings"), &JustAMCPToolExecutor::get_tool_schemas, DEFVAL(false), DEFVAL(false));
	ClassDB::bind_static_method("JustAMCPToolExecutor", D_METHOD("set_test_scene_root", "node"), &JustAMCPToolExecutor::set_test_scene_root);
}

Node *JustAMCPToolExecutor::test_scene_root = nullptr;
void JustAMCPToolExecutor::set_test_scene_root(Node *p_node) {
	test_scene_root = p_node;
}
Node *JustAMCPToolExecutor::get_test_scene_root() {
	return test_scene_root;
}

JustAMCPToolExecutor::JustAMCPToolExecutor() {
	_init_tools();
}

JustAMCPToolExecutor::~JustAMCPToolExecutor() {
	if (scene_tools) {
		memdelete(scene_tools);
	}
	if (analysis_tools) {
		memdelete(analysis_tools);
	}
	if (resource_tools) {
		memdelete(resource_tools);
	}
	if (animation_tools) {
		memdelete(animation_tools);
	}
	if (project_tools) {
		memdelete(project_tools);
	}
	if (profiling_tools) {
		memdelete(profiling_tools);
	}
	if (export_tools) {
		memdelete(export_tools);
	}
	if (batch_tools) {
		memdelete(batch_tools);
	}
	if (script_tools) {
		memdelete(script_tools);
	}
	if (node_tools) {
		memdelete(node_tools);
	}
	if (audio_tools) {
		memdelete(audio_tools);
	}
	if (input_tools) {
		memdelete(input_tools);
	}
	if (particle_tools) {
		memdelete(particle_tools);
	}
	if (physics_tools) {
		memdelete(physics_tools);
	}
	if (scene_3d_tools) {
		memdelete(scene_3d_tools);
	}
	if (shader_tools) {
		memdelete(shader_tools);
	}
	if (theme_tools) {
		memdelete(theme_tools);
	}
	if (tilemap_tools) {
		memdelete(tilemap_tools);
	}
#ifdef MODULE_AUTOWORK_ENABLED
	if (autowork_tools) {
		memdelete(autowork_tools);
	}
#endif
}

void JustAMCPToolExecutor::set_editor_plugin(JustAMCPEditorPlugin *p_plugin) {
	editor_plugin = p_plugin;

	if (scene_tools) {
		scene_tools->set_editor_plugin(p_plugin);
	}
	if (resource_tools) {
		resource_tools->set_editor_plugin(p_plugin);
	}
	if (animation_tools) {
		animation_tools->set_editor_plugin(p_plugin);
	}
	if (project_tools) {
		project_tools->set_editor_plugin(p_plugin);
	}
	if (profiling_tools) {
		profiling_tools->set_editor_plugin(p_plugin);
	}
	if (export_tools) {
		export_tools->set_editor_plugin(p_plugin);
	}
	if (batch_tools) {
		batch_tools->set_editor_plugin(p_plugin);
	}
	if (script_tools) {
		script_tools->set_editor_plugin(p_plugin);
	}
	if (node_tools) {
		node_tools->set_editor_plugin(p_plugin);
	}
	if (audio_tools) {
		audio_tools->set_editor_plugin(p_plugin);
	}
	if (input_tools) {
		input_tools->set_editor_plugin(p_plugin);
	}
	if (particle_tools) {
		particle_tools->set_editor_plugin(p_plugin);
	}
	if (analysis_tools) {
		analysis_tools->set_editor_plugin(p_plugin);
	}
	if (physics_tools) {
		physics_tools->set_editor_plugin(p_plugin);
	}
	if (scene_3d_tools) {
		scene_3d_tools->set_editor_plugin(p_plugin);
	}
	if (shader_tools) {
		shader_tools->set_editor_plugin(p_plugin);
	}
	if (theme_tools) {
		theme_tools->set_editor_plugin(p_plugin);
	}
	if (tilemap_tools) {
		tilemap_tools->set_editor_plugin(p_plugin);
	}
#ifdef MODULE_AUTOWORK_ENABLED
	if (autowork_tools) {
		autowork_tools->set_editor_plugin(p_plugin);
	}
#endif
}

void JustAMCPToolExecutor::_init_tools() {
	if (initialized) {
		return;
	}
	initialized = true;

	scene_tools = memnew(JustAMCPSceneTools);
	analysis_tools = memnew(JustAMCPAnalysisTools);
	resource_tools = memnew(JustAMCPResourceTools);
	animation_tools = memnew(JustAMCPAnimationTools);
	project_tools = memnew(JustAMCPProjectTools);
	profiling_tools = memnew(JustAMCPProfilingTools);
	export_tools = memnew(JustAMCPExportTools);
	batch_tools = memnew(JustAMCPBatchTools);
	script_tools = memnew(JustAMCPScriptTools);
	node_tools = memnew(JustAMCPNodeTools);
	audio_tools = memnew(JustAMCPAudioTools);
	input_tools = memnew(JustAMCPInputTools);
	particle_tools = memnew(JustAMCPParticleTools);
	physics_tools = memnew(JustAMCPPhysicsTools);
	scene_3d_tools = memnew(JustAMCPScene3DTools);
	shader_tools = memnew(JustAMCPShaderTools);
	theme_tools = memnew(JustAMCPThemeTools);
	tilemap_tools = memnew(JustAMCPTileMapTools);

#ifdef MODULE_AUTOWORK_ENABLED
	autowork_tools = memnew(JustAMCPAutoworkTools);
#endif

	if (editor_plugin) {
		set_editor_plugin(editor_plugin);
	}
}

void JustAMCPToolExecutor::register_tool_settings() {
	get_tool_schemas(true);
}

Array JustAMCPToolExecutor::get_tool_schemas(bool p_register_only, bool p_ignore_settings) {
	Array tools;
	String current_category = "";
	bool is_core = false;

	auto add_schema = [&](const String &p_name, const String &p_desc, const Vector<String> &p_props, const Vector<String> &p_req) {
		String full_name = "blazium_" + p_name;

		if (p_register_only) {
			if (current_category.is_empty()) {
				// Don't register meta tools as toggles in the UI, they are unconditionally available
				return;
			}
			String cat_path = "blazium/justamcp/tools/" + current_category;
			String tool_path = "blazium/justamcp/tools/" + current_category + "/" + full_name;

			GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, cat_path), is_core);
			GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, tool_path), true);

			if (EditorSettings::get_singleton()) {
				EDITOR_DEF_BASIC(cat_path, is_core);
				EDITOR_DEF_BASIC(tool_path, true);
			}
			return;
		}

		if (!current_category.is_empty() && !p_ignore_settings) {
			bool cat_enabled = true;
			bool tool_enabled = true;

			bool use_project_override = false;
			if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("blazium/justamcp/override_editor_settings")) {
				use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");
			}

			if (use_project_override || !EditorSettings::get_singleton()) {
				if (ProjectSettings::get_singleton()) {
					if (ProjectSettings::get_singleton()->has_setting("blazium/justamcp/tools/" + current_category)) {
						cat_enabled = GLOBAL_GET("blazium/justamcp/tools/" + current_category);
					}
					if (ProjectSettings::get_singleton()->has_setting("blazium/justamcp/tools/" + current_category + "/" + full_name)) {
						tool_enabled = GLOBAL_GET("blazium/justamcp/tools/" + current_category + "/" + full_name);
					}
				}
			} else {
				if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/tools/" + current_category)) {
					cat_enabled = EDITOR_GET("blazium/justamcp/tools/" + current_category);
				}
				if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/tools/" + current_category + "/" + full_name)) {
					tool_enabled = EDITOR_GET("blazium/justamcp/tools/" + current_category + "/" + full_name);
				}
			}

			if (!cat_enabled || !tool_enabled) {
				return;
			}
		}

		Dictionary t;
		t["name"] = full_name;
		t["description"] = p_desc;
		Dictionary schema;
		schema["type"] = "object";
		Dictionary props;

		if (!p_props.is_empty()) {
			for (int i = 0; i < p_props.size(); i += 2) {
				Dictionary p;
				p["type"] = p_props[i + 1];
				if (p_props[i + 1] == "any") {
					p["type"] = "string";
				} else if (p_props[i + 1] == "object") {
					p["properties"] = Dictionary();
				} else if (p_props[i + 1] == "array") {
					Dictionary items_dict;
					items_dict["type"] = "object";
					items_dict["properties"] = Dictionary();
					p["items"] = items_dict;
				}
				props[p_props[i]] = p;
			}
		}

		schema["properties"] = props;

		Array req;
		for (int i = 0; i < p_req.size(); i++) {
			req.push_back(p_req[i]);
		}
		if (!req.is_empty()) {
			schema["required"] = req;
		}
		t["inputSchema"] = schema;
		tools.push_back(t);
	};

	// Meta Tools
	add_schema("search_tools", "Searches the engine native capabilities for a specific tool name matching your needs.",
			Vector<String>{ "query", "string" }, Vector<String>{ "query" });
	add_schema("execute_tool", "Dynamically bypasses context omission to execute ANY tool in the engine by name.",
			Vector<String>{ "tool_name", "string", "arguments", "string" }, Vector<String>{ "tool_name", "arguments" });

	// Scene Tools
	current_category = "scene_tools";
	is_core = true;
	add_schema("create_scene", "Creates a new Godot scene containing the specified nodes structurally.",
			Vector<String>{ "scene_path", "string", "nodes", "array" }, Vector<String>{ "scene_path", "nodes" });
	add_schema("list_scene_nodes", "Lists the hierarchical node structure of a given scene.",
			Vector<String>{ "scene_path", "string", "depth", "number" }, Vector<String>{ "scene_path" });
	add_schema("add_node", "Adds a new node to a scene tree structure.",
			Vector<String>{ "scene_path", "string", "parent_path", "string", "node_type", "string", "node_name", "string", "properties", "object" },
			Vector<String>{ "scene_path", "parent_path", "node_type" });
	add_schema("delete_node", "Removes a specific node from a scene hierarchy.",
			Vector<String>{ "scene_path", "string", "node_path", "string" }, Vector<String>{ "scene_path", "node_path" });
	add_schema("duplicate_node", "Duplicates an existing node within a scene.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "new_name", "string" }, Vector<String>{ "scene_path", "node_path" });
	add_schema("reparent_node", "Changes the parent of a specific node within a scene.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "new_parent_path", "string" }, Vector<String>{ "scene_path", "node_path", "new_parent_path" });
	add_schema("set_node_properties", "Modifies the internal properties of a specified node.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "properties", "object" }, Vector<String>{ "scene_path", "node_path", "properties" });
	add_schema("set_node_property", "Sets a specific property value precisely onto a node.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "property_name", "string", "value", "string" }, Vector<String>{ "scene_path", "node_path", "property_name", "value" });
	add_schema("get_node_properties", "Retrieves the parameters and properties of a specified node.",
			Vector<String>{ "scene_path", "string", "node_path", "string" }, Vector<String>{ "scene_path", "node_path" });
	add_schema("load_sprite", "Loads and sets a texture onto a Sprite node.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "texture_path", "string" }, Vector<String>{ "scene_path", "node_path", "texture_path" });
	add_schema("save_scene", "Saves the current state of a scene to disk.",
			Vector<String>{ "scene_path", "string" }, Vector<String>{ "scene_path" });
	add_schema("connect_signal", "Connects a Godot signal dynamically.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "signal_name", "string", "target_path", "string", "method_name", "string" },
			Vector<String>{ "scene_path", "node_path", "signal_name", "target_path", "method_name" });
	add_schema("disconnect_signal", "Disconnects an existing signal binding.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "signal_name", "string", "target_path", "string", "method_name", "string" },
			Vector<String>{ "scene_path", "node_path", "signal_name", "target_path", "method_name" });
	add_schema("list_connections", "Lists active connections of a specified node.",
			Vector<String>{ "scene_path", "string", "node_path", "string" }, Vector<String>{ "scene_path", "node_path" });

	// Resource Tools
	current_category = "resource_tools";
	is_core = false;
	add_schema("create_resource", "Creates a generic Godot resource.",
			Vector<String>{ "resource_path", "string", "resource_type", "string" }, Vector<String>{ "resource_path", "resource_type" });
	add_schema("modify_resource", "Modifies an existing resource.",
			Vector<String>{ "resource_path", "string", "properties", "object" }, Vector<String>{ "resource_path", "properties" });
	add_schema("create_material", "Creates a material resource natively.",
			Vector<String>{ "resource_path", "string", "material_type", "string", "properties", "object" }, Vector<String>{ "resource_path", "material_type" });
	add_schema("create_shader", "Creates a shader code file natively.",
			Vector<String>{ "resource_path", "string", "shader_code", "string" }, Vector<String>{ "resource_path", "shader_code" });
	add_schema("create_tileset", "Creates a standard tileset natively.",
			Vector<String>{ "resource_path", "string" }, Vector<String>{ "resource_path" });
	add_schema("set_tilemap_cells", "Modifies bulk tilemap data.",
			Vector<String>{ "scene_path", "string", "node_path", "string", "cells", "array" }, Vector<String>{ "scene_path", "node_path", "cells" });
	add_schema("set_theme_resource_color", "Configures theme colors natively.",
			Vector<String>{ "resource_path", "string", "theme_type", "string", "color_name", "string", "color", "object" }, Vector<String>{ "resource_path", "color_name" });
	add_schema("set_theme_resource_font_size", "Configures theme font sizes natively.",
			Vector<String>{ "resource_path", "string", "theme_type", "string", "font_size_name", "string", "size", "number" }, Vector<String>{ "resource_path", "font_size_name" });
	add_schema("apply_theme_shader", "Applies shaders to UI theme elements natively.",
			Vector<String>{ "resource_path", "string", "shader_path", "string" }, Vector<String>{ "resource_path", "shader_path" });

	// Animation Tools
	current_category = "animation_tools";
	is_core = false;
	add_schema("create_animation", "Creates an animation data resource natively.",
			Vector<String>{ "resource_path", "string", "animation_name", "string", "length", "number" }, Vector<String>{ "resource_path", "animation_name" });
	add_schema("add_animation_track", "Injects animation tracks into an existing track layout.",
			Vector<String>{ "resource_path", "string", "animation_name", "string", "track_type", "string", "node_path", "string" }, Vector<String>{ "resource_path", "animation_name", "track_type", "node_path" });
	add_schema("create_animation_tree", "Creates an animation tree container.",
			Vector<String>{ "scene_path", "string", "parent_path", "string", "tree_name", "string" }, Vector<String>{ "scene_path", "parent_path" });
	add_schema("add_animation_state", "Injects structural states to node graphs natively.",
			Vector<String>{ "resource_path", "string", "state_name", "string", "animation_name", "string" }, Vector<String>{ "resource_path", "state_name" });
	add_schema("connect_animation_states", "Binds state machine states together via transitions.",
			Vector<String>{ "resource_path", "string", "from_state", "string", "to_state", "string" }, Vector<String>{ "resource_path", "from_state", "to_state" });
	add_schema("create_navigation_region", "Injects a navigation region structurally natively.",
			Vector<String>{ "scene_path", "string", "parent_path", "string", "region_name", "string" }, Vector<String>{ "scene_path", "parent_path" });
	add_schema("create_navigation_agent", "Injects a navigation agent node natively.",
			Vector<String>{ "scene_path", "string", "parent_path", "string", "agent_name", "string" }, Vector<String>{ "scene_path", "parent_path" });

	// Project Tools
	current_category = "project_tools";
	is_core = true;
	add_schema("get_project_info", "Returns metadata about the project, renderer, viewport, and autoloads.", Vector<String>{}, Vector<String>{});
	add_schema("get_filesystem_tree", "Returns a recursive file tree with an option to filter by extension.",
			Vector<String>{ "path", "string", "filter", "string", "max_depth", "number" }, Vector<String>{});
	add_schema("search_files", "Searches the file system with a glob or fuzzy query.",
			Vector<String>{ "query", "string", "path", "string", "file_type", "string", "max_results", "number" }, Vector<String>{ "query" });
	add_schema("search_in_files", "Searches file contents using string matching or regex.",
			Vector<String>{ "query", "string", "path", "string", "max_results", "number", "regex", "boolean", "file_type", "string" }, Vector<String>{ "query" });
	add_schema("get_project_settings", "Reads the project.godot configuration dynamically.",
			Vector<String>{ "section", "string", "key", "string" }, Vector<String>{});
	add_schema("set_project_setting", "Sets a specific global setting in project.godot.",
			Vector<String>{ "key", "string", "value", "string" }, Vector<String>{ "key", "value" });
	add_schema("uid_to_project_path", "Resolves a Godot UID representation to an absolute res:// path.",
			Vector<String>{ "uid", "string" }, Vector<String>{ "uid" });
	add_schema("project_path_to_uid", "Converts a godot absolute res:// path to its UUID equivalent.",
			Vector<String>{ "path", "string" }, Vector<String>{ "path" });
	add_schema("add_autoload", "Injects a singleton scene or script as an autoload in project scope.",
			Vector<String>{ "name", "string", "path", "string" }, Vector<String>{ "name", "path" });
	add_schema("remove_autoload", "Detaches a singleton autoload from project scope.",
			Vector<String>{ "name", "string" }, Vector<String>{ "name" });

	// Profiling Tools
	current_category = "profiling_tools";
	is_core = false;
	add_schema("get_performance_monitors", "Retrieves all performance monitors related to memory, FPS, navigation, rendering.",
			Vector<String>{ "category", "string" }, Vector<String>{});
	add_schema("get_editor_performance", "Retrieves a compact structural overview of the performance footprint of the godot editor process.",
			Vector<String>{}, Vector<String>{});

	// Export Tools
	current_category = "export_tools";
	is_core = false;
	add_schema("list_export_presets", "Reads and returns all export presets from export_presets.cfg.",
			Vector<String>{}, Vector<String>{});
	add_schema("export_project", "Triggers a headless Godot export operation.",
			Vector<String>{ "preset_index", "number", "preset_name", "string", "debug", "boolean" }, Vector<String>{});
	add_schema("get_export_info", "Returns metadata regarding absolute template directions and binary configurations.",
			Vector<String>{}, Vector<String>{});

	// Batch Tools
	current_category = "batch_tools";
	is_core = false;
	add_schema("find_nodes_by_type", "Recursively scans a scene hierarchy looking for class type matches.",
			Vector<String>{ "type", "string", "recursive", "boolean" }, Vector<String>{ "type" });
	add_schema("find_signal_connections", "Collects connections and signal maps spanning a particular node criteria.",
			Vector<String>{ "signal_name", "string", "node_path", "string" }, Vector<String>{});
	add_schema("batch_set_property", "Finds nodes of a class and assigns a batch property mutation dynamically.",
			Vector<String>{ "type", "string", "property", "string", "value", "any" }, Vector<String>{ "type", "property", "value" });
	add_schema("find_node_references", "Searches file system recursively for reference nodes by string pattern.",
			Vector<String>{ "pattern", "string" }, Vector<String>{ "pattern" });
	add_schema("cross_scene_set_property", "Modifies properties identically across ALL scene files matching parameters saving changes into file system.",
			Vector<String>{ "type", "string", "property", "string", "value", "any", "path_filter", "string", "exclude_addons", "boolean" }, Vector<String>{ "type", "property", "value" });
	add_schema("get_scene_dependencies", "Interrogates internal Godot dependencies structure string paths via ResourceLoader.",
			Vector<String>{ "path", "string" }, Vector<String>{ "path" });

	// Script Tools
	current_category = "script_tools";
	is_core = false;
	add_schema("list_scripts", "Locates .gd, .cs, .gdshader scripts within the project hierarchy.",
			Vector<String>{ "path", "string", "recursive", "boolean" }, Vector<String>{});
	add_schema("read_script", "Fetches source text directly from a godot script file.",
			Vector<String>{ "path", "string" }, Vector<String>{ "path" });
	add_schema("create_script", "Writes a script file providing a default extending template out of the box.",
			Vector<String>{ "path", "string", "content", "string", "extends", "string", "class_name", "string" }, Vector<String>{ "path" });
	add_schema("edit_script", "Modifies an existing script intelligently mapping regex replacements or direct injection.",
			Vector<String>{ "path", "string", "content", "string", "insert_at_line", "number", "text", "string", "replacements", "array" }, Vector<String>{ "path" });
	add_schema("attach_script", "Binds a target Godot Resource Script onto a Scene Node dynamically.",
			Vector<String>{ "node_path", "string", "script_path", "string" }, Vector<String>{ "node_path", "script_path" });
	add_schema("get_open_scripts", "Maps what files are actively opened within Godot's script editor GUI.",
			Vector<String>{}, Vector<String>{});
	add_schema("validate_script", "Compiles a GDScript implicitly returning if valid or syntax errors mapped out.",
			Vector<String>{ "path", "string" }, Vector<String>{ "path" });

	// Node Tools
	current_category = "node_tools";
	is_core = false;
	add_schema("node_add", "Spawns a godot node natively onto a target structural anchor.",
			Vector<String>{ "type", "string", "parent_path", "string", "name", "string", "properties", "object" }, Vector<String>{ "type" });
	add_schema("node_delete", "Removes a godot node structural anchor.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });
	add_schema("node_duplicate", "Clones a node struct into the scene graph dynamically.",
			Vector<String>{ "node_path", "string", "name", "string" }, Vector<String>{ "node_path" });
	add_schema("node_move", "Alters structural ownership hierarchy within the Scene Tree.",
			Vector<String>{ "node_path", "string", "new_parent_path", "string" }, Vector<String>{ "node_path", "new_parent_path" });
	add_schema("node_update_property", "Explicitly manages individual Variant properties assigned onto a node.",
			Vector<String>{ "node_path", "string", "property", "string", "value", "any" }, Vector<String>{ "node_path", "property", "value" });
	add_schema("node_get_properties", "Interrogates internal parameters within Node bindings structurally.",
			Vector<String>{ "node_path", "string", "category", "string" }, Vector<String>{ "node_path" });
	add_schema("node_add_resource", "Embeds a godot Resource directly assigned directly onto a node structure natively.",
			Vector<String>{ "node_path", "string", "property", "string", "resource_type", "string", "resource_properties", "object" }, Vector<String>{ "node_path", "property", "resource_type" });
	add_schema("node_set_anchor_preset", "Enables UI layout modifications bound towards native godot PRESET flags.",
			Vector<String>{ "node_path", "string", "preset", "string", "keep_offsets", "boolean" }, Vector<String>{ "node_path", "preset" });
	add_schema("node_rename", "Edits node semantic names directly within godot structures.",
			Vector<String>{ "node_path", "string", "new_name", "string" }, Vector<String>{ "node_path", "new_name" });
	add_schema("node_connect_signal", "Binds callable events between source nodes bounding into target instances natively.",
			Vector<String>{ "source_path", "string", "signal_name", "string", "target_path", "string", "method_name", "string" }, Vector<String>{ "source_path", "signal_name", "target_path", "method_name" });
	add_schema("node_disconnect_signal", "Snaps off existing bounding callable events recursively mapped over nodes.",
			Vector<String>{ "source_path", "string", "signal_name", "string", "target_path", "string", "method_name", "string" }, Vector<String>{ "source_path", "signal_name", "target_path", "method_name" });
	add_schema("node_get_groups", "Aggregates internal Godot Groups strings assigned over standard instances.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });
	add_schema("node_set_groups", "Mutates overlapping assignment instances mapped per godot string node groups.",
			Vector<String>{ "node_path", "string", "groups", "array" }, Vector<String>{ "node_path", "groups" });
	add_schema("node_find_in_group", "Provides lookup access globally towards Godot instance sets natively spanning specific groups.",
			Vector<String>{ "group", "string" }, Vector<String>{ "group" });

	// Audio Tools
	current_category = "audio_tools";
	is_core = false;
	add_schema("get_audio_bus_layout", "Returns native godot AudioServer state containing all available buses and their effects.",
			Vector<String>{}, Vector<String>{});
	add_schema("add_audio_bus", "Binds a new structural audio bus dynamically into the godot runtime.",
			Vector<String>{ "name", "string", "at_position", "number", "volume_db", "number", "send", "string", "solo", "boolean", "mute", "boolean" }, Vector<String>{ "name" });
	add_schema("set_audio_bus", "Modifies specific index Godot audio buses layout.",
			Vector<String>{ "name", "string", "volume_db", "number", "solo", "boolean", "mute", "boolean", "bypass_effects", "boolean", "send", "string", "rename", "string" }, Vector<String>{ "name" });
	add_schema("add_audio_bus_effect", "Maps Godot AudioEffects via reflection bridging directly onto buses.",
			Vector<String>{ "bus", "string", "effect_type", "string", "params", "object", "at_position", "number" }, Vector<String>{ "bus", "effect_type" });
	add_schema("add_audio_player", "Injects structural AudioStreamPlayer instances directly mapped inside Godot trees.",
			Vector<String>{ "node_path", "string", "name", "string", "type", "string", "stream", "string", "volume_db", "number", "bus", "string", "autoplay", "boolean", "max_distance", "number", "attenuation", "number", "attenuation_model", "number", "unit_size", "number" }, Vector<String>{ "node_path", "name" });
	add_schema("get_audio_info", "Returns hierarchical node maps wrapping natively instantiated stream wrappers.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });

	// Input Tools
	current_category = "input_tools";
	is_core = false;
	add_schema("simulate_key", "Mocks native godot InputEventKey structs sent across IPC towards godot debug runtimes.",
			Vector<String>{ "keycode", "string", "pressed", "boolean", "shift", "boolean", "ctrl", "boolean", "alt", "boolean" }, Vector<String>{ "keycode" });
	add_schema("simulate_mouse_click", "Mocks native godot InputEventMouseButton bounding structurally across OS IPC streams.",
			Vector<String>{ "button", "number", "pressed", "boolean", "double_click", "boolean", "auto_release", "boolean", "x", "number", "y", "number" }, Vector<String>{});
	add_schema("simulate_mouse_move", "Mocks native godot InputEventMouseMotion translating structural frames over OS IPC streams.",
			Vector<String>{ "x", "number", "y", "number", "relative_x", "number", "relative_y", "number", "button_mask", "number", "unhandled", "boolean" }, Vector<String>{});
	add_schema("simulate_action", "Mocks a generic mapped InputAction natively evaluating into the Godot debug tree.",
			Vector<String>{ "action", "string", "pressed", "boolean", "strength", "number" }, Vector<String>{ "action" });
	add_schema("simulate_sequence", "Mocks complex sequence payload arrays interpolating into the running executable via OS IPC.",
			Vector<String>{ "events", "array", "frame_delay", "number" }, Vector<String>{ "events" });

	// Particle Tools
	current_category = "particle_tools";
	is_core = false;
	add_schema("create_particles", "Instantiates a new particle emitter structure onto a godot node.",
			Vector<String>{ "parent_path", "string", "name", "string", "is_3d", "boolean", "amount", "number", "lifetime", "number", "one_shot", "boolean", "explosiveness", "number", "randomness", "number", "emitting", "boolean" }, Vector<String>{ "parent_path" });
	add_schema("set_particle_material", "Modifies inner Godot struct material values bounding Godot Variant types for particles.",
			Vector<String>{ "node_path", "string", "direction", "any", "spread", "number", "initial_velocity_min", "number", "initial_velocity_max", "number", "gravity", "any", "scale_min", "number", "scale_max", "number", "color", "string", "emission_shape", "string", "emission_sphere_radius", "number", "emission_box_extents", "any", "emission_ring_radius", "number", "emission_ring_inner_radius", "number", "emission_ring_height", "number" }, Vector<String>{ "node_path" });
	add_schema("set_particle_color_gradient", "Maps linear stop structs against color configurations into a proper GradientTexture1D resource natively.",
			Vector<String>{ "node_path", "string", "stops", "array" }, Vector<String>{ "node_path", "stops" });
	add_schema("apply_particle_preset", "Wraps multiple set calls interpolating variables representing default high quality VFX presets.",
			Vector<String>{ "node_path", "string", "preset", "string" }, Vector<String>{ "node_path", "preset" });
	add_schema("get_particle_info", "Interrogates godot nodes returning hierarchical configuration state back to the MCP stream.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });

	// Physics Tools
	current_category = "physics_tools";
	is_core = false;
	add_schema("setup_collision", "Spawns and attaches native 2D/3D collision boundaries onto physical rigid bodies dynamically.",
			Vector<String>{ "node_path", "string", "shape", "string", "dimension", "string", "width", "number", "height", "number", "depth", "number", "radius", "number", "disabled", "boolean", "one_way_collision", "boolean" }, Vector<String>{ "node_path", "shape" });
	add_schema("set_physics_layers", "Controls implicit bitmask configurations assigning node interaction overlap flags natively.",
			Vector<String>{ "node_path", "string", "layer", "number", "mask", "number" }, Vector<String>{ "node_path" });
	add_schema("get_physics_layers", "Reads intrinsic properties parsing bitmask configurations exposing layers visually.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });
	add_schema("add_raycast", "Instantiates native Godot RayCast queries pointing into logical coordinate targets asynchronously bounds.",
			Vector<String>{ "parent_path", "string", "name", "string", "dimension", "string", "target_position", "object", "enabled", "boolean", "collision_mask", "number" }, Vector<String>{ "parent_path" });
	add_schema("setup_physics_body", "Allocates pure native Object implementations defining Physics bodies bounds (Area, Character, Rigid).",
			Vector<String>{ "parent_path", "string", "body_type", "string", "name", "string", "dimension", "string", "collision_layer", "number", "collision_mask", "number" }, Vector<String>{ "parent_path", "body_type" });
	add_schema("get_collision_info", "Walks a sub-hierarchy scraping Godot shape configurations for runtime representations.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });

	// Scene3D Tools
	current_category = "scene3d_tools";
	is_core = false;
	add_schema("add_mesh_instance", "Mints and injects a MeshInstance3D primitive directly into Godot Scene tree natively.",
			Vector<String>{ "parent_path", "string", "name", "string", "mesh_type", "string", "mesh_file", "string", "mesh_properties", "object", "position", "any", "rotation", "any", "scale", "any" }, Vector<String>{ "parent_path" });
	add_schema("setup_lighting", "Configures high performance Godot lights (SpotLight3D, OmniLight3D, DirectionalLight3D) dynamically into the tree.",
			Vector<String>{ "parent_path", "string", "name", "string", "light_type", "string", "preset", "string", "color", "any", "energy", "number", "shadows", "boolean", "range", "number", "attenuation", "number", "spot_angle", "number", "spot_angle_attenuation", "number", "position", "any", "rotation", "any" }, Vector<String>{ "parent_path" });
	add_schema("set_material_3d", "Assigns and computes real-time StandardMaterial3D configurations over Godot primitive and loaded meshes.",
			Vector<String>{ "node_path", "string", "surface_index", "number", "albedo_color", "any", "albedo_texture", "string", "metallic", "number", "roughness", "number", "metallic_texture", "string", "roughness_texture", "string", "normal_texture", "string", "emission", "any", "emission_color", "any", "emission_energy", "number", "emission_texture", "string", "transparency", "any", "cull_mode", "any" }, Vector<String>{ "node_path" });
	add_schema("setup_environment", "Allocates rendering environments over Godot bindings (SSAO, SSR, SDFGI, Glow, Fog).",
			Vector<String>{ "parent_path", "string", "name", "string", "node_path", "string", "background_mode", "string", "background_color", "any", "sky", "object", "ambient_light_color", "any", "ambient_light_energy", "number", "ambient_light_source", "any", "tonemap_mode", "any", "tonemap_exposure", "number", "tonemap_white", "number", "fog_enabled", "boolean", "fog_light_color", "any", "fog_density", "number", "fog_light_energy", "number", "glow_enabled", "boolean", "glow_intensity", "number", "glow_strength", "number", "glow_bloom", "number", "ssao_enabled", "boolean", "ssao_radius", "number", "ssao_intensity", "number", "ssr_enabled", "boolean", "ssr_max_steps", "number", "ssr_fade_in", "number", "ssr_fade_out", "number", "sdfgi_enabled", "boolean" }, Vector<String>{ "parent_path" });
	add_schema("setup_camera_3d", "Allocates viewport Camera3D projection mapping native structural properties inside godot runtime.",
			Vector<String>{ "parent_path", "string", "name", "string", "node_path", "string", "projection", "string", "fov", "number", "size", "number", "near", "number", "far", "number", "cull_mask", "number", "current", "boolean", "position", "any", "rotation", "any", "look_at", "any", "environment_path", "string" }, Vector<String>{ "parent_path" });
	add_schema("add_gridmap", "Mints and instantiates Godot high performance GridMap bounding memory grids locally.",
			Vector<String>{ "parent_path", "string", "name", "string", "node_path", "string", "mesh_library_path", "string", "cell_size", "any", "position", "any", "cells", "array" }, Vector<String>{ "parent_path" });

	// Shader Tools
	current_category = "shader_tools";
	is_core = false;
	add_schema("create_shader", "Mints blank Godot Shader files injecting standard structure for canvas_item, script, and spatial types.",
			Vector<String>{ "path", "string", "content", "string", "shader_type", "string" }, Vector<String>{ "path" });
	add_schema("read_shader", "Loads string source buffers straight out from .gdshader files dynamically mapped in file system.",
			Vector<String>{ "path", "string" }, Vector<String>{ "path" });
	add_schema("edit_shader", "Provides file patching replacing code strings directly inside the Shader Resource loading path.",
			Vector<String>{ "path", "string", "content", "string", "replacements", "array" }, Vector<String>{ "path" });
	add_schema("assign_shader_material", "Mints native generic ShaderMaterial instances binding active .gdshader onto target nodes.",
			Vector<String>{ "node_path", "string", "shader_path", "string" }, Vector<String>{ "node_path", "shader_path" });
	add_schema("set_shader_param", "Reflective native evaluation bounding GDScript variable evaluation parsing shader uniforms natively.",
			Vector<String>{ "node_path", "string", "param", "string", "value", "any" }, Vector<String>{ "node_path", "param" });
	add_schema("get_shader_params", "Extracts Godot shader variables directly from Inspector metadata representations natively.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });

	// Theme Tools
	current_category = "theme_tools";
	is_core = false;
	add_schema("create_theme", "Generates a new Theme resource struct caching visual configuration layouts natively.",
			Vector<String>{ "path", "string", "default_font_size", "number" }, Vector<String>{ "path" });
	add_schema("set_control_theme_color", "Injects property mappings explicitly setting color maps inside control nodes.",
			Vector<String>{ "node_path", "string", "name", "string", "color", "string", "theme_type", "string" }, Vector<String>{ "node_path", "name", "color" });
	add_schema("set_theme_constant", "Maps godot uniform parameter spacings assigning values inside active nodes.",
			Vector<String>{ "node_path", "string", "name", "string", "value", "number" }, Vector<String>{ "node_path", "name" });
	add_schema("set_control_theme_font_size", "Overrides localized godot font sizes overriding global default assignments.",
			Vector<String>{ "node_path", "string", "name", "string", "size", "number" }, Vector<String>{ "node_path", "name" });
	add_schema("set_theme_stylebox", "Constructs standard StyleBoxFlat representations drawing boundaries over Control structures natively.",
			Vector<String>{ "node_path", "string", "name", "string", "bg_color", "string", "border_color", "string", "border_width", "number", "corner_radius", "number", "padding", "number" }, Vector<String>{ "node_path", "name" });
	add_schema("setup_control", "Automates standard Godot UI control flag assignments applying layout layouts.",
			Vector<String>{ "node_path", "string", "anchor_preset", "string", "min_size", "string", "size_flags_h", "string", "size_flags_v", "string", "margins", "object", "separation", "number", "grow_h", "string", "grow_v", "string" }, Vector<String>{ "node_path" });
	add_schema("get_theme_info", "Interrogates godot Inspector bindings wrapping metadata and dynamic lists natively.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });

	// TileMap Tools
	current_category = "tilemap_tools";
	is_core = false;
	add_schema("tilemap_set_cell", "Sets physical cell identifiers onto Grid memory iterations inside TileMapLayers natively.",
			Vector<String>{ "node_path", "string", "x", "number", "y", "number", "source_id", "number", "atlas_x", "number", "atlas_y", "number", "alternative", "number" }, Vector<String>{ "node_path" });
	add_schema("tilemap_fill_rect", "Calculates ranges iterating assignment coordinates spanning bounds uniformly for TileMapLayers natively.",
			Vector<String>{ "node_path", "string", "x1", "number", "y1", "number", "x2", "number", "y2", "number", "source_id", "number", "atlas_x", "number", "atlas_y", "number", "alternative", "number" }, Vector<String>{ "node_path" });
	add_schema("tilemap_get_cell", "Queries active TileSet state identifiers parsing out structural metadata from TileMapLayers natively.",
			Vector<String>{ "node_path", "string", "x", "number", "y", "number" }, Vector<String>{ "node_path" });
	add_schema("tilemap_clear", "Flushes runtime cell structs flushing grids locally for TileMapLayers natively.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });
	add_schema("tilemap_get_info", "Reads memory maps dumping resource state representations across TileSets dynamically natively.",
			Vector<String>{ "node_path", "string" }, Vector<String>{ "node_path" });
	add_schema("tilemap_get_used_cells", "Calculates bounds pulling physical active grid locations via vector pointers across TileMapLayers natively.",
			Vector<String>{ "node_path", "string", "max_count", "number" }, Vector<String>{ "node_path" });

	// Analysis Tools
	current_category = "analysis_tools";
	is_core = false;
	add_schema("find_unused_resources", "Traverses physical file system mapping orphaned godot assets unused by .tscn scenes automatically.",
			Vector<String>{ "path", "string", "include_addons", "boolean" }, Vector<String>{});
	add_schema("analyze_signal_flow", "Dumps all complex recursive callable/signal graphs connecting dynamically natively.",
			Vector<String>{}, Vector<String>{});
	add_schema("analyze_scene_complexity", "Counts bounds estimating tree node/logic allocation and depths.",
			Vector<String>{ "path", "string" }, Vector<String>{});
	add_schema("find_script_references", "Executes strict text boundary parses across source representations verifying usage paths.",
			Vector<String>{ "query", "string", "path", "string", "include_addons", "boolean" }, Vector<String>{ "query" });
	add_schema("detect_circular_dependencies", "Detects DFS recursion cycles detecting deep graph violations dynamically.",
			Vector<String>{ "path", "string", "include_addons", "boolean" }, Vector<String>{});
	add_schema("get_project_statistics", "Calculates physical byte limits checking file distribution and global dependencies.",
			Vector<String>{ "path", "string", "include_addons", "boolean" }, Vector<String>{});

#ifdef MODULE_AUTOWORK_ENABLED
	// Autowork Tools
	current_category = "autowork_tools";
	is_core = false;
	add_schema("autowork_run_all_tests", "Recursively traverses and executes all autowork test suites natively returning structured passing/failure statistics.",
			Vector<String>{}, Vector<String>{});
	add_schema("autowork_run_tests_in_directory", "Recursively finds and executes all Godot autowork unit tests inside a given directory, returning formatted results.",
			Vector<String>{ "directory_path", "string" }, Vector<String>{ "directory_path" });
	add_schema("autowork_run_test_script", "Executes an exact test script natively against the runtime test suite framework evaluating state.",
			Vector<String>{ "script_path", "string" }, Vector<String>{ "script_path" });
	add_schema("autowork_run_test_by_name", "Performs regex lookup isolating explicit test pattern function names universally across suites for debugging single logic instances.",
			Vector<String>{ "test_name", "string" }, Vector<String>{ "test_name" });
#endif

	return tools;
}

Dictionary JustAMCPToolExecutor::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	Dictionary result;

	if (!scene_tools || !resource_tools || !animation_tools || !project_tools || !profiling_tools || !export_tools || !batch_tools || !script_tools || !node_tools || !audio_tools || !input_tools || !particle_tools || !physics_tools || !scene_3d_tools || !shader_tools || !theme_tools || !tilemap_tools || !analysis_tools) {
		result["ok"] = false;
		result["error"] = "Tools not initialized";
		return result;
	}

	String internal_name = p_tool_name;
	if (internal_name.begins_with("blazium_")) {
		internal_name = internal_name.substr(8);
	}
	String full_name = "blazium_" + internal_name;

	// Runtime Tool Payload Validation
	Array schemas = get_tool_schemas(false, false);
	Dictionary target_schema;
	bool tool_found = false;
	for (int i = 0; i < schemas.size(); i++) {
		Dictionary schema_entry = schemas[i];
		if (String(schema_entry["name"]) == full_name) {
			target_schema = schema_entry;
			tool_found = true;
			break;
		}
	}

	if (!tool_found) {
		Dictionary err;
		err["code"] = -32601;
		err["message"] = "Tool not found or disabled: " + p_tool_name;
		result["ok"] = false;
		result["error"] = err;
		return result;
	}

	if (target_schema.has("inputSchema")) {
		Dictionary inputSchema = target_schema["inputSchema"];
		if (inputSchema.has("required")) {
			Array req = inputSchema["required"];
			for (int i = 0; i < req.size(); i++) {
				String req_arg = req[i];
				if (!p_args.has(req_arg)) {
					Dictionary err;
					err["code"] = -32602;
					err["message"] = "Missing required parameter for tool " + p_tool_name + ": " + req_arg;
					result["ok"] = false;
					result["error"] = err;
					return result;
				}
			}
		}

		if (inputSchema.has("properties")) {
			Dictionary props = inputSchema["properties"];
			Array prop_keys = props.keys();
			for (int i = 0; i < prop_keys.size(); i++) {
				String key = prop_keys[i];
				if (!p_args.has(key)) {
					continue;
				}
				Dictionary prop_def = props[key];
				if (prop_def.has("type")) {
					String exp_type = prop_def["type"];
					Variant val = p_args[key];
					bool valid = true;
					if (exp_type == "string" && val.get_type() != Variant::STRING) {
						valid = false;
					} else if (exp_type == "number" && val.get_type() != Variant::INT && val.get_type() != Variant::FLOAT) {
						valid = false;
					} else if (exp_type == "boolean" && val.get_type() != Variant::BOOL) {
						valid = false;
					} else if (exp_type == "object" && val.get_type() != Variant::DICTIONARY) {
						valid = false;
					} else if (exp_type == "array") {
						if (val.get_type() != Variant::ARRAY && val.get_type() != Variant::PACKED_STRING_ARRAY && val.get_type() != Variant::PACKED_INT32_ARRAY && val.get_type() != Variant::PACKED_INT64_ARRAY && val.get_type() != Variant::PACKED_FLOAT32_ARRAY && val.get_type() != Variant::PACKED_FLOAT64_ARRAY && val.get_type() != Variant::PACKED_BYTE_ARRAY) {
							valid = false;
						}
					}

					if (!valid) {
						Dictionary err;
						err["code"] = -32602;
						err["message"] = "Invalid type for parameter '" + key + "'. Expected " + exp_type + ".";
						result["ok"] = false;
						result["error"] = err;
						return result;
					}
				}
			}
		}
	}

	// Meta Tools
	if (internal_name == "search_tools") {
		String query = p_args.get("query", "");
		Array all_schemas = get_tool_schemas(false, true); // true = bypass ProjectSettings UI filters
		Array matched;
		for (int i = 0; i < all_schemas.size(); i++) {
			Dictionary schema = all_schemas[i];
			String name = schema["name"];
			String desc = schema["description"];
			if (query.is_empty() || name.containsn(query) || desc.containsn(query)) {
				matched.push_back(schema);
			}
		}
		result["ok"] = true;
		result["result"] = matched;
		return result;
	}
	if (internal_name == "execute_tool") {
		String target_tool = p_args.get("tool_name", "");
		Dictionary target_args = p_args.get("arguments", Dictionary());
		return execute_tool(target_tool, target_args);
	}

	// Project Tool Routes
	if (internal_name == "get_project_info") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_filesystem_tree") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "search_files") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "search_in_files") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_project_settings") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_project_setting") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "uid_to_project_path") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "project_path_to_uid") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_autoload") {
		return project_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "remove_autoload") {
		return project_tools->execute_tool(internal_name, p_args);
	}

	// Profiling Tool Routes
	if (internal_name == "get_performance_monitors") {
		return profiling_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_editor_performance") {
		return profiling_tools->execute_tool(internal_name, p_args);
	}

	// Export Tool Routes
	if (internal_name == "list_export_presets") {
		return export_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "export_project") {
		return export_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_export_info") {
		return export_tools->execute_tool(internal_name, p_args);
	}

	// Batch Tool Routes
	if (internal_name == "find_nodes_by_type") {
		return batch_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "find_signal_connections") {
		return batch_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "batch_set_property") {
		return batch_tools->execute_tool(internal_name, p_args);
	}

#ifdef MODULE_AUTOWORK_ENABLED
	// Autowork Tool Routes
	if (internal_name == "autowork_run_all_tests") {
		return autowork_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "autowork_run_tests_in_directory") {
		return autowork_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "autowork_run_test_script") {
		return autowork_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "autowork_run_test_by_name") {
		return autowork_tools->execute_tool(internal_name, p_args);
	}
#endif
	if (internal_name == "find_node_references") {
		return batch_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "cross_scene_set_property") {
		return batch_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_scene_dependencies") {
		return batch_tools->execute_tool(internal_name, p_args);
	}

	// Script Tool Routes
	if (internal_name == "list_scripts") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "read_script") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "create_script") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "edit_script") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "attach_script") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_open_scripts") {
		return script_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "validate_script") {
		return script_tools->execute_tool(internal_name, p_args);
	}

	// Node Tool Routes
	if (internal_name == "node_add") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_delete") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_duplicate") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_move") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_update_property") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_get_properties") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_add_resource") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_set_anchor_preset") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_rename") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_connect_signal") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_disconnect_signal") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_get_groups") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_set_groups") {
		return node_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "node_find_in_group") {
		return node_tools->execute_tool(internal_name, p_args);
	}

	// Audio Tool Routes
	if (internal_name == "get_audio_bus_layout") {
		return audio_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_audio_bus") {
		return audio_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_audio_bus") {
		return audio_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_audio_bus_effect") {
		return audio_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_audio_player") {
		return audio_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_audio_info") {
		return audio_tools->execute_tool(internal_name, p_args);
	}

	// Input Tool Routes
	if (internal_name == "simulate_key") {
		return input_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "simulate_mouse_click") {
		return input_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "simulate_mouse_move") {
		return input_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "simulate_action") {
		return input_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "simulate_sequence") {
		return input_tools->execute_tool(internal_name, p_args);
	}

	// Particle Tool Routes
	if (internal_name == "create_particles") {
		return particle_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_particle_material") {
		return particle_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_particle_color_gradient") {
		return particle_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "apply_particle_preset") {
		return particle_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_particle_info") {
		return particle_tools->execute_tool(internal_name, p_args);
	}

	// Physics Tool Routes
	if (internal_name == "setup_collision") {
		return physics_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_physics_layers") {
		return physics_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_physics_layers") {
		return physics_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_raycast") {
		return physics_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "setup_physics_body") {
		return physics_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_collision_info") {
		return physics_tools->execute_tool(internal_name, p_args);
	}

	// Scene3D Tool Routes
	if (internal_name == "add_mesh_instance") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "setup_lighting") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_material_3d") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "setup_environment") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "setup_camera_3d") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "add_gridmap") {
		return scene_3d_tools->execute_tool(internal_name, p_args);
	}

	// Shader Tool Routes
	if (internal_name == "create_shader") {
		return shader_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "read_shader") {
		return shader_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "edit_shader") {
		return shader_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "assign_shader_material") {
		return shader_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_shader_param") {
		return shader_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_shader_params") {
		return shader_tools->execute_tool(internal_name, p_args);
	}

	// Theme Tool Routes
	if (internal_name == "create_theme") {
		return theme_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_control_theme_color") {
		return theme_tools->execute_tool("set_theme_color", p_args);
	}
	if (internal_name == "set_theme_constant") {
		return theme_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "set_control_theme_font_size") {
		return theme_tools->execute_tool("set_theme_font_size", p_args);
	}
	if (internal_name == "set_theme_stylebox") {
		return theme_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "setup_control") {
		return theme_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_theme_info") {
		return theme_tools->execute_tool(internal_name, p_args);
	}

	// TileMap Tool Routes
	if (internal_name == "tilemap_set_cell") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "tilemap_fill_rect") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "tilemap_get_cell") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "tilemap_clear") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "tilemap_get_info") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "tilemap_get_used_cells") {
		return tilemap_tools->execute_tool(internal_name, p_args);
	}

	// Analysis Tool Routes
	if (internal_name == "find_unused_resources") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "analyze_signal_flow") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "analyze_scene_complexity") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "find_script_references") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "detect_circular_dependencies") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}
	if (internal_name == "get_project_statistics") {
		return analysis_tools->execute_tool(internal_name, p_args);
	}

	// Route based on standard tool map from GDScript
	if (internal_name == "create_scene") {
		return scene_tools->create_scene(p_args);
	}
	if (internal_name == "list_scene_nodes") {
		return scene_tools->list_scene_nodes(p_args);
	}
	if (internal_name == "add_node") {
		return scene_tools->add_node(p_args);
	}
	if (internal_name == "delete_node") {
		return scene_tools->delete_node(p_args);
	}
	if (internal_name == "duplicate_node") {
		return scene_tools->duplicate_node(p_args);
	}
	if (internal_name == "reparent_node") {
		return scene_tools->reparent_node(p_args);
	}
	if (internal_name == "set_node_properties") {
		return scene_tools->set_node_properties(p_args);
	}
	if (internal_name == "get_node_properties") {
		return scene_tools->get_node_properties(p_args);
	}
	if (internal_name == "load_sprite") {
		return scene_tools->load_sprite(p_args);
	}
	if (internal_name == "save_scene") {
		return scene_tools->save_scene(p_args);
	}
	if (internal_name == "connect_signal") {
		return scene_tools->connect_signal(p_args);
	}
	if (internal_name == "disconnect_signal") {
		return scene_tools->disconnect_signal(p_args);
	}
	if (internal_name == "list_connections") {
		return scene_tools->list_connections(p_args);
	}

	if (internal_name == "create_resource") {
		return resource_tools->create_resource(p_args);
	}
	if (internal_name == "modify_resource") {
		return resource_tools->modify_resource(p_args);
	}
	if (internal_name == "create_material") {
		return resource_tools->create_material(p_args);
	}
	if (internal_name == "create_shader") {
		return resource_tools->create_shader(p_args);
	}
	if (internal_name == "create_tileset") {
		return resource_tools->create_tileset(p_args);
	}
	if (internal_name == "set_tilemap_cells") {
		return resource_tools->set_tilemap_cells(p_args);
	}
	if (internal_name == "set_theme_resource_color") {
		return resource_tools->set_theme_color(p_args);
	}
	if (internal_name == "set_theme_resource_font_size") {
		return resource_tools->set_theme_font_size(p_args);
	}
	if (internal_name == "apply_theme_shader") {
		return resource_tools->apply_theme_shader(p_args);
	}

	if (internal_name == "create_animation") {
		return animation_tools->create_animation(p_args);
	}
	if (internal_name == "add_animation_track") {
		return animation_tools->add_animation_track(p_args);
	}
	if (internal_name == "create_animation_tree") {
		return animation_tools->create_animation_tree(p_args);
	}
	if (internal_name == "add_animation_state") {
		return animation_tools->add_animation_state(p_args);
	}
	if (internal_name == "connect_animation_states") {
		return animation_tools->connect_animation_states(p_args);
	}
	if (internal_name == "create_navigation_region") {
		return animation_tools->create_navigation_region(p_args);
	}
	if (internal_name == "create_navigation_agent") {
		return animation_tools->create_navigation_agent(p_args);
	}

	result["ok"] = false;
	result["error"] = "Unknown tool: " + p_tool_name;
	return result;
}

#endif // TOOLS_ENABLED
