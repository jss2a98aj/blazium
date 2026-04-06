/**************************************************************************/
/*  justamcp_tool_executor.h                                              */
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

#pragma once

#ifdef TOOLS_ENABLED

#include "core/object/object.h"
#include "scene/main/node.h"

class JustAMCPEditorPlugin;
class JustAMCPAnalysisTools;
class JustAMCPSceneTools;
class JustAMCPResourceTools;
class JustAMCPAnimationTools;
class JustAMCPProjectTools;
class JustAMCPProfilingTools;
class JustAMCPExportTools;
class JustAMCPBatchTools;
class JustAMCPScriptTools;
class JustAMCPNodeTools;
class JustAMCPAudioTools;
class JustAMCPInputTools;
class JustAMCPParticleTools;
class JustAMCPPhysicsTools;
class JustAMCPScene3DTools;
class JustAMCPShaderTools;
class JustAMCPThemeTools;
class JustAMCPTileMapTools;
class JustAMCPAutoworkTools;

class JustAMCPToolExecutor : public Object {
	GDCLASS(JustAMCPToolExecutor, Object);

private:
	JustAMCPEditorPlugin *editor_plugin = nullptr;

	JustAMCPAnalysisTools *analysis_tools = nullptr;
	JustAMCPSceneTools *scene_tools = nullptr;
	JustAMCPResourceTools *resource_tools = nullptr;
	JustAMCPAnimationTools *animation_tools = nullptr;
	JustAMCPProjectTools *project_tools = nullptr;
	JustAMCPProfilingTools *profiling_tools = nullptr;
	JustAMCPExportTools *export_tools = nullptr;
	JustAMCPBatchTools *batch_tools = nullptr;
	JustAMCPScriptTools *script_tools = nullptr;
	JustAMCPNodeTools *node_tools = nullptr;
	JustAMCPAudioTools *audio_tools = nullptr;
	JustAMCPInputTools *input_tools = nullptr;
	JustAMCPParticleTools *particle_tools = nullptr;
	JustAMCPPhysicsTools *physics_tools = nullptr;
	JustAMCPScene3DTools *scene_3d_tools = nullptr;
	JustAMCPShaderTools *shader_tools = nullptr;
	JustAMCPThemeTools *theme_tools = nullptr;
	JustAMCPTileMapTools *tilemap_tools = nullptr;
	JustAMCPAutoworkTools *autowork_tools = nullptr;

	bool initialized = false;

	void _init_tools();

protected:
	static void _bind_methods();

public:
	void set_editor_plugin(JustAMCPEditorPlugin *p_plugin);
	Dictionary execute_tool(const String &p_tool_name, const Dictionary &p_args);

	static void register_tool_settings();
	static Array get_tool_schemas(bool p_register_only = false, bool p_ignore_settings = false);

	static Node *test_scene_root;
	static void set_test_scene_root(Node *p_node);
	static Node *get_test_scene_root();

	JustAMCPToolExecutor();
	~JustAMCPToolExecutor();
};

#endif // TOOLS_ENABLED
