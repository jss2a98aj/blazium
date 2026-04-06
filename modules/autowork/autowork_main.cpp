/**************************************************************************/
/*  autowork_main.cpp                                                     */
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

#include "autowork_main.h"
#include "autowork_doubler.h"
#include "autowork_hook_script.h"
#include "autowork_json_exporter.h"
#include "autowork_junit_exporter.h"
#include "autowork_logger.h"
#include "autowork_signal_watcher.h"
#include "autowork_spy.h"
#include "autowork_stubber.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/object/object.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void Autowork::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_directory", "path", "prefix", "suffix"), &Autowork::add_directory, DEFVAL(""), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_script", "path"), &Autowork::add_script);
	ClassDB::bind_method(D_METHOD("set_select", "test_name"), &Autowork::set_select);
	ClassDB::bind_method(D_METHOD("run_tests", "run_rest"), &Autowork::run_tests, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("maximize"), &Autowork::maximize);
	ClassDB::bind_method(D_METHOD("clear_text"), &Autowork::clear_text);
	ClassDB::bind_method(D_METHOD("pause_before_teardown"), &Autowork::pause_before_teardown);
	ClassDB::bind_method(D_METHOD("show_orphans", "should"), &Autowork::show_orphans);
	ClassDB::bind_method(D_METHOD("p", "text", "level"), &Autowork::p, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("get_test_count"), &Autowork::get_test_count);
	ClassDB::bind_method(D_METHOD("get_assert_count"), &Autowork::get_assert_count);
	ClassDB::bind_method(D_METHOD("get_pass_count"), &Autowork::get_pass_count);
	ClassDB::bind_method(D_METHOD("get_fail_count"), &Autowork::get_fail_count);
	ClassDB::bind_method(D_METHOD("get_pending_count"), &Autowork::get_pending_count);
	ClassDB::bind_method(D_METHOD("get_test_script_count"), &Autowork::get_test_script_count);

	ClassDB::bind_method(D_METHOD("get_test_collector"), &Autowork::get_test_collector);
	ClassDB::bind_method(D_METHOD("get_logger"), &Autowork::get_logger);
	ClassDB::bind_method(D_METHOD("get_doubler"), &Autowork::get_doubler);
	ClassDB::bind_method(D_METHOD("get_spy"), &Autowork::get_spy);
	ClassDB::bind_method(D_METHOD("get_stubber"), &Autowork::get_stubber);
}

Autowork::Autowork() {
	collector.instantiate();
	logger.instantiate();
	doubler.instantiate();
	spy.instantiate();
	stubber.instantiate();
	doubler->set_spy(spy);
	doubler->set_stubber(stubber);
}

Autowork::~Autowork() {
}

void Autowork::add_directory(const String &p_path, const String &p_prefix, const String &p_suffix) {
	if (collector.is_valid()) {
		if (!p_prefix.is_empty()) {
			collector->script_prefix = p_prefix;
		}
		if (!p_suffix.is_empty()) {
			collector->script_suffix = p_suffix;
		}
		collector->process_directory(p_path);
	}
}

void Autowork::add_script(const String &p_path) {
	if (collector.is_valid()) {
		collector->add_script(p_path);
	}
}

void Autowork::set_select(const String &p_test_name) {
	if (collector.is_valid()) {
		collector->test_pattern = p_test_name;
	}
}

void Autowork::run_tests(bool p_run_rest) {
	if (collector.is_null()) {
		return;
	}

	if (OS::get_singleton()) {
		const List<String> &cmdline_args = OS::get_singleton()->get_cmdline_args();
		bool dir_overriden = false;
		String junit_path;
		String json_path;
		String pre_run_path;
		String post_run_path;

		// Load legacy .gutconfig.json or new .autoworkconfig.json if they exist
		String config_file = "";
		if (FileAccess::exists("res://.autoworkconfig.json")) {
			config_file = "res://.autoworkconfig.json";
		} else if (FileAccess::exists("res://.gutconfig.json")) {
			config_file = "res://.gutconfig.json";
		}

		if (!config_file.is_empty()) {
			Ref<FileAccess> f = FileAccess::open(config_file, FileAccess::READ);
			if (f.is_valid()) {
				String json_text = f->get_as_text();
				f->close();

				Ref<JSON> json;
				json.instantiate();
				Error err = json->parse(json_text);
				if (err == OK && json->get_data().get_type() == Variant::DICTIONARY) {
					Dictionary dict = json->get_data();
					if (dict.has("include_subdirs")) {
						collector->include_subdirectories = dict["include_subdirs"];
					}
					if (dict.has("prefix")) {
						collector->script_prefix = dict["prefix"];
					}
					if (dict.has("suffix")) {
						collector->script_suffix = dict["suffix"];
					}
					if (dict.has("inner_class")) {
						collector->inner_class_pattern = dict["inner_class"];
					}
					if (dict.has("unit_test_name")) {
						collector->test_pattern = dict["unit_test_name"];
					}
					if (dict.has("pre_run_script")) {
						pre_run_path = dict["pre_run_script"];
					}
					if (dict.has("post_run_script")) {
						post_run_path = dict["post_run_script"];
					}
					if (dict.has("junit_xml_file")) {
						junit_path = dict["junit_xml_file"];
					}
					if (dict.has("hide_orphans")) {
						logger->set_hide_orphans(dict["hide_orphans"]);
					}

					if (dict.has("dirs") && dict["dirs"].get_type() == Variant::ARRAY) {
						Array dirs = dict["dirs"];
						for (int i = 0; i < dirs.size(); i++) {
							collector->process_directory(dirs[i]);
							dir_overriden = true;
						}
					}
					if (dict.has("tests") && dict["tests"].get_type() == Variant::ARRAY) {
						Array tests = dict["tests"];
						for (int i = 0; i < tests.size(); i++) {
							collector->add_script(tests[i]);
						}
					}
				}
			}
		}

		for (const String &arg : cmdline_args) {
			String clean_arg = arg.trim_prefix("--"); // Handle both -gdir and gdir depending on OS
			if (clean_arg.begins_with("-g")) {
				clean_arg = clean_arg.substr(2); // Strip -g from legacy
			}

			if (arg.begins_with("--aw-dir=") || arg.begins_with("-gdir=")) {
				if (!dir_overriden) {
					collector->clear();
					dir_overriden = true;
				}
				collector->process_directory(arg.get_slice("=", 1));
			} else if (arg.begins_with("--aw-test=") || arg.begins_with("-gunit_test_name=")) {
				collector->test_pattern = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-select=") || arg.begins_with("-gselect=")) {
				collector->script_pattern = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-file=") || arg.begins_with("-gtest=")) {
				collector->add_script(arg.get_slice("=", 1));
			} else if (arg.begins_with("--aw-prefix=") || arg.begins_with("-gprefix=")) {
				collector->script_prefix = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-suffix=") || arg.begins_with("-gsuffix=")) {
				collector->script_suffix = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-inner-class=") || arg.begins_with("-ginner_class=")) {
				collector->inner_class_pattern = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-junit-xml=") || arg.begins_with("-gjunit_xml_file=")) {
				junit_path = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-junit=")) {
				junit_path = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-json=")) {
				json_path = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-pre-run=") || arg.begins_with("-gpre_run_script=")) {
				pre_run_path = arg.get_slice("=", 1);
			} else if (arg.begins_with("--aw-post-run=") || arg.begins_with("-gpost_run_script=")) {
				post_run_path = arg.get_slice("=", 1);
			} else if (arg.begins_with("-gexit")) {
				// Autowork naturally halts after tests if launched headless, nothing explicitly needed
			} else if (arg.begins_with("-ghide_orphans") || arg.begins_with("--aw-hide-orphans")) {
				logger->set_hide_orphans(true);
			} else if (arg.begins_with("-ginclude_subdirs") || arg.begins_with("--aw-include-subdirs")) {
				collector->include_subdirectories = true;
			} else if (arg.begins_with("-glog=") || arg.begins_with("--aw-log=")) {
				// Set explicitly on logger
			}
		}

		if (!pre_run_path.is_empty()) {
			Ref<Script> pre_s = ResourceLoader::load(pre_run_path);
			if (pre_s.is_valid()) {
				Ref<AutoworkHookScript> hook;
				hook.instantiate();
				hook->set_script(pre_s);
				hook->call("_run");
				if (hook->should_abort()) {
					print_line("Autowork Main: Aborting tests due to pre-run script abort() call.");
					return;
				}
			}
		}
	}

	Array scripts = collector->get_scripts();
	for (int i = 0; i < scripts.size(); i++) {
		Dictionary script_info = scripts[i];
		Ref<Script> test_script = script_info["script"];
		TypedArray<String> tests = script_info["tests"];

		print_line(vformat("Running Script: %s", script_info["path"]));

		Node *test_instance = Object::cast_to<Node>(ClassDB::instantiate(test_script->get_instance_base_type()));
		if (!test_instance) {
			continue;
		}

		test_instance->set_script(test_script);
		test_instance->set_name("TestInstance");
		add_child(test_instance);

		if (test_instance->has_method("set_logger") && logger.is_valid()) {
			test_instance->call("set_logger", logger);
		}
		if (test_instance->has_method("set_doubler") && doubler.is_valid()) {
			test_instance->call("set_doubler", doubler);
		}
		if (test_instance->has_method("set_spy") && spy.is_valid()) {
			test_instance->call("set_spy", spy);
		}
		if (test_instance->has_method("set_stubber") && stubber.is_valid()) {
			test_instance->call("set_stubber", stubber);
		}

		// _before_all
		if (test_instance->has_method("_before_all")) {
			test_instance->call("_before_all");
		}

		for (int j = 0; j < tests.size(); j++) {
			String test_name = tests[j];
			print_line(vformat("\t- %s", test_name));

			bool is_parameterized = false;
			int param_index = 0;

			if (test_instance->has_method("clear_parameters")) {
				test_instance->call("clear_parameters");
			}

			while (true) {
				if (is_parameterized && test_instance->has_method("set_current_parameter_index")) {
					test_instance->call("set_current_parameter_index", param_index);
				}

				if (logger.is_valid()) {
					String ds = is_parameterized ? vformat(" (param %d)", param_index) : "";
					logger->begin_test(script_info["path"], test_name + ds);
					if (!is_parameterized || param_index == 0) {
						logger->inc_test_count();
					}
				}

				if (test_instance->has_method("_before_each")) {
					test_instance->call("_before_each");
				}

				uint32_t objects_before = ObjectDB::get_object_count();

				Variant ret = test_instance->call(StringName(test_name)); // Run the test!
				if (ret.get_type() == Variant::SIGNAL) {
					Signal sig = ret;
					Object *sig_obj = sig.get_object();
					StringName sig_name = sig.get_name();
					if (sig_obj) {
						Ref<AutoworkSignalWatcher> watcher;
						watcher.instantiate();
						watcher->watch_signal(sig_obj, sig_name);
						while (!watcher->did_emit(sig_obj, sig_name)) {
							OS::get_singleton()->delay_usec(10000);
							SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
							if (scene_tree) {
								scene_tree->emit_signal("process_frame");
								scene_tree->emit_signal("physics_frame");
							}
						}
					}
				}

				if (test_instance->has_method("_after_each")) {
					test_instance->call("_after_each");
				}

				if (test_instance->has_method("free_all")) {
					test_instance->call("free_all");
				}

				uint32_t objects_after = ObjectDB::get_object_count();
				if (objects_after > objects_before && logger.is_valid()) {
					logger->inc_orphans(objects_after - objects_before);
				}

				if (logger.is_valid()) {
					logger->end_test();
				}

				bool has_p = test_instance->has_method("has_parameters") && test_instance->call("has_parameters");
				if (has_p) {
					is_parameterized = true;
					param_index++;
					int p_count = test_instance->call("get_parameter_count");
					if (param_index >= p_count) {
						break;
					}
				} else {
					break;
				}
			}
		}

		// _after_all
		if (test_instance->has_method("_after_all")) {
			test_instance->call("_after_all");
		}

		// Flush all suspended Godot 4 GDScript coroutines before test is freed
		for (int f = 0; f < 3; f++) {
			SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
			if (scene_tree) {
				scene_tree->emit_signal("process_frame");
				scene_tree->emit_signal("physics_frame");
			}
		}

		remove_child(test_instance);
		test_instance->queue_free();
	}

	if (logger.is_valid()) {
		logger->print_summary();
	}

	if (OS::get_singleton()) {
		String junit_p;
		String json_p;
		String post_p;
		for (const String &arg : OS::get_singleton()->get_cmdline_args()) {
			if (arg.begins_with("--aw-junit=")) {
				junit_p = arg.get_slice("=", 1);
			}
			if (arg.begins_with("--aw-json=")) {
				json_p = arg.get_slice("=", 1);
			}
			if (arg.begins_with("--aw-post-run=")) {
				post_p = arg.get_slice("=", 1);
			}
		}
		if (!post_p.is_empty()) {
			Ref<Script> post_s = ResourceLoader::load(post_p);
			if (post_s.is_valid()) {
				Ref<AutoworkHookScript> hook;
				hook.instantiate();
				hook->set_script(post_s);
				hook->call("_run");
			}
		}
		if (!junit_p.is_empty()) {
			Ref<AutoworkJUnitExporter> jx;
			jx.instantiate();
			jx->export_xml(logger, junit_p);
		}
		if (!json_p.is_empty()) {
			Ref<AutoworkJSONExporter> js;
			js.instantiate();
			js->export_json(logger, json_p);
		}

		bool is_headless = DisplayServer::get_singleton() && DisplayServer::get_singleton()->get_name() == "headless";
		if (is_headless) {
			SceneTree *scene_tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
			if (scene_tree) {
				scene_tree->quit(logger->get_fails());
			}
		}
	}
}
