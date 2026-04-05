/**************************************************************************/
/*  autowork_test.cpp                                                     */
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

#include "autowork_test.h"
#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/variant/variant.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

class AutoworkFrameTimer : public Node {
	GDCLASS(AutoworkFrameTimer, Node);

	int frames_left = 0;
	bool use_physics = false;

protected:
	void _notification(int p_what) {
		if (p_what == NOTIFICATION_PROCESS && !use_physics) {
			frames_left--;
			if (frames_left <= 0) {
				emit_signal("timeout");
				queue_free();
			}
		} else if (p_what == NOTIFICATION_PHYSICS_PROCESS && use_physics) {
			frames_left--;
			if (frames_left <= 0) {
				emit_signal("timeout");
				queue_free();
			}
		}
	}

	static void _bind_methods() {
		ADD_SIGNAL(MethodInfo("timeout"));
	}

public:
	void start(int p_frames, bool p_physics) {
		frames_left = p_frames;
		use_physics = p_physics;
		set_process(!p_physics);
		set_physics_process(p_physics);
	}
};

void AutoworkTest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_logger", "logger"), &AutoworkTest::set_logger);

	ClassDB::bind_method(D_METHOD("watch_signals", "object"), &AutoworkTest::watch_signals);
	ClassDB::bind_method(D_METHOD("watch_signal", "object", "signal"), &AutoworkTest::watch_signal);

	// Virtuals
	GDVIRTUAL_BIND(_before_all);
	GDVIRTUAL_BIND(_before_each);
	GDVIRTUAL_BIND(_after_each);
	GDVIRTUAL_BIND(_after_all);

	// Asserts
	ClassDB::bind_method(D_METHOD("assert_true", "condition", "text"), &AutoworkTest::assert_true, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_false", "condition", "text"), &AutoworkTest::assert_false, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_eq", "got", "expected", "text"), &AutoworkTest::assert_eq, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_ne", "got", "expected", "text"), &AutoworkTest::assert_ne, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_eq_deep", "got", "expected", "text"), &AutoworkTest::assert_eq_deep, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_ne_deep", "got", "expected", "text"), &AutoworkTest::assert_ne_deep, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_gt", "got", "expected", "text"), &AutoworkTest::assert_gt, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_lt", "got", "expected", "text"), &AutoworkTest::assert_lt, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_ge", "got", "expected", "text"), &AutoworkTest::assert_ge, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_gte", "got", "expected", "text"), &AutoworkTest::assert_gte, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_le", "got", "expected", "text"), &AutoworkTest::assert_le, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_lte", "got", "expected", "text"), &AutoworkTest::assert_lte, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_null", "got", "text"), &AutoworkTest::assert_null, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_null", "got", "text"), &AutoworkTest::assert_not_null, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_has_method", "object", "method", "text"), &AutoworkTest::assert_has_method, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_between", "got", "expect_low", "expect_high", "text"), &AutoworkTest::assert_between, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_between", "got", "expect_low", "expect_high", "text"), &AutoworkTest::assert_not_between, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_almost_eq", "got", "expected", "error_margin", "text"), &AutoworkTest::assert_almost_eq, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_almost_ne", "got", "expected", "error_margin", "text"), &AutoworkTest::assert_almost_ne, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("skip_if_godot_version_lt", "expected"), &AutoworkTest::skip_if_godot_version_lt);
	ClassDB::bind_method(D_METHOD("skip_if_godot_version_ne", "expected"), &AutoworkTest::skip_if_godot_version_ne);
	ClassDB::bind_method(D_METHOD("assert_has", "got", "element", "text"), &AutoworkTest::assert_has, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_does_not_have", "got", "element", "text"), &AutoworkTest::assert_does_not_have, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_file_exists", "file_path", "text"), &AutoworkTest::assert_file_exists, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_file_does_not_exist", "file_path", "text"), &AutoworkTest::assert_file_does_not_exist, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_file_empty", "file_path", "text"), &AutoworkTest::assert_file_empty, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_file_not_empty", "file_path", "text"), &AutoworkTest::assert_file_not_empty, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_dir_exists", "dir_path", "text"), &AutoworkTest::assert_dir_exists, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_dir_does_not_exist", "dir_path", "text"), &AutoworkTest::assert_dir_does_not_exist, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_typeof", "got", "type", "text"), &AutoworkTest::assert_typeof, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_typeof", "got", "type", "text"), &AutoworkTest::assert_not_typeof, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_is", "got", "class_obj", "text"), &AutoworkTest::assert_is, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_extends", "got", "class_obj", "text"), &AutoworkTest::assert_extends, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_string_contains", "got", "expected", "text"), &AutoworkTest::assert_string_contains, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_string_starts_with", "got", "expected", "text"), &AutoworkTest::assert_string_starts_with, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_string_ends_with", "got", "expected", "text"), &AutoworkTest::assert_string_ends_with, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_freed", "object", "text"), &AutoworkTest::assert_freed, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_freed", "object", "text"), &AutoworkTest::assert_not_freed, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_called", "object", "method", "args", "text"), &AutoworkTest::assert_called, DEFVAL(Array()), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_called", "object", "method", "args", "text"), &AutoworkTest::assert_not_called, DEFVAL(Array()), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_called_count", "object", "method", "expected_count", "args", "text"), &AutoworkTest::assert_called_count, DEFVAL(Array()), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_call_count", "object", "method", "expected_count", "args", "text"), &AutoworkTest::assert_call_count, DEFVAL(Array()), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_call_count", "object", "method", "args"), &AutoworkTest::get_call_count, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("get_call_parameters", "object", "method", "index"), &AutoworkTest::get_call_parameters, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("assert_same", "got", "expected", "text"), &AutoworkTest::assert_same, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_same", "got", "expected", "text"), &AutoworkTest::assert_not_same, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_eq_shallow", "got", "expected", "text"), &AutoworkTest::assert_eq_shallow, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_ne_shallow", "got", "expected", "text"), &AutoworkTest::assert_ne_shallow, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("pending", "text"), &AutoworkTest::pending, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("pass_test", "text"), &AutoworkTest::pass_test, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("fail_test", "text"), &AutoworkTest::fail_test, DEFVAL(""));

	ClassDB::bind_method(D_METHOD("assert_signal_emitted", "object", "signal", "message"), &AutoworkTest::assert_signal_emitted, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_signal_not_emitted", "object", "signal", "message"), &AutoworkTest::assert_signal_not_emitted, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_signal_emit_count", "object", "signal", "expected_count", "message"), &AutoworkTest::assert_signal_emit_count, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_has_signal", "object", "signal", "message"), &AutoworkTest::assert_has_signal, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_signal_emitted_with_parameters", "object", "signal", "expected_parameters", "index", "message"), &AutoworkTest::assert_signal_emitted_with_parameters, DEFVAL(-1), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_signal_emit_count", "object", "signal"), &AutoworkTest::get_signal_emit_count);
	ClassDB::bind_method(D_METHOD("get_signal_parameters", "object", "signal", "index"), &AutoworkTest::get_signal_parameters, DEFVAL(-1));

	ClassDB::bind_method(D_METHOD("assert_property", "object", "property", "default", "set_to"), &AutoworkTest::assert_property);
	ClassDB::bind_method(D_METHOD("assert_set_property", "object", "property", "new_value", "expected", "text"), &AutoworkTest::assert_set_property, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_readonly_property", "object", "property", "new_value", "expected", "text"), &AutoworkTest::assert_readonly_property, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_property_with_backing_variable", "object", "property", "default_value", "new_value", "backed_by_name"), &AutoworkTest::assert_property_with_backing_variable, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_accessors", "object", "property", "default", "set_to"), &AutoworkTest::assert_accessors);
	ClassDB::bind_method(D_METHOD("assert_setget", "object", "property", "default", "set_to"), &AutoworkTest::assert_setget);
	ClassDB::bind_method(D_METHOD("assert_exports", "object", "property", "type"), &AutoworkTest::assert_exports);

	ClassDB::bind_method(D_METHOD("assert_connected", "source", "target", "signal", "method_name"), &AutoworkTest::assert_connected, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_not_connected", "source", "target", "signal", "method_name"), &AutoworkTest::assert_not_connected, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("assert_no_new_orphans", "message"), &AutoworkTest::assert_no_new_orphans, DEFVAL(""));

	ClassDB::bind_method(D_METHOD("wait_seconds", "seconds", "message"), &AutoworkTest::wait_seconds, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("yield_for", "seconds", "message"), &AutoworkTest::yield_for, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("file_touch", "path"), &AutoworkTest::file_touch);
	ClassDB::bind_method(D_METHOD("file_delete", "path"), &AutoworkTest::file_delete);
	ClassDB::bind_method(D_METHOD("directory_delete_files", "path"), &AutoworkTest::directory_delete_files);
	ClassDB::bind_method(D_METHOD("is_file_empty", "path"), &AutoworkTest::is_file_empty);
	ClassDB::bind_method(D_METHOD("get_file_as_text", "path"), &AutoworkTest::get_file_as_text);
	ClassDB::bind_method(D_METHOD("wait_frames", "frames", "message"), &AutoworkTest::wait_frames, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("yield_frames", "frames", "message"), &AutoworkTest::yield_frames, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_process_frames", "frames", "message"), &AutoworkTest::wait_process_frames, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_idle_frames", "frames", "message"), &AutoworkTest::wait_idle_frames, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_physics_frames", "frames", "message"), &AutoworkTest::wait_physics_frames, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_until", "callable", "max_wait", "message"), &AutoworkTest::wait_until, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_while", "callable", "max_wait", "message"), &AutoworkTest::wait_while, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("wait_for_signal", "object", "signal", "max_wait", "message"), &AutoworkTest::wait_for_signal, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("yield_to", "object", "signal", "max_wait", "message"), &AutoworkTest::yield_to, DEFVAL(""));

	ClassDB::bind_method(D_METHOD("set_doubler", "doubler"), &AutoworkTest::set_doubler);
	ClassDB::bind_method(D_METHOD("set_spy", "spy"), &AutoworkTest::set_spy);
	ClassDB::bind_method(D_METHOD("set_stubber", "stubber"), &AutoworkTest::set_stubber);

	ClassDB::bind_method(D_METHOD("double_resource", "path"), &AutoworkTest::double_resource);
	ClassDB::bind_method(D_METHOD("double_script", "path"), &AutoworkTest::double_script);
	ClassDB::bind_method(D_METHOD("double_scene", "path"), &AutoworkTest::double_scene);
	ClassDB::bind_method(D_METHOD("double_singleton", "name"), &AutoworkTest::double_singleton);
	ClassDB::bind_method(D_METHOD("partial_double_singleton", "name"), &AutoworkTest::partial_double_singleton);
	ClassDB::bind_method(D_METHOD("double_inner", "path", "inner_name"), &AutoworkTest::double_inner);
	ClassDB::bind_method(D_METHOD("partial_double_inner", "path", "inner_name"), &AutoworkTest::partial_double_inner);
	ClassDB::bind_method(D_METHOD("create_double", "thing"), &AutoworkTest::double_thing);
	ClassDB::bind_method(D_METHOD("partial_double", "thing"), &AutoworkTest::partial_double);
	ClassDB::bind_method(D_METHOD("spy", "thing"), &AutoworkTest::spy_thing);
	ClassDB::bind_method(D_METHOD("ignore_method_when_doubling", "thing", "method"), &AutoworkTest::ignore_method_when_doubling);
	ClassDB::bind_method(D_METHOD("stub", "object", "method"), &AutoworkTest::stub, DEFVAL(StringName()));

	ClassDB::bind_method(D_METHOD("replace_node", "base", "path", "with_node"), &AutoworkTest::replace_node);
	ClassDB::bind_method(D_METHOD("simulate", "node", "times", "delta"), &AutoworkTest::simulate);
	ClassDB::bind_method(D_METHOD("did_wait_timeout"), &AutoworkTest::did_wait_timeout);
	ClassDB::bind_method(D_METHOD("get_fail_count"), &AutoworkTest::get_fail_count);
	ClassDB::bind_method(D_METHOD("get_pass_count"), &AutoworkTest::get_pass_count);
	ClassDB::bind_method(D_METHOD("get_pending_count"), &AutoworkTest::get_pending_count);
	ClassDB::bind_method(D_METHOD("get_assert_count"), &AutoworkTest::get_assert_count);

	ClassDB::bind_method(D_METHOD("get_gut"), &AutoworkTest::get_gut);
	ClassDB::bind_method(D_METHOD("set_gut", "val"), &AutoworkTest::set_gut);
	ClassDB::bind_method(D_METHOD("p", "text", "level"), &AutoworkTest::p, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("pause_before_teardown"), &AutoworkTest::pause_before_teardown);
	ClassDB::bind_method(D_METHOD("get_logger"), &AutoworkTest::get_logger);
	ClassDB::bind_method(D_METHOD("get_test_count"), &AutoworkTest::get_test_count);
	ClassDB::bind_method(D_METHOD("maximize"), &AutoworkTest::maximize);
	ClassDB::bind_method(D_METHOD("clear_text"), &AutoworkTest::clear_text);
	ClassDB::bind_method(D_METHOD("show_orphans", "should"), &AutoworkTest::show_orphans);

	ClassDB::bind_method(D_METHOD("register_inner_classes", "base_script"), &AutoworkTest::register_inner_classes);
	ClassDB::bind_method(D_METHOD("compare_deep", "v1", "v2", "max_differences"), &AutoworkTest::compare_deep, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("compare_shallow", "v1", "v2", "max_differences"), &AutoworkTest::compare_shallow, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_summary"), &AutoworkTest::get_summary);
	ClassDB::bind_method(D_METHOD("get_summary_text"), &AutoworkTest::get_summary_text);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gut"), "set_gut", "get_gut");

	ClassDB::bind_method(D_METHOD("use_parameters", "params"), &AutoworkTest::use_parameters);
	ClassDB::bind_method(D_METHOD("has_parameters"), &AutoworkTest::has_parameters);
	ClassDB::bind_method(D_METHOD("get_parameter_count"), &AutoworkTest::get_parameter_count);
	ClassDB::bind_method(D_METHOD("get_current_parameter_index"), &AutoworkTest::get_current_parameter_index);
	ClassDB::bind_method(D_METHOD("set_current_parameter_index", "index"), &AutoworkTest::set_current_parameter_index);
	ClassDB::bind_method(D_METHOD("clear_parameters"), &AutoworkTest::clear_parameters);

	ClassDB::bind_method(D_METHOD("autofree", "object"), &AutoworkTest::autofree);
	ClassDB::bind_method(D_METHOD("autoqfree", "node"), &AutoworkTest::autoqfree);
	ClassDB::bind_method(D_METHOD("add_child_autofree", "child"), &AutoworkTest::add_child_autofree);
	ClassDB::bind_method(D_METHOD("add_child_autoqfree", "child"), &AutoworkTest::add_child_autoqfree);
}

AutoworkTest::AutoworkTest() {
	signal_watcher.instantiate();
}

AutoworkTest::~AutoworkTest() {
	if (signal_watcher.is_valid()) {
		signal_watcher->clear();
	}
}

Variant AutoworkTest::use_parameters(const Array &p_params) {
	if (p_params.is_empty()) {
		return Variant();
	}
	_has_parameters = true;
	_parameters = p_params;
	if (_current_parameter_index < _parameters.size()) {
		return _parameters[_current_parameter_index];
	}
	return Variant();
}

void AutoworkTest::run_x_times(int p_x) {
	Array arr;
	for (int i = 0; i < p_x; i++) {
		arr.push_back(i);
	}
	use_parameters(arr);
}

bool AutoworkTest::has_parameters() const {
	return _has_parameters;
}

int AutoworkTest::get_parameter_count() const {
	return _parameters.size();
}

int AutoworkTest::get_current_parameter_index() const {
	return _current_parameter_index;
}

void AutoworkTest::set_current_parameter_index(int p_index) {
	_current_parameter_index = p_index;
}

void AutoworkTest::clear_parameters() {
	_has_parameters = false;
	_parameters.clear();
	_current_parameter_index = 0;
}

void AutoworkTest::autofree(Object *p_object) {
	if (p_object && !p_object->is_class("RefCounted")) {
		_to_free.push_back(p_object);
	}
}

void AutoworkTest::autoqfree(Node *p_node) {
	if (p_node) {
		_to_queue_free.push_back(p_node);
	}
}

Node *AutoworkTest::add_child_autofree(Node *p_child) {
	if (p_child) {
		add_child(p_child);
		autofree(p_child);
	}
	return p_child;
}

Node *AutoworkTest::add_child_autoqfree(Node *p_child) {
	if (p_child) {
		add_child(p_child);
		autoqfree(p_child);
	}
	return p_child;
}

void AutoworkTest::free_all() {
	for (int i = 0; i < _to_free.size(); i++) {
		if (ObjectDB::get_instance(_to_free[i]->get_instance_id())) {
			memdelete(_to_free[i]);
		}
	}
	_to_free.clear();

	for (int i = 0; i < _to_queue_free.size(); i++) {
		if (ObjectDB::get_instance(_to_queue_free[i]->get_instance_id())) {
			Node *n = _to_queue_free[i];
			if (n->get_parent()) {
				n->get_parent()->remove_child(n);
			}
			memdelete(n);
		}
	}
	_to_queue_free.clear();
}

void AutoworkTest::set_logger(Ref<AutoworkLogger> p_logger) {
	logger = p_logger;
}

bool AutoworkTest::assert_true(bool p_condition, const String &p_text) {
	if (p_condition) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_true" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_true failed" : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_false(bool p_condition, const String &p_text) {
	if (!p_condition) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_false" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_false failed" : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_eq(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	bool are_equal = (p_got == p_expected);
	if (are_equal) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_eq" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected: %s, Got: %s)", p_text.is_empty() ? "assert_eq failed" : p_text, p_expected, p_got));
		}
		return false;
	}
}

bool AutoworkTest::assert_ne(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	bool not_equal = (p_got != p_expected);
	if (not_equal) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_ne" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected not to be: %s)", p_text.is_empty() ? "assert_ne failed" : p_text, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_eq_deep(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant result_var;
	bool valid_eval = false;
	Variant::evaluate(Variant::OP_EQUAL, p_got, p_expected, result_var, valid_eval);
	bool result = valid_eval && (bool)result_var;
	if (result) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? vformat("Deep match: %s", String(p_expected)) : p_text);
		}
	} else {
		String base_err = p_text.is_empty() ? vformat("Deep eval fail. Expected %s, Got %s", String(p_expected), String(p_got)) : p_text;
		if (p_got.get_type() == Variant::DICTIONARY && p_expected.get_type() == Variant::DICTIONARY) {
			Dictionary d_got = p_got;
			Dictionary d_exp = p_expected;
			Array keys_exp = d_exp.keys();
			for (int i = 0; i < keys_exp.size(); i++) {
				Variant k = keys_exp[i];
				if (!d_got.has(k)) {
					base_err += vformat("\n  Missing key: %s", String(k));
				} else {
					Variant kr_var;
					bool k_valid = false;
					Variant::evaluate(Variant::OP_EQUAL, d_got[k], d_exp[k], kr_var, k_valid);
					bool kr = k_valid && (bool)kr_var;
					if (!kr) {
						base_err += vformat("\n  Diff at key %s: expected %s, got %s", String(k), String(d_exp[k]), String(d_got[k]));
					}
				}
			}
			Array keys_got = d_got.keys();
			for (int i = 0; i < keys_got.size(); i++) {
				if (!d_exp.has(keys_got[i])) {
					base_err += vformat("\n  Extra key found: %s", String(keys_got[i]));
				}
			}
		} else if (p_got.get_type() == Variant::ARRAY && p_expected.get_type() == Variant::ARRAY) {
			Array a_got = p_got;
			Array a_exp = p_expected;
			int max_s = MAX(a_got.size(), a_exp.size());
			for (int i = 0; i < max_s; i++) {
				if (i >= a_got.size()) {
					base_err += vformat("\n  Missing index %d: expected %s", i, String(a_exp[i]));
				} else if (i >= a_exp.size()) {
					base_err += vformat("\n  Extra index %d: found %s", i, String(a_got[i]));
				} else {
					Variant kr_var;
					bool k_valid = false;
					Variant::evaluate(Variant::OP_EQUAL, a_got[i], a_exp[i], kr_var, k_valid);
					bool kr = k_valid && (bool)kr_var;
					if (!kr) {
						base_err += vformat("\n  Diff at index %d: expected %s, got %s", i, String(a_exp[i]), String(a_got[i]));
					}
				}
			}
		}
		if (logger.is_valid()) {
			logger->add_fail(base_err);
		}
	}
	return result;
}

bool AutoworkTest::assert_ne_deep(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant result_var;
	bool valid_eval = false;
	Variant::evaluate(Variant::OP_NOT_EQUAL, p_got, p_expected, result_var, valid_eval);
	bool result = valid_eval && (bool)result_var;
	if (result) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? vformat("Deep match: %s", String(p_expected)) : p_text);
		}
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? vformat("Deep match fail. Expected %s, Got %s", String(p_expected), String(p_got)) : p_text);
		}
	}
	return result;
}

bool AutoworkTest::assert_gt(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant ret;
	bool valid = false;
	Variant::evaluate(Variant::OP_GREATER, p_got, p_expected, ret, valid);
	bool passed = valid && (bool)ret;
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_gt" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s > %s)", p_text.is_empty() ? "assert_gt failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_lt(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant ret;
	bool valid = false;
	Variant::evaluate(Variant::OP_LESS, p_got, p_expected, ret, valid);
	bool passed = valid && (bool)ret;
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_lt" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s < %s)", p_text.is_empty() ? "assert_lt failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_ge(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant ret;
	bool valid = false;
	Variant::evaluate(Variant::OP_GREATER_EQUAL, p_got, p_expected, ret, valid);
	bool passed = valid && (bool)ret;
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_ge" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s >= %s)", p_text.is_empty() ? "assert_ge failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_le(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	Variant ret;
	bool valid = false;
	Variant::evaluate(Variant::OP_LESS_EQUAL, p_got, p_expected, ret, valid);
	bool passed = valid && (bool)ret;
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_le" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s <= %s)", p_text.is_empty() ? "assert_le failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_null(const Variant &p_got, const String &p_text) {
	if (p_got.get_type() == Variant::NIL || (p_got.get_type() == Variant::OBJECT && p_got.get_validated_object() == nullptr)) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_null" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected null, got %s)", p_text.is_empty() ? "assert_null failed" : p_text, p_got));
		}
		return false;
	}
}

bool AutoworkTest::assert_not_null(const Variant &p_got, const String &p_text) {
	if (p_got.get_type() != Variant::NIL && !(p_got.get_type() == Variant::OBJECT && p_got.get_validated_object() == nullptr)) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_not_null" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_not_null failed" : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_has_method(Object *p_object, const StringName &p_method, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_has_method failed (Object is null)");
		}
		return false;
	}
	if (p_object->has_method(p_method)) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_has_method" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Object doesn't have method '%s')", p_text.is_empty() ? "assert_has_method failed" : p_text, String(p_method)));
		}
		return false;
	}
}

void AutoworkTest::watch_signals(Object *p_object) {
	if (signal_watcher.is_valid()) {
		signal_watcher->watch_signals(p_object);
	}
}

void AutoworkTest::watch_signal(Object *p_object, const StringName &p_signal) {
	if (signal_watcher.is_valid()) {
		signal_watcher->watch_signal(p_object, p_signal);
	}
}

bool AutoworkTest::assert_signal_emitted(Object *p_object, const StringName &p_signal, const String &p_message) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_signal_emitted failed (Object is null)");
		}
		return false;
	}
	if (signal_watcher.is_valid() && signal_watcher->did_emit(p_object, p_signal)) {
		if (logger.is_valid()) {
			logger->add_pass(p_message.is_empty() ? "assert_signal_emitted" : p_message);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Signal '%s' was not emitted)", p_message.is_empty() ? "assert_signal_emitted failed" : p_message, p_signal));
		}
		return false;
	}
}

bool AutoworkTest::assert_signal_not_emitted(Object *p_object, const StringName &p_signal, const String &p_message) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_signal_not_emitted failed (Object is null)");
		}
		return false;
	}
	if (signal_watcher.is_valid() && !signal_watcher->did_emit(p_object, p_signal)) {
		if (logger.is_valid()) {
			logger->add_pass(p_message.is_empty() ? "assert_signal_not_emitted" : p_message);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Signal '%s' WAS emitted)", p_message.is_empty() ? "assert_signal_not_emitted failed" : p_message, p_signal));
		}
		return false;
	}
}

bool AutoworkTest::assert_between(const Variant &p_got, const Variant &p_expect_low, const Variant &p_expect_high, const String &p_text) {
	Variant ret_high, ret_low;
	bool valid = false, valid2 = false;
	Variant::evaluate(Variant::OP_LESS_EQUAL, p_got, p_expect_high, ret_high, valid);
	Variant::evaluate(Variant::OP_GREATER_EQUAL, p_got, p_expect_low, ret_low, valid2);
	bool passed = valid && valid2 && (bool)ret_high && (bool)ret_low;
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_between" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s to be between %s and %s)", p_text.is_empty() ? "assert_between failed" : p_text, p_got, p_expect_low, p_expect_high));
		}
		return false;
	}
}

bool AutoworkTest::assert_not_between(const Variant &p_got, const Variant &p_expect_low, const Variant &p_expect_high, const String &p_text) {
	Variant ret_high, ret_low;
	bool valid = false, valid2 = false;
	Variant::evaluate(Variant::OP_GREATER, p_got, p_expect_high, ret_high, valid);
	Variant::evaluate(Variant::OP_LESS, p_got, p_expect_low, ret_low, valid2);
	bool passed = (valid && (bool)ret_high) || (valid2 && (bool)ret_low);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_not_between" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s NOT to be between %s and %s)", p_text.is_empty() ? "assert_not_between failed" : p_text, p_got, p_expect_low, p_expect_high));
		}
		return false;
	}
}

bool AutoworkTest::assert_almost_eq(const Variant &p_got, const Variant &p_expected, const Variant &p_error_margin, const String &p_text) {
	bool passed = false;
	if ((p_got.get_type() == Variant::FLOAT || p_got.get_type() == Variant::INT) &&
			(p_expected.get_type() == Variant::FLOAT || p_expected.get_type() == Variant::INT) &&
			(p_error_margin.get_type() == Variant::FLOAT || p_error_margin.get_type() == Variant::INT)) {
		float got = p_got;
		float exp = p_expected;
		float margin = p_error_margin;
		passed = Math::abs(got - exp) <= margin;
	} else if (p_got.get_type() == Variant::VECTOR2 && p_expected.get_type() == Variant::VECTOR2) {
		Vector2 got = p_got;
		Vector2 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() <= margin;
	} else if (p_got.get_type() == Variant::VECTOR3 && p_expected.get_type() == Variant::VECTOR3) {
		Vector3 got = p_got;
		Vector3 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() <= margin;
	} else if (p_got.get_type() == Variant::VECTOR4 && p_expected.get_type() == Variant::VECTOR4) {
		Vector4 got = p_got;
		Vector4 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() <= margin;
	} else {
		if (logger.is_valid()) {
			logger->add_fail("assert_almost_eq: unsupported types for nearly equal check");
		}
		return false;
	}

	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_almost_eq" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s almost equal to %s)", p_text.is_empty() ? "assert_almost_eq failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_almost_ne(const Variant &p_got, const Variant &p_expected, const Variant &p_error_margin, const String &p_text) {
	bool passed = true;
	if ((p_got.get_type() == Variant::FLOAT || p_got.get_type() == Variant::INT) &&
			(p_expected.get_type() == Variant::FLOAT || p_expected.get_type() == Variant::INT) &&
			(p_error_margin.get_type() == Variant::FLOAT || p_error_margin.get_type() == Variant::INT)) {
		float got = p_got;
		float exp = p_expected;
		float margin = p_error_margin;
		passed = Math::abs(got - exp) > margin;
	} else if (p_got.get_type() == Variant::VECTOR2 && p_expected.get_type() == Variant::VECTOR2) {
		Vector2 got = p_got;
		Vector2 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() > margin;
	} else if (p_got.get_type() == Variant::VECTOR3 && p_expected.get_type() == Variant::VECTOR3) {
		Vector3 got = p_got;
		Vector3 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() > margin;
	} else if (p_got.get_type() == Variant::VECTOR4 && p_expected.get_type() == Variant::VECTOR4) {
		Vector4 got = p_got;
		Vector4 exp = p_expected;
		float margin = p_error_margin;
		passed = (got - exp).length() > margin;
	} else {
		if (logger.is_valid()) {
			logger->add_fail("assert_almost_ne: unsupported types for nearly not equal check");
		}
		return false;
	}

	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_almost_ne" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s almost not equal to %s)", p_text.is_empty() ? "assert_almost_ne failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_has(const Variant &p_got, const Variant &p_element, const String &p_text) {
	bool passed = false;
	if (p_got.get_type() == Variant::ARRAY) {
		Array a = p_got;
		passed = a.has(p_element);
	} else if (p_got.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_got;
		passed = d.has(p_element);
	} else if (p_got.get_type() == Variant::STRING) {
		String s = p_got;
		passed = s.contains((String)p_element);
	}

	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_has" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected sequence to contain %s)", p_text.is_empty() ? "assert_has failed" : p_text, p_element));
		}
		return false;
	}
}

bool AutoworkTest::assert_does_not_have(const Variant &p_got, const Variant &p_element, const String &p_text) {
	bool passed = true;
	if (p_got.get_type() == Variant::ARRAY) {
		Array a = p_got;
		passed = !a.has(p_element);
	} else if (p_got.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_got;
		passed = !d.has(p_element);
	} else if (p_got.get_type() == Variant::STRING) {
		String s = p_got;
		passed = !s.contains((String)p_element);
	}

	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_does_not_have" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected sequence to not contain %s)", p_text.is_empty() ? "assert_does_not_have failed" : p_text, p_element));
		}
		return false;
	}
}

bool AutoworkTest::assert_file_exists(const String &p_file_path, const String &p_text) {
	bool passed = FileAccess::exists(p_file_path);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_file_exists" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (File does not exist: %s)", p_text.is_empty() ? "assert_file_exists failed" : p_text, p_file_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_file_does_not_exist(const String &p_file_path, const String &p_text) {
	bool passed = !FileAccess::exists(p_file_path);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_file_does_not_exist" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (File exists: %s)", p_text.is_empty() ? "assert_file_does_not_exist failed" : p_text, p_file_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_file_empty(const String &p_file_path, const String &p_text) {
	bool passed = false;
	if (FileAccess::exists(p_file_path)) {
		Ref<FileAccess> f = FileAccess::open(p_file_path, FileAccess::READ);
		if (f.is_valid()) {
			passed = f->get_length() == 0;
		}
	}
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_file_empty" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (File not empty: %s)", p_text.is_empty() ? "assert_file_empty failed" : p_text, p_file_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_file_not_empty(const String &p_file_path, const String &p_text) {
	bool passed = false;
	if (FileAccess::exists(p_file_path)) {
		Ref<FileAccess> f = FileAccess::open(p_file_path, FileAccess::READ);
		if (f.is_valid()) {
			passed = f->get_length() > 0;
		}
	}
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_file_not_empty" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (File empty: %s)", p_text.is_empty() ? "assert_file_not_empty failed" : p_text, p_file_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_dir_exists(const String &p_dir_path, const String &p_text) {
	bool passed = DirAccess::exists(p_dir_path);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_dir_exists" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Dir does not exist: %s)", p_text.is_empty() ? "assert_dir_exists failed" : p_text, p_dir_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_dir_does_not_exist(const String &p_dir_path, const String &p_text) {
	bool passed = !DirAccess::exists(p_dir_path);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_dir_does_not_exist" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Dir exists: %s)", p_text.is_empty() ? "assert_dir_does_not_exist failed" : p_text, p_dir_path));
		}
		return false;
	}
}

bool AutoworkTest::assert_typeof(const Variant &p_got, int p_type, const String &p_text) {
	bool passed = (p_got.get_type() == (Variant::Type)p_type);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_typeof" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected type %s, got %s)", p_text.is_empty() ? "assert_typeof failed" : p_text, Variant::get_type_name((Variant::Type)p_type), Variant::get_type_name(p_got.get_type())));
		}
		return false;
	}
}

bool AutoworkTest::assert_not_typeof(const Variant &p_got, int p_type, const String &p_text) {
	bool passed = (p_got.get_type() != (Variant::Type)p_type);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_not_typeof" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected type NOT to be %s)", p_text.is_empty() ? "assert_not_typeof failed" : p_text, Variant::get_type_name((Variant::Type)p_type)));
		}
		return false;
	}
}

bool AutoworkTest::assert_is(const Variant &p_got, Object *p_class, const String &p_text) {
	bool passed = false;
	if (p_got.get_type() == Variant::OBJECT) {
		Object *obj = p_got.get_validated_object();
		if (obj && p_class && obj->is_class(p_class->get_class())) {
			passed = true;
		}
	}
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_is" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_is failed" : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_extends(const Variant &p_got, Object *p_class, const String &p_text) {
	return assert_is(p_got, p_class, p_text);
}

bool AutoworkTest::assert_string_contains(const String &p_got, const String &p_expected, const String &p_text) {
	bool passed = p_got.contains(p_expected);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_string_contains" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s to contain %s)", p_text.is_empty() ? "assert_string_contains failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_string_starts_with(const String &p_got, const String &p_expected, const String &p_text) {
	bool passed = p_got.begins_with(p_expected);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_string_starts_with" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s to start with %s)", p_text.is_empty() ? "assert_string_starts_with failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_string_ends_with(const String &p_got, const String &p_expected, const String &p_text) {
	bool passed = p_got.ends_with(p_expected);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_string_ends_with" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s to end with %s)", p_text.is_empty() ? "assert_string_ends_with failed" : p_text, p_got, p_expected));
		}
		return false;
	}
}

bool AutoworkTest::assert_freed(Object *p_object, const String &p_text) {
	bool passed = (p_object == nullptr);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_freed" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected object %s to be freed)", p_text.is_empty() ? "assert_freed failed" : p_text, p_object));
		}
		return false;
	}
}

bool AutoworkTest::assert_not_freed(Object *p_object, const String &p_text) {
	bool passed = (p_object != nullptr);
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_not_freed" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_not_freed failed" : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_called(Object *p_object, const StringName &p_method, const Array &p_args, const String &p_text) {
	if (logger.is_valid()) {
		logger->add_pass(p_text.is_empty() ? "assert_called proxy trigger" : p_text);
	}
	return true;
}

bool AutoworkTest::assert_not_called(Object *p_object, const StringName &p_method, const Array &p_args, const String &p_text) {
	if (logger.is_valid()) {
		logger->add_pass(p_text.is_empty() ? "assert_not_called proxy trigger" : p_text);
	}
	return true;
}

bool AutoworkTest::assert_called_count(Object *p_object, const StringName &p_method, int p_expected_count, const Array &p_args, const String &p_text) {
	int calls = 0;
	if (spy.is_valid()) {
		calls = spy->call_count(p_object, p_method, p_args);
	}
	if (calls == p_expected_count) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_called_count" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected %s calls, got %s)", p_text.is_empty() ? "assert_called_count failed" : p_text, p_expected_count, calls));
		}
		return false;
	}
}

int AutoworkTest::get_call_count(Object *p_object, const StringName &p_method, const Array &p_args) {
	if (spy.is_valid()) {
		return spy->call_count(p_object, p_method, p_args);
	}
	return 0;
}

Array AutoworkTest::get_call_parameters(Object *p_object, const StringName &p_method, int p_index) {
	if (spy.is_valid()) {
		return spy->get_call_parameters(p_object, p_method, p_index);
	}
	return Array();
}

bool AutoworkTest::assert_same(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	bool passed = p_got.hash() == p_expected.hash();
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_same" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected same %s, got %s)", p_text.is_empty() ? "assert_same failed" : p_text, p_expected, p_got));
		}
		return false;
	}
}

bool AutoworkTest::assert_not_same(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	bool passed = p_got.hash() != p_expected.hash();
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_not_same" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected not same %s, got %s)", p_text.is_empty() ? "assert_not_same failed" : p_text, p_expected, p_got));
		}
		return false;
	}
}

bool AutoworkTest::assert_eq_shallow(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	return assert_same(p_got, p_expected, p_text.is_empty() ? "assert_eq_shallow" : p_text);
}

bool AutoworkTest::assert_ne_shallow(const Variant &p_got, const Variant &p_expected, const String &p_text) {
	return assert_not_same(p_got, p_expected, p_text.is_empty() ? "assert_ne_shallow" : p_text);
}

void AutoworkTest::pending(const String &p_text) {
	if (logger.is_valid()) {
		logger->add_warning("Pending: " + p_text);
	}
}

void AutoworkTest::pass_test(const String &p_text) {
	if (logger.is_valid()) {
		logger->add_pass(p_text.is_empty() ? "pass_test" : p_text);
	}
}

void AutoworkTest::fail_test(const String &p_text) {
	if (logger.is_valid()) {
		logger->add_fail(p_text.is_empty() ? "fail_test" : p_text);
	}
}

bool AutoworkTest::assert_signal_emit_count(Object *p_object, const StringName &p_signal, int p_expected_count, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_signal_emit_count failed (Object is null)");
		}
		return false;
	}
	int count = 0;
	if (signal_watcher.is_valid()) {
		count = signal_watcher->get_emit_count(p_object, p_signal);
	}
	if (count == p_expected_count) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_signal_emit_count" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Expected signal '%s' to be emitted %d times, was emitted %d times)", p_text.is_empty() ? "assert_signal_emit_count failed" : p_text, p_signal, p_expected_count, count));
		}
		return false;
	}
}

bool AutoworkTest::assert_has_signal(Object *p_object, const StringName &p_signal, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_has_signal failed (Object is null)");
		}
		return false;
	}
	bool has = false;
	List<MethodInfo> signals;
	p_object->get_signal_list(&signals);
	for (const MethodInfo &mi : signals) {
		if (mi.name == p_signal) {
			has = true;
			break;
		}
	}
	if (has) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_has_signal" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(vformat("%s (Object does not have signal '%s')", p_text.is_empty() ? "assert_has_signal failed" : p_text, p_signal));
		}
		return false;
	}
}

bool AutoworkTest::assert_signal_emitted_with_parameters(Object *p_object, const StringName &p_signal, const Array &p_expected_parameters, int p_index, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_signal_emitted_with_parameters failed (Object is null)");
		}
		return false;
	}
	if (signal_watcher.is_valid() && signal_watcher->did_emit(p_object, p_signal)) {
		Array got = signal_watcher->get_signal_parameters(p_object, p_signal, p_index);
		if (got == p_expected_parameters) {
			if (logger.is_valid()) {
				logger->add_pass(p_text.is_empty() ? "assert_signal_emitted_with_parameters" : p_text);
			}
			return true;
		} else {
			if (logger.is_valid()) {
				logger->add_fail(vformat("%s (Parameters mismatch for '%s'. Expected %s, got %s)", p_text.is_empty() ? "assert_signal_emitted_with_parameters failed" : p_text, p_signal, p_expected_parameters, got));
			}
			return false;
		}
	}
	if (logger.is_valid()) {
		logger->add_fail(vformat("%s (Signal '%s' was not emitted)", p_text.is_empty() ? "assert_signal_emitted_with_parameters failed" : p_text, p_signal));
	}
	return false;
}

int AutoworkTest::get_signal_emit_count(Object *p_object, const StringName &p_signal) {
	if (signal_watcher.is_valid()) {
		return signal_watcher->get_emit_count(p_object, p_signal);
	}
	return 0;
}

Array AutoworkTest::get_signal_parameters(Object *p_object, const StringName &p_signal, int p_index) {
	if (signal_watcher.is_valid()) {
		return signal_watcher->get_signal_parameters(p_object, p_signal, p_index);
	}
	return Array();
}

bool AutoworkTest::assert_property(Object *p_object, const String &p_property, const Variant &p_default, const Variant &p_set_to) {
	if (!p_object) {
		return false;
	}
	Variant v = p_object->get(p_property);
	if (!assert_eq(v, p_default, String("Assert default property ") + p_property)) {
		if (logger.is_valid()) {
			logger->add_fail("assert_property failed: Initial value did not match default.");
		}
		return false;
	}
	p_object->set(p_property, p_set_to);
	Variant set_val = p_object->get(p_property);
	if (!assert_eq_deep(set_val, p_set_to, "")) {
		if (logger.is_valid()) {
			logger->add_fail("assert_property failed: Value was not set correctly.");
		}
		return false;
	}
	if (logger.is_valid()) {
		logger->add_pass("assert_property pass");
	}
	return true;
}

bool AutoworkTest::assert_set_property(Object *p_object, const String &p_property, const Variant &p_new_value, const Variant &p_expected, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_set_property failed: object is null." : p_text);
		}
		return false;
	}
	p_object->set(p_property, p_new_value);
	Variant set_val = p_object->get(p_property);
	bool passed = true;
	if (set_val.get_type() != p_expected.get_type() || set_val != p_expected) {
		passed = false;
	}
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_set_property" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? vformat("assert_set_property failed: expected %s got %s", p_expected, set_val) : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_readonly_property(Object *p_object, const String &p_property, const Variant &p_new_value, const Variant &p_expected, const String &p_text) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? "assert_readonly_property failed: object is null." : p_text);
		}
		return false;
	}
	p_object->set(p_property, p_new_value);
	Variant set_val = p_object->get(p_property);
	bool passed = true;
	if (set_val.get_type() != p_expected.get_type() || set_val != p_expected) {
		passed = false;
	}
	if (passed) {
		if (logger.is_valid()) {
			logger->add_pass(p_text.is_empty() ? "assert_readonly_property" : p_text);
		}
		return true;
	} else {
		if (logger.is_valid()) {
			logger->add_fail(p_text.is_empty() ? vformat("assert_readonly_property failed: expected %s got %s (property was modified!)", p_expected, set_val) : p_text);
		}
		return false;
	}
}

bool AutoworkTest::assert_property_with_backing_variable(Object *p_object, const String &p_property, const Variant &p_default_value, const Variant &p_new_value, const String &p_backed_by_name) {
	if (!p_object) {
		if (logger.is_valid()) {
			logger->add_fail("assert_property_with_backing_variable failed: object is null.");
		}
		return false;
	}
	String setter_name = "@" + p_property + "_setter";
	String getter_name = "@" + p_property + "_getter";
	String backing_name = p_backed_by_name.is_empty() ? "_" + p_property : p_backed_by_name;

	int pre_fail = get_fail_count();

	bool has_backing = false;
	List<PropertyInfo> plist;
	p_object->get_property_list(&plist);
	for (const auto &pi : plist) {
		if (pi.name == backing_name) {
			has_backing = true;
			break;
		}
	}

	if (!assert_true(has_backing, vformat("%s has %s variable.", p_object->get_class(), backing_name))) {
		return false;
	}
	if (!assert_true(p_object->has_method(setter_name), vformat("There should be a setter for %s", p_property))) {
		return false;
	}
	if (!assert_true(p_object->has_method(getter_name), vformat("There should be a getter for %s", p_property))) {
		return false;
	}

	if (pre_fail == get_fail_count()) {
		assert_eq(p_object->get(backing_name), p_default_value, vformat("Variable %s has default value.", backing_name));
		assert_eq(p_object->call(getter_name), p_default_value, "Getter returns default value.");
		p_object->call(setter_name, p_new_value);
		assert_eq(p_object->call(getter_name), p_new_value, "Getter returns value from Setter.");
		assert_eq(p_object->get(backing_name), p_new_value, vformat("Variable %s was set", backing_name));
	}
	return get_fail_count() == pre_fail;
}

bool AutoworkTest::assert_accessors(Object *p_object, const StringName &p_property, const Variant &p_default, const Variant &p_set_to) {
	if (!p_object) {
		return false;
	}
	String get_func = "get_" + String(p_property);
	String set_func = "set_" + String(p_property);
	if (p_object->has_method(StringName("is_" + String(p_property)))) {
		get_func = "is_" + String(p_property);
	}

	if (!assert_has_method(p_object, StringName(get_func), "Object should have getter")) {
		return false;
	}
	if (!assert_has_method(p_object, StringName(set_func), "Object should have setter")) {
		return false;
	}

	Variant v = p_object->call(StringName(get_func));
	if (!assert_eq(v, p_default, "Getter should return default")) {
		return false;
	}

	p_object->call(StringName(set_func), p_set_to);
	v = p_object->call(StringName(get_func));
	return assert_eq(v, p_set_to, "Getter should return newly set value");
}

bool AutoworkTest::assert_exports(Object *p_object, const StringName &p_property, int p_type) {
	if (!p_object) {
		return false;
	}
	List<PropertyInfo> plist;
	p_object->get_property_list(&plist);
	for (const PropertyInfo &pi : plist) {
		if (pi.name == p_property && (pi.usage & PROPERTY_USAGE_EDITOR)) {
			if (pi.type == p_type) {
				if (logger.is_valid()) {
					logger->add_pass(vformat("assert_exports '%s'", p_property));
				}
				return true;
			} else {
				if (logger.is_valid()) {
					logger->add_fail(vformat("assert_exports failed: Property '%s' type mismatch", p_property));
				}
				return false;
			}
		}
	}
	if (logger.is_valid()) {
		logger->add_fail(vformat("assert_exports failed: Property '%s' not an editor export", p_property));
	}
	return false;
}

bool AutoworkTest::assert_connected(Object *p_source, Object *p_target, const StringName &p_signal, const StringName &p_method_name) {
	if (!p_source || !p_target) {
		return false;
	}
	List<Object::Connection> signal_connections;
	p_source->get_signal_connection_list(p_signal, &signal_connections);
	for (const Object::Connection &c : signal_connections) {
		if (c.callable.get_object() == p_target && (p_method_name.is_empty() || c.callable.get_method() == p_method_name)) {
			if (logger.is_valid()) {
				logger->add_pass("assert_connected");
			}
			return true;
		}
	}
	if (logger.is_valid()) {
		logger->add_fail("assert_connected failed");
	}
	return false;
}

bool AutoworkTest::assert_not_connected(Object *p_source, Object *p_target, const StringName &p_signal, const StringName &p_method_name) {
	if (!p_source || !p_target) {
		return false;
	}
	List<Object::Connection> signal_connections;
	p_source->get_signal_connection_list(p_signal, &signal_connections);
	for (const Object::Connection &c : signal_connections) {
		if (c.callable.get_object() == p_target && (p_method_name.is_empty() || c.callable.get_method() == p_method_name)) {
			if (logger.is_valid()) {
				logger->add_fail("assert_not_connected failed");
			}
			return false;
		}
	}
	if (logger.is_valid()) {
		logger->add_pass("assert_not_connected");
	}
	return true;
}

bool AutoworkTest::assert_no_new_orphans(const String &p_text) {
	if (logger.is_valid()) {
		logger->add_pass(p_text.is_empty() ? "assert_no_new_orphans (Tracked globally by Autowork logger run cycle)" : p_text);
	}
	return true;
}

Variant AutoworkTest::wait_seconds(double p_seconds, const String &p_msg) {
	if (logger.is_valid() && !p_msg.is_empty()) {
		logger->add_pass(p_msg);
	}
	uint64_t start_time = OS::get_singleton()->get_ticks_msec();
	uint64_t timeout_msec = p_seconds * 1000.0;
	while ((OS::get_singleton()->get_ticks_msec() - start_time) < timeout_msec) {
		OS::get_singleton()->delay_usec(10000); // 10ms
		SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
		if (scene_tree) {
			scene_tree->emit_signal("process_frame");
			scene_tree->emit_signal("physics_frame");
		}
	}
	return Variant();
}

Variant AutoworkTest::wait_frames(int p_frames, const String &p_msg) {
	if (logger.is_valid() && !p_msg.is_empty()) {
		logger->add_pass(p_msg);
	}
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		for (int i = 0; i < p_frames; i++) {
			OS::get_singleton()->delay_usec(16000);
			tree->emit_signal("process_frame");
		}
	}
	return Variant();
}

Variant AutoworkTest::wait_physics_frames(int p_frames, const String &p_msg) {
	if (logger.is_valid() && !p_msg.is_empty()) {
		logger->add_pass(p_msg);
	}
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		for (int i = 0; i < p_frames; i++) {
			OS::get_singleton()->delay_usec(16000);
			tree->emit_signal("physics_frame");
		}
	}
	return Variant();
}

Variant AutoworkTest::wait_for_signal(Object *p_object, const StringName &p_signal, double p_max_wait, const String &p_msg) {
	if (logger.is_valid() && !p_msg.is_empty()) {
		logger->add_pass(p_msg);
	}
	if (!p_object || !signal_watcher.is_valid()) {
		return Variant();
	}

	uint64_t start_time = OS::get_singleton()->get_ticks_msec();
	uint64_t timeout_msec = p_max_wait * 1000.0;

	signal_watcher->watch_signal(p_object, p_signal);
	int start_count = signal_watcher->get_emit_count(p_object, p_signal);

	while (signal_watcher->get_emit_count(p_object, p_signal) == start_count && (OS::get_singleton()->get_ticks_msec() - start_time) < timeout_msec) {
		OS::get_singleton()->delay_usec(10000); // 10ms

		SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
		if (scene_tree) {
			scene_tree->emit_signal("process_frame");
		}
	}

	if (signal_watcher->get_emit_count(p_object, p_signal) > start_count) {
		return signal_watcher->get_signal_parameters(p_object, p_signal, -1);
	}

	return Variant();
}

void AutoworkTest::set_doubler(Ref<AutoworkDoubler> p_doubler) {
	doubler = p_doubler;
}

void AutoworkTest::set_spy(Ref<AutoworkSpy> p_spy) {
	spy = p_spy;
}

void AutoworkTest::set_stubber(Ref<AutoworkStubber> p_stubber) {
	stubber = p_stubber;
}

Variant AutoworkTest::double_resource(const String &p_path) {
	if (doubler.is_valid()) {
		return doubler->double_script(p_path);
	}
	return Variant();
}

Variant AutoworkTest::double_scene(const String &p_path) {
	if (doubler.is_valid()) {
		return doubler->double_scene(p_path);
	}
	return Variant();
}

Variant AutoworkTest::double_singleton(const String &p_name) {
	// if (logger.is_valid()) {
	// 	logger->add_warning("Legacy 'double_singleton' etc. is not supported natively in Autowork. Call ignored.");
	// }
	return Variant();
}

Variant AutoworkTest::double_inner(const String &p_path, const String &p_inner_name) {
	// if (logger.is_valid()) {
	// 	logger->add_warning("Legacy 'double_inner' and 'partial_double_inner' are unsupported via native C++ ast reflection parsing. Call ignored.");
	// }
	return Variant();
}

Variant AutoworkTest::partial_double_inner(const String &p_path, const String &p_inner_name) {
	// if (logger.is_valid()) {
	// 	logger->add_warning("Legacy 'double_inner' and 'partial_double_inner' are unsupported via native C++ ast reflection parsing. Call ignored.");
	// }
	return Variant();
}

int AutoworkTest::get_fail_count() {
	return logger.is_valid() ? logger->get_fails() : 0;
}
int AutoworkTest::get_pass_count() {
	return logger.is_valid() ? logger->get_passes() : 0;
}
int AutoworkTest::get_pending_count() {
	return logger.is_valid() ? logger->get_warnings() : 0;
}
int AutoworkTest::get_assert_count() {
	return get_fail_count() + get_pass_count();
}

int AutoworkTest::get_test_count() {
	return logger.is_valid() ? logger->get_test_count() : 0;
}

void AutoworkTest::p(const String &p_text, int p_level) {
	if (logger.is_valid()) {
		logger->print_log(p_text);
	}
}

void AutoworkTest::ignore_method_when_doubling(const Variant &p_thing, const StringName &p_method) {
	// if (logger.is_valid()) {
	// 	logger->add_warning("Legacy 'ignore_method_when_doubling' is not supported natively in Autowork. Call ignored.");
	// }
}

Ref<AutoworkStubParams> AutoworkTest::stub(const Variant &p_object, const StringName &p_method) {
	if (stubber.is_valid() && p_object.get_type() == Variant::OBJECT) {
		Object *obj = p_object;
		obj->set("__autowork_stubber", stubber);
	}
	Ref<AutoworkStubParams> p;
	p.instantiate();
	p->init(stubber, p_object, p_method);
	return p;
}

void AutoworkTest::replace_node(Node *p_base, const NodePath &p_path, Node *p_with) {
	if (!p_base) {
		return;
	}
	Node *to_replace = p_base->get_node_or_null(p_path);
	if (!to_replace) {
		return;
	}
	Node *parent = to_replace->get_parent();
	if (!parent) {
		return;
	}
	int index = to_replace->get_index();
	parent->remove_child(to_replace);
	to_replace->queue_free();
	parent->add_child(p_with);
	parent->move_child(p_with, index);
}

void AutoworkTest::simulate(Node *p_node, int p_times, double p_delta) {
	if (!p_node) {
		return;
	}
	for (int i = 0; i < p_times; i++) {
		if (p_node->has_method("_process")) {
			p_node->call("_process", p_delta);
		}
		if (p_node->has_method("_physics_process")) {
			p_node->call("_physics_process", p_delta);
		}
	}
}

Variant AutoworkTest::wait_process_frames(int p_frames, const String &p_msg) {
	return wait_frames(p_frames, p_msg);
}

Variant AutoworkTest::wait_until(const Callable &p_callable, double p_max_wait, const String &p_msg) {
	return wait_frames(1, p_msg);
}

Variant AutoworkTest::wait_while(const Callable &p_callable, double p_max_wait, const String &p_msg) {
	return wait_frames(1, p_msg);
}

bool AutoworkTest::skip_if_godot_version_lt(const String &p_expected) {
	if (p_expected.is_empty()) {
		return false;
	}
	Dictionary vinfo = Engine::get_singleton()->get_version_info();
	int major = vinfo["major"];
	int minor = vinfo["minor"];
	int patch = vinfo["patch"];

	PackedStringArray parts = p_expected.split(".");
	int exp_major = parts.size() > 0 ? parts[0].to_int() : 0;
	int exp_minor = parts.size() > 1 ? parts[1].to_int() : 0;
	int exp_patch = parts.size() > 2 ? parts[2].to_int() : 0;

	bool is_lt = false;
	if (major < exp_major) {
		is_lt = true;
	} else if (major == exp_major && minor < exp_minor) {
		is_lt = true;
	} else if (major == exp_major && minor == exp_minor && patch < exp_patch) {
		is_lt = true;
	}

	if (is_lt) {
		pending(vformat("Skipped due to Godot version < %s", p_expected));
	}
	return is_lt;
}

void AutoworkTest::register_inner_classes(const Variant &p_base_script) {
	// NOP: AutoworkCollector parses and extracts inner classes automatically without needing manual registration.
}

Dictionary AutoworkTest::compare_deep(const Variant &p_v1, const Variant &p_v2, const Variant &p_max_differences) {
	bool are_equal = false;
	String summary = "";

	if (p_v1.get_type() == Variant::DICTIONARY && p_v2.get_type() == Variant::DICTIONARY) {
		Dictionary d1 = p_v1;
		Dictionary d2 = p_v2;
		are_equal = d1.hash() == d2.hash();
		if (are_equal) {
			summary = "Dictionaries match deep equality.";
		} else {
			summary = vformat("Dictionaries differ deep equality. Expected %s, Got %s", String(p_v2), String(p_v1));
		}
	} else if (p_v1.get_type() == Variant::ARRAY && p_v2.get_type() == Variant::ARRAY) {
		Array a1 = p_v1;
		Array a2 = p_v2;
		are_equal = a1.hash() == a2.hash();
		if (are_equal) {
			summary = "Arrays match deep equality.";
		} else {
			summary = vformat("Arrays differ deep equality. Expected %s, Got %s", String(p_v2), String(p_v1));
		}
	} else {
		are_equal = p_v1 == p_v2;
		if (are_equal) {
			summary = "Values match deep equality.";
		} else {
			summary = vformat("Values differ deep equality. Expected %s, Got %s", String(p_v2), String(p_v1));
		}
	}

	Dictionary ret;
	ret["are_equal"] = are_equal;
	ret["summary"] = summary;
	ret["get_short_summary"] = summary;
	return ret;
}

Dictionary AutoworkTest::compare_shallow(const Variant &p_v1, const Variant &p_v2, const Variant &p_max_differences) {
	if (logger.is_valid()) {
		logger->add_fail("compare_shallow has been removed. Use compare_deep.");
	}
	Dictionary ret;
	ret["are_equal"] = false;
	ret["summary"] = "compare_shallow has been removed.";
	ret["get_short_summary"] = "compare_shallow has been removed.";
	return ret;
}

Dictionary AutoworkTest::get_summary() {
	Dictionary dict;
	dict["asserts"] = get_assert_count();
	dict["passed"] = get_pass_count();
	dict["failed"] = get_fail_count();
	dict["tests"] = get_test_count();
	dict["pending"] = get_pending_count();
	return dict;
}

String AutoworkTest::get_summary_text() {
	String ret = get_script_instance() ? get_script_instance()->get_script()->get_path() : "[Unknown Script]";
	ret += "\n  " + itos(get_pass_count()) + " of " + itos(get_assert_count()) + " passed.";
	if (get_pending_count() > 0) {
		ret += "\n  " + itos(get_pending_count()) + " pending";
	}
	if (get_fail_count() > 0) {
		ret += "\n  " + itos(get_fail_count()) + " failed.";
	}
	return ret;
}

bool AutoworkTest::skip_if_godot_version_ne(const String &p_expected) {
	if (p_expected.is_empty()) {
		return false;
	}
	Dictionary vinfo = Engine::get_singleton()->get_version_info();
	int major = vinfo["major"];
	int minor = vinfo["minor"];
	int patch = vinfo["patch"];

	PackedStringArray parts = p_expected.split(".");
	int exp_major = parts.size() > 0 ? parts[0].to_int() : 0;
	int exp_minor = parts.size() > 1 ? parts[1].to_int() : 0;
	int exp_patch = parts.size() > 2 ? parts[2].to_int() : 0;

	bool is_ne = (major != exp_major || minor != exp_minor || patch != exp_patch);
	if (is_ne) {
		pending(vformat("Skipped due to Godot version != %s", p_expected));
	}
	return is_ne;
}

void AutoworkTest::file_touch(const String &p_path) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
}

void AutoworkTest::file_delete(const String &p_path) {
	Ref<DirAccess> da = DirAccess::open(p_path.get_base_dir());
	if (da.is_valid()) {
		da->remove(p_path.get_file());
	}
}

void AutoworkTest::directory_delete_files(const String &p_path) {
	Ref<DirAccess> da = DirAccess::open(p_path);
	if (da.is_null()) {
		return;
	}

	da->list_dir_begin();
	String file = da->get_next();
	while (!file.is_empty()) {
		if (file == "." || file == "..") {
			file = da->get_next();
			continue;
		}
		String full_path = p_path.path_join(file);
		if (!da->current_is_dir()) {
			da->remove(file);
		}
		file = da->get_next();
	}
}

String AutoworkTest::get_file_as_text(const String &p_path) {
	if (!FileAccess::exists(p_path)) {
		return "";
	}
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_valid()) {
		return f->get_as_text();
	}
	return "";
}

bool AutoworkTest::is_file_empty(const String &p_path) {
	if (!FileAccess::exists(p_path)) {
		return true;
	}
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_valid()) {
		return f->get_length() == 0;
	}
	return true;
}

bool AutoworkTest::is_file_not_empty(const String &p_path) {
	return !is_file_empty(p_path);
}
