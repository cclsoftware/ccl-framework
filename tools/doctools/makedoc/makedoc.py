#!/usr/bin/python
"""Sphinx Documentation Build Tool.
"""

import argparse
import logging
import pathlib
import platform
import sys

__copyright__ = "Copyright (c) 2025 CCL Software Licensing GmbH"
__version__ = "1.1.2"

from modules.buildenv import BuildEnvironment
from modules.builder import ProjectBuilder, MetaProjectBuilder
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
    doc_builder.build()


if __name__ == "__main__":
    main()
