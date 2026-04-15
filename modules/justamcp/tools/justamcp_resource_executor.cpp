/**************************************************************************/
/*  justamcp_resource_executor.cpp                                        */
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

#include "justamcp_resource_executor.h"
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "resources/justamcp_resource_project_file.h"
#include "resources/justamcp_resource_system_logs.h"

void JustAMCPResourceExecutor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("list_resources", "cursor"), &JustAMCPResourceExecutor::list_resources, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("list_resource_templates", "cursor"), &JustAMCPResourceExecutor::list_resource_templates, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("read_resource", "uri"), &JustAMCPResourceExecutor::read_resource);
	ClassDB::bind_method(D_METHOD("add_resource", "resource"), &JustAMCPResourceExecutor::add_resource);
}

JustAMCPResourceExecutor::JustAMCPResourceExecutor() {
	add_resource(memnew(JustAMCPResourceSystemLogs));
	add_resource(memnew(JustAMCPResourceProjectFile));
}

JustAMCPResourceExecutor::~JustAMCPResourceExecutor() {}

void JustAMCPResourceExecutor::add_resource(const Ref<JustAMCPResource> &p_resource) {
	if (p_resource.is_valid()) {
		registered_resources.push_back(p_resource);
	}
}

Dictionary JustAMCPResourceExecutor::list_resources(const String &cursor) {
	Dictionary result;
	Array resources;

	for (int i = 0; i < registered_resources.size(); i++) {
		if (registered_resources[i].is_valid() && !registered_resources[i]->is_template()) {
			resources.push_back(registered_resources[i]->get_schema());
		}
	}

	result["resources"] = resources;
	return result;
}

Dictionary JustAMCPResourceExecutor::list_resource_templates(const String &cursor) {
	Dictionary result;
	Array templates;

	for (int i = 0; i < registered_resources.size(); i++) {
		if (registered_resources[i].is_valid() && registered_resources[i]->is_template()) {
			templates.push_back(registered_resources[i]->get_schema());
		}
	}

	result["resourceTemplates"] = templates;
	return result;
}

Dictionary JustAMCPResourceExecutor::read_resource(const String &p_uri) {
	for (int i = 0; i < registered_resources.size(); i++) {
		if (registered_resources[i].is_valid()) {
			bool match = false;
			if (registered_resources[i]->is_template()) {
				// Naive match for "res://" if the template is "res://{path}"
				String tmpl = registered_resources[i]->get_uri();
				if (tmpl.begins_with("res://") && p_uri.begins_with("res://")) {
					match = true;
				}
			} else if (registered_resources[i]->get_uri() == p_uri) {
				match = true;
			}

			if (match) {
				Dictionary res = registered_resources[i]->read_resource(p_uri);
				if (res.has("ok") && bool(Variant(res["ok"]))) {
					return res; // Successfully read!
				}
			}
		}
	}

	Dictionary result;
	result["ok"] = false;
	result["error_code"] = -32602;
	result["error"] = "Unknown resource URI: " + p_uri;
	return result;
}

#endif // TOOLS_ENABLED
