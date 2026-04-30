def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "GodotTsonAnimation",
        "GodotTsonChunk",
        "GodotTsonEnumDefinition",
        "GodotTsonEnumValue",
        "GodotTsonFrame",
        "GodotTsonGrid",
        "GodotTsonLayer",
        "GodotTsonMap",
        "GodotTsonObject",
        "GodotTsonProject",
        "GodotTsonProjectData",
        "GodotTsonProjectFolder",
        "GodotTsonProjectPropertyTypes",
        "GodotTsonProperty",
        "GodotTsonTerrain",
        "GodotTsonText",
        "GodotTsonTile",
        "GodotTsonTiledClass",
        "GodotTsonTileObject",
        "GodotTsonTileset",
        "GodotTsonTileson",
        "GodotTsonTransformations",
        "GodotTsonWangColor",
        "GodotTsonWangSet",
        "GodotTsonWangTile",
        "GodotTsonWorld",
        "GodotTsonWorldMapData",
    ]


def get_doc_path():
    return "doc_classes"
