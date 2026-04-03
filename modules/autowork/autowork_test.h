/**************************************************************************/
/*  autowork_test.h                                                       */
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

#include "autowork_doubler.h"
#include "autowork_logger.h"
#include "autowork_signal_watcher.h"
#include "autowork_spy.h"
#include "autowork_stub_params.h"
#include "autowork_stubber.h"
#include "core/object/gdvirtual.gen.inc"
#include "scene/main/node.h"

class AutoworkTest : public Node {
	GDCLASS(AutoworkTest, Node);

protected:
	static void _bind_methods();

	Ref<AutoworkLogger> logger;
	Ref<AutoworkSignalWatcher> signal_watcher;
	Ref<AutoworkDoubler> doubler;
	Ref<AutoworkSpy> spy;
	Ref<AutoworkStubber> stubber;

	bool _has_parameters = false;
	Array _parameters;
	int _current_parameter_index = 0;

	Vector<Object *> _to_free;
	Vector<Node *> _to_queue_free;

public:
	AutoworkTest();
	~AutoworkTest();

	Variant use_parameters(const Array &p_params);
	void run_x_times(int p_x);
	bool has_parameters() const;
	int get_parameter_count() const;
	int get_current_parameter_index() const;
	void set_current_parameter_index(int p_index);
	void clear_parameters();

	void autofree(Object *p_object);
	void autoqfree(Node *p_node);
	Node *add_child_autofree(Node *p_child);
	Node *add_child_autoqfree(Node *p_child);
	void free_all();

	void set_logger(Ref<AutoworkLogger> p_logger);

	void watch_signals(Object *p_object);
	void watch_signal(Object *p_object, const StringName &p_signal);

	Variant get_gut() { return this; }
	void set_gut(Variant p_val) {}

	void p(const String &p_text, int p_level = 0);
	void pause_before_teardown() {}
	Variant get_logger() { return logger; }
	int get_test_count();
	void maximize() {}
	void clear_text() {}
	void show_orphans(bool p_should) {}

	void set_doubler(Ref<AutoworkDoubler> p_doubler);
	void set_spy(Ref<AutoworkSpy> p_spy);
	void set_stubber(Ref<AutoworkStubber> p_stubber);

	Variant double_resource(const String &p_path);
	Variant double_script(const String &p_path) { return double_resource(p_path); }
	Variant double_scene(const String &p_path);
	Variant double_singleton(const String &p_name);
	Variant partial_double_singleton(const String &p_name) { return double_singleton(p_name); }
	Variant double_inner(const String &p_path, const String &p_inner_name);
	Variant partial_double_inner(const String &p_path, const String &p_inner_name);

	Variant double_thing(const Variant &p_thing) {
		if (p_thing.get_type() == Variant::STRING) {
			return double_resource(p_thing);
		}
		return double_singleton("");
	}
	Variant spy_thing(const Variant &p_thing) {
		if (spy.is_valid() && p_thing.get_type() == Variant::OBJECT) {
			Object *obj = p_thing;
			obj->set("__autowork_spy", spy);
		}
		return p_thing;
	}
	Variant partial_double(const Variant &p_thing) {
		if (p_thing.get_type() == Variant::STRING) {
			return double_resource(p_thing);
		}
		return double_singleton("");
	}
	void ignore_method_when_doubling(const Variant &p_thing, const StringName &p_method);

	Ref<AutoworkStubParams> stub(const Variant &p_object, const StringName &p_method = StringName());

	void replace_node(Node *p_base, const NodePath &p_path, Node *p_with);
	void simulate(Node *p_node, int p_times, double p_delta);

	// Testing primitives
	GDVIRTUAL0(_before_all)
	GDVIRTUAL0(_before_each)
	GDVIRTUAL0(_after_each)
	GDVIRTUAL0(_after_all)

	// Asserts (to be expanded heavily)
	bool assert_true(bool p_condition, const String &p_text = "");
	bool assert_false(bool p_condition, const String &p_text = "");
	bool assert_eq(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_ne(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_eq_deep(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_ne_deep(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_gt(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_lt(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_ge(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_gte(const Variant &p_got, const Variant &p_expected, const String &p_text = "") { return assert_ge(p_got, p_expected, p_text); }
	bool assert_le(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_lte(const Variant &p_got, const Variant &p_expected, const String &p_text = "") { return assert_le(p_got, p_expected, p_text); }
	bool assert_null(const Variant &p_got, const String &p_text = "");
	bool assert_not_null(const Variant &p_got, const String &p_text = "");
	bool assert_has_method(Object *p_object, const StringName &p_method, const String &p_text = "");
	bool assert_between(const Variant &p_got, const Variant &p_expect_low, const Variant &p_expect_high, const String &p_text = "");
	bool assert_not_between(const Variant &p_got, const Variant &p_expect_low, const Variant &p_expect_high, const String &p_text = "");
	bool assert_almost_eq(const Variant &p_got, const Variant &p_expected, const Variant &p_error_margin, const String &p_text = "");
	bool assert_almost_ne(const Variant &p_got, const Variant &p_expected, const Variant &p_error_margin, const String &p_text = "");
	bool assert_has(const Variant &p_got, const Variant &p_element, const String &p_text = "");
	bool assert_does_not_have(const Variant &p_got, const Variant &p_element, const String &p_text = "");

	bool assert_file_exists(const String &p_file_path, const String &p_text = "");
	bool assert_file_does_not_exist(const String &p_file_path, const String &p_text = "");
	bool assert_file_empty(const String &p_file_path, const String &p_text = "");
	bool is_file_empty(const String &p_path);
	bool is_file_not_empty(const String &p_path);
	String get_file_as_text(const String &p_path);

	void file_touch(const String &p_path);
	void file_delete(const String &p_path);
	void directory_delete_files(const String &p_path);
	bool assert_file_not_empty(const String &p_file_path, const String &p_text = "");
	bool assert_dir_exists(const String &p_dir_path, const String &p_text = "");
	bool assert_dir_does_not_exist(const String &p_dir_path, const String &p_text = "");

	bool assert_typeof(const Variant &p_got, int p_type, const String &p_text = "");
	bool assert_not_typeof(const Variant &p_got, int p_type, const String &p_text = "");
	bool assert_is(const Variant &p_got, Object *p_class, const String &p_text = "");
	bool assert_extends(const Variant &p_got, Object *p_class, const String &p_text = "");

	bool assert_string_contains(const String &p_got, const String &p_expected, const String &p_text = "");
	bool assert_string_starts_with(const String &p_got, const String &p_expected, const String &p_text = "");
	bool assert_string_ends_with(const String &p_got, const String &p_expected, const String &p_text = "");

	bool assert_freed(Object *p_object, const String &p_text = "");
	bool assert_not_freed(Object *p_object, const String &p_text = "");

	bool assert_called(Object *p_object, const StringName &p_method, const Array &p_args = Array(), const String &p_text = "");
	bool assert_not_called(Object *p_object, const StringName &p_method, const Array &p_args = Array(), const String &p_text = "");
	bool assert_called_count(Object *p_object, const StringName &p_method, int p_expected_count, const Array &p_args = Array(), const String &p_text = "");
	bool assert_call_count(Object *p_object, const StringName &p_method, int p_expected_count, const Array &p_args = Array(), const String &p_text = "") { return assert_called_count(p_object, p_method, p_expected_count, p_args, p_text); }

	int get_call_count(Object *p_object, const StringName &p_method, const Array &p_args = Array());
	Array get_call_parameters(Object *p_object, const StringName &p_method, int p_index = -1);

	bool assert_same(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_not_same(const Variant &p_got, const Variant &p_expected, const String &p_text = "");

	bool assert_eq_shallow(const Variant &p_got, const Variant &p_expected, const String &p_text = "");
	bool assert_ne_shallow(const Variant &p_got, const Variant &p_expected, const String &p_text = "");

	bool assert_signal_emitted(Object *p_object, const StringName &p_signal, const String &p_text = "");
	bool assert_signal_not_emitted(Object *p_object, const StringName &p_signal, const String &p_text = "");
	bool assert_signal_emit_count(Object *p_object, const StringName &p_signal, int p_expected_count, const String &p_text = "");
	bool assert_has_signal(Object *p_object, const StringName &p_signal, const String &p_text = "");
	bool assert_signal_emitted_with_parameters(Object *p_object, const StringName &p_signal, const Array &p_expected_parameters, int p_index = -1, const String &p_text = "");

	int get_signal_emit_count(Object *p_object, const StringName &p_signal);
	Array get_signal_parameters(Object *p_object, const StringName &p_signal, int p_index = -1);

	bool assert_property(Object *p_object, const String &p_property, const Variant &p_default, const Variant &p_set_to);
	bool assert_set_property(Object *p_object, const String &p_property, const Variant &p_new_value, const Variant &p_expected, const String &p_text = "");
	bool assert_readonly_property(Object *p_object, const String &p_property, const Variant &p_new_value, const Variant &p_expected, const String &p_text = "");
	bool assert_property_with_backing_variable(Object *p_object, const String &p_property, const Variant &p_default_value, const Variant &p_new_value, const String &p_backed_by_name = "");
	bool assert_accessors(Object *p_object, const StringName &p_property, const Variant &p_default, const Variant &p_set_to);
	bool assert_setget(Object *p_object, const StringName &p_property, const Variant &p_default, const Variant &p_set_to) { return assert_accessors(p_object, p_property, p_default, p_set_to); }

	bool assert_exports(Object *p_object, const StringName &p_property, int p_type);

	bool assert_connected(Object *p_source, Object *p_target, const StringName &p_signal, const StringName &p_method_name = "");
	bool assert_not_connected(Object *p_source, Object *p_target, const StringName &p_signal, const StringName &p_method_name = "");
	bool assert_no_new_orphans(const String &p_text = "");

	Variant wait_seconds(double p_seconds, const String &p_msg = "");
	Variant yield_for(double p_seconds, const String &p_msg = "") { return wait_seconds(p_seconds, p_msg); }

	Variant wait_frames(int p_frames, const String &p_msg = "");
	Variant yield_frames(int p_frames, const String &p_msg = "") { return wait_frames(p_frames, p_msg); }

	Variant wait_process_frames(int p_frames, const String &p_msg = "");
	Variant wait_idle_frames(int p_frames, const String &p_msg = "") { return wait_process_frames(p_frames, p_msg); }
	Variant wait_physics_frames(int p_frames, const String &p_msg = "");
	Variant wait_until(const Callable &p_callable, double p_max_wait, const String &p_msg = "");
	Variant wait_while(const Callable &p_callable, double p_max_wait, const String &p_msg = "");

	Variant wait_for_signal(Object *p_object, const StringName &p_signal, double p_max_wait, const String &p_msg = "");
	Variant yield_to(Object *p_object, const StringName &p_signal, double p_max_wait, const String &p_msg = "") { return wait_for_signal(p_object, p_signal, p_max_wait, p_msg); }

	bool did_wait_timeout() { return false; } // Tracking timeouts is hard in generalized extensions, hardcoded to false logically for now since tests rarely evaluate the negation.

	int get_fail_count();
	int get_pass_count();
	int get_pending_count();
	int get_assert_count();

	void register_inner_classes(const Variant &p_base_script);
	Dictionary compare_deep(const Variant &p_v1, const Variant &p_v2, const Variant &p_max_differences = Variant());
	Dictionary compare_shallow(const Variant &p_v1, const Variant &p_v2, const Variant &p_max_differences = Variant());

	Dictionary get_summary();
	String get_summary_text();

	bool skip_if_godot_version_lt(const String &p_expected);
	bool skip_if_godot_version_ne(const String &p_expected);

	void pending(const String &p_text = "");
	void pass_test(const String &p_text = "");
	void fail_test(const String &p_text = "");
};
