/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "justamcp_runtime.h"

#ifdef TOOLS_ENABLED
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "editor/editor_settings.h"
#include "justamcp_editor_plugin.h"
#include "justamcp_server.h"
#include "tools/justamcp_analysis_tools.h"
#include "tools/justamcp_animation_tools.h"
#include "tools/justamcp_audio_tools.h"
#include "tools/justamcp_batch_tools.h"
#include "tools/justamcp_export_tools.h"
#include "tools/justamcp_input_tools.h"
#include "tools/justamcp_node_tools.h"
#include "tools/justamcp_particle_tools.h"
#include "tools/justamcp_physics_tools.h"
#include "tools/justamcp_profiling_tools.h"
#include "tools/justamcp_project_tools.h"
#include "tools/justamcp_resource_tools.h"
#include "tools/justamcp_scene_3d_tools.h"
#include "tools/justamcp_scene_tools.h"
#include "tools/justamcp_script_tools.h"
#include "tools/justamcp_shader_tools.h"
#include "tools/justamcp_theme_tools.h"
#include "tools/justamcp_tilemap_tools.h"
#include "tools/justamcp_tool_executor.h"
#endif

#ifndef TOOLS_ENABLED
#include "core/config/project_settings.h"
#include "core/os/os.h"
#endif

static bool _is_justamcp_enabled() {
	if (OS::get_singleton()->get_cmdline_args().find("--enable-mcp")) {
		return true;
	}

	bool use_project_override = false;
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("blazium/justamcp/override_editor_settings")) {
		use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");
	}

#ifdef TOOLS_ENABLED
	if (use_project_override || !EditorSettings::get_singleton()) {
#else
	{
#endif
		if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("blazium/justamcp/server_enabled")) {
			return GLOBAL_GET("blazium/justamcp/server_enabled");
		}
#ifdef TOOLS_ENABLED
	} else if (EditorSettings::get_singleton() && EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_enabled")) {
		return EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_enabled");
#endif
	}

	return false;
}

void initialize_justamcp_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(JustAMCPRuntime);
		if (_is_justamcp_enabled()) {
			JustAMCPRuntime *runtime = memnew(JustAMCPRuntime);
			Engine::get_singleton()->add_singleton(Engine::Singleton("JustAMCPRuntime", runtime));
		}
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(JustAMCPServer);
		GDREGISTER_CLASS(JustAMCPToolExecutor);
		GDREGISTER_CLASS(JustAMCPSceneTools);
		GDREGISTER_CLASS(JustAMCPResourceTools);
		GDREGISTER_CLASS(JustAMCPAnimationTools);
		GDREGISTER_CLASS(JustAMCPAnalysisTools);
		GDREGISTER_CLASS(JustAMCPAudioTools);
		GDREGISTER_CLASS(JustAMCPBatchTools);
		GDREGISTER_CLASS(JustAMCPExportTools);
		GDREGISTER_CLASS(JustAMCPInputTools);
		GDREGISTER_CLASS(JustAMCPNodeTools);
		GDREGISTER_CLASS(JustAMCPParticleTools);
		GDREGISTER_CLASS(JustAMCPPhysicsTools);
		GDREGISTER_CLASS(JustAMCPProfilingTools);
		GDREGISTER_CLASS(JustAMCPProjectTools);
		GDREGISTER_CLASS(JustAMCPScene3DTools);
		GDREGISTER_CLASS(JustAMCPScriptTools);
		GDREGISTER_CLASS(JustAMCPShaderTools);
		GDREGISTER_CLASS(JustAMCPThemeTools);
		GDREGISTER_CLASS(JustAMCPTileMapTools);
	}
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (_is_justamcp_enabled()) {
			EditorPlugins::add_by_type<JustAMCPEditorPlugin>();
		}
	}
#endif
}

void uninitialize_justamcp_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (Engine::get_singleton()->has_singleton("JustAMCPRuntime")) {
			if (JustAMCPRuntime *runtime = Object::cast_to<JustAMCPRuntime>(Engine::get_singleton()->get_singleton_object("JustAMCPRuntime"))) {
				Engine::get_singleton()->remove_singleton("JustAMCPRuntime");
				memdelete(runtime);
			}
		}
	}
}
