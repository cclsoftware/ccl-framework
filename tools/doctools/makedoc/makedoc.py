#!/usr/bin/python
"""Sphinx Documentation Build Tool.
"""

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"
__version__ = "1.1.4"

import argparse
import logging
import pathlib
import platform
import sys
import importlib.util

# Ensure 'ccl_extras' is importable. If not installed in the active environment,
# add the repository extras path to 'sys.path'. Needs to be done before importing any
# module that depends on 'ccl_extras'.

if importlib.util.find_spec("ccl_extras") is None:
    extras_path = pathlib.Path(__file__).resolve().parents[3] / "ccl" / "extras" / "python"
    extras_path_str = str(extras_path)
    if extras_path.is_dir() and extras_path_str not in sys.path:
        sys.path.insert(0, extras_path_str)


from modules.buildenv import BuildEnvironment
from modules.builder import ProjectBuilder, MetaProjectBuilder, BuildException
from modules.project import ProjectHandler

makedoc_path = pathlib.Path(__file__).resolve()
makedoc_folder = makedoc_path.parent


def main() -> None:
    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
    logging.info(f"MakeDoc v{__version__}, {__copyright__}")

    build_env = BuildEnvironment(makedoc_folder)
    project_handler = ProjectHandler(env=build_env)

    supported_builders = ["html", "latex", "pdf", "zip", "tar"]
    supported_project_ids = project_handler.get_all_project_ids()

    parser = argparse.ArgumentParser(
        description='Documentation project builder',
        usage="%(prog)s [OPTIONS] PROJECT_ID BUILDER [REVISION]\nExample: %(prog)s dev.ccl.doc.skinlanguage html"
    )

    parser.add_argument('project_id',
                        choices=supported_project_ids,
                        metavar='PROJECT_ID',
                        help=f'Documentation project to build, choose from: {supported_project_ids}')

    parser.add_argument('builder',
                        choices=supported_builders,
                        metavar='BUILDER',
                        help=f'Build output format, choose from: {supported_builders}')

    parser.add_argument('revision',
                        metavar='REVISION',
                        help='Documentation revision, optional. Example: 52324.',
                        nargs="?",
                        default="")

    parser.add_argument("-v", "-verbose",
                        default=False,
                        action="store_true")

    parser.add_argument("-r", "-no-rebuild",
                        default=False,
                        action="store_true")

    args = parser.parse_args()

    logging.info(f"running on {build_env.system}, {build_env.machine}, Python {platform.python_version()}")

    project_id = args.project_id
    project_handler = ProjectHandler(env=build_env)
    project_info = project_handler.get_project_info(project_id)
    if project_info is None:
        logging.error(f"could not load project config for {project_id}")
        sys.exit(1)

    builder = args.builder
    if builder not in ["html", "latex", "pdf", "zip", "tar"]:
        logging.warning(f"unsupported builder {builder}")
        sys.exit(1)

    rev = args.revision
    rev_log = f"using revision {rev}" if rev else ""
    logging.info(f"building {builder} of '{project_id}' at '{project_info.path}' {rev_log}")

    verbose = args.v
    rebuild = not args.r

    # Decide whether to build a meta or single project depending on the 'meta'
    # configuration attribute inside the makedoc.json config file.
    if project_info.config.meta:
        doc_builder = MetaProjectBuilder(build_env=build_env, project_handler=project_handler,
                                         project_info=project_info,
                                         builder=builder, revision=rev, verbose=verbose, rebuild=rebuild)
    else:
        doc_builder = ProjectBuilder(build_env=build_env, project_info=project_info, builder=builder, revision=rev,
                                     verbose=verbose, rebuild=rebuild)

    try:
        doc_builder.build()
    except BuildException as e:
        logging.error(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
