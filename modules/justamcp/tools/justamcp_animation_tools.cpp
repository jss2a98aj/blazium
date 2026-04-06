/**************************************************************************/
/*  justamcp_animation_tools.cpp                                          */
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

#include "justamcp_animation_tools.h"
#include "../justamcp_editor_plugin.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_file_system.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "scene/resources/packed_scene.h"

#include "scene/animation/animation_blend_space_1d.h"
#include "scene/animation/animation_blend_space_2d.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/animation/animation_node_state_machine.h"
#include "scene/animation/animation_player.h"
#include "scene/animation/animation_tree.h"
#include "scene/resources/animation_library.h"

#include "scene/2d/navigation_agent_2d.h"
#include "scene/2d/navigation_region_2d.h"
#include "scene/3d/navigation_agent_3d.h"
#include "scene/3d/navigation_region_3d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "scene/resources/navigation_mesh.h"

void JustAMCPAnimationTools::_bind_methods() {
}

JustAMCPAnimationTools::JustAMCPAnimationTools() {
}

JustAMCPAnimationTools::~JustAMCPAnimationTools() {
}

String JustAMCPAnimationTools::_ensure_res_path(const String &p_path) {
	if (!p_path.begins_with("res://")) {
		return "res://" + p_path;
	}
	return p_path;
}

void JustAMCPAnimationTools::_refresh_and_reload(const String &p_scene_path) {
	_refresh_filesystem();
	_reload_scene_in_editor(p_scene_path);
}

void JustAMCPAnimationTools::_refresh_filesystem() {
	if (editor_plugin) {
		EditorFileSystem::get_singleton()->scan();
	}
}

void JustAMCPAnimationTools::_reload_scene_in_editor(const String &p_scene_path) {
	if (!editor_plugin) {
		return;
	}
	Node *edited = EditorInterface::get_singleton()->get_edited_scene_root();
	if (edited && edited->get_scene_file_path() == p_scene_path) {
		EditorInterface::get_singleton()->reload_scene_from_path(p_scene_path);
	}
}

Array JustAMCPAnimationTools::_load_scene(const String &p_scene_path) {
	Array ret;
	ret.resize(2);
	ret[0] = (Object *)nullptr;
	Dictionary err;

	if (!FileAccess::exists(p_scene_path)) {
		err["ok"] = false;
		err["error"] = "Scene not found: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	Ref<PackedScene> packed = ResourceLoader::load(p_scene_path);
	if (packed.is_null()) {
		err["ok"] = false;
		err["error"] = "Failed to load: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	Node *root = packed->instantiate();
	if (!root) {
		err["ok"] = false;
		err["error"] = "Failed to instantiate: " + p_scene_path;
		ret[1] = err;
		return ret;
	}

	ret[0] = root;
	ret[1] = Dictionary();
	return ret;
}

Dictionary JustAMCPAnimationTools::_save_scene(Node *p_scene_root, const String &p_scene_path) {
	Dictionary ret;
	Ref<PackedScene> packed;
	packed.instantiate();
	if (packed->pack(p_scene_root) != OK) {
		memdelete(p_scene_root);
		ret["ok"] = false;
		ret["error"] = "Failed to pack scene";
		return ret;
	}
	if (ResourceSaver::save(packed, p_scene_path) != OK) {
		memdelete(p_scene_root);
		ret["ok"] = false;
		ret["error"] = "Failed to save scene";
		return ret;
	}
	memdelete(p_scene_root);
	_refresh_and_reload(p_scene_path);
	return Dictionary();
}

Node *JustAMCPAnimationTools::_find_node(Node *p_root, const String &p_path) {
	if (p_path == "." || p_path.is_empty()) {
		return p_root;
	}
	return p_root->get_node_or_null(p_path);
}

Variant JustAMCPAnimationTools::_parse_value(const Variant &p_value) {
	if (p_value.get_type() == Variant::DICTIONARY) {
		Dictionary value = p_value;
		String t;
		if (value.has("type")) {
			t = value["type"];
		} else if (value.has("_type")) {
			t = value["_type"];
		}
		if (!t.is_empty()) {
			if (t == "Vector2") {
				return Vector2(value.get("x", 0), value.get("y", 0));
			}
			if (t == "Vector3") {
				return Vector3(value.get("x", 0), value.get("y", 0), value.get("z", 0));
			}
			if (t == "Color") {
				return Color(value.get("r", 1), value.get("g", 1), value.get("b", 1), value.get("a", 1));
			}
		}
	} else if (p_value.get_type() == Variant::ARRAY) {
		Array arr = p_value;
		Array result;
		for (int i = 0; i < arr.size(); i++) {
			result.push_back(_parse_value(arr[i]));
		}
		return result;
	}
	return p_value;
}

Variant JustAMCPAnimationTools::_parse_json_maybe(const Variant &p_value) {
	if (p_value.get_type() != Variant::STRING) {
		return p_value;
	}
	String text = p_value;
	Ref<JSON> json;
	json.instantiate();
	if (json->parse(text) == OK) {
		return json->get_data();
	}
	if (text == "null") {
		return Variant();
	}
	return p_value;
}

Array JustAMCPAnimationTools::_parse_method_args(const Array &p_raw_args) {
	Array parsed_args;
	for (int i = 0; i < p_raw_args.size(); i++) {
		Variant parsed = _parse_json_maybe(p_raw_args[i]);
		parsed_args.push_back(_parse_value(parsed));
	}
	return parsed_args;
}

Ref<Resource> JustAMCPAnimationTools::_get_default_animation_library(Node *p_player) {
	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(p_player);
	if (!player) {
		return Ref<Resource>();
	}

	Ref<AnimationLibrary> anim_lib;
	if (player->has_animation_library("")) {
		anim_lib = player->get_animation_library("");
		if (anim_lib.is_valid()) {
			return anim_lib;
		}
	}

	anim_lib.instantiate();
	Error err = player->add_animation_library("", anim_lib);
	if (err != OK) {
		return Ref<Resource>();
	}
	return anim_lib;
}

Ref<Resource> JustAMCPAnimationTools::_get_state_machine(Node *p_anim_tree, const String &p_state_machine_path) {
	AnimationTree *anim_tree = Object::cast_to<AnimationTree>(p_anim_tree);
	if (!anim_tree) {
		return Ref<Resource>();
	}

	Ref<AnimationNode> current = anim_tree->get_root_animation_node();
	if (p_state_machine_path.is_empty() || p_state_machine_path == "root") {
		return current; // Assuming it's a StateMachine
	}

	Vector<String> segments = p_state_machine_path.split("/", false);
	for (int i = 0; i < segments.size(); i++) {
		String segment = segments[i];
		if (segment.is_empty()) {
			continue;
		}

		Ref<AnimationNodeStateMachine> sm = current;
		Ref<AnimationNodeBlendTree> bt = current;

		if (sm.is_valid()) {
			if (sm->has_node(StringName(segment))) {
				current = sm->get_node(StringName(segment));
			} else {
				return Ref<Resource>();
			}
		} else if (bt.is_valid()) {
			if (bt->has_node(StringName(segment))) {
				current = bt->get_node(StringName(segment));
			} else {
				return Ref<Resource>();
			}
		} else {
			return Ref<Resource>();
		}
	}
	return current;
}

Dictionary JustAMCPAnimationTools::create_animation(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String player_node_path = p_args.get("playerNodePath", ".");
	String animation_name = p_args.get("animationName", "");
	String loop_mode_name = p_args.get("loopMode", "none");

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (animation_name.strip_edges().is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing animationName";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(_find_node(scene_root, player_node_path));
	if (!player) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationPlayer not found at: " + player_node_path;
		return ret;
	}

	Ref<AnimationLibrary> anim_lib = _get_default_animation_library(player);
	if (anim_lib.is_null()) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to create default AnimationLibrary";
		return ret;
	}
	if (anim_lib->has_animation(StringName(animation_name))) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Animation already exists: " + animation_name;
		return ret;
	}

	Animation::LoopMode loop_mode = Animation::LOOP_NONE;
	if (loop_mode_name == "linear") {
		loop_mode = Animation::LOOP_LINEAR;
	} else if (loop_mode_name == "pingpong") {
		loop_mode = Animation::LOOP_PINGPONG;
	}

	Ref<Animation> anim;
	anim.instantiate();
	anim->set_length(p_args.get("length", 1.0));
	anim->set_loop_mode(loop_mode);
	anim->set_step(p_args.get("step", 0.1));

	Error add_err = anim_lib->add_animation(StringName(animation_name), anim);
	if (add_err != OK) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Failed to add animation: " + itos(add_err);
		return ret;
	}

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		return save_err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["animationName"] = animation_name;
	ret["length"] = anim->get_length();
	ret["loopMode"] = loop_mode_name;
	return ret;
}

Dictionary JustAMCPAnimationTools::add_animation_track(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String player_node_path = p_args.get("playerNodePath", ".");
	String animation_name = p_args.get("animationName", "");
	Dictionary track = p_args.get("track", Dictionary());

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (animation_name.strip_edges().is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing animationName";
		return ret;
	}
	if (track.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing track";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	AnimationPlayer *player = Object::cast_to<AnimationPlayer>(_find_node(scene_root, player_node_path));
	if (!player) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationPlayer not found at: " + player_node_path;
		return ret;
	}

	Ref<AnimationLibrary> anim_lib;
	if (player->has_animation_library("")) {
		anim_lib = player->get_animation_library("");
	}
	if (anim_lib.is_null()) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Default AnimationLibrary not found";
		return ret;
	}

	Ref<Animation> anim;
	if (anim_lib->has_animation(StringName(animation_name))) {
		anim = anim_lib->get_animation(StringName(animation_name));
	}
	if (anim.is_null()) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Animation not found: " + animation_name;
		return ret;
	}

	String track_type = track.get("type", "");
	int track_idx = -1;
	Array keyframes = track.get("keyframes", Array());

	if (track_type == "property") {
		String node_path_str = track.get("nodePath", "");
		String prop_name = track.get("property", "");
		if (prop_name.is_empty()) {
			memdelete(scene_root);
			Dictionary ret;
			ret["ok"] = false;
			ret["error"] = "track.property is required for property track";
			return ret;
		}
		track_idx = anim->add_track(Animation::TYPE_VALUE);
		anim->track_set_path(track_idx, NodePath(node_path_str + ":" + prop_name));
		for (int i = 0; i < keyframes.size(); i++) {
			if (keyframes[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary keyframe = keyframes[i];
			Variant raw_value = keyframe.get("value", Variant());
			Variant parsed_value = (raw_value.get_type() == Variant::STRING) ? _parse_json_maybe(raw_value) : raw_value;
			anim->track_insert_key(track_idx, (float)keyframe.get("time", 0.0), _parse_value(parsed_value));
		}
	} else if (track_type == "method") {
		String method_node_path = track.get("nodePath", "");
		String method_name = track.get("method", "");
		if (method_name.is_empty()) {
			memdelete(scene_root);
			Dictionary ret;
			ret["ok"] = false;
			ret["error"] = "track.method is required for method track";
			return ret;
		}
		track_idx = anim->add_track(Animation::TYPE_METHOD);
		anim->track_set_path(track_idx, NodePath(method_node_path));
		for (int i = 0; i < keyframes.size(); i++) {
			if (keyframes[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary keyframe = keyframes[i];
			Dictionary m_dict;
			m_dict["method"] = method_name;
			m_dict["args"] = _parse_method_args(keyframe.get("args", Array()));
			anim->track_insert_key(track_idx, (float)keyframe.get("time", 0.0), m_dict);
		}
	} else {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Unsupported track.type: " + track_type;
		return ret;
	}

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		return save_err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["trackType"] = track_type;
	ret["trackIndex"] = track_idx;
	return ret;
}

Dictionary JustAMCPAnimationTools::create_animation_tree(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String parent_path = p_args.get("parentPath", ".");
	String node_name = p_args.get("nodeName", "AnimationTree");
	String anim_player_path = p_args.get("animPlayerPath", "");
	String root_type = p_args.get("rootType", "StateMachine");

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (anim_player_path.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing animPlayerPath";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	Node *parent = _find_node(scene_root, parent_path);
	if (!parent) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Parent node not found: " + parent_path;
		return ret;
	}

	AnimationTree *anim_tree = memnew(AnimationTree);
	anim_tree->set_name(node_name);
	anim_tree->set_animation_player(NodePath(anim_player_path));

	Ref<AnimationRootNode> root;
	if (root_type == "StateMachine") {
		root.instantiate();
		Ref<AnimationNodeStateMachine> sm;
		sm.instantiate();
		root = sm;
	} else if (root_type == "BlendTree") {
		root.instantiate();
		Ref<AnimationNodeBlendTree> bt;
		bt.instantiate();
		root = bt;
	} else if (root_type == "BlendSpace1D") {
		root.instantiate();
		// In Godot 4, it's AnimationNodeBlendSpace1D or similar
		Ref<AnimationNodeBlendSpace1D> bs1d;
		bs1d.instantiate();
		root = bs1d;
	} else if (root_type == "BlendSpace2D") {
		root.instantiate();
		Ref<AnimationNodeBlendSpace2D> bs2d;
		bs2d.instantiate();
		root = bs2d;
	} else {
		memdelete(scene_root);
		memdelete(anim_tree);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Unsupported rootType: " + root_type;
		return ret;
	}

	anim_tree->set_root_animation_node(root);
	parent->add_child(anim_tree);
	anim_tree->set_owner(scene_root);

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		return save_err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["nodeName"] = node_name;
	ret["rootType"] = root_type;
	return ret;
}

Dictionary JustAMCPAnimationTools::add_animation_state(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String anim_tree_path = p_args.get("animTreePath", "");
	String state_name = p_args.get("stateName", "");
	String animation_name = p_args.get("animationName", "");
	String state_machine_path = p_args.get("stateMachinePath", "");

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (anim_tree_path.is_empty() || state_name.is_empty() || animation_name.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing animTreePath, stateName or animationName";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	AnimationTree *anim_tree = Object::cast_to<AnimationTree>(_find_node(scene_root, anim_tree_path));
	if (!anim_tree) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationTree not found at: " + anim_tree_path;
		return ret;
	}

	Ref<AnimationNodeStateMachine> sm = _get_state_machine(anim_tree, state_machine_path);
	if (sm.is_null()) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationNodeStateMachine not found";
		return ret;
	}

	Ref<AnimationNodeAnimation> anim_node;
	anim_node.instantiate();
	anim_node->set_animation(StringName(animation_name));
	sm->add_node(StringName(state_name), anim_node);

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		return save_err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["stateName"] = state_name;
	ret["animationName"] = animation_name;
	return ret;
}

Dictionary JustAMCPAnimationTools::connect_animation_states(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String anim_tree_path = p_args.get("animTreePath", "");
	String from_state = p_args.get("fromState", "");
	String to_state = p_args.get("toState", "");
	String transition_type = p_args.get("transitionType", "immediate");
	String state_machine_path = p_args.get("stateMachinePath", "");
	String advance_condition = p_args.get("advanceCondition", "");

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}
	if (anim_tree_path.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing animTreePath";
		return ret;
	}
	if (from_state.is_empty() || to_state.is_empty()) {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing fromState or toState";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	AnimationTree *anim_tree = Object::cast_to<AnimationTree>(_find_node(scene_root, anim_tree_path));
	if (!anim_tree) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationTree not found at: " + anim_tree_path;
		return ret;
	}

	Ref<AnimationNodeStateMachine> sm = _get_state_machine(anim_tree, state_machine_path);
	if (sm.is_null()) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "AnimationNodeStateMachine not found";
		return ret;
	}

	Ref<AnimationNodeStateMachineTransition> transition;
	transition.instantiate();

	if (transition_type == "sync") {
		transition->set_switch_mode(AnimationNodeStateMachineTransition::SWITCH_MODE_SYNC);
	} else if (transition_type == "at_end") {
		transition->set_switch_mode(AnimationNodeStateMachineTransition::SWITCH_MODE_AT_END);
	} else if (transition_type == "immediate") {
		transition->set_switch_mode(AnimationNodeStateMachineTransition::SWITCH_MODE_IMMEDIATE);
	} else {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Unsupported transitionType: " + transition_type;
		return ret;
	}

	if (!advance_condition.is_empty()) {
		transition->set_advance_condition(StringName(advance_condition));
	}

	sm->add_transition(StringName(from_state), StringName(to_state), transition);

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		return save_err;
	}

	Dictionary ret;
	ret["ok"] = true;
	ret["from"] = from_state;
	ret["to"] = to_state;
	return ret;
}

Dictionary JustAMCPAnimationTools::create_navigation_region(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String parent_path = p_args.get("parentPath", ".");
	String node_name = p_args.get("nodeName", "NavigationRegion");
	bool is_3d = p_args.get("is3D", false);

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	Node *parent = _find_node(scene_root, parent_path);
	if (!parent) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Parent node not found: " + parent_path;
		return ret;
	}

	Node *nav = nullptr;
	if (is_3d) {
		NavigationRegion3D *nav3d = memnew(NavigationRegion3D);
		Ref<NavigationMesh> nmesh;
		nmesh.instantiate();
		nav3d->set_navigation_mesh(nmesh);
		nav = nav3d;
	} else {
		NavigationRegion2D *nav2d = memnew(NavigationRegion2D);
		Ref<NavigationPolygon> npoly;
		npoly.instantiate();
		nav2d->set_navigation_polygon(npoly);
		nav = nav2d;
	}

	nav->set_name(node_name);
	parent->add_child(nav);
	nav->set_owner(scene_root);

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		memdelete(scene_root);
		return save_err;
	}

	memdelete(scene_root);
	Dictionary ret;
	ret["ok"] = true;
	ret["nodeName"] = node_name;
	ret["is3D"] = is_3d;
	return ret;
}

Dictionary JustAMCPAnimationTools::create_navigation_agent(const Dictionary &p_args) {
	String scene_path = _ensure_res_path(p_args.get("scenePath", ""));
	String parent_path = p_args.get("parentPath", ".");
	String node_name = p_args.get("nodeName", "NavigationAgent");
	bool is_3d = p_args.get("is3D", false);

	if (scene_path == "res://") {
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Missing scenePath";
		return ret;
	}

	Array loaded = _load_scene(scene_path);
	Dictionary err = loaded[1];
	if (!err.is_empty()) {
		return err;
	}

	Node *scene_root = Object::cast_to<Node>(loaded[0]);
	Node *parent = _find_node(scene_root, parent_path);
	if (!parent) {
		memdelete(scene_root);
		Dictionary ret;
		ret["ok"] = false;
		ret["error"] = "Parent node not found: " + parent_path;
		return ret;
	}

	Node *agent = nullptr;
	if (is_3d) {
		NavigationAgent3D *agent3d = memnew(NavigationAgent3D);
		agent3d->set_name(node_name);
		if (p_args.has("pathDesiredDistance")) {
			agent3d->set_path_desired_distance((float)p_args["pathDesiredDistance"]);
		}
		if (p_args.has("targetDesiredDistance")) {
			agent3d->set_target_desired_distance((float)p_args["targetDesiredDistance"]);
		}
		agent = agent3d;
	} else {
		NavigationAgent2D *agent2d = memnew(NavigationAgent2D);
		agent2d->set_name(node_name);
		if (p_args.has("pathDesiredDistance")) {
			agent2d->set_path_desired_distance((float)p_args["pathDesiredDistance"]);
		}
		if (p_args.has("targetDesiredDistance")) {
			agent2d->set_target_desired_distance((float)p_args["targetDesiredDistance"]);
		}
		agent = agent2d;
	}

	parent->add_child(agent);
	agent->set_owner(scene_root);

	Dictionary save_err = _save_scene(scene_root, scene_path);
	if (!save_err.is_empty()) {
		memdelete(scene_root);
		return save_err;
	}

	memdelete(scene_root);
	Dictionary ret;
	ret["ok"] = true;
	ret["nodeName"] = node_name;
	ret["is3D"] = is_3d;
	return ret;
}

#endif // TOOLS_ENABLED
