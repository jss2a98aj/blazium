/**************************************************************************/
/*  autowork_logger.cpp                                                   */
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

#include "autowork_logger.h"
#include "core/os/os.h"

void AutoworkLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_pass", "message"), &AutoworkLogger::add_pass);
	ClassDB::bind_method(D_METHOD("add_fail", "message"), &AutoworkLogger::add_fail);
	ClassDB::bind_method(D_METHOD("add_warning", "message"), &AutoworkLogger::add_warning);
	ClassDB::bind_method(D_METHOD("inc_orphans", "count"), &AutoworkLogger::inc_orphans);
	ClassDB::bind_method(D_METHOD("inc_test_count"), &AutoworkLogger::inc_test_count);
	ClassDB::bind_method(D_METHOD("print_summary"), &AutoworkLogger::print_summary);
}

AutoworkLogger::AutoworkLogger() {
}

AutoworkLogger::~AutoworkLogger() {
}

void AutoworkLogger::begin_test(const StringName &p_script, const StringName &p_method) {
	if (current_test.script_name != p_script) {
		print_log(vformat("\n[b]=== Script: %s ===[/b]", p_script));
	}

	current_test.script_name = p_script;
	current_test.method_name = p_method;
	current_test.passes = 0;
	current_test.fails = 0;
	current_test.orphans = 0;
	current_test.fail_messages.clear();

	print_log(vformat("  [b]%s[/b]", p_method));
}

void AutoworkLogger::end_test() {
	test_results.push_back(current_test);
	current_test.script_name = StringName();
	current_test.method_name = StringName();
	current_test.passes = 0;
	current_test.fails = 0;
	current_test.orphans = 0;
	current_test.fail_messages.clear();
}

void AutoworkLogger::set_output_ui(Object *p_ui) {
	output_ui = p_ui;
}

void AutoworkLogger::print_log(const String &p_msg, bool p_error) {
	String bbmsg = p_msg;
	String console_msg = p_msg;

	// UI bbcode formatting
	if (p_error || p_msg.contains("[FAIL]")) {
		bbmsg = "[color=red]" + p_msg + "[/color]";
		console_msg = "\033[31;1m" + p_msg + "\033[0m";
	} else if (p_msg.contains("[PASS]")) {
		bbmsg = "[color=green]" + p_msg + "[/color]";
		console_msg = "\033[32;1m" + p_msg + "\033[0m";
	} else if (p_msg.contains("[WARN]")) {
		bbmsg = "[color=yellow]" + p_msg + "[/color]";
		console_msg = "\033[33;1m" + p_msg + "\033[0m";
	}

	if (p_error) {
		print_error(p_msg); // Error stream gets plain text natively
	} else {
		// Drop the literal [b] from script titles on console
		console_msg = console_msg.replace("[b]", "\033[1m").replace("[/b]", "\033[0m");
		print_line(console_msg);
	}

	if (output_ui && output_ui->has_method("append_text")) {
		output_ui->call("append_text", bbmsg + "\n");
	}
}

void AutoworkLogger::add_pass(const String &p_message) {
	passes++;
	current_test.passes++;
	print_log(vformat("\t\t[PASS] %s", p_message));
}

void AutoworkLogger::add_fail(const String &p_message) {
	fails++;
	current_test.fails++;
	current_test.fail_messages.push_back(p_message);
	print_log(vformat("\t\t[FAIL] %s", p_message), true);
}

void AutoworkLogger::add_warning(const String &p_message) {
	warnings++;
	print_log(vformat("\t\t[WARN] %s", p_message));
}

void AutoworkLogger::inc_orphans(int p_count) {
	orphans += p_count;
	current_test.orphans += p_count;
}

void AutoworkLogger::inc_test_count() {
	test_count++;
}

void AutoworkLogger::print_summary() {
	print_log("\n=================================");
	print_log("AUTOWORK TEST SUMMARY");
	print_log(vformat("Tests Run: %d", test_count));
	print_log(vformat("Passing Asserts: %d", passes));
	if (fails > 0) {
		print_log(vformat("Failing Asserts: %d", fails));
	} else {
		print_log("Failing Asserts: None!");
	}
	if (warnings > 0) {
		print_log(vformat("Warnings: %d", warnings));
	}
	if (!hide_orphans && orphans > 0) {
		print_log(vformat("Orphaned Objects: %d", orphans));
	}
	print_log("=================================\n");
}
