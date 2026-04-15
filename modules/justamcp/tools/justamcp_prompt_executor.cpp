/**************************************************************************/
/*  justamcp_prompt_executor.cpp                                          */
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

#include "justamcp_prompt_executor.h"
#include "core/config/project_settings.h"
#include "prompts/justamcp_prompt_blazium_context.h"
#include "prompts/justamcp_prompt_editor_state.h"
#include "prompts/justamcp_prompt_project_info.h"

void JustAMCPPromptExecutor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_prompt", "name", "args"), &JustAMCPPromptExecutor::get_prompt);
	ClassDB::bind_method(D_METHOD("list_prompts", "cursor"), &JustAMCPPromptExecutor::list_prompts, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("complete_prompt", "ref", "argument"), &JustAMCPPromptExecutor::complete_prompt);
	ClassDB::bind_method(D_METHOD("add_prompt", "prompt"), &JustAMCPPromptExecutor::add_prompt);
}

JustAMCPPromptExecutor::JustAMCPPromptExecutor() {
	add_prompt(memnew(JustAMCPPromptBlaziumContext));
	add_prompt(memnew(JustAMCPPromptProjectInfo));
	add_prompt(memnew(JustAMCPPromptEditorState));
}

JustAMCPPromptExecutor::~JustAMCPPromptExecutor() {}

void JustAMCPPromptExecutor::add_prompt(const Ref<JustAMCPPrompt> &p_prompt) {
	if (p_prompt.is_valid()) {
		registered_prompts.push_back(p_prompt);
	}
}

Dictionary JustAMCPPromptExecutor::list_prompts(const String &cursor) {
	Dictionary result;
	Array prompts;

	for (int i = 0; i < registered_prompts.size(); i++) {
		if (registered_prompts[i].is_valid()) {
			prompts.push_back(registered_prompts[i]->get_prompt());
		}
	}

	result["prompts"] = prompts;
	return result;
}

Dictionary JustAMCPPromptExecutor::get_prompt(const String &p_name, const Dictionary &p_args) {
	for (int i = 0; i < registered_prompts.size(); i++) {
		if (registered_prompts[i].is_valid() && registered_prompts[i]->get_name() == p_name) {
			return registered_prompts[i]->get_messages(p_args);
		}
	}

	Dictionary result;
	result["ok"] = false;
	result["error_code"] = -32602;
	result["error"] = "Unknown prompt: " + p_name;
	return result;
}

Dictionary JustAMCPPromptExecutor::complete_prompt(const Dictionary &p_ref, const Dictionary &p_argument) {
	String ref_name = p_ref.has("name") ? String(Variant(p_ref["name"])) : "";

	for (int i = 0; i < registered_prompts.size(); i++) {
		if (registered_prompts[i].is_valid() && registered_prompts[i]->get_name() == ref_name) {
			Dictionary result;
			result["completion"] = registered_prompts[i]->complete(p_argument);
			return result;
		}
	}

	Dictionary result;
	Dictionary completion;
	completion["values"] = Array();
	completion["total"] = 0;
	completion["hasMore"] = false;
	result["completion"] = completion;
	return result;
}

#endif // TOOLS_ENABLED
