/**************************************************************************/
/*  justamcp_server.cpp                                                   */
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

#include "justamcp_server.h"
#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "editor/editor_settings.h"
#include "modules/modules_enabled.gen.h"
#include "tools/justamcp_tool_executor.h"

void JustAMCPServer::_bind_methods() {
	ADD_SIGNAL(MethodInfo("tool_requested", PropertyInfo(Variant::STRING, "request_id"), PropertyInfo(Variant::STRING, "tool_name"), PropertyInfo(Variant::DICTIONARY, "args")));
	ADD_SIGNAL(MethodInfo("server_status_changed", PropertyInfo(Variant::BOOL, "started")));
}

JustAMCPServer::JustAMCPServer() {
}

JustAMCPServer::~JustAMCPServer() {
	_stop_server();
}

void JustAMCPServer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_setup_settings();
#ifdef TOOLS_ENABLED
			if (EditorSettings::get_singleton()) {
				EditorSettings::get_singleton()->connect("settings_changed", callable_mp(this, &JustAMCPServer::_on_settings_changed));
			}
#endif
			if (ProjectSettings::get_singleton()) {
				ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &JustAMCPServer::_on_settings_changed));
			}
			_start_server();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_stop_server();
		} break;
	}
}

void JustAMCPServer::_on_settings_changed() {
#ifdef TOOLS_ENABLED
	bool is_enabled = false;
	bool use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");

	if (use_project_override || !EditorSettings::get_singleton()) {
		is_enabled = GLOBAL_GET("blazium/justamcp/server_enabled");
	} else if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_enabled")) {
		is_enabled = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_enabled");
	}

	// If it's disabled but we are running, stop it
	if (!is_enabled && server_started) {
		_stop_server();
	}
	// If it's enabled but NOT running, start it
	else if (is_enabled && !server_started) {
		_start_server();
	}
	// If it IS enabled and IS running, but the port changed, we might need a restart.
	// We'll leave that to manual restarts for now to prevent spamming server stops.
#endif
}

void JustAMCPServer::_setup_settings() {
#ifdef TOOLS_ENABLED
	EDITOR_DEF_BASIC("blazium/justamcp/server_enabled", false);
	EDITOR_DEF_BASIC("blazium/justamcp/server_port", 6506);
	EDITOR_DEF_BASIC("blazium/justamcp/oauth_enabled", false);
	EDITOR_DEF_BASIC("blazium/justamcp/client_id", "");
	EDITOR_DEF_BASIC("blazium/justamcp/client_secret", "");
	EDITOR_DEF_BASIC("blazium/justamcp/z_mcp_config", "");
	EDITOR_DEF_BASIC("blazium/justamcp/enable_debug_logging", true);
	EDITOR_DEF_BASIC("blazium/justamcp/bind_to_localhost_only", true);
#endif

	// Ensure ProjectSettings equivalents exist for override
	GLOBAL_DEF_BASIC("blazium/justamcp/override_editor_settings", false);
	GLOBAL_DEF_BASIC("blazium/justamcp/server_enabled", false);
	GLOBAL_DEF_BASIC("blazium/justamcp/server_port", 6506);
	GLOBAL_DEF_BASIC("blazium/justamcp/oauth_enabled", false);
	GLOBAL_DEF_BASIC("blazium/justamcp/client_id", String());
	GLOBAL_DEF_BASIC("blazium/justamcp/client_secret", String());
	GLOBAL_DEF_BASIC("blazium/justamcp/z_mcp_config", String());
	GLOBAL_DEF_BASIC("blazium/justamcp/enable_debug_logging", true);
	GLOBAL_DEF_BASIC("blazium/justamcp/bind_to_localhost_only", true);

#ifdef TOOLS_ENABLED
	JustAMCPToolExecutor::register_tool_settings();
#endif
}

void JustAMCPServer::_start_server() {
	if (server_started) {
		return;
	}

	const List<String> &args = OS::get_singleton()->get_cmdline_args();
	for (const String &arg : args) {
		if (arg == "--test" || arg == "--tests" || arg.begins_with("--aw-") ||
				arg == "--help" || arg == "-h" || arg == "/?" || arg == "--version" ||
				arg == "--check-only" || arg.begins_with("--export")) {
			return; // Do not start Server during CLI tools or testing workflows
		}
	}

	bool enabled = false;
	int port = 6506;
	bool bind_to_localhost = true;

#ifdef TOOLS_ENABLED
	bool use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");

	if (use_project_override || !EditorSettings::get_singleton()) {
		enabled = GLOBAL_GET("blazium/justamcp/server_enabled");
		port = GLOBAL_GET("blazium/justamcp/server_port");
		if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("blazium/justamcp/bind_to_localhost_only")) {
			bind_to_localhost = GLOBAL_GET("blazium/justamcp/bind_to_localhost_only");
		}
	} else {
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_enabled")) {
			enabled = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_enabled");
		}
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_port")) {
			port = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_port");
		}
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/bind_to_localhost_only")) {
			bind_to_localhost = EditorSettings::get_singleton()->get_setting("blazium/justamcp/bind_to_localhost_only");
		}
	}
#endif

	if (!enabled) {
		return;
	}

#if defined(MODULE_HTTPSERVER_ENABLED)
	if (HTTPServer::get_singleton()) {
		if (!HTTPServer::get_singleton()->is_listening()) {
			String bind_address = bind_to_localhost ? "127.0.0.1" : "*";
			HTTPServer::get_singleton()->listen(port, bind_address, false);
		}

		HTTPServer::get_singleton()->register_route("GET", "/sse", callable_mp(this, &JustAMCPServer::_handle_sse_connect));
		HTTPServer::get_singleton()->register_route("POST", "/message", callable_mp(this, &JustAMCPServer::_handle_message_post));
		HTTPServer::get_singleton()->register_route("POST", "/mcp", callable_mp(this, &JustAMCPServer::_handle_mcp_stateless_post));

		if (!HTTPServer::get_singleton()->is_connected("sse_connection_opened", callable_mp(this, &JustAMCPServer::_on_sse_connection_opened))) {
			HTTPServer::get_singleton()->connect("sse_connection_opened", callable_mp(this, &JustAMCPServer::_on_sse_connection_opened));
		}

		server_started = true;
		print_line("JustAMCP: Server Activated on port " + itos(port));
		emit_signal("server_status_changed", true);
	}
#else
	ERR_PRINT("HTTPServer module is not enabled! JustAMCP requires it to act as an MCP server.");
#endif
}

void JustAMCPServer::_stop_server() {
	if (!server_started) {
		return;
	}

#if defined(MODULE_HTTPSERVER_ENABLED)
	if (HTTPServer::get_singleton()) {
		HTTPServer::get_singleton()->unregister_route("GET", "/sse");
		HTTPServer::get_singleton()->unregister_route("POST", "/message");
		HTTPServer::get_singleton()->unregister_route("POST", "/mcp");
	}
#endif
	server_started = false;
	print_line("JustAMCP: Server Disabled.");
	emit_signal("server_status_changed", false);
	current_sse_connection_id = -1;
}

#if defined(MODULE_HTTPSERVER_ENABLED)
void JustAMCPServer::_handle_sse_connect(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response) {
	bool debug_logging = GLOBAL_GET("blazium/justamcp/enable_debug_logging");
	if (debug_logging) {
		print_line("JustAMCP: Incoming GET /sse connection attempt...");
	}
	// Optional OAuth validation
#ifdef TOOLS_ENABLED
	bool oauth_enabled = false;
	String required_client_id = "";
	String required_client_secret = "";

	bool use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");

	if (use_project_override || !EditorSettings::get_singleton()) {
		oauth_enabled = GLOBAL_GET("blazium/justamcp/oauth_enabled");
		required_client_id = String(GLOBAL_GET("blazium/justamcp/client_id"));
		required_client_secret = String(GLOBAL_GET("blazium/justamcp/client_secret"));
	} else if (EditorSettings::get_singleton()) {
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/oauth_enabled")) {
			oauth_enabled = EditorSettings::get_singleton()->get_setting("blazium/justamcp/oauth_enabled");
		}
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/client_id")) {
			required_client_id = String(EditorSettings::get_singleton()->get_setting("blazium/justamcp/client_id"));
		}
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/client_secret")) {
			required_client_secret = String(EditorSettings::get_singleton()->get_setting("blazium/justamcp/client_secret"));
		}
	}

	if (oauth_enabled) {
		if (!required_client_id.is_empty() || !required_client_secret.is_empty()) {
			Dictionary headers = p_context->get_headers();

			// Note: Usually the AI sends 'Authorization: Bearer <token>' or similar custom headers.
			// We gracefully respond Unauthorized if we actively required them and didn't find them fitting our basic validation standard.
			// This is a minimal placeholder for basic token checks:
			if (!headers.has("authorization")) {
				ERR_PRINT("JustAMCP: Unauthorized connection attempt.");
				p_response->set_status(401);
				p_response->set_body("Unauthorized - Missing Authorization header");
				return;
			}
		}
	}
#endif

	p_response->start_sse();
}

void JustAMCPServer::_on_sse_connection_opened(int p_connection_id, const String &p_path, const Dictionary &p_headers) {
	if (p_path == "/sse") {
		bool debug_logging = GLOBAL_GET("blazium/justamcp/enable_debug_logging");
		if (debug_logging) {
			print_line("JustAMCP: New SSE connection opened on " + p_path + " (ID: " + itos(p_connection_id) + ")");
		}
		current_sse_connection_id = p_connection_id;

		int port = 6506;
		bool use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");
		if (use_project_override) {
			port = GLOBAL_GET("blazium/justamcp/server_port");
		} else if (EditorSettings::get_singleton() && EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_port")) {
			port = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_port");
		}

		String endpoint_url = "http://127.0.0.1:" + itos(port) + "/message?sessionId=" + itos(p_connection_id);
		HTTPServer::get_singleton()->send_sse_event(p_connection_id, "endpoint", endpoint_url);
	}
}

void JustAMCPServer::_handle_message_post(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response) {
	String body = p_context->get_body();
	if (body.is_empty()) {
		ERR_PRINT("JustAMCP: Received empty message POST body.");
		p_response->set_status(400);
		p_response->set_body("Empty request body");
		return;
	}

	bool debug_logging = GLOBAL_GET("blazium/justamcp/enable_debug_logging");
	if (debug_logging) {
		print_line("JustAMCP: Message POST Received: " + body);
	}

	// We reply HTTP 202 explicitly to acknowledge receipt of the POST buffer natively for MCP.
	p_response->set_status(202);
	p_response->set_body("Accepted");

	// Then handle it via JSON-RPC, emitting the event down our active SSE pipeline!
	Dictionary result = _handle_json_rpc(body, p_response);
	if (!result.is_empty()) {
		_send_sse_message(JSON::stringify(result));
	}
}

void JustAMCPServer::_handle_mcp_stateless_post(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response) {
	String body = p_context->get_body();
	if (body.is_empty()) {
		ERR_PRINT("JustAMCP: Received empty message POST body on /mcp.");
		p_response->set_status(400);
		Dictionary err;
		err["jsonrpc"] = "2.0";
		Dictionary error_dict;
		error_dict["code"] = -32700;
		error_dict["message"] = "Invalid JSON or empty body";
		err["error"] = error_dict;
		p_response->set_json(err);
		return;
	}

	bool debug_logging = GLOBAL_GET("blazium/justamcp/enable_debug_logging");
	if (debug_logging) {
		print_line("JustAMCP: Stateless POST Received on /mcp: " + body);
	}

	Dictionary result = _handle_json_rpc(body, p_response);

	// If the payload returns empty, it's a notification, or we are pending an async tool execution via HTTP.
	// Only return if we haven't already completed the HTTP cycle.
	if (!p_response->is_sent()) {
		if (result.is_empty()) {
			// For stateless JSON-RPC over POST, the spec technically expects a response to tools/call.
			// If we return 202 Accepted, Cursor drops it. We will hold the connection open.
			// But for notifications, 202 is fine. We will distinguish by method.
			String method = "";
			Ref<JSON> json;
			json.instantiate();
			if (json->parse(body) == OK && json->get_data().get_type() == Variant::DICTIONARY) {
				Dictionary data_dict = json->get_data();
				if (data_dict.has("method")) {
					method = String(data_dict["method"]);
				}
			}
			if (method != "tools/call") {
				p_response->set_status(202);
				p_response->set_body("");
			}
		} else {
			p_response->set_status(200);
			p_response->set_json(result);
		}
	}
}

Dictionary JustAMCPServer::_handle_json_rpc(const String &p_body, Ref<HTTPResponse> p_response) {
	Ref<JSON> json;
	json.instantiate();

	if (json->parse(p_body) != OK) {
		ERR_PRINT("JustAMCP: Failed to parse MCP JSON-RPC Payload: " + p_body);
		Dictionary err;
		err["jsonrpc"] = "2.0";
		Dictionary error_dict;
		error_dict["code"] = -32700;
		error_dict["message"] = "Invalid JSON";
		err["error"] = error_dict;
		return err;
	}

	Dictionary payload = json->get_data();
	if (!payload.has("method")) {
		return Dictionary(); // Ignore non-requests
	}

	String method = payload["method"];
	String request_id = payload.has("id") ? String(Variant(payload["id"])) : "";

	bool debug_logging = GLOBAL_GET("blazium/justamcp/enable_debug_logging");
	if (debug_logging) {
		print_line("JustAMCP: Executing JSON-RPC Method: " + method + " (ID: " + request_id + ")");
	}

	if (method == "initialize") {
		Dictionary result;
		result["protocolVersion"] = "2024-11-05";

		Dictionary capabilities;
		capabilities["tools"] = Dictionary();
		result["capabilities"] = capabilities;

		Dictionary serverInfo;
		serverInfo["name"] = "justamcp-godot-server";
		serverInfo["version"] = "1.0.0";
		result["serverInfo"] = serverInfo;

		Dictionary rpc_result;
		rpc_result["jsonrpc"] = "2.0";
		rpc_result["id"] = payload["id"];
		rpc_result["result"] = result;

		return rpc_result;
	}

	if (method == "ping") {
		Dictionary result;

		Dictionary rpc_result;
		rpc_result["jsonrpc"] = "2.0";
		rpc_result["id"] = payload["id"];
		rpc_result["result"] = result;

		return rpc_result;
	}

	if (method == "tools/list") {
		Dictionary result;
#ifdef TOOLS_ENABLED
		result["tools"] = JustAMCPToolExecutor::get_tool_schemas();
#else
		result["tools"] = Array();
#endif

		Dictionary rpc_result;
		rpc_result["jsonrpc"] = "2.0";
		rpc_result["id"] = payload["id"];
		rpc_result["result"] = result;

		return rpc_result;
	}

	if (method == "tools/call") {
		Dictionary params = payload.has("params") ? Dictionary(payload["params"]) : Dictionary();
		String tool_name = params.has("name") ? String(params["name"]) : "";
		Dictionary args = params.has("arguments") ? Dictionary(params["arguments"]) : Dictionary();

		// Remember the response wrapper if it's statistically POST routed.
		// And execute across the emitter!
		pending_stateless_response = p_response;

		emit_signal("tool_requested", request_id, tool_name, args);
		return Dictionary();
	}

	// Method not found fallback
	Dictionary err;
	err["jsonrpc"] = "2.0";
	err["id"] = payload.has("id") ? payload["id"] : Variant();
	Dictionary error_dict;
	error_dict["code"] = -32601;
	error_dict["message"] = "Method not found: " + method;
	err["error"] = error_dict;
	return err;
}

void JustAMCPServer::_send_sse_message(const String &p_json_string) {
	Array active_connections = HTTPServer::get_singleton()->get_active_sse_connections();
	for (int i = 0; i < active_connections.size(); i++) {
		int conn_id = active_connections[i];
		// 'message' is the default standard SSE event type for MCP Json-RPC streams
		HTTPServer::get_singleton()->send_sse_event(conn_id, "message", p_json_string);
	}
}

#endif // MODULE_HTTPSERVER_ENABLED

void JustAMCPServer::send_tool_result(const String &p_request_id, bool p_success, const Variant &p_result, const String &p_error) {
#if defined(MODULE_HTTPSERVER_ENABLED)
	Dictionary rpc_result;
	rpc_result["jsonrpc"] = "2.0";

	// request_id could be an integer or string in json-rpc, but we cast it blindly to string locally
	if (p_request_id.is_valid_int()) {
		rpc_result["id"] = p_request_id.to_int();
	} else {
		rpc_result["id"] = p_request_id;
	}

	if (p_success) {
		Dictionary result;
		// Tool call result
		Array content;
		Dictionary content_item;
		content_item["type"] = "text";
		content_item["text"] = JSON::stringify(p_result);
		content.push_back(content_item);

		result["content"] = content;
		result["isError"] = false;

		rpc_result["result"] = result;
	} else {
		Dictionary error;
		error["code"] = -32603; // Internal error
		error["message"] = p_error;
		rpc_result["error"] = error;
	}

	// Output to active SSE paths
	_send_sse_message(JSON::stringify(rpc_result));

	// Output to pending stateless HTTP paths
	if (pending_stateless_response.is_valid()) {
		if (!pending_stateless_response->is_sent()) {
			pending_stateless_response->set_status(200);
			pending_stateless_response->set_json(rpc_result);

			// Native HTTPServer doesn't push asynchronously unpolled. We can't reach the peer from HTTPResponse.
			// Luckily the client will loop poll or we will rely on blazium httpserver flushing mechanism on the next poll cycle if we just queue the body!
			// Wait, if _send_response is not called by us, it won't be sent until the client times out?
			// The Godot HTTP response actually just buffers inside HTTPResponse. The HTTP Server checks active clients.
			// Is there a way to notify HTTPServer?
			// In Blazium, there is NO public way. We might have to rely on polling loop or just disconnect.
			// Actually, just wait, `rune_interface` handled this synchronously! So it didn't have this async response issue!
			// We MUST return synchronously for tools/call if it's statistically POST routed.
		}
		pending_stateless_response.unref();
	}
#endif
}
