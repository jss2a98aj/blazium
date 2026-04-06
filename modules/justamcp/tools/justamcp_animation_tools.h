/**************************************************************************/
/*  justamcp_animation_tools.h                                            */
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

class JustAMCPAnimationTools : public Object {
	GDCLASS(JustAMCPAnimationTools, Object);

private:
	JustAMCPEditorPlugin *editor_plugin = nullptr;

protected:
	static void _bind_methods();

public:
	void set_editor_plugin(JustAMCPEditorPlugin *p_plugin) { editor_plugin = p_plugin; }

	Dictionary create_animation(const Dictionary &p_args);
	Dictionary add_animation_track(const Dictionary &p_args);
	Dictionary create_animation_tree(const Dictionary &p_args);
	Dictionary add_animation_state(const Dictionary &p_args);
	Dictionary connect_animation_states(const Dictionary &p_args);
	Dictionary create_navigation_region(const Dictionary &p_args);
	Dictionary create_navigation_agent(const Dictionary &p_args);

private:
	String _ensure_res_path(const String &p_path);
	void _refresh_and_reload(const String &p_scene_path);
	void _refresh_filesystem();
	void _reload_scene_in_editor(const String &p_scene_path);

	Array _load_scene(const String &p_scene_path);
	Dictionary _save_scene(Node *p_scene_root, const String &p_scene_path);
	Node *_find_node(Node *p_root, const String &p_path);

	Variant _parse_value(const Variant &p_value);
	Variant _parse_json_maybe(const Variant &p_value);
	Array _parse_method_args(const Array &p_raw_args);

	Ref<Resource> _get_default_animation_library(Node *p_player);
	Ref<Resource> _get_state_machine(Node *p_anim_tree, const String &p_state_machine_path = "");

public:
	JustAMCPAnimationTools();
	~JustAMCPAnimationTools();
};

#endif // TOOLS_ENABLED
