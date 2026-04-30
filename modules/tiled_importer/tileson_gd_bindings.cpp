/**************************************************************************/
/*  tileson_gd_bindings.cpp                                               */
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

#include "tileson_gd_bindings.h"
#include "core/io/file_access.h"

// -------------------------------------------------------------
// GodotTsonTileson
// -------------------------------------------------------------
void GodotTsonTileson::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse_file", "path"), &GodotTsonTileson::parse_file);
	ClassDB::bind_method(D_METHOD("parse_string", "json"), &GodotTsonTileson::parse_string);
}

GodotTsonTileson::GodotTsonTileson() {}

Ref<GodotTsonMap> GodotTsonTileson::parse_file(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	if (fa.is_null()) {
		ERR_PRINT("GodotTsonTileson: Cannot open file " + p_path);
		return Ref<GodotTsonMap>();
	}

	String content = fa->get_as_text();
	return parse_string(content);
}

Ref<GodotTsonMap> GodotTsonTileson::parse_string(const String &p_json) {
	tson::Tileson t;
	std::unique_ptr<tson::Map> parsed_map = t.parse(to_std_string(p_json).c_str(), p_json.utf8().length());

	if (parsed_map && parsed_map->getStatus() == tson::ParseStatus::OK) {
		Ref<GodotTsonMap> godot_map;
		godot_map.instantiate();
		godot_map->set_map(std::move(parsed_map));
		return godot_map;
	}

	ERR_PRINT("GodotTsonTileson: Failed to parse map.");
	return Ref<GodotTsonMap>();
}

// -------------------------------------------------------------
// GodotTsonFrame
// -------------------------------------------------------------
void GodotTsonFrame::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_tile_id"), &GodotTsonFrame::get_tile_id);
	ClassDB::bind_method(D_METHOD("get_duration"), &GodotTsonFrame::get_duration);
}

int GodotTsonFrame::get_tile_id() const {
	return frame ? frame->getTileId() : 0;
}
int GodotTsonFrame::get_duration() const {
	return frame ? frame->getDuration() : 0;
}

// -------------------------------------------------------------
// GodotTsonAnimation
// -------------------------------------------------------------
void GodotTsonAnimation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_frames"), &GodotTsonAnimation::get_frames);
}

Array GodotTsonAnimation::get_frames() const {
	Array arr;
	if (animation) {
		for (auto &frm : animation->getFrames()) {
			Ref<GodotTsonFrame> godot_frm;
			godot_frm.instantiate();
			godot_frm->set_frame(const_cast<tson::Frame *>(&frm));
			arr.push_back(godot_frm);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonTile
// -------------------------------------------------------------
void GodotTsonTile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &GodotTsonTile::get_id);
	ClassDB::bind_method(D_METHOD("get_image"), &GodotTsonTile::get_image);
	ClassDB::bind_method(D_METHOD("get_image_size"), &GodotTsonTile::get_image_size);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonTile::get_tson_type);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonTile::get_class_type);
	ClassDB::bind_method(D_METHOD("get_terrain"), &GodotTsonTile::get_terrain);
	ClassDB::bind_method(D_METHOD("get_drawing_rect"), &GodotTsonTile::get_drawing_rect);
	ClassDB::bind_method(D_METHOD("get_sub_rectangle"), &GodotTsonTile::get_sub_rectangle);
	ClassDB::bind_method(D_METHOD("get_flip_flags"), &GodotTsonTile::get_flip_flags);
	ClassDB::bind_method(D_METHOD("get_gid"), &GodotTsonTile::get_gid);
	ClassDB::bind_method(D_METHOD("get_animation"), &GodotTsonTile::get_animation);
	ClassDB::bind_method(D_METHOD("get_objectgroup"), &GodotTsonTile::get_objectgroup);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonTile::get_properties);
}

int GodotTsonTile::get_id() const {
	return tile ? tile->getId() : 0;
}
String GodotTsonTile::get_image() const {
	return tile ? to_godot_string(tile->getImage().string()) : String();
}
Vector2i GodotTsonTile::get_image_size() const {
	return tile ? Vector2i(tile->getImageSize().x, tile->getImageSize().y) : Vector2i();
}
String GodotTsonTile::get_tson_type() const {
	return tile ? to_godot_string(tile->getType()) : String();
}
String GodotTsonTile::get_class_type() const {
	return tile ? to_godot_string(tile->getClassType()) : String();
}
Array GodotTsonTile::get_terrain() const {
	Array arr;
	if (tile) {
		for (uint32_t val : tile->getTerrain()) {
			arr.push_back(val);
		}
	}
	return arr;
}
Rect2i GodotTsonTile::get_drawing_rect() const {
	return tile ? Rect2i(tile->getDrawingRect().x, tile->getDrawingRect().y, tile->getDrawingRect().width, tile->getDrawingRect().height) : Rect2i();
}
Rect2i GodotTsonTile::get_sub_rectangle() const {
	return tile ? Rect2i(tile->getSubRectangle().x, tile->getSubRectangle().y, tile->getSubRectangle().width, tile->getSubRectangle().height) : Rect2i();
}
int GodotTsonTile::get_flip_flags() const {
	return tile ? (int)tile->getFlipFlags() : 0;
}
int GodotTsonTile::get_gid() const {
	return tile ? tile->getGid() : 0;
}

Ref<GodotTsonLayer> GodotTsonTile::get_objectgroup() const {
	if (tile) {
		tson::Layer *l = const_cast<tson::Layer *>(&tile->getObjectgroup());
		if (l->getObjects().size() > 0) {
			Ref<GodotTsonLayer> layer;
			layer.instantiate();
			layer->set_layer(l);
			return layer;
		}
	}
	return Ref<GodotTsonLayer>();
}

Ref<GodotTsonAnimation> GodotTsonTile::get_animation() const {
	if (tile && tile->getAnimation().any()) {
		Ref<GodotTsonAnimation> godot_anim;
		godot_anim.instantiate();
		godot_anim->set_animation(&tile->getAnimation());
		return godot_anim;
	}
	return Ref<GodotTsonAnimation>();
}

Array GodotTsonTile::get_properties() {
	Array arr;
	if (tile) {
		for (auto &prop : tile->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonObject
// -------------------------------------------------------------
void GodotTsonObject::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &GodotTsonObject::get_id);
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonObject::get_name);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonObject::get_tson_type);
	ClassDB::bind_method(D_METHOD("get_position"), &GodotTsonObject::get_position);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonObject::get_size);
	ClassDB::bind_method(D_METHOD("get_rotation"), &GodotTsonObject::get_rotation);
	ClassDB::bind_method(D_METHOD("is_visible"), &GodotTsonObject::is_visible);
	ClassDB::bind_method(D_METHOD("is_ellipse"), &GodotTsonObject::is_ellipse);
	ClassDB::bind_method(D_METHOD("is_point"), &GodotTsonObject::is_point);
	ClassDB::bind_method(D_METHOD("get_polygon"), &GodotTsonObject::get_polygon);
	ClassDB::bind_method(D_METHOD("get_polyline"), &GodotTsonObject::get_polyline);
	ClassDB::bind_method(D_METHOD("get_template"), &GodotTsonObject::get_template);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonObject::get_class_type);
	ClassDB::bind_method(D_METHOD("get_object_type"), &GodotTsonObject::get_object_type);
	ClassDB::bind_method(D_METHOD("get_gid"), &GodotTsonObject::get_gid);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonObject::get_properties);
	ClassDB::bind_method(D_METHOD("get_text"), &GodotTsonObject::get_text);
}

int GodotTsonObject::get_id() const {
	return object ? object->getId() : 0;
}
String GodotTsonObject::get_name() const {
	return object ? to_godot_string(object->getName()) : String();
}
String GodotTsonObject::get_tson_type() const {
	return object ? to_godot_string(object->getType()) : String();
}
Vector2i GodotTsonObject::get_position() const {
	return object ? Vector2i(object->getPosition().x, object->getPosition().y) : Vector2i();
}
Vector2i GodotTsonObject::get_size() const {
	return object ? Vector2i(object->getSize().x, object->getSize().y) : Vector2i();
}
float GodotTsonObject::get_rotation() const {
	return object ? object->getRotation() : 0.0f;
}
bool GodotTsonObject::is_visible() const {
	return object ? object->isVisible() : false;
}
bool GodotTsonObject::is_ellipse() const {
	return object ? object->isEllipse() : false;
}
bool GodotTsonObject::is_point() const {
	return object ? object->isPoint() : false;
}

Array GodotTsonObject::get_polygon() const {
	Array arr;
	if (object) {
		for (auto &pt : object->getPolygons()) {
			arr.push_back(Vector2i(pt.x, pt.y));
		}
	}
	return arr;
}
Array GodotTsonObject::get_polyline() const {
	Array arr;
	if (object) {
		for (auto &pt : object->getPolylines()) {
			arr.push_back(Vector2i(pt.x, pt.y));
		}
	}
	return arr;
}
String GodotTsonObject::get_template() const {
	return object ? to_godot_string(object->getTemplate()) : String();
}
String GodotTsonObject::get_class_type() const {
	return object ? to_godot_string(object->getClassType()) : String();
}
int GodotTsonObject::get_object_type() const {
	return object ? (int)object->getObjectType() : 0;
}
int GodotTsonObject::get_gid() const {
	return object ? object->getGid() : 0;
}

Array GodotTsonObject::get_properties() {
	Array arr;
	if (object) {
		for (auto &prop : object->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonChunk
// -------------------------------------------------------------
void GodotTsonChunk::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_position"), &GodotTsonChunk::get_position);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonChunk::get_size);
	ClassDB::bind_method(D_METHOD("get_data"), &GodotTsonChunk::get_data);
}

Vector2i GodotTsonChunk::get_position() const {
	return chunk ? Vector2i(chunk->getPosition().x, chunk->getPosition().y) : Vector2i();
}
Vector2i GodotTsonChunk::get_size() const {
	return chunk ? Vector2i(chunk->getSize().x, chunk->getSize().y) : Vector2i();
}
Array GodotTsonChunk::get_data() const {
	Array arr;
	if (chunk) {
		for (int val : chunk->getData()) {
			arr.push_back(val);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonLayer
// -------------------------------------------------------------
void GodotTsonLayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonLayer::get_name);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonLayer::get_tson_type);
	ClassDB::bind_method(D_METHOD("is_visible"), &GodotTsonLayer::is_visible);
	ClassDB::bind_method(D_METHOD("get_objects"), &GodotTsonLayer::get_objects);
	ClassDB::bind_method(D_METHOD("get_chunks"), &GodotTsonLayer::get_chunks);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonLayer::get_properties);
	ClassDB::bind_method(D_METHOD("get_tile_objects"), &GodotTsonLayer::get_tile_objects);
	ClassDB::bind_method(D_METHOD("get_compression"), &GodotTsonLayer::get_compression);
	ClassDB::bind_method(D_METHOD("get_data_base64"), &GodotTsonLayer::get_data_base64);
	ClassDB::bind_method(D_METHOD("get_base64_data"), &GodotTsonLayer::get_base64_data);
	ClassDB::bind_method(D_METHOD("get_draw_order"), &GodotTsonLayer::get_draw_order);
	ClassDB::bind_method(D_METHOD("get_encoding"), &GodotTsonLayer::get_encoding);
	ClassDB::bind_method(D_METHOD("get_id"), &GodotTsonLayer::get_id);
	ClassDB::bind_method(D_METHOD("get_image"), &GodotTsonLayer::get_image);
	ClassDB::bind_method(D_METHOD("get_offset"), &GodotTsonLayer::get_offset);
	ClassDB::bind_method(D_METHOD("get_opacity"), &GodotTsonLayer::get_opacity);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonLayer::get_size);
	ClassDB::bind_method(D_METHOD("get_transparent_color"), &GodotTsonLayer::get_transparent_color);
	ClassDB::bind_method(D_METHOD("get_parallax"), &GodotTsonLayer::get_parallax);
	ClassDB::bind_method(D_METHOD("has_repeat_x"), &GodotTsonLayer::has_repeat_x);
	ClassDB::bind_method(D_METHOD("has_repeat_y"), &GodotTsonLayer::has_repeat_y);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonLayer::get_class_type);
	ClassDB::bind_method(D_METHOD("get_x"), &GodotTsonLayer::get_x);
	ClassDB::bind_method(D_METHOD("get_y"), &GodotTsonLayer::get_y);
	ClassDB::bind_method(D_METHOD("get_tint_color"), &GodotTsonLayer::get_tint_color);
}

String GodotTsonLayer::get_name() const {
	return layer ? to_godot_string(layer->getName()) : String();
}
String GodotTsonLayer::get_tson_type() const {
	if (!layer) {
		return String();
	}
	switch (layer->getType()) {
		case tson::LayerType::TileLayer:
			return "TileLayer";
		case tson::LayerType::ObjectGroup:
			return "ObjectGroup";
		case tson::LayerType::ImageLayer:
			return "ImageLayer";
		case tson::LayerType::Group:
			return "Group";
		default:
			return "Undefined";
	}
}
bool GodotTsonLayer::is_visible() const {
	return layer ? layer->isVisible() : false;
}

String GodotTsonLayer::get_compression() const {
	return layer ? to_godot_string(layer->getCompression()) : String();
}
Array GodotTsonLayer::get_data_base64() const {
	Array arr;
	if (layer) {
		for (uint32_t val : layer->getData()) {
			arr.push_back(val);
		}
	}
	return arr;
}
String GodotTsonLayer::get_base64_data() const {
	return layer ? to_godot_string(layer->getBase64Data()) : String();
}
String GodotTsonLayer::get_draw_order() const {
	return layer ? to_godot_string(layer->getDrawOrder()) : String();
}
String GodotTsonLayer::get_encoding() const {
	return layer ? to_godot_string(layer->getEncoding()) : String();
}
int GodotTsonLayer::get_id() const {
	return layer ? layer->getId() : 0;
}
String GodotTsonLayer::get_image() const {
	return layer ? to_godot_string(layer->getImage()) : String();
}
Vector2 GodotTsonLayer::get_offset() const {
	return layer ? Vector2(layer->getOffset().x, layer->getOffset().y) : Vector2();
}
float GodotTsonLayer::get_opacity() const {
	return layer ? layer->getOpacity() : 1.0f;
}
Vector2i GodotTsonLayer::get_size() const {
	return layer ? Vector2i(layer->getSize().x, layer->getSize().y) : Vector2i();
}

Color GodotTsonLayer::get_transparent_color() const {
	if (!layer) {
		return Color();
	}
	tson::Colori col = layer->getTransparentColor();
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}

Vector2 GodotTsonLayer::get_parallax() const {
	return layer ? Vector2(layer->getParallax().x, layer->getParallax().y) : Vector2();
}
bool GodotTsonLayer::has_repeat_x() const {
	return layer ? layer->hasRepeatX() : false;
}
bool GodotTsonLayer::has_repeat_y() const {
	return layer ? layer->hasRepeatY() : false;
}
String GodotTsonLayer::get_class_type() const {
	return layer ? to_godot_string(layer->getClassType()) : String();
}
int GodotTsonLayer::get_x() const {
	return layer ? layer->getX() : 0;
}
int GodotTsonLayer::get_y() const {
	return layer ? layer->getY() : 0;
}
Color GodotTsonLayer::get_tint_color() const {
	if (!layer) {
		return Color();
	}
	tson::Colori col = layer->getTintColor();
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}

Array GodotTsonLayer::get_objects() {
	Array arr;
	if (layer) {
		for (auto &obj : layer->getObjects()) {
			Ref<GodotTsonObject> godot_obj;
			godot_obj.instantiate();
			godot_obj->set_object(&obj); // Vector stores them directly, reference inside is stable as long as Map survives
			arr.push_back(godot_obj);
		}
	}
	return arr;
}

Array GodotTsonLayer::get_chunks() {
	Array arr;
	if (layer) {
		for (auto &chk : layer->getChunks()) {
			Ref<GodotTsonChunk> godot_chk;
			godot_chk.instantiate();
			godot_chk->set_chunk(&chk);
			arr.push_back(godot_chk);
		}
	}
	return arr;
}

Array GodotTsonLayer::get_properties() {
	Array arr;
	if (layer) {
		for (auto &prop : layer->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

Array GodotTsonLayer::get_tile_objects() {
	Array arr;
	if (layer) {
		for (auto &to : layer->getTileObjects()) {
			Ref<GodotTsonTileObject> t;
			t.instantiate();
			t->set_tile_object(const_cast<tson::TileObject *>(&to.second));
			arr.push_back(t);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonTileset
// -------------------------------------------------------------
void GodotTsonTileset::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonTileset::get_name);
	ClassDB::bind_method(D_METHOD("get_image"), &GodotTsonTileset::get_image);
	ClassDB::bind_method(D_METHOD("get_tiles"), &GodotTsonTileset::get_tiles);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonTileset::get_properties);
	ClassDB::bind_method(D_METHOD("get_wang_sets"), &GodotTsonTileset::get_wang_sets);
	ClassDB::bind_method(D_METHOD("get_grid"), &GodotTsonTileset::get_grid);
	ClassDB::bind_method(D_METHOD("get_terrains"), &GodotTsonTileset::get_terrains);
	ClassDB::bind_method(D_METHOD("get_transformations"), &GodotTsonTileset::get_transformations);
	ClassDB::bind_method(D_METHOD("get_columns"), &GodotTsonTileset::get_columns);
	ClassDB::bind_method(D_METHOD("get_first_gid"), &GodotTsonTileset::get_first_gid);
	ClassDB::bind_method(D_METHOD("get_image_path"), &GodotTsonTileset::get_image_path);
	ClassDB::bind_method(D_METHOD("get_full_image_path"), &GodotTsonTileset::get_full_image_path);
	ClassDB::bind_method(D_METHOD("get_image_size"), &GodotTsonTileset::get_image_size);
	ClassDB::bind_method(D_METHOD("get_margin"), &GodotTsonTileset::get_margin);
	ClassDB::bind_method(D_METHOD("get_spacing"), &GodotTsonTileset::get_spacing);
	ClassDB::bind_method(D_METHOD("get_tile_count"), &GodotTsonTileset::get_tile_count);
	ClassDB::bind_method(D_METHOD("get_transparent_color"), &GodotTsonTileset::get_transparent_color);
	ClassDB::bind_method(D_METHOD("get_type_str"), &GodotTsonTileset::get_type_str);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonTileset::get_class_type);
	ClassDB::bind_method(D_METHOD("get_tile_offset"), &GodotTsonTileset::get_tile_offset);
	ClassDB::bind_method(D_METHOD("get_tile_render_size"), &GodotTsonTileset::get_tile_render_size);
	ClassDB::bind_method(D_METHOD("get_fill_mode"), &GodotTsonTileset::get_fill_mode);
	ClassDB::bind_method(D_METHOD("get_object_alignment"), &GodotTsonTileset::get_object_alignment);
}

String GodotTsonTileset::get_name() const {
	return tileset ? to_godot_string(tileset->getName()) : String();
}
String GodotTsonTileset::get_image() const {
	return tileset ? to_godot_string(tileset->getImage().string()) : String();
}
int GodotTsonTileset::get_columns() const {
	return tileset ? tileset->getColumns() : 0;
}
int GodotTsonTileset::get_first_gid() const {
	return tileset ? tileset->getFirstgid() : 0;
}
String GodotTsonTileset::get_image_path() const {
	return tileset ? to_godot_string(tileset->getImagePath().string()) : String();
}
String GodotTsonTileset::get_full_image_path() const {
	return tileset ? to_godot_string(tileset->getFullImagePath().string()) : String();
}
Vector2i GodotTsonTileset::get_image_size() const {
	return tileset ? Vector2i(tileset->getImageSize().x, tileset->getImageSize().y) : Vector2i();
}
int GodotTsonTileset::get_margin() const {
	return tileset ? tileset->getMargin() : 0;
}
int GodotTsonTileset::get_spacing() const {
	return tileset ? tileset->getSpacing() : 0;
}
int GodotTsonTileset::get_tile_count() const {
	return tileset ? tileset->getTileCount() : 0;
}

Color GodotTsonTileset::get_transparent_color() const {
	if (!tileset) {
		return Color();
	}
	tson::Colori col = tileset->getTransparentColor();
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}

String GodotTsonTileset::get_type_str() const {
	return tileset ? to_godot_string(tileset->getTypeStr()) : String();
}
String GodotTsonTileset::get_class_type() const {
	return tileset ? to_godot_string(tileset->getClassType()) : String();
}
Vector2i GodotTsonTileset::get_tile_offset() const {
	return tileset ? Vector2i(tileset->getTileOffset().x, tileset->getTileOffset().y) : Vector2i();
}
int GodotTsonTileset::get_tile_render_size() const {
	return tileset ? (int)tileset->getTileRenderSize() : 0;
}
int GodotTsonTileset::get_fill_mode() const {
	return tileset ? (int)tileset->getFillMode() : 0;
}
int GodotTsonTileset::get_object_alignment() const {
	return tileset ? (int)tileset->getObjectAlignment() : 0;
}

Array GodotTsonTileset::get_tiles() const {
	Array arr;
	if (tileset) {
		for (auto &tile : tileset->getTiles()) {
			Ref<GodotTsonTile> godot_tile;
			godot_tile.instantiate();
			godot_tile->set_tile(const_cast<tson::Tile *>(&tile));
			arr.push_back(godot_tile);
		}
	}
	return arr;
}

Array GodotTsonTileset::get_properties() {
	Array arr;
	if (tileset) {
		for (auto &prop : tileset->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

Array GodotTsonTileset::get_wang_sets() const {
	Array arr;
	if (tileset) {
		for (auto &w : tileset->getWangsets()) {
			Ref<GodotTsonWangSet> godot_w;
			godot_w.instantiate();
			godot_w->set_wang_set(const_cast<tson::WangSet *>(&w));
			arr.push_back(godot_w);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonMap
// -------------------------------------------------------------
void GodotTsonMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_status"), &GodotTsonMap::get_status);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonMap::get_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &GodotTsonMap::get_tile_size);
	ClassDB::bind_method(D_METHOD("get_layers"), &GodotTsonMap::get_layers);
	ClassDB::bind_method(D_METHOD("get_tilesets"), &GodotTsonMap::get_tilesets);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonMap::get_properties);
	ClassDB::bind_method(D_METHOD("get_background_color"), &GodotTsonMap::get_background_color);
	ClassDB::bind_method(D_METHOD("get_hexside_length"), &GodotTsonMap::get_hexside_length);
	ClassDB::bind_method(D_METHOD("is_infinite"), &GodotTsonMap::is_infinite);
	ClassDB::bind_method(D_METHOD("get_next_layer_id"), &GodotTsonMap::get_next_layer_id);
	ClassDB::bind_method(D_METHOD("get_next_object_id"), &GodotTsonMap::get_next_object_id);
	ClassDB::bind_method(D_METHOD("get_orientation"), &GodotTsonMap::get_orientation);
	ClassDB::bind_method(D_METHOD("get_render_order"), &GodotTsonMap::get_render_order);
	ClassDB::bind_method(D_METHOD("get_stagger_axis"), &GodotTsonMap::get_stagger_axis);
	ClassDB::bind_method(D_METHOD("get_stagger_index"), &GodotTsonMap::get_stagger_index);
	ClassDB::bind_method(D_METHOD("get_tiled_version"), &GodotTsonMap::get_tiled_version);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonMap::get_tson_type);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonMap::get_class_type);
	ClassDB::bind_method(D_METHOD("get_parallax_origin"), &GodotTsonMap::get_parallax_origin);
	ClassDB::bind_method(D_METHOD("get_compression_level"), &GodotTsonMap::get_compression_level);
}

String GodotTsonMap::get_status() const {
	if (!map) {
		return "NOT_INITIALIZED";
	}
	switch (map->getStatus()) {
		case tson::ParseStatus::OK:
			return "OK";
		case tson::ParseStatus::FileNotFound:
			return "FileNotFound";
		case tson::ParseStatus::ParseError:
			return "ParseError";
		case tson::ParseStatus::MissingData:
			return "MissingData";
		case tson::ParseStatus::DecompressionError:
			return "DecompressionError";
	}
	return "UNKNOWN";
}

Vector2i GodotTsonMap::get_size() const {
	return map ? Vector2i(map->getSize().x, map->getSize().y) : Vector2i();
}
Vector2i GodotTsonMap::get_tile_size() const {
	return map ? Vector2i(map->getTileSize().x, map->getTileSize().y) : Vector2i();
}

Color GodotTsonMap::get_background_color() const {
	if (!map) {
		return Color();
	}
	tson::Colori col = map->getBackgroundColor();
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}
int GodotTsonMap::get_hexside_length() const {
	return map ? map->getHexsideLength() : 0;
}
bool GodotTsonMap::is_infinite() const {
	return map ? map->isInfinite() : false;
}
int GodotTsonMap::get_next_layer_id() const {
	return map ? map->getNextLayerId() : 0;
}
int GodotTsonMap::get_next_object_id() const {
	return map ? map->getNextObjectId() : 0;
}
String GodotTsonMap::get_orientation() const {
	return map ? to_godot_string(map->getOrientation()) : String();
}
String GodotTsonMap::get_render_order() const {
	return map ? to_godot_string(map->getRenderOrder()) : String();
}
String GodotTsonMap::get_stagger_axis() const {
	return map ? to_godot_string(map->getStaggerAxis()) : String();
}
String GodotTsonMap::get_stagger_index() const {
	return map ? to_godot_string(map->getStaggerIndex()) : String();
}
String GodotTsonMap::get_tiled_version() const {
	return map ? to_godot_string(map->getTiledVersion()) : String();
}
String GodotTsonMap::get_tson_type() const {
	return map ? to_godot_string(map->getType()) : String();
}
String GodotTsonMap::get_class_type() const {
	return map ? to_godot_string(map->getClassType()) : String();
}
Vector2 GodotTsonMap::get_parallax_origin() const {
	return map ? Vector2(map->getParallaxOrigin().x, map->getParallaxOrigin().y) : Vector2();
}
int GodotTsonMap::get_compression_level() const {
	return map ? map->getCompressionLevel() : 0;
}

Array GodotTsonMap::get_layers() {
	Array arr;
	if (map) {
		for (auto &layer : map->getLayers()) {
			Ref<GodotTsonLayer> l;
			l.instantiate();
			l->set_layer(&layer);
			arr.push_back(l);
		}
	}
	return arr;
}

Array GodotTsonMap::get_tilesets() {
	Array arr;
	if (map) {
		for (auto &tileset : map->getTilesets()) {
			Ref<GodotTsonTileset> t;
			t.instantiate();
			t->set_tileset(&tileset);
			arr.push_back(t);
		}
	}
	return arr;
}

Array GodotTsonMap::get_properties() {
	Array arr;
	if (map) {
		for (auto &prop : map->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonProperty
// -------------------------------------------------------------
void GodotTsonProperty::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonProperty::get_name);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonProperty::get_tson_type);
	ClassDB::bind_method(D_METHOD("get_property_type"), &GodotTsonProperty::get_property_type);
	ClassDB::bind_method(D_METHOD("get_value"), &GodotTsonProperty::get_value);
}

String GodotTsonProperty::get_name() const {
	return property ? to_godot_string(property->getName()) : String();
}
int GodotTsonProperty::get_tson_type() const {
	return property ? (int)property->getType() : 0;
}
String GodotTsonProperty::get_property_type() const {
	return property ? to_godot_string(property->getPropertyType()) : String();
}

Variant GodotTsonProperty::get_value() const {
	if (!property) {
		return Variant();
	}

	switch (property->getType()) {
		case tson::Type::Boolean:
			return property->getValue<bool>();
		case tson::Type::Int:
			return property->getValue<int>();
		case tson::Type::Float:
			return property->getValue<float>();
		case tson::Type::String:
			return to_godot_string(property->getValue<std::string>());
		case tson::Type::File:
			return to_godot_string(property->getValue<std::string>());
		case tson::Type::Color: {
			tson::Colori col = property->getValue<tson::Colori>();
			return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
		}
		case tson::Type::Object:
			return property->getValue<int>();
		case tson::Type::Class:
			return to_godot_string(property->getPropertyType());
		default:
			return Variant();
	}
}

// -------------------------------------------------------------
// GodotTsonProjectFolder
// -------------------------------------------------------------
void GodotTsonProjectFolder::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_path"), &GodotTsonProjectFolder::get_path);
	ClassDB::bind_method(D_METHOD("has_world_file"), &GodotTsonProjectFolder::has_world_file);
	ClassDB::bind_method(D_METHOD("get_sub_folders"), &GodotTsonProjectFolder::get_sub_folders);
	ClassDB::bind_method(D_METHOD("get_files"), &GodotTsonProjectFolder::get_files);
}

String GodotTsonProjectFolder::get_path() const {
	return folder ? to_godot_string(folder->getPath().string()) : String();
}
bool GodotTsonProjectFolder::has_world_file() const {
	return folder ? folder->hasWorldFile() : false;
}

Array GodotTsonProjectFolder::get_sub_folders() const {
	Array arr;
	if (folder) {
		for (auto &f : folder->getSubFolders()) {
			Ref<GodotTsonProjectFolder> godot_f;
			godot_f.instantiate();
			godot_f->set_folder(const_cast<tson::ProjectFolder *>(&f));
			arr.push_back(godot_f);
		}
	}
	return arr;
}

Array GodotTsonProjectFolder::get_files() const {
	Array arr;
	if (folder) {
		for (auto &f : folder->getFiles()) {
			arr.push_back(to_godot_string(f.string()));
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonProjectData
// -------------------------------------------------------------
void GodotTsonProjectData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_base_path"), &GodotTsonProjectData::get_base_path);
	ClassDB::bind_method(D_METHOD("get_automapping_rules_file"), &GodotTsonProjectData::get_automapping_rules_file);
	ClassDB::bind_method(D_METHOD("get_commands"), &GodotTsonProjectData::get_commands);
	ClassDB::bind_method(D_METHOD("get_extensions_path"), &GodotTsonProjectData::get_extensions_path);
	ClassDB::bind_method(D_METHOD("get_folders"), &GodotTsonProjectData::get_folders);
	ClassDB::bind_method(D_METHOD("get_object_types_file"), &GodotTsonProjectData::get_object_types_file);
	ClassDB::bind_method(D_METHOD("get_project_property_types"), &GodotTsonProjectData::get_project_property_types);
}

String GodotTsonProjectData::get_base_path() const {
	return project_data ? to_godot_string(project_data->basePath.string()) : String();
}
String GodotTsonProjectData::get_automapping_rules_file() const {
	return project_data ? to_godot_string(project_data->automappingRulesFile) : String();
}

Array GodotTsonProjectData::get_commands() const {
	Array arr;
	if (project_data) {
		for (auto &c : project_data->commands) {
			arr.push_back(to_godot_string(c));
		}
	}
	return arr;
}

String GodotTsonProjectData::get_extensions_path() const {
	return project_data ? to_godot_string(project_data->extensionsPath) : String();
}

Array GodotTsonProjectData::get_folders() const {
	Array arr;
	if (project_data) {
		for (auto &f : project_data->folderPaths) {
			Ref<GodotTsonProjectFolder> godot_f;
			godot_f.instantiate();
			godot_f->set_folder(const_cast<tson::ProjectFolder *>(&f));
			arr.push_back(godot_f);
		}
	}
	return arr;
}

String GodotTsonProjectData::get_object_types_file() const {
	return project_data ? to_godot_string(project_data->objectTypesFile) : String();
}

Ref<GodotTsonProjectPropertyTypes> GodotTsonProjectData::get_project_property_types() const {
	if (project_data) {
		Ref<GodotTsonProjectPropertyTypes> godot;
		godot.instantiate();
		godot->set_project_property_types(&project_data->projectPropertyTypes);
		return godot;
	}
	return Ref<GodotTsonProjectPropertyTypes>();
}

// -------------------------------------------------------------
// GodotTsonProject
// -------------------------------------------------------------
void GodotTsonProject::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse", "path"), &GodotTsonProject::parse);
	ClassDB::bind_method(D_METHOD("get_path"), &GodotTsonProject::get_path);
	ClassDB::bind_method(D_METHOD("get_data"), &GodotTsonProject::get_data);
	ClassDB::bind_method(D_METHOD("get_folders"), &GodotTsonProject::get_folders);
	ClassDB::bind_method(D_METHOD("get_tiled_class", "name"), &GodotTsonProject::get_tiled_class);
	ClassDB::bind_method(D_METHOD("get_enum_definition", "name"), &GodotTsonProject::get_enum_definition);
}

bool GodotTsonProject::parse(const String &p_path) {
	project = std::make_unique<tson::Project>();
	return project->parse(std::string(p_path.utf8().get_data()));
}

String GodotTsonProject::get_path() const {
	return project ? to_godot_string(project->getPath().string()) : String();
}

Ref<GodotTsonProjectData> GodotTsonProject::get_data() const {
	if (project) {
		Ref<GodotTsonProjectData> data;
		data.instantiate();
		data->set_data(&project->getData());
		return data;
	}
	return Ref<GodotTsonProjectData>();
}

Array GodotTsonProject::get_folders() const {
	Array arr;
	if (project) {
		for (auto &f : project->getFolders()) {
			Ref<GodotTsonProjectFolder> godot_f;
			godot_f.instantiate();
			godot_f->set_folder(const_cast<tson::ProjectFolder *>(&f));
			arr.push_back(godot_f);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonWorldMapData
// -------------------------------------------------------------
void GodotTsonWorldMapData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_folder"), &GodotTsonWorldMapData::get_folder);
	ClassDB::bind_method(D_METHOD("get_path"), &GodotTsonWorldMapData::get_path);
	ClassDB::bind_method(D_METHOD("get_file_name"), &GodotTsonWorldMapData::get_file_name);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonWorldMapData::get_size);
	ClassDB::bind_method(D_METHOD("get_position"), &GodotTsonWorldMapData::get_position);
}

String GodotTsonWorldMapData::get_folder() const {
	return world_map_data ? to_godot_string(world_map_data->folder.string()) : String();
}
String GodotTsonWorldMapData::get_path() const {
	return world_map_data ? to_godot_string(world_map_data->path.string()) : String();
}
String GodotTsonWorldMapData::get_file_name() const {
	return world_map_data ? to_godot_string(world_map_data->fileName) : String();
}
Vector2i GodotTsonWorldMapData::get_size() const {
	return world_map_data ? Vector2i(world_map_data->size.x, world_map_data->size.y) : Vector2i();
}
Vector2i GodotTsonWorldMapData::get_position() const {
	return world_map_data ? Vector2i(world_map_data->position.x, world_map_data->position.y) : Vector2i();
}

// -------------------------------------------------------------
// GodotTsonWorld
// -------------------------------------------------------------
void GodotTsonWorld::_bind_methods() {
	ClassDB::bind_method(D_METHOD("parse", "path"), &GodotTsonWorld::parse);
	ClassDB::bind_method(D_METHOD("get_path"), &GodotTsonWorld::get_path);
	ClassDB::bind_method(D_METHOD("get_folder"), &GodotTsonWorld::get_folder);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonWorld::get_tson_type);
	ClassDB::bind_method(D_METHOD("only_show_adjacent_maps"), &GodotTsonWorld::only_show_adjacent_maps);
	ClassDB::bind_method(D_METHOD("get_map_data"), &GodotTsonWorld::get_map_data);
}

void GodotTsonWorld::parse(const String &p_path) {
	world = std::make_unique<tson::World>();
	world->parse(std::string(p_path.utf8().get_data()));
}

String GodotTsonWorld::get_path() const {
	return world ? to_godot_string(world->getPath().string()) : String();
}
String GodotTsonWorld::get_folder() const {
	return world ? to_godot_string(world->getFolder().string()) : String();
}
String GodotTsonWorld::get_tson_type() const {
	return world ? to_godot_string(world->getType()) : String();
}
bool GodotTsonWorld::only_show_adjacent_maps() const {
	return world ? world->onlyShowAdjacentMaps() : false;
}

Array GodotTsonWorld::get_map_data() const {
	Array arr;
	if (world) {
		for (auto &data : world->getMapData()) {
			Ref<GodotTsonWorldMapData> godot_data;
			godot_data.instantiate();
			godot_data->set_data(const_cast<tson::WorldMapData *>(&data));
			arr.push_back(godot_data);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonWangColor
// -------------------------------------------------------------
void GodotTsonWangColor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_color"), &GodotTsonWangColor::get_color);
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonWangColor::get_name);
	ClassDB::bind_method(D_METHOD("get_probability"), &GodotTsonWangColor::get_probability);
	ClassDB::bind_method(D_METHOD("get_tile"), &GodotTsonWangColor::get_tile);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonWangColor::get_class_type);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonWangColor::get_properties);
}

Color GodotTsonWangColor::get_color() const {
	if (!wang_color) {
		return Color();
	}
	tson::Colori col = wang_color->getColor();
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}
String GodotTsonWangColor::get_name() const {
	return wang_color ? to_godot_string(wang_color->getName()) : String();
}
float GodotTsonWangColor::get_probability() const {
	return wang_color ? wang_color->getProbability() : 0.0f;
}
int GodotTsonWangColor::get_tile() const {
	return wang_color ? wang_color->getTile() : 0;
}
String GodotTsonWangColor::get_class_type() const {
	return wang_color ? to_godot_string(wang_color->getClassType()) : String();
}

Array GodotTsonWangColor::get_properties() {
	Array arr;
	if (wang_color) {
		for (auto &prop : wang_color->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonWangTile
// -------------------------------------------------------------
void GodotTsonWangTile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_tile_id"), &GodotTsonWangTile::get_tile_id);
	ClassDB::bind_method(D_METHOD("has_d_flip"), &GodotTsonWangTile::has_d_flip);
	ClassDB::bind_method(D_METHOD("has_h_flip"), &GodotTsonWangTile::has_h_flip);
	ClassDB::bind_method(D_METHOD("has_v_flip"), &GodotTsonWangTile::has_v_flip);
	ClassDB::bind_method(D_METHOD("get_wang_id"), &GodotTsonWangTile::get_wang_id);
}

int GodotTsonWangTile::get_tile_id() const {
	return wang_tile ? wang_tile->getTileid() : 0;
}
bool GodotTsonWangTile::has_d_flip() const {
	return wang_tile ? wang_tile->hasDFlip() : false;
}
bool GodotTsonWangTile::has_h_flip() const {
	return wang_tile ? wang_tile->hasHFlip() : false;
}
bool GodotTsonWangTile::has_v_flip() const {
	return wang_tile ? wang_tile->hasVFlip() : false;
}

Array GodotTsonWangTile::get_wang_id() const {
	Array arr;
	if (wang_tile) {
		for (uint32_t val : wang_tile->getWangIds()) {
			arr.push_back(val);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonWangSet
// -------------------------------------------------------------
void GodotTsonWangSet::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonWangSet::get_name);
	ClassDB::bind_method(D_METHOD("get_tile"), &GodotTsonWangSet::get_tile);
	ClassDB::bind_method(D_METHOD("get_class_type"), &GodotTsonWangSet::get_class_type);
	ClassDB::bind_method(D_METHOD("get_wang_tiles"), &GodotTsonWangSet::get_wang_tiles);
	ClassDB::bind_method(D_METHOD("get_colors"), &GodotTsonWangSet::get_colors);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonWangSet::get_properties);
}

String GodotTsonWangSet::get_name() const {
	return wang_set ? to_godot_string(wang_set->getName()) : String();
}
int GodotTsonWangSet::get_tile() const {
	return wang_set ? wang_set->getTile() : 0;
}
String GodotTsonWangSet::get_class_type() const {
	return wang_set ? to_godot_string(wang_set->getClassType()) : String();
}

Array GodotTsonWangSet::get_wang_tiles() const {
	Array arr;
	if (wang_set) {
		for (auto &t : wang_set->getWangTiles()) {
			Ref<GodotTsonWangTile> godot_t;
			godot_t.instantiate();
			godot_t->set_wang_tile(const_cast<tson::WangTile *>(&t));
			arr.push_back(godot_t);
		}
	}
	return arr;
}

Array GodotTsonWangSet::get_colors() const {
	Array arr;
	if (wang_set) {
		for (auto &c : wang_set->getColors()) {
			Ref<GodotTsonWangColor> godot_c;
			godot_c.instantiate();
			godot_c->set_wang_color(const_cast<tson::WangColor *>(&c));
			arr.push_back(godot_c);
		}
	}
	return arr;
}

Array GodotTsonWangSet::get_properties() {
	Array arr;
	if (wang_set) {
		for (auto &prop : wang_set->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonGrid
// -------------------------------------------------------------
void GodotTsonGrid::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_orientation"), &GodotTsonGrid::get_orientation);
	ClassDB::bind_method(D_METHOD("get_size"), &GodotTsonGrid::get_size);
}

String GodotTsonGrid::get_orientation() const {
	return grid ? to_godot_string(grid->getOrientation()) : String();
}
Vector2i GodotTsonGrid::get_size() const {
	return grid ? Vector2i(grid->getSize().x, grid->getSize().y) : Vector2i();
}

// -------------------------------------------------------------
// GodotTsonTerrain
// -------------------------------------------------------------
void GodotTsonTerrain::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonTerrain::get_name);
	ClassDB::bind_method(D_METHOD("get_tile"), &GodotTsonTerrain::get_tile);
	ClassDB::bind_method(D_METHOD("get_properties"), &GodotTsonTerrain::get_properties);
}

String GodotTsonTerrain::get_name() const {
	return terrain ? to_godot_string(terrain->getName()) : String();
}
int GodotTsonTerrain::get_tile() const {
	return terrain ? terrain->getTile() : 0;
}

Array GodotTsonTerrain::get_properties() {
	Array arr;
	if (terrain) {
		for (auto &prop : terrain->getProperties().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonText
// -------------------------------------------------------------
void GodotTsonText::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_text"), &GodotTsonText::get_text);
	ClassDB::bind_method(D_METHOD("get_color"), &GodotTsonText::get_color);
	ClassDB::bind_method(D_METHOD("is_wrap"), &GodotTsonText::is_wrap);
	ClassDB::bind_method(D_METHOD("is_bold"), &GodotTsonText::is_bold);
	ClassDB::bind_method(D_METHOD("get_font_family"), &GodotTsonText::get_font_family);
	ClassDB::bind_method(D_METHOD("is_italic"), &GodotTsonText::is_italic);
	ClassDB::bind_method(D_METHOD("is_kerning"), &GodotTsonText::is_kerning);
	ClassDB::bind_method(D_METHOD("get_pixel_size"), &GodotTsonText::get_pixel_size);
	ClassDB::bind_method(D_METHOD("is_strikeout"), &GodotTsonText::is_strikeout);
	ClassDB::bind_method(D_METHOD("is_underline"), &GodotTsonText::is_underline);
	ClassDB::bind_method(D_METHOD("get_horizontal_alignment"), &GodotTsonText::get_horizontal_alignment);
	ClassDB::bind_method(D_METHOD("get_vertical_alignment"), &GodotTsonText::get_vertical_alignment);
}

String GodotTsonText::get_text() const {
	return text ? to_godot_string(text->text) : String();
}
Color GodotTsonText::get_color() const {
	if (!text) {
		return Color();
	}
	tson::Colori col = text->color;
	return Color(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
}
bool GodotTsonText::is_wrap() const {
	return text ? text->wrap : false;
}
bool GodotTsonText::is_bold() const {
	return text ? text->bold : false;
}
String GodotTsonText::get_font_family() const {
	return text ? to_godot_string(text->fontFamily) : String("sans-serif");
}
bool GodotTsonText::is_italic() const {
	return text ? text->italic : false;
}
bool GodotTsonText::is_kerning() const {
	return text ? text->kerning : true;
}
int GodotTsonText::get_pixel_size() const {
	return text ? text->pixelSize : 16;
}
bool GodotTsonText::is_strikeout() const {
	return text ? text->strikeout : false;
}
bool GodotTsonText::is_underline() const {
	return text ? text->underline : false;
}
int GodotTsonText::get_horizontal_alignment() const {
	return text ? (int)text->horizontalAlignment : 0;
}
int GodotTsonText::get_vertical_alignment() const {
	return text ? (int)text->verticalAlignment : 0;
}

// -------------------------------------------------------------
// GodotTsonTransformations
// -------------------------------------------------------------
void GodotTsonTransformations::_bind_methods() {
	ClassDB::bind_method(D_METHOD("allow_hflip"), &GodotTsonTransformations::allow_hflip);
	ClassDB::bind_method(D_METHOD("allow_preferuntransformed"), &GodotTsonTransformations::allow_preferuntransformed);
	ClassDB::bind_method(D_METHOD("allow_rotation"), &GodotTsonTransformations::allow_rotation);
	ClassDB::bind_method(D_METHOD("allow_vflip"), &GodotTsonTransformations::allow_vflip);
}

bool GodotTsonTransformations::allow_hflip() const {
	return transformations ? transformations->allowHflip() : false;
}
bool GodotTsonTransformations::allow_preferuntransformed() const {
	return transformations ? transformations->allowPreferuntransformed() : false;
}
bool GodotTsonTransformations::allow_rotation() const {
	return transformations ? transformations->allowRotation() : false;
}
bool GodotTsonTransformations::allow_vflip() const {
	return transformations ? transformations->allowVflip() : false;
}

// -------------------------------------------------------------
// GodotTsonTiledClass
// -------------------------------------------------------------
void GodotTsonTiledClass::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &GodotTsonTiledClass::get_id);
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonTiledClass::get_name);
	ClassDB::bind_method(D_METHOD("get_tson_type"), &GodotTsonTiledClass::get_tson_type);
	ClassDB::bind_method(D_METHOD("get_members"), &GodotTsonTiledClass::get_members);
}

int GodotTsonTiledClass::get_id() const {
	return tiled_class ? tiled_class->getId() : 0;
}
String GodotTsonTiledClass::get_name() const {
	return tiled_class ? to_godot_string(tiled_class->getName()) : String();
}
String GodotTsonTiledClass::get_tson_type() const {
	return tiled_class ? to_godot_string(tiled_class->getType()) : String();
}

Array GodotTsonTiledClass::get_members() {
	Array arr;
	if (tiled_class) {
		for (auto &prop : tiled_class->getMembers().getProperties()) {
			Ref<GodotTsonProperty> p;
			p.instantiate();
			p->set_property(const_cast<tson::Property *>(&prop.second));
			arr.push_back(p);
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonEnumDefinition
// -------------------------------------------------------------
void GodotTsonEnumDefinition::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &GodotTsonEnumDefinition::get_id);
	ClassDB::bind_method(D_METHOD("get_max_value"), &GodotTsonEnumDefinition::get_max_value);
	ClassDB::bind_method(D_METHOD("get_storage_type"), &GodotTsonEnumDefinition::get_storage_type);
	ClassDB::bind_method(D_METHOD("get_name"), &GodotTsonEnumDefinition::get_name);
	ClassDB::bind_method(D_METHOD("has_values_as_flags"), &GodotTsonEnumDefinition::has_values_as_flags);
	ClassDB::bind_method(D_METHOD("has_value", "str"), &GodotTsonEnumDefinition::has_value);
	ClassDB::bind_method(D_METHOD("has_value_id", "num"), &GodotTsonEnumDefinition::has_value_id);
	ClassDB::bind_method(D_METHOD("get_values", "num"), &GodotTsonEnumDefinition::get_values);
}

int GodotTsonEnumDefinition::get_id() const {
	return enum_def ? enum_def->getId() : 0;
}
int GodotTsonEnumDefinition::get_max_value() const {
	return enum_def ? enum_def->getMaxValue() : 0;
}
int GodotTsonEnumDefinition::get_storage_type() const {
	return enum_def ? (int)enum_def->getStorageType() : 0;
}
String GodotTsonEnumDefinition::get_name() const {
	return enum_def ? to_godot_string(enum_def->getName()) : String();
}
bool GodotTsonEnumDefinition::has_values_as_flags() const {
	return enum_def ? enum_def->hasValuesAsFlags() : false;
}
bool GodotTsonEnumDefinition::has_value(const String &str) const {
	return enum_def ? enum_def->exists(std::string(str.utf8().get_data())) : false;
}
bool GodotTsonEnumDefinition::has_value_id(uint32_t num) const {
	return enum_def ? enum_def->exists(num) : false;
}

Array GodotTsonEnumDefinition::get_values(int p_num) {
	Array arr;
	if (enum_def) {
		for (auto &v : enum_def->getValues(p_num)) {
			arr.push_back(to_godot_string(v));
		}
	}
	return arr;
}

// -------------------------------------------------------------
// GodotTsonEnumValue
// -------------------------------------------------------------
void GodotTsonEnumValue::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_value"), &GodotTsonEnumValue::get_value);
	ClassDB::bind_method(D_METHOD("get_value_name"), &GodotTsonEnumValue::get_value_name);
	ClassDB::bind_method(D_METHOD("contains_value_name", "name"), &GodotTsonEnumValue::contains_value_name);
	ClassDB::bind_method(D_METHOD("get_value_names"), &GodotTsonEnumValue::get_value_names);
	ClassDB::bind_method(D_METHOD("get_definition"), &GodotTsonEnumValue::get_definition);
}

int GodotTsonEnumValue::get_value() const {
	return enum_val ? enum_val->getValue() : 0;
}
String GodotTsonEnumValue::get_value_name() const {
	return enum_val ? to_godot_string(enum_val->getValueName()) : String();
}
bool GodotTsonEnumValue::contains_value_name(const String &name) const {
	return enum_val ? enum_val->containsValueName(std::string(name.utf8().get_data())) : false;
}

Array GodotTsonEnumValue::get_value_names() const {
	Array arr;
	if (enum_val) {
		for (auto &v : enum_val->getValueNames()) {
			arr.push_back(to_godot_string(v));
		}
	}
	return arr;
}

Ref<GodotTsonEnumDefinition> GodotTsonEnumValue::get_definition() const {
	if (enum_val && enum_val->getDefinition()) {
		Ref<GodotTsonEnumDefinition> d;
		d.instantiate();
		d->set_definition(enum_val->getDefinition());
		return d;
	}
	return Ref<GodotTsonEnumDefinition>();
}

Ref<GodotTsonGrid> GodotTsonTileset::get_grid() const {
	if (tileset) {
		Ref<GodotTsonGrid> godot;
		godot.instantiate();
		godot->set_grid(&tileset->getGrid());
		return godot;
	}
	return Ref<GodotTsonGrid>();
}

Array GodotTsonTileset::get_terrains() const {
	Array arr;
	if (tileset) {
		for (auto &t : tileset->getTerrains()) {
			Ref<GodotTsonTerrain> godot;
			godot.instantiate();
			godot->set_terrain(const_cast<tson::Terrain *>(&t));
			arr.push_back(godot);
		}
	}
	return arr;
}

Ref<GodotTsonTransformations> GodotTsonTileset::get_transformations() const {
	if (tileset) {
		Ref<GodotTsonTransformations> godot;
		godot.instantiate();
		godot->set_transformations(&tileset->getTransformations());
		return godot;
	}
	return Ref<GodotTsonTransformations>();
}

Ref<GodotTsonText> GodotTsonObject::get_text() const {
	if (object && object->getText().text != "") {
		Ref<GodotTsonText> godot;
		godot.instantiate();
		godot->set_text(&object->getText());
		return godot;
	}
	return Ref<GodotTsonText>();
}

Ref<GodotTsonTiledClass> GodotTsonProject::get_tiled_class(const String &p_name) const {
	if (project) {
		tson::TiledClass *cls = project->getClass(std::string(p_name.utf8().get_data()));
		if (cls) {
			Ref<GodotTsonTiledClass> godot;
			godot.instantiate();
			godot->set_class(cls);
			return godot;
		}
	}
	return Ref<GodotTsonTiledClass>();
}

Ref<GodotTsonEnumDefinition> GodotTsonProject::get_enum_definition(const String &p_name) const {
	if (project) {
		tson::EnumDefinition *enm = project->getEnumDefinition(std::string(p_name.utf8().get_data()));
		if (enm) {
			Ref<GodotTsonEnumDefinition> godot;
			godot.instantiate();
			godot->set_definition(enm);
			return godot;
		}
	}
	return Ref<GodotTsonEnumDefinition>();
}

// -------------------------------------------------------------
// GodotTsonTileObject
// -------------------------------------------------------------
void GodotTsonTileObject::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_position_in_tile_units"), &GodotTsonTileObject::get_position_in_tile_units);
	ClassDB::bind_method(D_METHOD("get_position"), &GodotTsonTileObject::get_position);
}

Vector2i GodotTsonTileObject::get_position_in_tile_units() const {
	return tile_object ? Vector2i(tile_object->getPositionInTileUnits().x, tile_object->getPositionInTileUnits().y) : Vector2i();
}
Vector2 GodotTsonTileObject::get_position() const {
	return tile_object ? Vector2(tile_object->getPosition().x, tile_object->getPosition().y) : Vector2();
}

// -------------------------------------------------------------
// GodotTsonProjectPropertyTypes
// -------------------------------------------------------------
void GodotTsonProjectPropertyTypes::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_enums"), &GodotTsonProjectPropertyTypes::get_enums);
	ClassDB::bind_method(D_METHOD("get_classes"), &GodotTsonProjectPropertyTypes::get_classes);
	ClassDB::bind_method(D_METHOD("get_enum_definition", "name"), &GodotTsonProjectPropertyTypes::get_enum_definition);
	ClassDB::bind_method(D_METHOD("get_tiled_class", "name"), &GodotTsonProjectPropertyTypes::get_tiled_class);
}

Array GodotTsonProjectPropertyTypes::get_enums() const {
	Array arr;
	if (prop_types) {
		for (auto &e : prop_types->getEnums()) {
			Ref<GodotTsonEnumDefinition> ed;
			ed.instantiate();
			ed->set_definition(const_cast<tson::EnumDefinition *>(&e));
			arr.push_back(ed);
		}
	}
	return arr;
}

Array GodotTsonProjectPropertyTypes::get_classes() const {
	Array arr;
	if (prop_types) {
		for (auto &c : prop_types->getClasses()) {
			Ref<GodotTsonTiledClass> tc;
			tc.instantiate();
			tc->set_class(const_cast<tson::TiledClass *>(&c));
			arr.push_back(tc);
		}
	}
	return arr;
}

Ref<GodotTsonEnumDefinition> GodotTsonProjectPropertyTypes::get_enum_definition(const String &name) const {
	if (prop_types) {
		tson::EnumDefinition *e = const_cast<tson::ProjectPropertyTypes *>(prop_types)->getEnumDefinition(std::string(name.utf8().get_data()));
		if (e) {
			Ref<GodotTsonEnumDefinition> ed;
			ed.instantiate();
			ed->set_definition(e);
			return ed;
		}
	}
	return Ref<GodotTsonEnumDefinition>();
}

Ref<GodotTsonTiledClass> GodotTsonProjectPropertyTypes::get_tiled_class(const String &name) const {
	if (prop_types) {
		tson::TiledClass *c = const_cast<tson::ProjectPropertyTypes *>(prop_types)->getClass(std::string(name.utf8().get_data()));
		if (c) {
			Ref<GodotTsonTiledClass> tc;
			tc.instantiate();
			tc->set_class(c);
			return tc;
		}
	}
	return Ref<GodotTsonTiledClass>();
}
