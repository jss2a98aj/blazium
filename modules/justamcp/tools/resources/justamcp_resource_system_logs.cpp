/**************************************************************************/
/*  justamcp_resource_system_logs.cpp                                     */
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

#include "justamcp_resource_system_logs.h"
#include "../../justamcp_server.h"

void JustAMCPResourceSystemLogs::_bind_methods() {}

JustAMCPResourceSystemLogs::JustAMCPResourceSystemLogs() {}
JustAMCPResourceSystemLogs::~JustAMCPResourceSystemLogs() {}

String JustAMCPResourceSystemLogs::get_uri() const {
	return "blazium://system/logs";
}

String JustAMCPResourceSystemLogs::get_name() const {
	return "Engine System Logs";
}

bool JustAMCPResourceSystemLogs::is_template() const {
	return false;
}

Dictionary JustAMCPResourceSystemLogs::get_schema() const {
	Dictionary resource;
	resource["uri"] = get_uri();
	resource["name"] = get_name();
	resource["mimeType"] = "text/plain";
	resource["description"] = "Recent internal engine messages mapping to the active process.";
	return resource;
}

Dictionary JustAMCPResourceSystemLogs::read_resource(const String &p_uri) {
	Dictionary result;

	if (p_uri != get_uri() && p_uri != "godot://system/logs") {
		result["ok"] = false;
		result["error_code"] = -32602;
		result["error"] = "Mismatch URI.";
		return result;
	}

	result["ok"] = true;
	Array contents;
	Dictionary text_content;
	text_content["uri"] = p_uri;
	String full_log = "";
	if (JustAMCPServer::get_singleton()) {
		Vector<String> engine_logs = JustAMCPServer::get_singleton()->get_engine_logs();
		for (int i = 0; i < engine_logs.size(); i++) {
			full_log += engine_logs[i] + "\n";
		}
		if (full_log.is_empty()) {
			full_log = "Log is empty or hasn't started capturing yet.";
		}
	} else {
		full_log = "JustAMCP System Log Capture Not Available.";
	}

	text_content["mimeType"] = "text/plain";
	text_content["text"] = full_log;
	contents.push_back(text_content);

	result["contents"] = contents;
	return result;
}

#endif // TOOLS_ENABLED
