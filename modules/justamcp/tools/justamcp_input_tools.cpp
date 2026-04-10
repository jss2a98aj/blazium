/**************************************************************************/
/*  justamcp_input_tools.cpp                                              */
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

#include "justamcp_input_tools.h"

#include "core/io/file_access.h"
#include "core/io/json.h"

static inline Dictionary _MCP_SUCCESS(const Variant &data) {
	Dictionary r;
	r["ok"] = true;
	r["result"] = data;
	return r;
}
static inline Dictionary _MCP_ERROR_INTERNAL(int code, const String &msg) {
	Dictionary e, r;
	e["code"] = code;
	e["message"] = msg;
	r["ok"] = false;
	r["error"] = e;
	return r;
}

#undef MCP_SUCCESS
#undef MCP_ERROR
#undef MCP_INVALID_PARAMS
#undef MCP_INTERNAL
#define MCP_SUCCESS(data) _MCP_SUCCESS(data)
#define MCP_ERROR(code, msg) _MCP_ERROR_INTERNAL(code, msg)
#define MCP_INVALID_PARAMS(msg) _MCP_ERROR_INTERNAL(-32602, msg)
#define MCP_INTERNAL(msg) _MCP_ERROR_INTERNAL(-32603, String("Internal error: ") + msg)

void JustAMCPInputTools::_bind_methods() {}

JustAMCPInputTools::JustAMCPInputTools() {}
JustAMCPInputTools::~JustAMCPInputTools() {}

void JustAMCPInputTools::_write_commands(const Array &p_events) {
	String json = JSON::stringify(p_events);
	Ref<FileAccess> file = FileAccess::open(COMMANDS_PATH, FileAccess::WRITE);
	if (file.is_valid()) {
		file->store_string(json);
		file->close();
	} else {
		ERR_PRINT("[MCP Input] Failed to write commands");
	}
}

Dictionary JustAMCPInputTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "simulate_key") {
		return _simulate_key(p_args);
	}
	if (p_tool_name == "simulate_mouse_click") {
		return _simulate_mouse_click(p_args);
	}
	if (p_tool_name == "simulate_mouse_move") {
		return _simulate_mouse_move(p_args);
	}
	if (p_tool_name == "simulate_action") {
		return _simulate_action(p_args);
	}
	if (p_tool_name == "simulate_sequence") {
		return _simulate_sequence(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPInputTools::_simulate_key(const Dictionary &p_params) {
	if (!p_params.has("keycode")) {
		return MCP_INVALID_PARAMS("Missing param: keycode");
	}
	String keycode = p_params["keycode"];

	bool pressed = p_params.has("pressed") ? bool(p_params["pressed"]) : true;
	bool shift = p_params.has("shift") ? bool(p_params["shift"]) : false;
	bool ctrl = p_params.has("ctrl") ? bool(p_params["ctrl"]) : false;
	bool alt = p_params.has("alt") ? bool(p_params["alt"]) : false;

	Dictionary event;
	event["type"] = "key";
	event["keycode"] = keycode;
	event["pressed"] = pressed;
	event["shift"] = shift;
	event["ctrl"] = ctrl;
	event["alt"] = alt;

	Array events;
	events.push_back(event);
	_write_commands(events);

	Dictionary res;
	res["sent"] = true;
	res["event"] = event;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPInputTools::_simulate_mouse_click(const Dictionary &p_params) {
	int button = p_params.has("button") ? int(p_params["button"]) : 1; // MOUSE_BUTTON_LEFT
	bool pressed = p_params.has("pressed") ? bool(p_params["pressed"]) : true;
	bool double_click = p_params.has("double_click") ? bool(p_params["double_click"]) : false;
	bool auto_release = p_params.has("auto_release") ? bool(p_params["auto_release"]) : true;
	float x = p_params.has("x") ? float(p_params["x"]) : 0.0;
	float y = p_params.has("y") ? float(p_params["y"]) : 0.0;

	Dictionary press_event;
	press_event["type"] = "mouse_button";
	press_event["button"] = button;
	press_event["pressed"] = pressed;
	press_event["double_click"] = double_click;
	Dictionary pos;
	pos["x"] = x;
	pos["y"] = y;
	press_event["position"] = pos;

	if (pressed && auto_release) {
		Dictionary release_event = press_event.duplicate();
		release_event["pressed"] = false;

		Dictionary sequence_data;
		Array seq_events;
		seq_events.push_back(press_event);
		seq_events.push_back(release_event);
		sequence_data["sequence_events"] = seq_events;
		sequence_data["frame_delay"] = 1;

		String json = JSON::stringify(sequence_data);
		Ref<FileAccess> file = FileAccess::open(COMMANDS_PATH, FileAccess::WRITE);
		if (file.is_null()) {
			return MCP_INTERNAL("Failed to write commands");
		}
		file->store_string(json);
		file->close();

		Dictionary res;
		res["sent"] = true;
		res["event"] = press_event;
		res["auto_release"] = true;
		return MCP_SUCCESS(res);
	}

	Array events;
	events.push_back(press_event);
	_write_commands(events);

	Dictionary res;
	res["sent"] = true;
	res["event"] = press_event;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPInputTools::_simulate_mouse_move(const Dictionary &p_params) {
	float x = p_params.has("x") ? float(p_params["x"]) : 0.0;
	float y = p_params.has("y") ? float(p_params["y"]) : 0.0;
	float rel_x = p_params.has("relative_x") ? float(p_params["relative_x"]) : 0.0;
	float rel_y = p_params.has("relative_y") ? float(p_params["relative_y"]) : 0.0;
	int button_mask = p_params.has("button_mask") ? int(p_params["button_mask"]) : 0;
	bool unhandled = p_params.has("unhandled") ? bool(p_params["unhandled"]) : false;

	Dictionary event;
	event["type"] = "mouse_motion";
	Dictionary pos;
	pos["x"] = x;
	pos["y"] = y;
	event["position"] = pos;
	Dictionary rel;
	rel["x"] = rel_x;
	rel["y"] = rel_y;
	event["relative"] = rel;
	event["button_mask"] = button_mask;

	if (unhandled || button_mask > 0) {
		event["unhandled"] = true;
	}

	Array events;
	events.push_back(event);
	_write_commands(events);

	Dictionary res;
	res["sent"] = true;
	res["event"] = event;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPInputTools::_simulate_action(const Dictionary &p_params) {
	if (!p_params.has("action")) {
		return MCP_INVALID_PARAMS("Missing param: action");
	}
	String action_name = p_params["action"];

	bool pressed = p_params.has("pressed") ? bool(p_params["pressed"]) : true;
	float strength = p_params.has("strength") ? float(p_params["strength"]) : 1.0;

	Dictionary event;
	event["type"] = "action";
	event["action"] = action_name;
	event["pressed"] = pressed;
	event["strength"] = strength;

	Array events;
	events.push_back(event);
	_write_commands(events);

	Dictionary res;
	res["sent"] = true;
	res["event"] = event;
	return MCP_SUCCESS(res);
}

Dictionary JustAMCPInputTools::_simulate_sequence(const Dictionary &p_params) {
	if (!p_params.has("events") || p_params["events"].get_type() != Variant::ARRAY) {
		return MCP_INVALID_PARAMS("Missing required parameter: events (Array)");
	}
	Array events = p_params["events"];
	if (events.is_empty()) {
		return MCP_INVALID_PARAMS("Events array is empty");
	}

	int frame_delay = p_params.has("frame_delay") ? int(p_params["frame_delay"]) : 1;

	for (int i = 0; i < events.size(); i++) {
		if (events[i].get_type() != Variant::DICTIONARY) {
			return MCP_INVALID_PARAMS("Invalid event in sequence");
		}
		Dictionary event_data = events[i];
		if (!event_data.has("type") || String(event_data["type"]).is_empty()) {
			return MCP_INVALID_PARAMS("Invalid event in sequence");
		}
	}

	if (frame_delay <= 0) {
		_write_commands(events);
	} else {
		Dictionary sequence_data;
		sequence_data["sequence_events"] = events;
		sequence_data["frame_delay"] = frame_delay;

		String json = JSON::stringify(sequence_data);
		Ref<FileAccess> file = FileAccess::open(COMMANDS_PATH, FileAccess::WRITE);
		if (file.is_null()) {
			return MCP_INTERNAL("Failed to write commands");
		}
		file->store_string(json);
		file->close();
	}

	Dictionary res;
	res["sent"] = true;
	res["event_count"] = events.size();
	res["frame_delay"] = frame_delay;
	return MCP_SUCCESS(res);
}
