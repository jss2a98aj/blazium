/**************************************************************************/
/*  justamcp_project_tools.h                                              */
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

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/main/node.h"

class JustAMCPProjectTools : public Object {
	GDCLASS(JustAMCPProjectTools, Object);

private:
	EditorPlugin *editor_plugin = nullptr;

	Dictionary _get_project_info(const Dictionary &p_params);
	Dictionary _get_filesystem_tree(const Dictionary &p_params);
	Dictionary _scan_directory(const String &p_path, const String &p_filter, int p_max_depth, int p_depth);
	Dictionary _search_files(const Dictionary &p_params);
	void _search_recursive(const String &p_path, const String &p_query, const String &p_file_type, Array &r_matches, int p_max_results);
	Dictionary _search_in_files(const Dictionary &p_params);
	void _search_in_files_recursive(const String &p_path, const String &p_query, const Ref<RefCounted> &p_regex, const String &p_file_type, Array &r_matches, int p_max_results);
	Dictionary _get_project_settings(const Dictionary &p_params);
	Dictionary _set_project_setting(const Dictionary &p_params);
	Dictionary _uid_to_project_path(const Dictionary &p_params);
	Dictionary _project_path_to_uid(const Dictionary &p_params);
	Dictionary _add_autoload(const Dictionary &p_params);
	Dictionary _remove_autoload(const Dictionary &p_params);

protected:
	static void _bind_methods();

public:
	void set_editor_plugin(EditorPlugin *p_plugin) { editor_plugin = p_plugin; }
	Dictionary execute_tool(const String &p_tool_name, const Dictionary &p_args);

	JustAMCPProjectTools();
	~JustAMCPProjectTools();
};
