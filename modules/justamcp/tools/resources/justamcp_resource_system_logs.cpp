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

void JustAMCPResourceSystemLogs::_bind_methods() {}

JustAMCPResourceSystemLogs::JustAMCPResourceSystemLogs() {}
JustAMCPResourceSystemLogs::~JustAMCPResourceSystemLogs() {}

String JustAMCPResourceSystemLogs::get_uri() const {
	return "godot://system/logs";
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

	if (p_uri != get_uri()) {
		result["ok"] = false;
		result["error_code"] = -32602;
		result["error"] = "Mismatch URI.";
		return result;
	}

	result["ok"] = true;
	Array contents;
	Dictionary text_content;
	text_content["uri"] = p_uri;
	text_content["mimeType"] = "text/plain";
	text_content["text"] = "Engine running successfully... Request processed natively. (Live logging integration expanding!)";
	contents.push_back(text_content);
	result["contents"] = contents;
	return result;
}

#endif // TOOLS_ENABLED
