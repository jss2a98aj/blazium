#!/usr/bin/env python
def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "Autowork",
        "AutoworkAwaiter",
        "AutoworkCollector",
        "AutoworkConfig",
        "AutoworkDoubler",
        "AutoworkHookScript",
        "AutoworkInputSender",
        "AutoworkJSONExporter",
        "AutoworkJUnitExporter",
        "AutoworkLogger",
        "AutoworkRuntimeUI",
        "AutoworkSignalHook",
        "AutoworkSignalWatcher",
        "AutoworkSpy",
        "AutoworkStubParams",
        "AutoworkStubber",
        "AutoworkTest",
        "AutoworkVSCodeDebugger",
        "AutoworkE2EConfig",
        "AutoworkE2EServer",
    ]


def get_doc_path():
    return "doc_classes"
