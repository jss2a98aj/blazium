/**************************************************************************/
/*  justamcp_tilemap_tools.cpp                                            */
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

#include "justamcp_tilemap_tools.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "justamcp_tool_executor.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/resources/2d/tile_set.h"

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

JustAMCPTileMapTools::JustAMCPTileMapTools() {
}

JustAMCPTileMapTools::~JustAMCPTileMapTools() {
}

Node *JustAMCPTileMapTools::_get_edited_root() {
	if (JustAMCPToolExecutor::get_test_scene_root()) {
		return JustAMCPToolExecutor::get_test_scene_root();
	}
	if (!EditorNode::get_singleton() || !EditorInterface::get_singleton()) {
		return nullptr;
	}
	return EditorInterface::get_singleton()->get_edited_scene_root();
}

Node *JustAMCPTileMapTools::_find_node_by_path(const String &p_path) {
	if (p_path == "." || p_path.is_empty()) {
		return _get_edited_root();
	}
	Node *root = _get_edited_root();
	if (!root) {
		return nullptr;
	}
	return root->get_node_or_null(NodePath(p_path));
}

Dictionary JustAMCPTileMapTools::execute_tool(const String &p_tool_name, const Dictionary &p_args) {
	if (p_tool_name == "tilemap_set_cell") {
		return tilemap_set_cell(p_args);
	}
	if (p_tool_name == "tilemap_fill_rect") {
		return tilemap_fill_rect(p_args);
	}
	if (p_tool_name == "tilemap_get_cell") {
		return tilemap_get_cell(p_args);
	}
	if (p_tool_name == "tilemap_clear") {
		return tilemap_clear(p_args);
	}
	if (p_tool_name == "tilemap_get_info") {
		return tilemap_get_info(p_args);
	}
	if (p_tool_name == "tilemap_get_used_cells") {
		return tilemap_get_used_cells(p_args);
	}

	MCP_ERROR(-32601, "Method not found: " + p_tool_name);
}

Dictionary JustAMCPTileMapTools::tilemap_set_cell(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	int x = p_params.get("x", 0);
	int y = p_params.get("y", 0);
	int source_id = p_params.get("source_id", 0);
	int atlas_x = p_params.get("atlas_x", 0);
	int atlas_y = p_params.get("atlas_y", 0);
	int alternative = p_params.get("alternative", 0);

	tilemap->set_cell(Vector2i(x, y), source_id, Vector2i(atlas_x, atlas_y), alternative);

	Dictionary res;
	res["x"] = x;
	res["y"] = y;
	res["source_id"] = source_id;
	Array atlas_coords;
	atlas_coords.push_back(atlas_x);
	atlas_coords.push_back(atlas_y);
	res["atlas_coords"] = atlas_coords;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPTileMapTools::tilemap_fill_rect(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	int x1 = p_params.get("x1", 0);
	int y1 = p_params.get("y1", 0);
	int x2 = p_params.get("x2", 0);
	int y2 = p_params.get("y2", 0);
	int source_id = p_params.get("source_id", 0);
	int atlas_x = p_params.get("atlas_x", 0);
	int atlas_y = p_params.get("atlas_y", 0);
	int alternative = p_params.get("alternative", 0);

	int min_x = MIN(x1, x2);
	int max_x = MAX(x1, x2);
	int min_y = MIN(y1, y2);
	int max_y = MAX(y1, y2);

	int count = 0;
	for (int cx = min_x; cx <= max_x; cx++) {
		for (int cy = min_y; cy <= max_y; cy++) {
			tilemap->set_cell(Vector2i(cx, cy), source_id, Vector2i(atlas_x, atlas_y), alternative);
			count++;
		}
	}

	Dictionary res;
	res["filled"] = count;
	Array rect;
	rect.push_back(x1);
	rect.push_back(y1);
	rect.push_back(x2);
	rect.push_back(y2);
	res["rect"] = rect;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPTileMapTools::tilemap_get_cell(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	int x = p_params.get("x", 0);
	int y = p_params.get("y", 0);
	Vector2i coords(x, y);

	int source_id = tilemap->get_cell_source_id(coords);
	Vector2i atlas_coords = tilemap->get_cell_atlas_coords(coords);
	int alternative = tilemap->get_cell_alternative_tile(coords);

	Dictionary res;
	res["x"] = x;
	res["y"] = y;
	res["source_id"] = source_id;
	Array ac;
	ac.push_back(atlas_coords.x);
	ac.push_back(atlas_coords.y);
	res["atlas_coords"] = ac;
	res["alternative"] = alternative;
	res["empty"] = (source_id == -1);
	MCP_SUCCESS(res);
}

Dictionary JustAMCPTileMapTools::tilemap_clear(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	tilemap->clear();

	Dictionary res;
	res["cleared"] = true;
	MCP_SUCCESS(res);
}

Dictionary JustAMCPTileMapTools::tilemap_get_info(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	Ref<TileSet> tile_set = tilemap->get_tile_set();
	Array sources;

	if (tile_set.is_valid()) {
		for (int i = 0; i < tile_set->get_source_count(); i++) {
			int source_id = tile_set->get_source_id(i);
			Ref<TileSetSource> source = tile_set->get_source(source_id);
			if (source.is_valid()) {
				Dictionary info;
				info["id"] = source_id;
				info["type"] = source->get_class();

				Ref<TileSetAtlasSource> atlas = source;
				if (atlas.is_valid()) {
					info["texture"] = atlas->get_texture().is_valid() ? atlas->get_texture()->get_path() : String("");
					info["tile_count"] = atlas->get_tiles_count();
				}
				sources.push_back(info);
			}
		}
	}

	Dictionary res;
	res["node_path"] = node_path;
	res["used_cells"] = tilemap->get_used_cells().size();
	res["tile_set_sources"] = sources;

	Array tile_size;
	if (tile_set.is_valid()) {
		tile_size.push_back(tile_set->get_tile_size().x);
		tile_size.push_back(tile_set->get_tile_size().y);
	} else {
		tile_size.push_back(0);
		tile_size.push_back(0);
	}
	res["tile_size"] = tile_size;

	MCP_SUCCESS(res);
}

Dictionary JustAMCPTileMapTools::tilemap_get_used_cells(const Dictionary &p_params) {
	if (!p_params.has("node_path")) {
		MCP_INVALID_PARAMS("Missing node_path");
	}
	String node_path = p_params["node_path"];

	Node *node = _find_node_by_path(node_path);
	TileMapLayer *tilemap = Object::cast_to<TileMapLayer>(node);
	if (!tilemap) {
		MCP_ERROR(-32000, "Node is not a TileMapLayer: " + node_path);
	}

	int max_count = p_params.get("max_count", 500);
	Array cells;
	TypedArray<Vector2i> used = tilemap->get_used_cells();

	int iter_max = MIN((int)used.size(), max_count);
	for (int i = 0; i < iter_max; i++) {
		Vector2i pos = used[i];
		Dictionary cell;
		cell["x"] = pos.x;
		cell["y"] = pos.y;
		cell["source_id"] = tilemap->get_cell_source_id(pos);
		cells.push_back(cell);
	}

	Dictionary res;
	res["cells"] = cells;
	res["total"] = used.size();
	res["returned"] = cells.size();
	MCP_SUCCESS(res);
}

#endif // TOOLS_ENABLED
