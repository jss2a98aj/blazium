/**************************************************************************/
/*  autowork_junit_exporter.cpp                                           */
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

#include "autowork_junit_exporter.h"
#include "core/io/file_access.h"

void AutoworkJUnitExporter::_bind_methods() {}

String AutoworkJUnitExporter::_escape_cdata(const String &p_text) {
	return "<![CDATA[" + p_text + "]]>";
}

String AutoworkJUnitExporter::_xml_indent(int p_level) {
	String out;
	for (int i = 0; i < p_level; i++) {
		out += "  ";
	}
	return out;
}

AutoworkJUnitExporter::AutoworkJUnitExporter() {}
AutoworkJUnitExporter::~AutoworkJUnitExporter() {}

bool AutoworkJUnitExporter::export_xml(Ref<AutoworkLogger> p_logger, const String &p_path) {
	if (p_path.is_empty() || p_logger.is_null()) {
		return false;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		ERR_PRINT("AutoworkJUnitExporter: Cannot open path for writing: " + p_path);
		return false;
	}

	int total_tests = p_logger->get_passes() + p_logger->get_fails() + p_logger->get_warnings();
	int total_failures = p_logger->get_fails();

	f->store_string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	f->store_string(vformat("<testsuites name=\"GutTests\" failures=\"%d\" tests=\"%d\">\n", total_failures, total_tests));

	// We export a single suite since the Logger doesn't deeply segregate script boundaries internally for simplistic summary reporting yet
	f->store_string(_xml_indent(1) + vformat("<testsuite name=\"summary\" tests=\"%d\" failures=\"%d\" skipped=\"%d\" time=\"0.0\">\n", total_tests, total_failures, p_logger->get_warnings()));

	// Instead of deep iteration for now, we just log a generic testcase based on the summary so CI doesn't crash on parse
	// A complete mapping would require logger tracking individual assert vectors
	if (total_failures > 0) {
		f->store_string(_xml_indent(2) + vformat("<testcase name=\"failing_tests\" assertions=\"%d\" status=\"fail\" classname=\"summary\" time=\"0.0\">\n", total_failures));
		f->store_string(_xml_indent(3) + "<failure message=\"failed\"><![CDATA[There were failing tests in the suite.]]></failure>\n");
		f->store_string(_xml_indent(2) + "</testcase>\n");
	}

	if (p_logger->get_passes() > 0) {
		f->store_string(_xml_indent(2) + vformat("<testcase name=\"passing_tests\" assertions=\"%d\" status=\"pass\" classname=\"summary\" time=\"0.0\">\n", p_logger->get_passes()));
		f->store_string(_xml_indent(2) + "</testcase>\n");
	}

	f->store_string(_xml_indent(1) + "</testsuite>\n");
	f->store_string("</testsuites>\n");
	f->close();

	return true;
}
