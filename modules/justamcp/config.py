def can_build(env, platform):
    # Depending on websocket and mbedtls for networking
    env.module_add_dependencies("justamcp", ["websocket", "mbedtls"], True)
    return env.editor_build


def configure(env):
    pass


def get_doc_classes():
    return [
        "JustAMCPRuntime",
        "JustAMCPServer",
        "JustAMCPToolExecutor",
        "JustAMCPSceneTools",
        "JustAMCPResourceTools",
        "JustAMCPAnimationTools",
        "JustAMCPAnalysisTools",
        "JustAMCPAudioTools",
        "JustAMCPBatchTools",
        "JustAMCPExportTools",
        "JustAMCPInputTools",
        "JustAMCPNodeTools",
        "JustAMCPParticleTools",
        "JustAMCPPhysicsTools",
        "JustAMCPProfilingTools",
        "JustAMCPProjectTools",
        "JustAMCPScene3DTools",
        "JustAMCPScriptTools",
        "JustAMCPShaderTools",
        "JustAMCPThemeTools",
        "JustAMCPTileMapTools",
    ]


def get_doc_path():
    return "doc_classes"
