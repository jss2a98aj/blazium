/**************************************************************************/
/*  justamcp_resource_executor.h                                          */
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

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "resources/justamcp_resource.h"

class Node;

class JustAMCPResourceExecutor : public Object {
	GDCLASS(JustAMCPResourceExecutor, Object);

	Vector<Ref<JustAMCPResource>> registered_resources;

	Dictionary _make_resource_schema(const String &p_uri, const String &p_name, const String &p_description, const String &p_mime_type = "application/json") const;
	Dictionary _make_resource_template_schema(const String &p_uri_template, const String &p_name, const String &p_description, const String &p_mime_type = "application/json") const;
	Dictionary _make_json_contents(const String &p_uri, const Dictionary &p_payload) const;
	Dictionary _make_text_contents(const String &p_uri, const String &p_text, const String &p_mime_type = "text/markdown") const;
	Dictionary _make_json_error_payload(const String &p_uri, const String &p_error) const;
	String _canonicalize_resource_uri(const String &p_uri) const;
	Dictionary _read_blazium_resource(const String &p_uri) const;
	Dictionary _read_guide_resource(const String &p_uri) const;
	Dictionary _read_node_resource(const String &p_uri, const String &p_suffix) const;
	Dictionary _read_script_resource(const String &p_uri) const;
	Dictionary _serialize_node_brief(Node *p_node, Node *p_root) const;
	Variant _serialize_value(const Variant &p_value) const;
	Node *_get_edited_root() const;
	Node *_find_node_by_resource_path(const String &p_path) const;
	void _append_node_tree(Node *p_node, Node *p_root, int p_depth, int p_max_depth, Array &r_nodes) const;
	void _collect_materials(const String &p_path, Array &r_materials) const;

protected:
	static void _bind_methods();

public:
	static void register_settings();
	void add_resource(const Ref<JustAMCPResource> &p_resource);

	Dictionary list_resources(const String &cursor = "");
	Dictionary list_resource_templates(const String &cursor = "");
	Dictionary read_resource(const String &p_uri);

	JustAMCPResourceExecutor();
	~JustAMCPResourceExecutor();
};

#endif // TOOLS_ENABLED
