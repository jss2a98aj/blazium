/**************************************************************************/
/*  autowork_json_exporter.cpp                                            */
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

#include "autowork_json_exporter.h"
#include "core/io/file_access.h"
#include "core/io/json.h"

void AutoworkJSONExporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("export_json", "logger", "file_path"), &AutoworkJSONExporter::export_json);
}

AutoworkJSONExporter::AutoworkJSONExporter() {
}

AutoworkJSONExporter::~AutoworkJSONExporter() {
}

bool AutoworkJSONExporter::export_json(Ref<AutoworkLogger> p_logger, const String &p_file_path) {
	if (p_logger.is_null()) {
		return false;
	}

	Dictionary root;
	root["test_count"] = p_logger->get_test_count();
	root["pass_count"] = p_logger->get_passes();
	root["fail_count"] = p_logger->get_fails();
	root["warning_count"] = p_logger->get_warnings();
	root["orphan_count"] = p_logger->get_orphans();

	Array scripts_arr;
	const Vector<AutoworkTestMethodResult> &results = p_logger->get_test_results();

	// Group by script
	HashMap<StringName, Array> script_map;

	for (int i = 0; i < results.size(); i++) {
		const AutoworkTestMethodResult &res = results[i];
		Dictionary t_dict;
		t_dict["name"] = res.method_name;
		t_dict["passes"] = res.passes;
		t_dict["fails"] = res.fails;
		t_dict["orphans"] = res.orphans;

		Array errors_arr;
		for (int k = 0; k < res.fail_messages.size(); k++) {
			errors_arr.push_back(res.fail_messages[k]);
		}
		t_dict["errors"] = errors_arr;
		if (!script_map.has(res.script_name)) {
			script_map.insert(res.script_name, Array());
		}
		script_map[res.script_name].push_back(t_dict);
	}

	for (const KeyValue<StringName, Array> &E : script_map) {
		Dictionary s_dict;
		s_dict["name"] = E.key;
		s_dict["tests"] = E.value;
		scripts_arr.push_back(s_dict);
	}

	root["scripts"] = scripts_arr;

	String json_text = JSON::stringify(root, "  ");
	Ref<FileAccess> file = FileAccess::open(p_file_path, FileAccess::WRITE);
	if (file.is_null()) {
		return false;
	}

	file->store_string(json_text);
	return true;
}
