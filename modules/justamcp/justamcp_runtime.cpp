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
#include "core/io/image.h"
#include "core/io/json.h"
#include "core/math/expression.h"
#include "core/object/message_queue.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "main/performance.h"
#include "scene/gui/base_button.h"
#include "scene/gui/control.h"
#include "scene/main/multiplayer_api.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "servers/audio_server.h"
#include "tools/justamcp_tool_executor.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_settings.h"
#endif

// Note: Many specifics port from gdscript mcp_runtime_autoload.gd

JustAMCPRuntime *JustAMCPRuntime::singleton = nullptr;

// Security blocklist — methods that must not be callable via call_node_method
const char *JustAMCPRuntime::_BLOCKED_METHODS[] = {
	"execute", "create_process", "shell", "spawn", "create_thread",
	"load", "load_file", "load_buffer", "load_script", "load_resource_pack",
	"save", "save_file", "store_buffer", "open", "open_encrypted",
	"write", "write_buffer", "write_file", "delete_file", "move_file", "copy_file",
	"eval", "compile", "compile_file", "set_script",
	"system", "exec", "request_permissions", "quit", "crash",
	"set_meta", "remove_meta", "get_meta", // Prevent metadata tampering if sensitive
	nullptr
};

// Security blocklist — patterns forbidden in eval_expression
const char *JustAMCPRuntime::_EVAL_BLOCKED_PATTERNS[] = {
	"OS.", "Engine.", "FileAccess.", "DirAccess.", "Directory.", "ClassDB.",
	"create_process", "execute", "shell", "load(", "save(", "open(", "delete(",
	"write_file", "load_file", "save_file", "store_buffer", "set_script",
	"Thread.", "Mutex.", "Semaphore.", "HttpRequest.", "TCPServer.", "StreamPeer.",
	nullptr
};

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

	ClassDB::bind_method(D_METHOD("register_custom_command", "name", "callable"), &JustAMCPRuntime::register_custom_command);
	ClassDB::bind_method(D_METHOD("unregister_custom_command", "name"), &JustAMCPRuntime::unregister_custom_command);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "port"), "set_port", "get_port");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

	ADD_SIGNAL(MethodInfo("client_connected"));
	ADD_SIGNAL(MethodInfo("client_disconnected"));
	ADD_SIGNAL(MethodInfo("command_received", PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::DICTIONARY, "params")));
}

JustAMCPRuntime::JustAMCPRuntime() {
	singleton = this;
	server.instantiate();
	executor = memnew(JustAMCPToolExecutor);

	_print_handler.printfunc = _print_handler_callback;
	_print_handler.userdata = this;
	add_print_handler(&_print_handler);

	if (enabled) {
		_start_server();
	}
}

JustAMCPRuntime::~JustAMCPRuntime() {
	remove_print_handler(&_print_handler);
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
		OS::get_singleton()->delay_usec(16000);
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

void JustAMCPRuntime::_print_handler_callback(void *p_user_data, const String &p_string, bool p_error, bool p_rich) {
	JustAMCPRuntime *runtime = static_cast<JustAMCPRuntime *>(p_user_data);
	if (runtime) {
		runtime->push_error_log(p_string, p_error);
	}
}

void JustAMCPRuntime::push_error_log(const String &p_message, bool p_is_error) {
	MutexLock lock(_error_log_mutex);
	Dictionary entry;
	entry["message"] = p_message;
	if (p_is_error) {
		entry["type"] = "error";
	} else if (p_message.begins_with("WARNING:")) {
		entry["type"] = "warning";
	} else {
		entry["type"] = "log";
	}
	entry["timestamp"] = Time::get_singleton()->get_unix_time_from_system();
	_error_log.push_back(entry);

	if (_error_log.size() > 500) {
		_error_log.remove_at(0);
	}
}

void JustAMCPRuntime::_start_server() {
	const List<String> &args = OS::get_singleton()->get_cmdline_args();
	for (const String &arg : args) {
		if (arg == "--test" || arg == "--tests" || arg.begins_with("--aw-") ||
				arg == "--help" || arg == "-h" || arg == "/?" || arg == "--version" ||
				arg == "--check-only" || arg.begins_with("--export")) {
			return;
		}
	}

	// Feature flag: --disable-game-mcp overrides everything
	bool mcp_disabled = false;
	for (const String &E : args) {
		if (E == "--disable-game-mcp") {
			mcp_disabled = true;
			break;
		}
	}
	if (mcp_disabled) {
		return;
	}

#ifdef TOOLS_ENABLED
	bool is_headless = false;
	for (const String &E : args) {
		if (E == "--headless") {
			is_headless = true;
			break;
		}
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
	// Non-editor (game) build: check feature flags
	if (ProjectSettings::get_singleton()->has_setting("blazium/justamcp/disable_game_mcp")) {
		if ((bool)GLOBAL_GET("blazium/justamcp/disable_game_mcp")) {
			return; // Game-side MCP explicitly disabled
		}
	}

	bool game_control_enabled = false;
	if (ProjectSettings::get_singleton()->has_setting("blazium/justamcp/game_control_enabled")) {
		game_control_enabled = GLOBAL_GET("blazium/justamcp/game_control_enabled");
	}
	if (!game_control_enabled && !args.find("--enable-mcp-game-control")) {
		return; // Game MCP not enabled in this build
	}

	enabled = GLOBAL_GET("blazium/justamcp/server_enabled");
	port = GLOBAL_GET("blazium/justamcp/server_port");
#endif

	if (OS::get_singleton()->get_cmdline_args().find("--enable-mcp")) {
		enabled = true;
	}
	if (OS::get_singleton()->get_cmdline_args().find("--enable-mcp-game-control")) {
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
		print_line(vformat("BLAZIUM_READY:{\"proto\":\"blazium/1\",\"port\":%d,\"engine\":\"Blazium\"}", port));
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

	if (executor) {
		memdelete(executor);
		executor = nullptr;
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
	welcome["engine"] = "Blazium JustAMCP";
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
	// Existing commands
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
	// New Blazium commands
	else if (p_command == "inject_drag") {
		return _cmd_inject_drag(p_params);
	} else if (p_command == "inject_scroll") {
		return _cmd_inject_scroll(p_params);
	} else if (p_command == "inject_gesture") {
		return _cmd_inject_gesture(p_params);
	} else if (p_command == "find_nodes") {
		return _cmd_find_nodes(p_params);
	} else if (p_command == "get_node_property") {
		return _cmd_get_node_property(p_params);
	} else if (p_command == "call_node_method") {
		return _cmd_call_node_method(p_params);
	} else if (p_command == "press_button") {
		return _cmd_press_button(p_params);
	} else if (p_command == "wait_for_property") {
		return _cmd_wait_for_property(p_params);
	} else if (p_command == "runtime_info") {
		return _cmd_runtime_info(p_params);
	} else if (p_command == "runtime_get_errors") {
		return _cmd_runtime_get_errors(p_params);
	} else if (p_command == "runtime_capabilities") {
		return _cmd_runtime_capabilities(p_params);
	} else if (p_command == "eval_expression") {
		return _cmd_eval_expression(p_params);
	} else if (p_command == "quit" || p_command == "runtime_quit") {
		return _cmd_runtime_quit(p_params);
	} else if (p_command == "network_state" || p_command == "get_network_info") {
		return _cmd_get_network_info(p_params);
	} else if (p_command == "audio_state" || p_command == "get_audio_info") {
		return _cmd_get_audio_info(p_params);
	} else if (p_command == "gamepad" || p_command == "inject_gamepad") {
		return _cmd_inject_gamepad(p_params);
	} else if (p_command == "run_custom_command") {
		return _cmd_run_custom_command(p_params);
	} else if (p_command == "get_autoload") {
		return _cmd_get_autoload(p_params);
	} else if (p_command == "find_nodes_by_script") {
		return _cmd_find_nodes_by_script(p_params);
	} else if (p_command == "batch_get_properties") {
		return _cmd_batch_get_properties(p_params);
	} else if (p_command == "find_ui_elements") {
		return _cmd_find_ui_elements(p_params);
	} else if (p_command == "click_button_by_text") {
		return _cmd_click_button_by_text(p_params);
	} else if (p_command == "navigate_to" || p_command == "move_to" || p_command == "move_node") {
		return _cmd_move_node(p_params);
	} else if (p_command == "monitor_properties") {
		return _cmd_monitor_properties(p_params);
	} else if (p_command == "auth_info") {
		return _cmd_auth_info(p_params);
	} else if (p_command == "performance" || p_command == "blazium_performance" || p_command == "get_performance_metrics") {
		return _cmd_get_metrics(p_params);
	} else if (p_command == "tool_call") {
		return _cmd_tool_call(p_params);
	} else if (p_command == "list_tools") {
		return _cmd_list_tools(p_params);
	} else if (p_command == "diagnose" || p_command == "runtime_diagnose") {
		return _cmd_diagnose(p_params);
	} else if (p_command == "get_log_tail" || p_command == "runtime_get_log_tail") {
		return _cmd_get_log_tail(p_params);
	}

	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "Unknown command: " + p_command;
	return ret;
}

// ── Helpers ──

void JustAMCPRuntime::_inject_event(const Ref<InputEvent> &p_event) {
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree && tree->get_root()) {
		tree->get_root()->push_input(p_event);

		// Support OS input mode for parity with legacy Blazium features
		static String input_mode = OS::get_singleton()->get_environment("BLAZIUM_MCP_INPUT_MODE");
		if (input_mode == "os") {
			Ref<InputEventMouse> mouse_ev = p_event;
			if (mouse_ev.is_valid()) {
				Input::get_singleton()->warp_mouse(mouse_ev->get_global_position());
			}
		}
	}
}

void JustAMCPRuntime::_find_nodes_recursive(Node *p_node, const String &p_name, const String &p_type, const String &p_group, int p_limit, Array &r_results) {
	if (r_results.size() >= p_limit) {
		return;
	}
	bool hit = true;
	if (!p_name.is_empty() && p_name != "*") {
		hit = hit && String(p_node->get_name()).containsn(p_name);
	}
	if (!p_type.is_empty()) {
		hit = hit && p_node->is_class(p_type);
	}
	if (!p_group.is_empty()) {
		hit = hit && p_node->is_in_group(p_group);
	}
	if (hit && (!p_name.is_empty() || !p_type.is_empty() || !p_group.is_empty())) {
		Dictionary entry;
		entry["name"] = String(p_node->get_name());
		entry["type"] = p_node->get_class();
		entry["path"] = String(p_node->get_path());
		Array groups;
		List<Node::GroupInfo> glist;
		p_node->get_groups(&glist);
		for (const Node::GroupInfo &g : glist) {
			groups.push_back(String(g.name));
		}
		entry["groups"] = groups;
		r_results.push_back(entry);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_nodes_recursive(p_node->get_child(i), p_name, p_type, p_group, p_limit, r_results);
	}
}

void JustAMCPRuntime::_find_nodes_by_script_recursive(Node *p_node, const String &p_script_path, const String &p_class_name, int p_limit, Array &r_results) {
	if (r_results.size() >= p_limit) {
		return;
	}

	Ref<Script> node_script = p_node->get_script();
	bool hit = false;
	if (node_script.is_valid()) {
		if (!p_script_path.is_empty() && node_script->get_path() == p_script_path) {
			hit = true;
		}
		if (!p_class_name.is_empty()) {
			String global_name = node_script->get_global_name();
			hit = hit || global_name == p_class_name || node_script->get_instance_base_type() == p_class_name;
		}
	}

	if (hit) {
		Dictionary entry;
		entry["name"] = String(p_node->get_name());
		entry["type"] = p_node->get_class();
		entry["path"] = String(p_node->get_path());
		entry["script_path"] = node_script->get_path();
		entry["script_class"] = node_script->get_global_name();
		r_results.push_back(entry);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_nodes_by_script_recursive(p_node->get_child(i), p_script_path, p_class_name, p_limit, r_results);
	}
}

void JustAMCPRuntime::_find_ui_elements_recursive(Node *p_node, const String &p_text, const String &p_type, bool p_visible_only, int p_limit, Array &r_results) {
	if (r_results.size() >= p_limit) {
		return;
	}

	Control *control = Object::cast_to<Control>(p_node);
	if (control) {
		bool visible = control->is_visible_in_tree();
		bool hit = !p_visible_only || visible;
		if (!p_type.is_empty()) {
			hit = hit && control->is_class(p_type);
		}
		String text;
		bool valid = false;
		Variant raw_text = control->get("text", &valid);
		if (valid) {
			text = raw_text;
		}
		if (!p_text.is_empty()) {
			hit = hit && text.containsn(p_text);
		}
		if (hit) {
			Dictionary entry;
			entry["name"] = String(control->get_name());
			entry["type"] = control->get_class();
			entry["path"] = String(control->get_path());
			entry["text"] = text;
			entry["visible"] = visible;
			entry["position"] = control->get_global_position();
			entry["size"] = control->get_size();
			r_results.push_back(entry);
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_ui_elements_recursive(p_node->get_child(i), p_text, p_type, p_visible_only, p_limit, r_results);
	}
}

Node *JustAMCPRuntime::_find_button_by_text_recursive(Node *p_node, const String &p_text) {
	BaseButton *button = Object::cast_to<BaseButton>(p_node);
	if (button) {
		bool valid = false;
		Variant raw_text = button->get("text", &valid);
		if (valid && String(raw_text).containsn(p_text)) {
			return button;
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *found = _find_button_by_text_recursive(p_node->get_child(i), p_text);
		if (found) {
			return found;
		}
	}
	return nullptr;
}

Node *JustAMCPRuntime::_find_button_recursive(Node *p_node, const String &p_name) {
	if (String(p_node->get_name()) == p_name) {
		return p_node;
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *found = _find_button_recursive(p_node->get_child(i), p_name);
		if (found) {
			return found;
		}
	}
	return nullptr;
}

// ── Existing Commands (now fully implemented) ──

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
	Variant result_val = node->callp(method, argptrs, variant_args.size(), ce);

	Dictionary ret;
	ret["type"] = "method_result";
	ret["path"] = node_path;
	ret["method"] = method;
	ret["result"] = _serialize_value(result_val);
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
	metrics_data["render_video_mem_used"] = perf->get_monitor(Performance::RENDER_VIDEO_MEM_USED);

	ret["data"] = metrics_data;
	return ret;
}

// ── Screenshot ──

Dictionary JustAMCPRuntime::_cmd_capture_screenshot(const Dictionary &p_params) {
	// Rate limit: max 10 captures per second
	uint64_t now_ms = Time::get_singleton()->get_ticks_msec();
	uint64_t cutoff = now_ms > (uint64_t)_SCREENSHOT_RATE_WINDOW_MS ? now_ms - (uint64_t)_SCREENSHOT_RATE_WINDOW_MS : 0;

	Vector<uint64_t> kept;
	for (int i = 0; i < _screenshot_timestamps.size(); i++) {
		if (_screenshot_timestamps[i] > cutoff) {
			kept.push_back(_screenshot_timestamps[i]);
		}
	}
	_screenshot_timestamps = kept;

	if (_screenshot_timestamps.size() >= _SCREENSHOT_RATE_MAX) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = vformat("Screenshot rate limit exceeded (max %d per second)", _SCREENSHOT_RATE_MAX);
		return ret;
	}
	_screenshot_timestamps.push_back(now_ms);

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree || !tree->get_root()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree or viewport available";
		return ret;
	}

	Ref<ViewportTexture> tex = tree->get_root()->get_texture();
	if (tex.is_null()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No viewport texture";
		return ret;
	}

	Ref<Image> img = tex->get_image();
	if (img.is_null()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "get_image() failed";
		return ret;
	}

	Vector<uint8_t> png_data = img->save_png_to_buffer();
	String b64 = CryptoCore::b64_encode_str(png_data.ptr(), png_data.size());

	Dictionary ret;
	ret["type"] = "screenshot";
	ret["width"] = img->get_width();
	ret["height"] = img->get_height();
	ret["png_base64"] = b64;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_capture_viewport(const Dictionary &p_params) {
	return _cmd_capture_screenshot(p_params);
}

// ── Input Injection ──

Dictionary JustAMCPRuntime::_cmd_inject_action(const Dictionary &p_params) {
	String action = p_params.get("action", "");
	if (action.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing 'action'";
		return ret;
	}

	Ref<InputEventAction> press;
	press.instantiate();
	press->set_action(action);
	press->set_pressed(true);
	_inject_event(press);

	Ref<InputEventAction> release;
	release.instantiate();
	release->set_action(action);
	release->set_pressed(false);
	_inject_event(release);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["action"] = action;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_key(const Dictionary &p_params) {
	String action = p_params.get("action", "");
	int keycode = p_params.get("keycode", -1);

	if (!action.is_empty()) {
		return _cmd_inject_action(p_params);
	}

	if (keycode < 0) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Provide 'action' or 'keycode'";
		return ret;
	}

	Ref<InputEventKey> press;
	press.instantiate();
	press->set_keycode((Key)keycode);
	press->set_pressed(true);
	_inject_event(press);

	Ref<InputEventKey> release;
	release.instantiate();
	release->set_keycode((Key)keycode);
	release->set_pressed(false);
	_inject_event(release);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["keycode"] = keycode;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_mouse_click(const Dictionary &p_params) {
	float x = p_params.get("x", 0.0f);
	float y = p_params.get("y", 0.0f);
	Vector2 pos(x, y);

	Ref<InputEventMouseButton> press;
	press.instantiate();
	press->set_position(pos);
	press->set_global_position(pos);
	press->set_button_index(MouseButton::LEFT);
	press->set_button_mask(MouseButtonMask::LEFT);
	press->set_pressed(true);
	_inject_event(press);

	Ref<InputEventMouseButton> release;
	release.instantiate();
	release->set_position(pos);
	release->set_global_position(pos);
	release->set_button_index(MouseButton::LEFT);
	release->set_button_mask(MouseButtonMask::NONE);
	release->set_pressed(false);
	_inject_event(release);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["x"] = x;
	ret["y"] = y;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_mouse_motion(const Dictionary &p_params) {
	float x = p_params.get("x", 0.0f);
	float y = p_params.get("y", 0.0f);
	float rel_x = p_params.get("relative_x", 0.0f);
	float rel_y = p_params.get("relative_y", 0.0f);
	Vector2 pos(x, y);

	Ref<InputEventMouseMotion> motion;
	motion.instantiate();
	motion->set_position(pos);
	motion->set_global_position(pos);
	motion->set_relative(Vector2(rel_x, rel_y));
	_inject_event(motion);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["x"] = x;
	ret["y"] = y;
	return ret;
}

// ── New Blazium Commands ──

Dictionary JustAMCPRuntime::_cmd_inject_drag(const Dictionary &p_params) {
	Variant from_v = p_params.get("from", Variant());
	Variant to_v = p_params.get("to", Variant());

	if (from_v.get_type() != Variant::ARRAY || to_v.get_type() != Variant::ARRAY) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "'from' and 'to' must be [x, y] arrays";
		return ret;
	}

	Array from_arr = from_v;
	Array to_arr = to_v;
	Vector2 pos_from = Vector2(float(from_arr[0]), float(from_arr[1]));
	Vector2 pos_to = Vector2(float(to_arr[0]), float(to_arr[1]));

	// Motion to start
	Ref<InputEventMouseMotion> motion_start;
	motion_start.instantiate();
	motion_start->set_position(pos_from);
	motion_start->set_global_position(pos_from);
	motion_start->set_relative(Vector2());
	_inject_event(motion_start);

	// Press at from
	Ref<InputEventMouseButton> press;
	press.instantiate();
	press->set_position(pos_from);
	press->set_global_position(pos_from);
	press->set_button_index(MouseButton::LEFT);
	press->set_button_mask(MouseButtonMask::LEFT);
	press->set_pressed(true);
	_inject_event(press);

	// Drag motion to destination
	Ref<InputEventMouseMotion> motion_drag;
	motion_drag.instantiate();
	motion_drag->set_position(pos_to);
	motion_drag->set_global_position(pos_to);
	motion_drag->set_relative(pos_to - pos_from);
	motion_drag->set_button_mask(MouseButtonMask::LEFT);
	_inject_event(motion_drag);

	// Release at destination
	Ref<InputEventMouseButton> release;
	release.instantiate();
	release->set_position(pos_to);
	release->set_global_position(pos_to);
	release->set_button_index(MouseButton::LEFT);
	release->set_button_mask(MouseButtonMask::NONE);
	release->set_pressed(false);
	_inject_event(release);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["command"] = "inject_drag";
	ret["from"] = from_v;
	ret["to"] = to_v;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_scroll(const Dictionary &p_params) {
	float x = p_params.get("x", 0.0f);
	float y = p_params.get("y", 0.0f);
	float delta = p_params.get("delta", -3.0f);
	Vector2 pos(x, y);

	MouseButton button = delta < 0 ? MouseButton::WHEEL_DOWN : MouseButton::WHEEL_UP;

	Ref<InputEventMouseButton> press;
	press.instantiate();
	press->set_position(pos);
	press->set_global_position(pos);
	press->set_button_index(button);
	press->set_factor(Math::abs(delta));
	press->set_pressed(true);
	_inject_event(press);

	Ref<InputEventMouseButton> release;
	release.instantiate();
	release->set_position(pos);
	release->set_global_position(pos);
	release->set_button_index(button);
	release->set_factor(Math::abs(delta));
	release->set_pressed(false);
	_inject_event(release);

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["command"] = "inject_scroll";
	ret["x"] = x;
	ret["y"] = y;
	ret["delta"] = delta;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_gesture(const Dictionary &p_params) {
	String gtype = String(p_params.get("type", "")).to_lower();
	Dictionary params_dict = p_params.get("params", Dictionary());

	Variant center_v = params_dict.get("center", Variant());
	Vector2 center;
	if (center_v.get_type() == Variant::ARRAY) {
		Array ca = center_v;
		if (ca.size() >= 2) {
			center = Vector2(float(ca[0]), float(ca[1]));
		}
	}

	if (gtype == "pinch") {
		float scale_val = params_dict.get("scale", 1.1f);
		Ref<InputEventMagnifyGesture> ev;
		ev.instantiate();
		ev->set_position(center);
		ev->set_factor(scale_val);
		_inject_event(ev);

		Dictionary ret;
		ret["type"] = "input_injected";
		ret["command"] = "inject_gesture";
		ret["gesture"] = "pinch";
		return ret;
	} else if (gtype == "swipe") {
		Variant delta_v = params_dict.get("delta", Variant());
		Vector2 delta_vec;
		if (delta_v.get_type() == Variant::ARRAY) {
			Array da = delta_v;
			if (da.size() >= 2) {
				delta_vec = Vector2(float(da[0]), float(da[1]));
			}
		}
		Ref<InputEventPanGesture> ev;
		ev.instantiate();
		ev->set_position(center);
		ev->set_delta(delta_vec);
		_inject_event(ev);

		Dictionary ret;
		ret["type"] = "input_injected";
		ret["command"] = "inject_gesture";
		ret["gesture"] = "swipe";
		return ret;
	}

	Dictionary ret;
	ret["type"] = "error";
	ret["message"] = "gesture 'type' must be 'pinch' or 'swipe'";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_find_nodes(const Dictionary &p_params) {
	String name_filter = p_params.get("name", "");
	String type_filter = p_params.get("type", "");
	String group_filter = p_params.get("group", "");
	int limit = p_params.get("limit", 50);

	if (name_filter.is_empty() && type_filter.is_empty() && group_filter.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Requires at least one of: 'name', 'type', 'group'";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Array results;
	_find_nodes_recursive(tree->get_root(), name_filter, type_filter, group_filter, limit, results);

	Dictionary ret;
	ret["type"] = "find_nodes_result";
	ret["matches"] = results;
	ret["count"] = results.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_node_property(const Dictionary &p_params) {
	String node_path = p_params.get("node", "");
	String property = p_params.get("property", "");

	if (node_path.is_empty() || property.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Requires 'node' (NodePath) and 'property'";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}

	Dictionary ret;
	ret["type"] = "node_property";
	ret["node"] = node_path;
	ret["property"] = property;
	ret["value"] = _serialize_value(node->get(property));
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_call_node_method(const Dictionary &p_params) {
	String node_path = p_params.get("node", "");
	String method = p_params.get("method", "");
	Array args = p_params.get("args", Array());

	if (node_path.is_empty() || method.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Requires 'node' and 'method'";
		return ret;
	}

	// Security blocklist check
	String method_lower = method.to_lower();
	for (int i = 0; _BLOCKED_METHODS[i] != nullptr; i++) {
		if (method_lower == String(_BLOCKED_METHODS[i]).to_lower()) {
			Dictionary ret;
			ret["type"] = "error";
			ret["message"] = "Method not allowed: " + method;
			return ret;
		}
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}

	if (!node->has_method(method)) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Method not found: " + method;
		return ret;
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
	Variant result_variant = node->callp(method, argptrs, variant_args.size(), ce);

	Dictionary ret;
	ret["type"] = "method_result";
	ret["node"] = node_path;
	ret["method"] = method;
	ret["result"] = _serialize_value(result_variant);
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_press_button(const Dictionary &p_params) {
	String button_name = p_params.get("name", "");
	if (button_name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing 'name'";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Node *found = _find_button_recursive(tree->get_root(), button_name);
	if (!found) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + button_name;
		return ret;
	}

	BaseButton *btn = Object::cast_to<BaseButton>(found);
	if (!btn) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node is not a BaseButton: " + found->get_class();
		return ret;
	}

	if (btn->is_toggle_mode()) {
		btn->set_pressed(!btn->is_pressed());
	}

	// Call all connected callables on the pressed signal
	List<Object::Connection> connection_list;
	btn->get_signal_connection_list("pressed", &connection_list);
	int connection_count = 0;
	for (const Object::Connection &c : connection_list) {
		c.callable.call();
		connection_count++;
	}

	Dictionary ret;
	ret["type"] = "button_pressed";
	ret["node"] = String(found->get_path());
	ret["connections_triggered"] = connection_count;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_autoload(const Dictionary &p_params) {
	String name = p_params.get("name", "");
	if (name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing 'name'";
		return ret;
	}

	String setting_key = "autoload/" + name;
	Dictionary ret;
	ret["type"] = "autoload";
	ret["name"] = name;
	ret["configured"] = ProjectSettings::get_singleton()->has_setting(setting_key);
	ret["resource_path"] = ProjectSettings::get_singleton()->get_setting(setting_key, "");

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	Node *node = tree && tree->get_root() ? tree->get_root()->get_node_or_null(NodePath(name)) : nullptr;
	ret["loaded"] = node != nullptr;
	if (node) {
		ret["node"] = _serialize_node(node, false);
	}
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_find_nodes_by_script(const Dictionary &p_params) {
	String script_path = p_params.get("script_path", p_params.get("path", ""));
	String class_name = p_params.get("class_name", "");
	int limit = p_params.get("limit", 100);
	if (script_path.is_empty() && class_name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "script_path or class_name is required";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree || !tree->get_root()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Array matches;
	_find_nodes_by_script_recursive(tree->get_root(), script_path, class_name, limit, matches);
	Dictionary ret;
	ret["type"] = "nodes_by_script";
	ret["matches"] = matches;
	ret["count"] = matches.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_batch_get_properties(const Dictionary &p_params) {
	Array node_paths = p_params.get("nodes", p_params.get("node_paths", Array()));
	Array properties = p_params.get("properties", Array());
	if (node_paths.is_empty() || properties.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "nodes/node_paths and properties are required";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree || !tree->get_root()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Array rows;
	for (int i = 0; i < node_paths.size(); i++) {
		String node_path = node_paths[i];
		Node *node = tree->get_root()->get_node_or_null(NodePath(node_path));
		Dictionary row;
		row["path"] = node_path;
		row["found"] = node != nullptr;
		Dictionary values;
		if (node) {
			for (int j = 0; j < properties.size(); j++) {
				String property = properties[j];
				values[property] = _serialize_value(node->get(property));
			}
		}
		row["properties"] = values;
		rows.push_back(row);
	}

	Dictionary ret;
	ret["type"] = "batch_properties";
	ret["nodes"] = rows;
	ret["count"] = rows.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_find_ui_elements(const Dictionary &p_params) {
	String text = p_params.get("text", "");
	String type = p_params.get("ui_type", p_params.get("type", ""));
	bool visible_only = p_params.get("visible_only", true);
	int limit = p_params.get("limit", 100);
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree || !tree->get_root()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Array elements;
	_find_ui_elements_recursive(tree->get_root(), text, type, visible_only, limit, elements);
	Dictionary ret;
	ret["type"] = "ui_elements";
	ret["elements"] = elements;
	ret["count"] = elements.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_click_button_by_text(const Dictionary &p_params) {
	String text = p_params.get("text", "");
	if (text.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing 'text'";
		return ret;
	}
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	Node *found = tree && tree->get_root() ? _find_button_by_text_recursive(tree->get_root(), text) : nullptr;
	BaseButton *button = Object::cast_to<BaseButton>(found);
	if (!button) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Button not found with text: " + text;
		return ret;
	}

	List<Object::Connection> connection_list;
	button->get_signal_connection_list("pressed", &connection_list);
	int connection_count = 0;
	for (const Object::Connection &c : connection_list) {
		c.callable.call();
		connection_count++;
	}
	Dictionary ret;
	ret["type"] = "button_pressed";
	ret["node"] = String(button->get_path());
	ret["text"] = text;
	ret["connections_triggered"] = connection_count;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_move_node(const Dictionary &p_params) {
	String node_path = p_params.get("node", p_params.get("path", ""));
	Variant position = p_params.get("position", p_params.get("target", Variant()));
	if (node_path.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing node/path";
		return ret;
	}
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	Node *node = tree && tree->get_root() ? tree->get_root()->get_node_or_null(NodePath(node_path)) : nullptr;
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}
	node->set(p_params.get("property", "position"), _deserialize_value(position));
	Dictionary ret;
	ret["type"] = "node_moved";
	ret["node"] = node_path;
	ret["position"] = _serialize_value(node->get(p_params.get("property", "position")));
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_monitor_properties(const Dictionary &p_params) {
	String node_path = p_params.get("node", p_params.get("path", ""));
	Array properties = p_params.get("properties", Array());
	if (node_path.is_empty() || properties.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "node/path and properties are required";
		return ret;
	}
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	Node *node = tree && tree->get_root() ? tree->get_root()->get_node_or_null(NodePath(node_path)) : nullptr;
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}

	Dictionary values;
	for (int i = 0; i < properties.size(); i++) {
		String property = properties[i];
		values[property] = _serialize_value(node->get(property));
	}
	Dictionary ret;
	ret["type"] = "property_snapshot";
	ret["node"] = node_path;
	ret["properties"] = values;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_wait_for_property(const Dictionary &p_params) {
	String node_path = p_params.get("node", "");
	String property = p_params.get("property", "");
	Variant expected = p_params.get("value", Variant());

	if (node_path.is_empty() || property.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Requires 'node', 'property', 'value'";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}

	Variant current = node->get(property);
	bool matched = (String(_serialize_value(current)) == String(_serialize_value(expected)));

	Dictionary ret;
	ret["type"] = "wait_for_property_result";
	ret["node"] = node_path;
	ret["property"] = property;
	ret["matched"] = matched;
	ret["current_value"] = _serialize_value(current);
	ret["expected_value"] = _serialize_value(expected);
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_runtime_info(const Dictionary &p_params) {
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());

	String current_scene_path;
	String current_scene_name;
	int node_count = 0;

	if (tree) {
		Node *scene = tree->get_current_scene();
		if (scene) {
			current_scene_path = scene->get_scene_file_path();
			current_scene_name = String(scene->get_name());
		}
		node_count = (int)Performance::get_singleton()->get_monitor(Performance::OBJECT_NODE_COUNT);
	}

	Dictionary ret;
	ret["type"] = "runtime_info";
	ret["engine_version"] = Engine::get_singleton()->get_version_info().get("string", "unknown");
	ret["fps"] = Engine::get_singleton()->get_frames_per_second();
	ret["process_frames"] = (int64_t)Engine::get_singleton()->get_process_frames();
	ret["time_scale"] = Engine::get_singleton()->get_time_scale();
	ret["current_scene"] = current_scene_path;
	ret["current_scene_name"] = current_scene_name;
	ret["node_count"] = node_count;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_runtime_get_errors(const Dictionary &p_params) {
	int since_index = p_params.get("since_index", 0);

	MutexLock lock(_error_log_mutex);

	Array entries;
	for (int i = since_index; i < _error_log.size(); i++) {
		entries.push_back(_error_log[i]);
	}

	int error_count = 0;
	int warning_count = 0;
	for (int i = 0; i < _error_log.size(); i++) {
		String t = _error_log[i].get("type", "");
		if (t == "error") {
			error_count++;
		} else {
			warning_count++;
		}
	}

	Dictionary ret;
	ret["type"] = "runtime_errors";
	ret["errors"] = entries;
	ret["next_index"] = _error_log.size();
	ret["error_count"] = error_count;
	ret["warning_count"] = warning_count;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_runtime_capabilities(const Dictionary &p_params) {
	Array commands;
	// List all registered commands
	const char *all_cmds[] = {
		"ping", "get_tree", "get_node", "set_property", "call_method",
		"get_metrics", "capture_screenshot", "capture_viewport",
		"inject_action", "inject_key", "inject_mouse_click", "inject_mouse_motion",
		"watch_signal", "unwatch_signal",
		"inject_drag", "inject_scroll", "inject_gesture", "inject_gamepad",
		"find_nodes", "get_node_property", "call_node_method",
		"press_button", "wait_for_property",
		"runtime_info", "runtime_get_errors", "runtime_capabilities",
		"eval_expression", "runtime_quit", "get_network_info", "get_audio_info",
		"run_custom_command", "get_autoload", "find_nodes_by_script",
		"batch_get_properties", "find_ui_elements", "click_button_by_text",
		"navigate_to", "move_to", "monitor_properties",
		"auth_info", "performance",
		nullptr
	};
	for (int i = 0; all_cmds[i] != nullptr; i++) {
		commands.push_back(String(all_cmds[i]));
	}

	Dictionary ret;
	ret["type"] = "capabilities";
	ret["commands"] = commands;
	ret["count"] = commands.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_eval_expression(const Dictionary &p_params) {
	String expr_str = p_params.get("expr", "");
	if (expr_str.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing 'expr'";
		return ret;
	}

	// Security: reject forbidden patterns
	String expr_lower = expr_str.to_lower();
	for (int i = 0; _EVAL_BLOCKED_PATTERNS[i] != nullptr; i++) {
		if (expr_lower.contains(String(_EVAL_BLOCKED_PATTERNS[i]).to_lower())) {
			Dictionary ret;
			ret["type"] = "error";
			ret["message"] = vformat("Expression contains disallowed pattern: %s", _EVAL_BLOCKED_PATTERNS[i]);
			return ret;
		}
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	Node *context = tree ? tree->get_root() : nullptr;

	Expression expr;
	Error err = expr.parse(expr_str);
	if (err != OK) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Parse error: " + expr.get_error_text();
		return ret;
	}

	Variant result_v = expr.execute(Array(), context);
	if (expr.has_execute_failed()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Execution error: " + expr.get_error_text();
		return ret;
	}

	Dictionary ret;
	ret["type"] = "eval_result";
	ret["result"] = String(result_v);
	return ret;
}

// ── Signal watch ──

Dictionary JustAMCPRuntime::_cmd_watch_signal(const Dictionary &p_params) {
	String node_path = p_params.get("node", "");
	String signal_name = p_params.get("signal", "");

	if (node_path.is_empty() || signal_name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Requires 'node' and 'signal'";
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "No SceneTree available";
		return ret;
	}

	Node *node = tree->get_root()->get_node_or_null(node_path);
	if (!node) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Node not found: " + node_path;
		return ret;
	}

	if (!node->has_signal(signal_name)) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Signal not found on node: " + signal_name;
		return ret;
	}

	String key = node_path + ":" + signal_name;
	if (watched_signals.has(key)) {
		Dictionary ret;
		ret["type"] = "signal_watching";
		ret["node"] = node_path;
		ret["signal"] = signal_name;
		ret["status"] = "already_watching";
		return ret;
	}

	Callable c = callable_mp(this, &JustAMCPRuntime::_broadcast_signal_event).bind(node_path, signal_name);
	node->connect(signal_name, c);
	watched_signals[key] = c;

	Dictionary ret;
	ret["type"] = "signal_watching";
	ret["node"] = node_path;
	ret["signal"] = signal_name;
	ret["status"] = "started";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_unwatch_signal(const Dictionary &p_params) {
	String node_path = p_params.get("node", "");
	String signal_name = p_params.get("signal", "");

	String key = node_path + ":" + signal_name;
	if (!watched_signals.has(key)) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Not watching signal: " + key;
		return ret;
	}

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		Node *node = tree->get_root()->get_node_or_null(node_path);
		if (node) {
			node->disconnect(signal_name, watched_signals[key]);
		}
	}

	watched_signals.erase(key);

	Dictionary ret;
	ret["type"] = "signal_unwatched";
	ret["node"] = node_path;
	ret["signal"] = signal_name;
	return ret;
}

void JustAMCPRuntime::_broadcast_signal_event(const String &p_node_path, const String &p_signal_name, const Array &p_args) {
	Dictionary event;
	event["type"] = "signal_event";
	event["node"] = p_node_path;
	event["signal"] = p_signal_name;

	Array serialized_args;
	for (int i = 0; i < p_args.size(); i++) {
		serialized_args.push_back(_serialize_value(p_args[i]));
	}
	event["args"] = serialized_args;

	MutexLock lock(clients_mutex);
	for (int i = 0; i < clients.size(); i++) {
		_send_response(clients[i], event);
	}
}

// ── Serialization ──

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
	switch (p_value.get_type()) {
		case Variant::NIL:
			return p_value;
		case Variant::BOOL:
		case Variant::INT:
		case Variant::FLOAT:
		case Variant::STRING:
			return p_value;
		case Variant::VECTOR2: {
			Vector2 v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::VECTOR2I: {
			Vector2i v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			return d;
		}
		case Variant::RECT2: {
			Rect2 r = p_value;
			Dictionary d;
			d["x"] = r.position.x;
			d["y"] = r.position.y;
			d["w"] = r.size.x;
			d["h"] = r.size.y;
			return d;
		}
		case Variant::RECT2I: {
			Rect2i r = p_value;
			Dictionary d;
			d["x"] = r.position.x;
			d["y"] = r.position.y;
			d["w"] = r.size.x;
			d["h"] = r.size.y;
			return d;
		}
		case Variant::VECTOR3: {
			Vector3 v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::VECTOR3I: {
			Vector3i v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			return d;
		}
		case Variant::TRANSFORM2D: {
			Transform2D t = p_value;
			Array a;
			a.push_back(_serialize_value(t.get_origin()));
			a.push_back(_serialize_value(t[0]));
			a.push_back(_serialize_value(t[1]));
			return a;
		}
		case Variant::VECTOR4: {
			Vector4 v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			d["w"] = v.w;
			return d;
		}
		case Variant::VECTOR4I: {
			Vector4i v = p_value;
			Dictionary d;
			d["x"] = v.x;
			d["y"] = v.y;
			d["z"] = v.z;
			d["w"] = v.w;
			return d;
		}
		case Variant::PLANE: {
			Plane p = p_value;
			Dictionary d;
			d["normal"] = _serialize_value(p.normal);
			d["d"] = p.d;
			return d;
		}
		case Variant::QUATERNION: {
			Quaternion q = p_value;
			Dictionary d;
			d["x"] = q.x;
			d["y"] = q.y;
			d["z"] = q.z;
			d["w"] = q.w;
			return d;
		}
		case Variant::AABB: {
			AABB a = p_value;
			Dictionary d;
			d["position"] = _serialize_value(a.position);
			d["size"] = _serialize_value(a.size);
			return d;
		}
		case Variant::BASIS: {
			Basis b = p_value;
			Array a;
			a.push_back(_serialize_value(b.get_column(0)));
			a.push_back(_serialize_value(b.get_column(1)));
			a.push_back(_serialize_value(b.get_column(2)));
			return a;
		}
		case Variant::TRANSFORM3D: {
			Transform3D t = p_value;
			Dictionary d;
			d["basis"] = _serialize_value(t.basis);
			d["origin"] = _serialize_value(t.origin);
			return d;
		}
		case Variant::PROJECTION: {
			Projection p = p_value;
			Array a;
			a.push_back(_serialize_value(p.columns[0]));
			a.push_back(_serialize_value(p.columns[1]));
			a.push_back(_serialize_value(p.columns[2]));
			a.push_back(_serialize_value(p.columns[3]));
			return a;
		}
		case Variant::COLOR: {
			Color c = p_value;
			return c.to_html(true);
		}
		case Variant::NODE_PATH:
			return String(p_value);
		case Variant::RID: {
			RID rid = p_value;
			return (int64_t)rid.get_id();
		}
		case Variant::OBJECT: {
			Object *obj = p_value;
			if (!obj) {
				return Variant();
			}
			Node *n = Object::cast_to<Node>(obj);
			if (n) {
				return String(n->get_path());
			}
			return obj->get_class();
		}
		case Variant::DICTIONARY: {
			Dictionary d = p_value;
			Dictionary res;
			List<Variant> keys;
			d.get_key_list(&keys);
			for (const Variant &k : keys) {
				res[String(k)] = _serialize_value(d[k]);
			}
			return res;
		}
		case Variant::ARRAY: {
			Array a = p_value;
			Array res;
			for (int i = 0; i < a.size(); i++) {
				res.push_back(_serialize_value(a[i]));
			}
			return res;
		}
		default:
			return String(p_value);
	}
}

Variant JustAMCPRuntime::_deserialize_value(const Variant &p_value) {
	if (p_value.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_value;
		if (d.has("x") && d.has("y") && d.size() == 2) {
			return Vector2(float(d["x"]), float(d["y"]));
		}
		if (d.has("x") && d.has("y") && d.has("z") && d.size() == 3) {
			return Vector3(float(d["x"]), float(d["y"]), float(d["z"]));
		}
		if (d.has("x") && d.has("y") && d.has("z") && d.has("w") && d.size() == 4) {
			return Vector4(float(d["x"]), float(d["y"]), float(d["z"]), float(d["w"]));
		}
		if (d.has("r") && d.has("g") && d.has("b")) {
			float a = d.has("a") ? float(d["a"]) : 1.0f;
			return Color(float(d["r"]), float(d["g"]), float(d["b"]), a);
		}
		// Recursive dictionary deserialization
		Dictionary res;
		List<Variant> keys;
		d.get_key_list(&keys);
		for (const Variant &k : keys) {
			res[k] = _deserialize_value(d[k]);
		}
		return res;
	} else if (p_value.get_type() == Variant::ARRAY) {
		Array a = p_value;
		Array res;
		for (int i = 0; i < a.size(); i++) {
			res.push_back(_deserialize_value(a[i]));
		}
		return res;
	} else if (p_value.get_type() == Variant::STRING) {
		String s = p_value;
		if (s.begins_with("#") && (s.length() == 7 || s.length() == 9)) {
			return Color::html(s);
		}
	}
	return p_value;
}
Dictionary JustAMCPRuntime::_cmd_runtime_quit(const Dictionary &p_params) {
	MessageQueue::get_singleton()->push_callable(callable_mp(this, &JustAMCPRuntime::_quit_engine));

	Dictionary ret;
	ret["status"] = "quitting";
	return ret;
}

void JustAMCPRuntime::_quit_engine() {
	OS::get_singleton()->kill(OS::get_singleton()->get_process_id());
}

Dictionary JustAMCPRuntime::_cmd_auth_info(const Dictionary &p_params) {
	Dictionary ret;
	ret["proto"] = "blazium/1";
	ret["tier"] = 3;
	ret["danger_enabled"] = true;
	ret["engine"] = "Blazium";
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_network_info(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "network_info";

	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (!tree) {
		ret["error"] = "No SceneTree";
		return ret;
	}

	Ref<MultiplayerAPI> mp = tree->get_multiplayer();
	if (mp.is_valid()) {
		ret["is_active"] = true;
		ret["unique_id"] = mp->get_unique_id();
		ret["is_server"] = mp->is_server();

		Array peers;
		Vector<int> peer_ids = mp->get_peer_ids();
		for (int i = 0; i < peer_ids.size(); i++) {
			peers.push_back(peer_ids[i]);
		}
		ret["peers"] = peers;
	} else {
		ret["is_active"] = false;
	}

	return ret;
}

Dictionary JustAMCPRuntime::_cmd_get_audio_info(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "audio_info";

	AudioServer *as = AudioServer::get_singleton();
	Array buses;
	for (int i = 0; i < as->get_bus_count(); i++) {
		Dictionary bus;
		bus["name"] = as->get_bus_name(i);
		bus["volume_db"] = as->get_bus_volume_db(i);
		bus["mute"] = as->is_bus_mute(i);
		bus["bypass_effects"] = as->is_bus_bypassing_effects(i);
		bus["channels"] = as->get_bus_channels(i);
		buses.push_back(bus);
	}
	ret["buses"] = buses;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_inject_gamepad(const Dictionary &p_params) {
	int device = p_params.get("device", 0);
	String input_type = p_params.get("type", "button"); // "button" or "axis"

	if (input_type == "button") {
		int button_index = p_params.get("index", (int)JoyButton::A);
		float pressure = p_params.get("pressure", 1.0f);
		bool pressed = p_params.get("pressed", true);

		Ref<InputEventJoypadButton> ev;
		ev.instantiate();
		ev->set_device(device);
		ev->set_button_index((JoyButton)button_index);
		ev->set_pressure(pressure);
		ev->set_pressed(pressed);
		_inject_event(ev);

	} else if (input_type == "axis") {
		int axis_index = p_params.get("index", (int)JoyAxis::LEFT_X);
		float value = p_params.get("value", 1.0f);

		Ref<InputEventJoypadMotion> ev;
		ev.instantiate();
		ev->set_device(device);
		ev->set_axis((JoyAxis)axis_index);
		ev->set_axis_value(value);
		_inject_event(ev);
	}

	Dictionary ret;
	ret["type"] = "input_injected";
	ret["device"] = device;
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_run_custom_command(const Dictionary &p_params) {
	String name = p_params.get("name", "");
	Array args = p_params.get("args", Array());

	if (name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing custom command 'name'";
		return ret;
	}

	if (!_custom_commands.has(name)) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Custom command not found: " + name;
		return ret;
	}

	Callable c = _custom_commands[name];
	if (!c.is_valid()) {
		_custom_commands.erase(name);
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Custom command callable is no longer valid: " + name;
		return ret;
	}

	Variant result = c.callv(args);

	Dictionary ret;
	ret["type"] = "custom_command_result";
	ret["name"] = name;
	ret["result"] = _serialize_value(result);
	return ret;
}

void JustAMCPRuntime::register_custom_command(const String &p_name, const Callable &p_callable) {
	_custom_commands[p_name] = p_callable;
}

void JustAMCPRuntime::unregister_custom_command(const String &p_name) {
	_custom_commands.erase(p_name);
}
Dictionary JustAMCPRuntime::_cmd_tool_call(const Dictionary &p_params) {
	if (!executor) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Tool executor not initialized";
		return ret;
	}

	String tool_name = p_params.get("name", "");
	Dictionary tool_args = p_params.get("args", Dictionary());

	if (tool_name.is_empty()) {
		Dictionary ret;
		ret["type"] = "error";
		ret["message"] = "Missing tool 'name'";
		return ret;
	}

	return executor->execute_tool(tool_name, tool_args);
}

Dictionary JustAMCPRuntime::_cmd_list_tools(const Dictionary &p_params) {
	Dictionary ret;
	ret["type"] = "tool_list";
	ret["tools"] = JustAMCPToolExecutor::get_tool_schemas(false, true);
	return ret;
}
Dictionary JustAMCPRuntime::_cmd_get_log_tail(const Dictionary &p_params) {
	int count = p_params.get("count", 20);
	if (count <= 0) {
		count = 20;
	}
	if (count > 500) {
		count = 500;
	}

	MutexLock lock(_error_log_mutex);
	Array tail;
	int start = MAX(0, (int)_error_log.size() - count);
	for (int i = start; i < _error_log.size(); i++) {
		tail.push_back(_error_log[i]);
	}

	Dictionary ret;
	ret["type"] = "log_tail";
	ret["logs"] = tail;
	ret["total_available"] = _error_log.size();
	return ret;
}

Dictionary JustAMCPRuntime::_cmd_diagnose(const Dictionary &p_params) {
	Dictionary report;
	report["type"] = "diagnostic_report";
	report["timestamp"] = Time::get_singleton()->get_unix_time_from_system();
	report["engine"] = "Blazium Engine (JustAMCP Module)";

	// 1. Performance metrics
	report["metrics"] = _cmd_get_metrics(Dictionary());

	// 2. Latest logs
	Dictionary log_params;
	log_params["count"] = 10;
	report["recent_logs"] = _cmd_get_log_tail(log_params).get("logs", Array());

	// 3. Scene Tree Summary
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		Node *root = tree->get_root();
		Dictionary scene;
		scene["node_count"] = tree->get_node_count();
		if (tree->get_current_scene()) {
			scene["current_scene"] = tree->get_current_scene()->get_scene_file_path();
			scene["root_name"] = tree->get_current_scene()->get_name();
		} else {
			scene["current_scene"] = "None";
			scene["root_name"] = root ? String(root->get_name()) : "null";
		}
		report["scene"] = scene;
	}

	return report;
}
