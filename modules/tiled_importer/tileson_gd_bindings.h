/**************************************************************************/
/*  tileson_gd_bindings.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             BLAZIUM ENGINE                             */
/*                          https://blazium.app                           */
/**************************************************************************/
/* Copyright (c) 2024-present Blazium Engine contributors.                */
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
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

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#include "tileson.hpp"
#include <memory>

class GodotTsonMap;
class GodotTsonLayer;
class GodotTsonTileset;
class GodotTsonGrid;
class GodotTsonTerrain;
class GodotTsonTransformations;
class GodotTsonText;
class GodotTsonTiledClass;
class GodotTsonEnumDefinition;
class GodotTsonTileObject;
class GodotTsonProjectPropertyTypes;

// Helper to convert std::string to Godot String
inline String to_godot_string(const std::string &p_str) {
	return String::utf8(p_str.c_str(), p_str.length());
}

// Helper to convert Godot String to std::string
inline std::string to_std_string(const String &p_str) {
	return std::string(p_str.utf8().get_data());
}

// -------------------------------------------------------------
// GodotTsonTileson (Parser Interface)
// -------------------------------------------------------------
class GodotTsonTileson : public RefCounted {
	GDCLASS(GodotTsonTileson, RefCounted);

protected:
	static void _bind_methods();

public:
	GodotTsonTileson();

	Ref<GodotTsonMap> parse_file(const String &p_path);
	Ref<GodotTsonMap> parse_string(const String &p_json);
};

// -------------------------------------------------------------
// GodotTsonLayer
// -------------------------------------------------------------
class GodotTsonLayer : public RefCounted {
	GDCLASS(GodotTsonLayer, RefCounted);

private:
	tson::Layer *layer = nullptr; // Reference to inner layer

protected:
	static void _bind_methods();

public:
	GodotTsonLayer() {}
	void set_layer(tson::Layer *p_layer) { layer = p_layer; }
	tson::Layer *get_layer() const { return layer; }

	String get_name() const;
	String get_tson_type() const;
	bool is_visible() const;

	String get_compression() const;
	Array get_data_base64() const; // Data raw int list is omitted to save memory in gdscript unless specifically base64
	String get_base64_data() const;
	String get_draw_order() const;
	String get_encoding() const;
	int get_id() const;
	String get_image() const;
	Vector2 get_offset() const;
	float get_opacity() const;
	Vector2i get_size() const;
	Color get_transparent_color() const;
	Vector2 get_parallax() const;
	bool has_repeat_x() const;
	bool has_repeat_y() const;
	String get_class_type() const;
	int get_x() const;
	int get_y() const;
	Color get_tint_color() const;

	Array get_objects();
	Array get_chunks();
	Array get_properties();
	Array get_tile_objects();
};

// -------------------------------------------------------------
// GodotTsonTileset
// -------------------------------------------------------------
class GodotTsonTileset : public RefCounted {
	GDCLASS(GodotTsonTileset, RefCounted);

private:
	tson::Tileset *tileset = nullptr; // Reference to inner tileset

protected:
	static void _bind_methods();

public:
	GodotTsonTileset() {}
	void set_tileset(tson::Tileset *p_tileset) { tileset = p_tileset; }
	tson::Tileset *get_tileset() const { return tileset; }

	String get_name() const;
	String get_image() const;
	int get_columns() const;
	int get_first_gid() const;
	String get_image_path() const;
	String get_full_image_path() const;
	Vector2i get_image_size() const;
	int get_margin() const;
	int get_spacing() const;
	int get_tile_count() const;
	Color get_transparent_color() const;
	String get_type_str() const;
	String get_class_type() const;
	Vector2i get_tile_offset() const;
	int get_tile_render_size() const; // Cast from enum
	int get_fill_mode() const; // Cast from enum
	int get_object_alignment() const; // Cast from enum

	Array get_tiles() const;
	Array get_properties();
	Array get_wang_sets() const;
	Ref<GodotTsonGrid> get_grid() const;
	Array get_terrains() const;
	Ref<GodotTsonTransformations> get_transformations() const;
};

// -------------------------------------------------------------
// GodotTsonWangColor
// -------------------------------------------------------------
class GodotTsonWangColor : public RefCounted {
	GDCLASS(GodotTsonWangColor, RefCounted);

private:
	tson::WangColor *wang_color = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonWangColor() {}
	void set_wang_color(tson::WangColor *p_color) { wang_color = p_color; }

	Color get_color() const;
	String get_name() const;
	float get_probability() const;
	int get_tile() const;
	String get_class_type() const;
	Array get_properties();
};

// -------------------------------------------------------------
// GodotTsonWangTile
// -------------------------------------------------------------
class GodotTsonWangTile : public RefCounted {
	GDCLASS(GodotTsonWangTile, RefCounted);

private:
	tson::WangTile *wang_tile = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonWangTile() {}
	void set_wang_tile(tson::WangTile *p_tile) { wang_tile = p_tile; }

	int get_tile_id() const;
	bool has_d_flip() const;
	bool has_h_flip() const;
	bool has_v_flip() const;
	Array get_wang_id() const;
};

// -------------------------------------------------------------
// GodotTsonWangSet
// -------------------------------------------------------------
class GodotTsonWangSet : public RefCounted {
	GDCLASS(GodotTsonWangSet, RefCounted);

private:
	tson::WangSet *wang_set = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonWangSet() {}
	void set_wang_set(tson::WangSet *p_wang_set) { wang_set = p_wang_set; }

	String get_name() const;
	int get_tile() const;
	String get_class_type() const;
	Array get_wang_tiles() const;
	Array get_colors() const;
	Array get_properties();
};

// -------------------------------------------------------------
// GodotTsonFrame
// -------------------------------------------------------------
class GodotTsonFrame : public RefCounted {
	GDCLASS(GodotTsonFrame, RefCounted);

private:
	tson::Frame *frame = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonFrame() {}
	void set_frame(tson::Frame *p_frame) { frame = p_frame; }
	tson::Frame *get_frame() const { return frame; }

	int get_tile_id() const;
	int get_duration() const;
};

// -------------------------------------------------------------
// GodotTsonAnimation
// -------------------------------------------------------------
class GodotTsonAnimation : public RefCounted {
	GDCLASS(GodotTsonAnimation, RefCounted);

private:
	tson::Animation *animation = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonAnimation() {}
	void set_animation(tson::Animation *p_animation) { animation = p_animation; }
	tson::Animation *get_animation() const { return animation; }

	Array get_frames() const;
};

// -------------------------------------------------------------
// GodotTsonTile
// -------------------------------------------------------------
class GodotTsonTile : public RefCounted {
	GDCLASS(GodotTsonTile, RefCounted);

private:
	tson::Tile *tile = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonTile() {}
	void set_tile(tson::Tile *p_tile) { tile = p_tile; }
	tson::Tile *get_tile() const { return tile; }

	int get_id() const;
	String get_image() const;
	Vector2i get_image_size() const;
	String get_tson_type() const;
	String get_class_type() const;
	Array get_terrain() const;
	Rect2i get_drawing_rect() const;
	Rect2i get_sub_rectangle() const;
	int get_flip_flags() const;
	int get_gid() const;

	Ref<GodotTsonAnimation> get_animation() const;
	Ref<GodotTsonLayer> get_objectgroup() const;
	Array get_properties();
};

// -------------------------------------------------------------
// GodotTsonObject
// -------------------------------------------------------------
class GodotTsonObject : public RefCounted {
	GDCLASS(GodotTsonObject, RefCounted);

private:
	tson::Object *object = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonObject() {}
	void set_object(tson::Object *p_object) { object = p_object; }
	tson::Object *get_object() const { return object; }

	int get_id() const;
	String get_name() const;
	String get_tson_type() const;
	Vector2i get_position() const;
	Vector2i get_size() const;
	float get_rotation() const;
	bool is_visible() const;
	bool is_ellipse() const;
	bool is_point() const;
	Array get_polygon() const;
	Array get_polyline() const;
	String get_template() const;
	String get_class_type() const;
	int get_object_type() const;
	int get_gid() const;

	Array get_properties();
	Ref<GodotTsonText> get_text() const;
};

// -------------------------------------------------------------
// GodotTsonChunk
// -------------------------------------------------------------
class GodotTsonChunk : public RefCounted {
	GDCLASS(GodotTsonChunk, RefCounted);

private:
	tson::Chunk *chunk = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonChunk() {}
	void set_chunk(tson::Chunk *p_chunk) { chunk = p_chunk; }
	tson::Chunk *get_chunk() const { return chunk; }

	Vector2i get_position() const;
	Vector2i get_size() const;
	Array get_data() const;
};

// -------------------------------------------------------------
// GodotTsonMap
// -------------------------------------------------------------
class GodotTsonMap : public RefCounted {
	GDCLASS(GodotTsonMap, RefCounted);

private:
	std::shared_ptr<tson::Map> map;

protected:
	static void _bind_methods();

public:
	GodotTsonMap() {}
	void set_map(std::unique_ptr<tson::Map> p_map) { map = std::shared_ptr<tson::Map>(std::move(p_map)); }
	std::shared_ptr<tson::Map> get_map() { return map; }

	String get_status() const;
	Vector2i get_size() const;
	Vector2i get_tile_size() const;
	Color get_background_color() const;
	int get_hexside_length() const;
	bool is_infinite() const;
	int get_next_layer_id() const;
	int get_next_object_id() const;
	String get_orientation() const;
	String get_render_order() const;
	String get_stagger_axis() const;
	String get_stagger_index() const;
	String get_tiled_version() const;
	String get_tson_type() const;
	String get_class_type() const;
	Vector2 get_parallax_origin() const;
	int get_compression_level() const;

	Array get_layers();
	Array get_tilesets();
	Array get_properties();
};

// -------------------------------------------------------------
// GodotTsonProperty
// -------------------------------------------------------------
class GodotTsonProperty : public RefCounted {
	GDCLASS(GodotTsonProperty, RefCounted);

private:
	tson::Property *property = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonProperty() {}
	void set_property(tson::Property *p_property) { property = p_property; }
	tson::Property *get_property() const { return property; }

	String get_name() const;
	int get_tson_type() const;
	String get_property_type() const;
	Variant get_value() const;
};

// -------------------------------------------------------------
// GodotTsonProjectFolder
// -------------------------------------------------------------
class GodotTsonProjectFolder : public RefCounted {
	GDCLASS(GodotTsonProjectFolder, RefCounted);

private:
	tson::ProjectFolder *folder = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonProjectFolder() {}
	void set_folder(tson::ProjectFolder *p_folder) { folder = p_folder; }

	String get_path() const;
	bool has_world_file() const;
	Array get_sub_folders() const;
	Array get_files() const;
};

// -------------------------------------------------------------
// GodotTsonProjectData
// -------------------------------------------------------------
class GodotTsonProjectData : public RefCounted {
	GDCLASS(GodotTsonProjectData, RefCounted);

private:
	const tson::ProjectData *project_data = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonProjectData() {}
	void set_data(const tson::ProjectData *p_data) { project_data = p_data; }

	String get_base_path() const;
	String get_automapping_rules_file() const;
	Array get_commands() const;
	String get_extensions_path() const;
	Array get_folders() const;
	String get_object_types_file() const;
	Ref<GodotTsonProjectPropertyTypes> get_project_property_types() const;
};

// -------------------------------------------------------------
// GodotTsonProject
// -------------------------------------------------------------
class GodotTsonProject : public RefCounted {
	GDCLASS(GodotTsonProject, RefCounted);

private:
	std::unique_ptr<tson::Project> project;

protected:
	static void _bind_methods();

public:
	GodotTsonProject() {}
	tson::Project *get_project() const { return project.get(); }
	bool parse(const String &p_path);

	String get_path() const;
	Ref<GodotTsonProjectData> get_data() const;
	Array get_folders() const;
	Ref<GodotTsonTiledClass> get_tiled_class(const String &p_name) const;
	Ref<GodotTsonEnumDefinition> get_enum_definition(const String &p_name) const;
};

// -------------------------------------------------------------
// GodotTsonWorldMapData
// -------------------------------------------------------------
class GodotTsonWorldMapData : public RefCounted {
	GDCLASS(GodotTsonWorldMapData, RefCounted);

private:
	tson::WorldMapData *world_map_data = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonWorldMapData() {}
	void set_data(tson::WorldMapData *p_data) { world_map_data = p_data; }

	String get_folder() const;
	String get_path() const;
	String get_file_name() const;
	Vector2i get_size() const;
	Vector2i get_position() const;
};

// -------------------------------------------------------------
// GodotTsonWorld
// -------------------------------------------------------------
class GodotTsonWorld : public RefCounted {
	GDCLASS(GodotTsonWorld, RefCounted);

private:
	std::unique_ptr<tson::World> world;

protected:
	static void _bind_methods();

public:
	GodotTsonWorld() {}
	tson::World *get_world() const { return world.get(); }
	void parse(const String &p_path);

	String get_path() const;
	String get_folder() const;
	String get_tson_type() const;
	bool only_show_adjacent_maps() const;
	Array get_map_data() const;
};

// -------------------------------------------------------------
// GodotTsonGrid
// -------------------------------------------------------------
class GodotTsonGrid : public RefCounted {
	GDCLASS(GodotTsonGrid, RefCounted);

private:
	const tson::Grid *grid = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonGrid() {}
	void set_grid(const tson::Grid *p_grid) { grid = p_grid; }

	String get_orientation() const;
	Vector2i get_size() const;
};

// -------------------------------------------------------------
// GodotTsonTerrain
// -------------------------------------------------------------
class GodotTsonTerrain : public RefCounted {
	GDCLASS(GodotTsonTerrain, RefCounted);

private:
	tson::Terrain *terrain = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonTerrain() {}
	void set_terrain(tson::Terrain *p_terrain) { terrain = p_terrain; }

	String get_name() const;
	int get_tile() const;
	Array get_properties();
};

// -------------------------------------------------------------
// GodotTsonText
// -------------------------------------------------------------
class GodotTsonText : public RefCounted {
	GDCLASS(GodotTsonText, RefCounted);

private:
	const tson::Text *text = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonText() {}
	void set_text(const tson::Text *p_text) { text = p_text; }

	String get_text() const;
	Color get_color() const;
	bool is_wrap() const;
	bool is_bold() const;
	String get_font_family() const;
	bool is_italic() const;
	bool is_kerning() const;
	int get_pixel_size() const;
	bool is_strikeout() const;
	bool is_underline() const;
	int get_horizontal_alignment() const;
	int get_vertical_alignment() const;
};

// -------------------------------------------------------------
// GodotTsonTransformations
// -------------------------------------------------------------
class GodotTsonTransformations : public RefCounted {
	GDCLASS(GodotTsonTransformations, RefCounted);

private:
	const tson::Transformations *transformations = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonTransformations() {}
	void set_transformations(const tson::Transformations *p_transform) { transformations = p_transform; }

	bool allow_hflip() const;
	bool allow_preferuntransformed() const;
	bool allow_rotation() const;
	bool allow_vflip() const;
};

// -------------------------------------------------------------
// GodotTsonTiledClass
// -------------------------------------------------------------
class GodotTsonTiledClass : public RefCounted {
	GDCLASS(GodotTsonTiledClass, RefCounted);

private:
	tson::TiledClass *tiled_class = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonTiledClass() {}
	void set_class(tson::TiledClass *p_class) { tiled_class = p_class; }

	int get_id() const;
	String get_name() const;
	String get_tson_type() const;
	Array get_members();
};

// -------------------------------------------------------------
// GodotTsonEnumDefinition
// -------------------------------------------------------------
class GodotTsonEnumDefinition : public RefCounted {
	GDCLASS(GodotTsonEnumDefinition, RefCounted);

private:
	tson::EnumDefinition *enum_def = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonEnumDefinition() {}
	void set_definition(tson::EnumDefinition *p_def) { enum_def = p_def; }

	int get_id() const;
	int get_max_value() const;
	int get_storage_type() const;
	String get_name() const;
	bool has_values_as_flags() const;
	bool has_value(const String &str) const;
	bool has_value_id(uint32_t num) const;
	Array get_values(int p_num);
};

// -------------------------------------------------------------
// GodotTsonEnumValue
// -------------------------------------------------------------
class GodotTsonEnumValue : public RefCounted {
	GDCLASS(GodotTsonEnumValue, RefCounted);

private:
	tson::EnumValue *enum_val = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonEnumValue() {}
	void set_value(tson::EnumValue *p_val) { enum_val = p_val; }

	int get_value() const;
	String get_value_name() const;
	bool contains_value_name(const String &name) const;
	Array get_value_names() const;
	Ref<GodotTsonEnumDefinition> get_definition() const;
};

// -------------------------------------------------------------
// GodotTsonTileObject
// -------------------------------------------------------------
class GodotTsonTileObject : public RefCounted {
	GDCLASS(GodotTsonTileObject, RefCounted);

private:
	tson::TileObject *tile_object = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonTileObject() {}
	void set_tile_object(tson::TileObject *p_tile_object) { tile_object = p_tile_object; }

	Vector2i get_position_in_tile_units() const;
	Vector2 get_position() const;
};

// -------------------------------------------------------------
// GodotTsonProjectPropertyTypes
// -------------------------------------------------------------
class GodotTsonProjectPropertyTypes : public RefCounted {
	GDCLASS(GodotTsonProjectPropertyTypes, RefCounted);

private:
	const tson::ProjectPropertyTypes *prop_types = nullptr;

protected:
	static void _bind_methods();

public:
	GodotTsonProjectPropertyTypes() {}
	void set_project_property_types(const tson::ProjectPropertyTypes *p_prop_types) { prop_types = p_prop_types; }

	Array get_enums() const;
	Array get_classes() const;
	Ref<GodotTsonEnumDefinition> get_enum_definition(const String &name) const;
	Ref<GodotTsonTiledClass> get_tiled_class(const String &name) const;
};
