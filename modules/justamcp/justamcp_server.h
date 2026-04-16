/**************************************************************************/
/*  justamcp_server.h                                                     */
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

#include "modules/modules_enabled.gen.h"
#include "scene/main/node.h"

#if defined(MODULE_HTTPSERVER_ENABLED)
#include "core/os/mutex.h"
#include "core/os/semaphore.h"
#include "modules/httpserver/http_request_context.h"
#include "modules/httpserver/http_response.h"
#include "modules/httpserver/http_server.h"
#endif

class JustAMCPServer : public Node {
	GDCLASS(JustAMCPServer, Node);

private:
	Ref<HTTPResponse> pending_stateless_response;
	Semaphore pending_stateless_semaphore;
	Mutex pending_stateless_mutex;
	Dictionary pending_stateless_result;

	int current_sse_connection_id = -1;
	bool server_started = false;
	String current_log_level = "info";

	class JustAMCPPromptExecutor *prompt_executor = nullptr;
	class JustAMCPResourceExecutor *resource_executor = nullptr;
	class JustAMCPTaskManager *task_manager = nullptr;

	void _setup_settings();
	void _start_server();
	void _stop_server();
	void _on_settings_changed();

#if defined(MODULE_HTTPSERVER_ENABLED)
	void _handle_sse_connect(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response);
	void _handle_message_post(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response);
	void _handle_mcp_stateless_post(Ref<HTTPRequestContext> p_context, Ref<HTTPResponse> p_response);
	Dictionary _handle_json_rpc(const String &p_body, Ref<HTTPResponse> p_response);
	void _send_sse_message(const String &p_json_string);
	void _on_sse_connection_opened(int p_connection_id, const String &p_path, const Dictionary &p_headers);
#endif

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void send_tool_result(const Variant &p_request_id, bool p_success, const Variant &p_result = Variant(), const String &p_error = "");
	void send_elicitation_request(const String &p_request_id, const String &p_mode, const String &p_message, const Variant &p_url_or_schema);
	void send_url_elicitation_error(const String &p_request_id, const String &p_elicitation_id, const String &p_url, const String &p_message);
	void broadcast_prompts_list_changed();
	void broadcast_tools_list_changed();
	void broadcast_resources_list_changed();
	void broadcast_resource_updated(const String &p_uri);
	void send_log_message(const String &p_level, const String &p_logger, const Variant &p_data);
	void send_progress_notification(const String &p_token, double p_progress, double p_total, const String &p_message);
	void broadcast_task_status(const String &p_task_id);

	bool is_server_started() const { return server_started; }

	JustAMCPServer();
	~JustAMCPServer();
};
