/**************************************************************************/
/*  justamcp_task_manager.h                                               */
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

#ifdef TOOLS_ENABLED

#include "core/object/class_db.h"
#include "core/object/object.h"

class JustAMCPTaskManager : public Object {
	GDCLASS(JustAMCPTaskManager, Object);

	Dictionary active_tasks;

protected:
	static void _bind_methods();

public:
	Dictionary create_task(const String &p_task_id, int p_ttl = 60000);
	Dictionary list_tasks(const String &cursor = "");
	Dictionary get_task(const String &p_task_id);
	Dictionary get_task_result(const String &p_task_id);
	Dictionary cancel_task(const String &p_task_id);

	void complete_task(const String &p_task_id, const Dictionary &p_result);
	void fail_task(const String &p_task_id, const String &p_error);

	JustAMCPTaskManager();
	~JustAMCPTaskManager();
};

#endif // TOOLS_ENABLED
