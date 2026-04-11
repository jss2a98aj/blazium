/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/tiled_editor_plugin.h"
#include "resource_importer_tiled.h"
#endif

#include "tileson_gd_bindings.h"

void initialize_tiled_importer_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(GodotTsonTileson);
		GDREGISTER_CLASS(GodotTsonMap);
		GDREGISTER_CLASS(GodotTsonLayer);
		GDREGISTER_CLASS(GodotTsonTileset);
		GDREGISTER_CLASS(GodotTsonObject);
		GDREGISTER_CLASS(GodotTsonChunk);
		GDREGISTER_CLASS(GodotTsonTile);
		GDREGISTER_CLASS(GodotTsonAnimation);
		GDREGISTER_CLASS(GodotTsonFrame);
		GDREGISTER_CLASS(GodotTsonProperty);
		GDREGISTER_CLASS(GodotTsonWorld);
		GDREGISTER_CLASS(GodotTsonWorldMapData);
		GDREGISTER_CLASS(GodotTsonProject);
		GDREGISTER_CLASS(GodotTsonProjectData);
		GDREGISTER_CLASS(GodotTsonProjectFolder);
		GDREGISTER_CLASS(GodotTsonWangSet);
		GDREGISTER_CLASS(GodotTsonWangTile);
		GDREGISTER_CLASS(GodotTsonWangColor);
		GDREGISTER_CLASS(GodotTsonGrid);
		GDREGISTER_CLASS(GodotTsonTerrain);
		GDREGISTER_CLASS(GodotTsonText);
		GDREGISTER_CLASS(GodotTsonTransformations);
		GDREGISTER_CLASS(GodotTsonTiledClass);
		GDREGISTER_CLASS(GodotTsonEnumDefinition);
		GDREGISTER_CLASS(GodotTsonEnumValue);
		GDREGISTER_CLASS(GodotTsonTileObject);
		GDREGISTER_CLASS(GodotTsonProjectPropertyTypes);
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
		return;
	}

#ifdef TOOLS_ENABLED
	ResourceFormatImporter::get_singleton()->add_importer(memnew(ResourceImporterTiled));
	ResourceFormatImporter::get_singleton()->add_importer(memnew(ResourceImporterTiledTileset));
	EditorPlugins::add_by_type<TiledEditorPlugin>();
#endif
}

void uninitialize_tiled_importer_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
		return;
	}
}
