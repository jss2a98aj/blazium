"""Functions used to generate source files during build time"""

import os

from methods import generated_wrapper


def make_debug_mingw(target, source, env):
    dst = str(target[0])
    # Force separate debug symbols if executable size is larger than 1.9 GB.
    if env["separate_debug_symbols"] or os.stat(dst).st_size >= 2040109465:
        os.system("{0} --only-keep-debug {1} {1}.debugsymbols".format(env["OBJCOPY"], dst))
        os.system("{0} --strip-debug --strip-unneeded {1}".format(env["STRIP"], dst))
        os.system("{0} --add-gnu-debuglink={1}.debugsymbols {1}".format(env["OBJCOPY"], dst))


def rc_defs_builder(target, source, env):
    dst = str(target[0])

    with generated_wrapper(dst) as file:
        file.write(
            """\
#define ICON_PATH platform/windows/icons/blazium_{external_status}.ico
#define ICON_WRAP_PATH platform/windows/icons/blazium_console_{external_status}.ico
""".format(**source[0].read())
        )
