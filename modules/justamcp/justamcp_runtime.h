/**************************************************************************/
/*  justamcp_runtime.h                                                    */
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

#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "core/object/object.h"
#include "core/os/mutex.h"
#include "core/os/thread.h"
#include <atomic>

class JustAMCPRuntime : public Object {
	GDCLASS(JustAMCPRuntime, Object);

private:
	static JustAMCPRuntime *singleton;

	Ref<TCPServer> server;
	Vector<Ref<StreamPeerTCP>> clients;
	HashMap<Ref<StreamPeerTCP>, String> client_buffers;
	Mutex clients_mutex;
	int port = 7777;
	bool enabled = false;
	Thread *server_thread = nullptr;
	std::atomic<bool> quit_thread{ false };
	void _thread_poll();
	static void _thread_poll_wrapper(void *p_user);
	HashMap<String, Callable> watched_signals;

	void _start_server();
	void _send_welcome(Ref<StreamPeerTCP> p_client);
	void _handle_message(Ref<StreamPeerTCP> p_client, const String &p_data);
	void _deferred_execute_command(Ref<StreamPeerTCP> p_client, String p_command, Dictionary p_params, Variant p_request_id);

public:
	Dictionary execute_command(const String &p_command, const Dictionary &p_params);

private:
	// Commands
	Dictionary _cmd_get_tree(const Dictionary &p_params);
	Dictionary _cmd_get_node(const Dictionary &p_params);
	Dictionary _cmd_set_property(const Dictionary &p_params);
	Dictionary _cmd_call_method(const Dictionary &p_params);
	Dictionary _cmd_get_metrics(const Dictionary &p_params);
	Dictionary _cmd_capture_screenshot(const Dictionary &p_params);
	Dictionary _cmd_capture_viewport(const Dictionary &p_params);
	Dictionary _cmd_inject_action(const Dictionary &p_params);
	Dictionary _cmd_inject_key(const Dictionary &p_params);
	Dictionary _cmd_inject_mouse_click(const Dictionary &p_params);
	Dictionary _cmd_inject_mouse_motion(const Dictionary &p_params);
	Dictionary _cmd_watch_signal(const Dictionary &p_params);
	Dictionary _cmd_unwatch_signal(const Dictionary &p_params);

	void _broadcast_signal_event(const String &p_node_path, const String &p_signal_name, const Array &p_args);

	// Serialization
	Dictionary _serialize_node_tree(Node *p_node, int p_depth, int p_max_depth, bool p_include_properties);
	Dictionary _serialize_node(Node *p_node, bool p_include_properties);
	Variant _serialize_value(const Variant &p_value);
	Variant _deserialize_value(const Variant &p_value);

	void _send_response(Ref<StreamPeerTCP> p_client, const Dictionary &p_data);
	void _send_error(Ref<StreamPeerTCP> p_client, const String &p_message);
	void _cleanup();

protected:
	static void _bind_methods();
	void poll();

public:
	static JustAMCPRuntime *get_singleton();

	void set_port(int p_port);
	int get_port() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	JustAMCPRuntime();
	~JustAMCPRuntime();
};
