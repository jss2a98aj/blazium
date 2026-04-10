/**************************************************************************/
/*  justamcp_profiling_tools.cpp                                          */
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

#include "justamcp_profiling_tools.h"
#include "main/performance.h"

// Helper macros for returning errors analogous to base_command.gd
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
[[maybe_unused]] static inline Dictionary _MCP_ERROR_DATA(int code, const String &msg, const Variant &data) {
	Dictionary e, r;
	e["code"] = code;
	e["message"] = msg;
	e["data"] = data;
	r["ok"] = false;
	r["error"] = e;
	return r;
}
#undef MCP_SUCCESS
#undef MCP_ERROR
#undef MCP_ERROR_DATA
#undef MCP_INVALID_PARAMS
#undef MCP_NOT_FOUND
#undef MCP_INTERNAL
#define MCP_SUCCESS(data) return _MCP_SUCCESS(data)
#define MCP_ERROR(code, msg) return _MCP_ERROR_INTERNAL(code, msg)
#define MCP_ERROR_DATA(code, msg, data) return _MCP_ERROR_DATA(code, msg, data)
#define MCP_INVALID_PARAMS(msg) return _MCP_ERROR_INTERNAL(-32602, msg)
#define MCP_NOT_FOUND(msg) return _MCP_ERROR_DATA(-32001, String(msg) + " not found", Dictionary())
#define MCP_INTERNAL(msg) return _MCP_ERROR_INTERNAL(-32603, String("Internal error: ") + msg)

void JustAMCPProfilingTools::_bind_methods() {}

JustAMCPProfilingTools::JustAMCPProfilingTools() {}

JustAMCPProfilingTools::~JustAMCPProfilingTools() {}

Dictionary JustAMCPProfilingTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "get_performance_monitors") {
		return _get_performance_monitors(p_args);
	} else if (p_tool_name == "get_editor_performance") {
		return _get_editor_performance(p_args);
	}

	Dictionary err;
	err["code"] = -32601;
	err["message"] = "Method not found: " + p_tool_name;
	Dictionary res;
	res["error"] = err;
	return res;
}

Dictionary JustAMCPProfilingTools::_get_performance_monitors(const Dictionary &p_params) {
	Dictionary monitors;
	Performance *perf = Performance::get_singleton();
	if (!perf) {
		MCP_ERROR(-32603, "Performance singleton is not available");
	}

	monitors["fps"] = perf->get_monitor(Performance::TIME_FPS);
	monitors["frame_time_msec"] = perf->get_monitor(Performance::TIME_PROCESS) * 1000.0;
	monitors["physics_frame_time_msec"] = perf->get_monitor(Performance::TIME_PHYSICS_PROCESS) * 1000.0;
	monitors["navigation_process_msec"] = perf->get_monitor(Performance::TIME_NAVIGATION_PROCESS) * 1000.0;

	monitors["memory_static"] = perf->get_monitor(Performance::MEMORY_STATIC);
	monitors["memory_static_max"] = perf->get_monitor(Performance::MEMORY_STATIC_MAX);

	monitors["object_count"] = perf->get_monitor(Performance::OBJECT_COUNT);
	monitors["object_resource_count"] = perf->get_monitor(Performance::OBJECT_RESOURCE_COUNT);
	monitors["object_node_count"] = perf->get_monitor(Performance::OBJECT_NODE_COUNT);
	monitors["object_orphan_node_count"] = perf->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);

	monitors["render_total_objects_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
	monitors["render_total_primitives_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	monitors["render_total_draw_calls_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	monitors["render_video_mem_used"] = perf->get_monitor(Performance::RENDER_VIDEO_MEM_USED);

	monitors["physics_2d_active_objects"] = perf->get_monitor(Performance::PHYSICS_2D_ACTIVE_OBJECTS);
	monitors["physics_2d_collision_pairs"] = perf->get_monitor(Performance::PHYSICS_2D_COLLISION_PAIRS);
	monitors["physics_2d_island_count"] = perf->get_monitor(Performance::PHYSICS_2D_ISLAND_COUNT);

	monitors["physics_3d_active_objects"] = perf->get_monitor(Performance::PHYSICS_3D_ACTIVE_OBJECTS);
	monitors["physics_3d_collision_pairs"] = perf->get_monitor(Performance::PHYSICS_3D_COLLISION_PAIRS);
	monitors["physics_3d_island_count"] = perf->get_monitor(Performance::PHYSICS_3D_ISLAND_COUNT);

	monitors["navigation_active_maps"] = perf->get_monitor(Performance::NAVIGATION_ACTIVE_MAPS);
	monitors["navigation_region_count"] = perf->get_monitor(Performance::NAVIGATION_REGION_COUNT);
	monitors["navigation_agent_count"] = perf->get_monitor(Performance::NAVIGATION_AGENT_COUNT);

	String category = p_params.has("category") ? String(p_params["category"]) : "";
	if (!category.is_empty()) {
		Dictionary filtered;
		Array keys = monitors.keys();
		for (int i = 0; i < keys.size(); i++) {
			String key = keys[i];
			if (key.begins_with(category)) {
				filtered[key] = monitors[key];
			}
		}
		Dictionary res;
		res["monitors"] = filtered;
		res["category"] = category;
		MCP_SUCCESS(res);
	}

	Dictionary res;
	res["monitors"] = monitors;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPProfilingTools::_get_editor_performance(const Dictionary &p_params) {
	Performance *perf = Performance::get_singleton();
	if (!perf) {
		MCP_ERROR(-32603, "Performance singleton is not available");
	}

	Dictionary summary;
	summary["fps"] = perf->get_monitor(Performance::TIME_FPS);
	summary["frame_time_msec"] = perf->get_monitor(Performance::TIME_PROCESS) * 1000.0;
	summary["draw_calls"] = perf->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	summary["objects_in_frame"] = perf->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME);
	summary["node_count"] = perf->get_monitor(Performance::OBJECT_NODE_COUNT);
	summary["orphan_nodes"] = perf->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);
	summary["memory_static_mb"] = double(perf->get_monitor(Performance::MEMORY_STATIC)) / (1024.0 * 1024.0);
	summary["video_mem_mb"] = double(perf->get_monitor(Performance::RENDER_VIDEO_MEM_USED)) / (1024.0 * 1024.0);

	MCP_SUCCESS(summary);
}
