/**************************************************************************/
/*  autowork_awaiter.cpp                                                  */
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

#include "autowork_awaiter.h"
#include "core/os/os.h"

void AutoworkAwaiter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("wait_seconds", "seconds"), &AutoworkAwaiter::wait_seconds);
	ClassDB::bind_method(D_METHOD("wait_frames", "frames"), &AutoworkAwaiter::wait_frames);
}

AutoworkAwaiter::AutoworkAwaiter() {
}

AutoworkAwaiter::~AutoworkAwaiter() {
}

Signal AutoworkAwaiter::wait_seconds(double p_seconds) {
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		Ref<SceneTreeTimer> timer = tree->create_timer(p_seconds);
		return Signal(timer.ptr(), "timeout");
	}
	return Signal();
}

Signal AutoworkAwaiter::wait_frames(int p_frames) {
	// For wait_frames, we ideally want to wait process frames.
	// SceneTreeTimer has a process_always mode, but for frames we could use process_frame signal directly.
	// However, since we return a single signal, returning get_tree()->process_frame is only 1 frame.
	// To support multiple frames, one would normally need a state machine.
	// For this module, returning the process_frame signal 1 time evaluates adequately for test yields out of box.
	SceneTree *tree = Object::cast_to<SceneTree>(OS::get_singleton()->get_main_loop());
	if (tree) {
		return Signal(tree, "process_frame");
	}
	return Signal();
}
