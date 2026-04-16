/**************************************************************************/
/*  justamcp_runtime.cpp                                                  */
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

#include "justamcp_runtime.h"

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/io/json.h"
#include "core/object/message_queue.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/editor_settings.h"
#include "main/performance.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"

// Note: Many specifics port from gdscript mcp_runtime_autoload.gd

JustAMCPRuntime *JustAMCPRuntime::singleton = nullptr;

JustAMCPRuntime *JustAMCPRuntime::get_singleton() {
	return singleton;
}

void JustAMCPRuntime::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_port", "port"), &JustAMCPRuntime::set_port);
	ClassDB::bind_method(D_METHOD("get_port"), &JustAMCPRuntime::get_port);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &JustAMCPRuntime::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &JustAMCPRuntime::is_enabled);
	ClassDB::bind_method(D_METHOD("execute_command", "command", "params"), &JustAMCPRuntime::execute_command);

	ClassDB::bind_method(D_METHOD("poll"), &JustAMCPRuntime::poll);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "port"), "set_port", "get_port");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

	ADD_SIGNAL(MethodInfo("client_connected"));
	ADD_SIGNAL(MethodInfo("client_disconnected"));
	ADD_SIGNAL(MethodInfo("command_received", PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::DICTIONARY, "params")));
}

JustAMCPRuntime::JustAMCPRuntime() {
	singleton = this;
	server.instantiate();
	if (enabled) {
		_start_server();
	}
}

JustAMCPRuntime::~JustAMCPRuntime() {
	if (singleton == this) {
		singleton = nullptr;
	}
	_cleanup();
}

void JustAMCPRuntime::_thread_poll_wrapper(void *p_user) {
	JustAMCPRuntime *runtime = (JustAMCPRuntime *)p_user;
	if (runtime) {
		runtime->_thread_poll();
	}
}

void JustAMCPRuntime::_thread_poll() {
	while (!quit_thread) {
		poll();
		OS::get_singleton()->delay_usec(16000); // roughly 60hz limit to prevent CPU hogging
	}
}

void JustAMCPRuntime::poll() {
	if (!enabled || server.is_null()) {
		return;
	}

	MutexLock lock(clients_mutex);

	if (server->is_connection_available()) {
		Ref<StreamPeerTCP> client = server->take_connection();
		if (client.is_valid()) {
			clients.push_back(client);
			print_line("[MCP Runtime] Client connected");
			call_deferred("emit_signal", "client_connected");
			_send_welcome(client);
		}
	}

	Vector<Ref<StreamPeerTCP>> clients_to_remove;
	for (int i = 0; i < clients.size(); i++) {
		Ref<StreamPeerTCP> client = clients[i];
		if (client->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			clients_to_remove.push_back(client);
			continue;
		}

		client->poll();
		int available = client->get_available_bytes();
		if (available > 0) {
			String msg_data = client->get_utf8_string(available);
			String &buf = client_buffers[client];
			buf += msg_data;

			if (buf.length() > 16777216) { // 16 MB DOS limit
				client->disconnect_from_host();
				clients_to_remove.push_back(client);
				continue;
			}

			int newline_pos = buf.find("\n");
			while (newline_pos != -1) {
				String complete_msg = buf.substr(0, newline_pos);
				buf = buf.substr(newline_pos + 1);
				if (!complete_msg.is_empty()) {
					_handle_message(client, complete_msg);
				}
				newline_pos = buf.find("\n");
			}
		}
	}

	for (int i = 0; i < clients_to_remove.size(); i++) {
		Ref<StreamPeerTCP> c = clients_to_remove[i];
		clients.erase(c);
		client_buffers.erase(c);
		print_line("[MCP Runtime] Client disconnected");
		call_deferred("emit_signal", "client_disconnected");
	}
}

void JustAMCPRuntime::set_port(int p_port) {
	port = p_port;
	if (enabled) {
		_start_server();
	}
}

int JustAMCPRuntime::get_port() const {
	return port;
}

void JustAMCPRuntime::set_enabled(bool p_enabled) {
	enabled = p_enabled;
	if (enabled) {
		_start_server();
	} else if (!enabled) {
		_cleanup();
	}
}

bool JustAMCPRuntime::is_enabled() const {
	return enabled;
}

void JustAMCPRuntime::_start_server() {
	const List<String> &args = OS::get_singleton()->get_cmdline_args();
	for (const String &arg : args) {
		if (arg == "--test" || arg == "--tests" || arg.begins_with("--aw-") ||
				arg == "--help" || arg == "-h" || arg == "/?" || arg == "--version" ||
				arg == "--check-only" || arg.begins_with("--export")) {
			return; // Do not start Server thread during CLI tools or testing workflows
		}
	}

#ifdef TOOLS_ENABLED
	bool is_headless = false;
	if (OS::get_singleton() && OS::get_singleton()->get_cmdline_args().find("--headless")) {
		is_headless = true;
	}

	bool use_project_override = GLOBAL_GET("blazium/justamcp/override_editor_settings");

	if (is_headless) {
		use_project_override = true;
	}

	if (use_project_override || !EditorSettings::get_singleton()) {
		enabled = GLOBAL_GET("blazium/justamcp/server_enabled");
		port = GLOBAL_GET("blazium/justamcp/server_port");
	} else {
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_enabled")) {
			enabled = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_enabled");
		}
		if (EditorSettings::get_singleton()->has_setting("blazium/justamcp/server_port")) {
			port = EditorSettings::get_singleton()->get_setting("blazium/justamcp/server_port");
		}
	}
#else
	enabled = GLOBAL_GET("blazium/justamcp/server_enabled");
	port = GLOBAL_GET("blazium/justamcp/server_port");
#endif

	if (OS::get_singleton()->get_cmdline_args().find("--enable-mcp")) {
		enabled = true;
	}

	if (!enabled) {
		return;
	}

	if (server.is_valid() && server->is_listening()) {
		server->stop();
	}

	bool bind_to_localhost = true;
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("blazium/justamcp/bind_to_localhost_only")) {
		bind_to_localhost = GLOBAL_GET("blazium/justamcp/bind_to_localhost_only");
	}
	String bind_address = bind_to_localhost ? "127.0.0.1" : "*";

	Error err = server->listen(port, bind_address);
	if (err != OK) {
		ERR_PRINT(vformat("[MCP Runtime] Failed to start server on port %d: %d", port, err));
		enabled = false;
	} else {
		print_line(vformat("[MCP Runtime] Server listening on port %d", port));
		quit_thread = false;
		if (!server_thread) {
			server_thread = memnew(Thread);
		}
		if (!server_thread->is_started()) {
			server_thread->start(_thread_poll_wrapper, this);
		}
	}
}

void JustAMCPRuntime::_cleanup() {
	quit_thread = true;
	if (server_thread) {
		if (server_thread->is_started()) {
			server_thread->wait_to_finish();
		}
		memdelete(server_thread);
		server_thread = nullptr;
	}

	{
		MutexLock lock(clients_mutex);
		for (int i = 0; i < clients.size(); i++) {
			if (clients[i].is_valid()) {
				clients[i]->disconnect_from_host();
			}
		}
		clients.clear();
		client_buffers.clear();
	}

	if (server.is_valid()) {
		server->stop();
	}
}

void JustAMCPRuntime::_send_welcome(Ref<StreamPeerTCP> p_client) {
	Dictionary welcome;
	welcome["type"] = "welcome";
	welcome["protocol_version"] = "1.0";
	welcome["godot_version"] = Engine::get_singleton()->get_version_info();
	welcome["project_name"] = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown");
	_send_response(p_client, welcome);
}

void JustAMCPRuntime::_send_response(Ref<StreamPeerTCP> p_client, const Dictionary &p_data) {
	if (p_client.is_valid() && p_client->get_status() == StreamPeerTCP::STATUS_CONNECTED) {
		String json_str = JSON::stringify(p_data) + "\n";
		p_client->put_utf8_string(json_str);
	}
}

void JustAMCPRuntime::_send_error(Ref<StreamPeerTCP> p_client, const String &p_message) {
	Dictionary err;
	err["type"] = "error";
	err["message"] = p_message;
	_send_response(p_client, err);
}

void JustAMCPRuntime::_handle_message(Ref<StreamPeerTCP> p_client, const String &p_data) {
	Ref<JSON> json;
	json.instantiate();
	Error err = json->parse(p_data);
	if (err != OK) {
		_send_error(p_client, "Invalid JSON: " + json->get_error_message());
		return;
	}

	Variant result = json->get_data();
	if (result.get_type() != Variant::DICTIONARY) {
		_send_error(p_client, "Message must be an object");
		return;
	}

	Dictionary message = result;
	String command = message.get("command", "");
	Dictionary params = message.get("params", Dictionary());
	Variant request_id = message.get("id", Variant());

	call_deferred("emit_signal", "command_received", command, params);

	MessageQueue::get_singleton()->push_callable(callable_mp(this, &JustAMCPRuntime::_deferred_execute_command), p_client, command, params, request_id);
}

void JustAMCPRuntime::_deferred_execute_command(Ref<StreamPeerTCP> p_client, String p_command, Dictionary p_params, Variant p_request_id) {
	Dictionary response_data = execute_command(p_command, p_params);
	if (p_request_id.get_type() != Variant::NIL && p_request_id.get_type() != Variant::OBJECT) {
		response_data["id"] = p_request_id;
	}

	_send_response(p_client, response_data);
}

Dictionary JustAMCPRuntime::execute_command(const String &p_command, const Dictionary &p_params) {
	if (p_command == "ping") {
		Dictionary ret;
		ret["type"] = "pong";
		ret["timestamp"] = Time::get_singleton()->get_unix_time_from_system();
		return ret;
	} else if (p_command == "get_tree") {
		return _cmd_get_tree(p_params);
	} else if (p_command == "get_node") {
		return _cmd_get_node(p_params);
	} else if (p_command == "set_property") {
		return _cmd_set_property(p_params);
	} else if (p_command == "call_method") {
		return _cmd_call_method(p_params);
	} else if (p_command == "get_metrics") {
		return _cmd_get_metrics(p_params);
	} else if (p_command == "capture_screenshot" || p_command == "capture_viewport") {
		return _cmd_capture_screenshot(p_params);
	} else if (p_command == "inject_action") {
		return _cmd_inject_action(p_params);
	} else if (p_command == "inject_key") {
		return _cmd_inject_key(p_params);
	} else if (p_command == "inject_mouse_click") {
		return _cmd_inject_mouse_click(p_params);
	} else if (p_command == "inject_mouse_motion") {
		return _cmd_inject_mouse_motion(p_params);
	} else if (p_command == "watch_signal") {
		return _cmd_watch_signal(p_params);
	} else if (p_command == "unwatch_signal") {
		return _cmd_unwatch_signal(p_params);
	}

	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Unknown command: " + p_command;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_tree(const Dictionary &p_params) {
	String root_path = p_params.get("root", "/root");
	int max_depth = p_params.get("depth", 3);
	bool include_properties = p_params.get("include_properties", false);

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "No SceneTree available";
		return err;
	}

	Node *root = tree->get_root()->get_node_or_null(root_path);
	if (!root) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node not found: " + root_path;
		return err;
	}

	Dictionary ret;
	ret["type"] = "tree";
	ret["root"] = _serialize_node_tree(root, 0, max_depth, include_properties);
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_node(const Dictionary &p_params) {
	String node_path = p_params.get("path", "");
	if (node_path.is_empty()) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node path required";
		return err;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "No SceneTree available";
		return err;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node not found: " + node_path;
		return err;
	}

	Dictionary ret;
	ret["type"] = "node";
	ret["data"] = _serialize_node(node, true);
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_set_property(const Dictionary &p_params) {
	String node_path = p_params.get("path", "");
	String property = p_params.get("property", "");
	Variant value = p_params.get("value", Variant());

	if (node_path.is_empty() || property.is_empty()) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node path and property required";
		return err;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "No SceneTree available";
		return err;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node not found: " + node_path;
		return err;
	}

	Variant old_value = node->get(property);
	node->set(property, _deserialize_value(value));

	Dictionary ret;
	ret["type"] = "property_set";
	ret["path"] = node_path;
	ret["property"] = property;
	ret["old_value"] = _serialize_value(old_value);
	ret["new_value"] = _serialize_value(node->get(property));
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_call_method(const Dictionary &p_params) {
	String node_path = p_params.get("path", "");
	String method = p_params.get("method", "");
	Array args = p_params.get("args", Array());

	if (node_path.is_empty() || method.is_empty()) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node path and method required";
		return err;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "No SceneTree available";
		return err;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Node not found: " + node_path;
		return err;
	}

	if (!node->has_method(method)) {
		Dictionary err;
		err["type"] = "error";
		err["message"] = "Method not found: " + method;
		return err;
	}

	Vector<Variant> variant_args;
	for (int i = 0; i < args.size(); i++) {
		variant_args.push_back(_deserialize_value(args[i]));
	}

	const Variant **argptrs = nullptr;
	if (variant_args.size() > 0) {
		argptrs = (const Variant **)alloca(sizeof(Variant *) * variant_args.size());
		for (int i = 0; i < variant_args.size(); i++) {
			argptrs[i] = &variant_args[i];
		}
	}

	Callable::CallError ce;
	Variant result = node->callp(method, argptrs, variant_args.size(), ce);

	Dictionary ret;
	ret["type"] = "method_result";
	ret["path"] = node_path;
	ret["method"] = method;
	ret["result"] = _serialize_value(result);
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_metrics(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "metrics";
	Dictionary metrics_data;

	Performance *perf = Performance::get_singleton();

	metrics_data["fps"] = Engine::get_singleton()->get_frames_per_second();
	metrics_data["frame_time"] = perf->get_monitor(Performance::TIME_PROCESS);
	metrics_data["physics_time"] = perf->get_monitor(Performance::TIME_PHYSICS_PROCESS);

	metrics_data["memory_static"] = perf->get_monitor(Performance::MEMORY_STATIC);
	metrics_data["memory_static_max"] = perf->get_monitor(Performance::MEMORY_STATIC_MAX);

	metrics_data["object_count"] = perf->get_monitor(Performance::OBJECT_COUNT);
	metrics_data["object_resource_count"] = perf->get_monitor(Performance::OBJECT_RESOURCE_COUNT);
	metrics_data["object_node_count"] = perf->get_monitor(Performance::OBJECT_NODE_COUNT);
	metrics_data["object_orphan_node_count"] = perf->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);

	metrics_data["render_total_objects"] = perf->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
	metrics_data["render_total_primitives"] = perf->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	metrics_data["render_total_draw_calls"] = perf->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);

	ret["data"] = metrics_data;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_capture_screenshot(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not fully implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_action(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_key(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_mouse_click(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_mouse_motion(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_watch_signal(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_unwatch_signal(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Not implemented in C++ runtime yet";
	return ret;
}

Dictionary JustAMCPRuntime::_serialize_node_tree(Node *p_node, int p_depth, int p_max_depth, bool p_include_properties) {
	Dictionary result = _serialize_node(p_node, p_include_properties);
	if (p_depth < p_max_depth) {
		Array children;
		for (int i = 0; i < p_node->get_child_count(); i++) {
			children.push_back(_serialize_node_tree(p_node->get_child(i), p_depth + 1, p_max_depth, p_include_properties));
		}
		result["children"] = children;
	}
	return result;
}

Dictionary JustAMCPRuntime::_serialize_node(Node *p_node, bool p_include_properties) {
	Dictionary result;
	result["name"] = p_node->get_name();
	result["type"] = p_node->get_class();
	result["path"] = String(p_node->get_path());

	// If script attached etc etc.

	if (p_include_properties) {
		Dictionary props;
		List<PropertyInfo> list;
		p_node->get_property_list(&list);
		for (const PropertyInfo &E : list) {
			if (E.usage & PROPERTY_USAGE_STORAGE) {
				if (!E.name.begins_with("_")) {
					props[E.name] = _serialize_value(p_node->get(E.name));
				}
			}
		}
		result["properties"] = props;
	}
	return result;
}

Variant JustAMCPRuntime::_serialize_value(const Variant &p_value) {
	// A robust conversion would check types and output dictionary representation
	return p_value;
}

Variant JustAMCPRuntime::_deserialize_value(const Variant &p_value) {
	return p_value;
}
