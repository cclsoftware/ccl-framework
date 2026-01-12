#!/usr/bin/python3
"""
TypeScript projects build helper script, tasks:

    * recursively lookup TypeScript projects in a directory structure based on presence of tsconfig.json file
    * run tsc compiler on all projects found with --verbose and --force options
    * optional: generate Jenkins compatible junit XML report file, use as Jenkins build script
    * convert tsconfig.json.in files to tsconfig.json, allowing introduction of placeholder paths

Intended for use with CCL-based source code repositories. Uses the repository root as reference directory.
The script is portable and can be called from outside its storage location.

Usage examples:
===============

    1) Build TS projects in current working directory without generating a report:
    > $ python tsbuilder.py

    2) Build TS projects in working directory, generate junit report:
    > $ python tsbuilder.py --report=/c/myreport.xml

    3) Build all TS projects in myscriptfolder, generate report:
    > $ python tsbuilder.py --path myscriptfolder --report=/c/myreport.xml

    4) Convert tsconfig.json.in files only
    > $ python tsbuilder.py --path myscriptfolder --config-only


Non-standard modules in use
===========================

    * junit_xml (https://pypi.org/project/junit-xml/)

"""

import argparse
import logging
import os
import pathlib
import subprocess
import sys
from datetime import datetime
from typing import List

from junit_xml import TestCase, TestSuite, to_xml_report_string

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"
__version__ = "1.0.0"

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "../../../../ccl/extras/python"))

from tools.gitmodules import GitModulesReader, GitModuleInfo
from tools.repoinfo import RepositoryInfo

tsbuilder_file = pathlib.Path(__file__).resolve()
tsbuilder_path = tsbuilder_file.parent

shared_config_path = pathlib.Path(tsbuilder_path, "configs").resolve()


class Builder:
    TS_COMPILER = "tsc"
    TSCONFIG_FILE = "tsconfig.json"
    TSCONFIG_TEMPLATE_FILE = "tsconfig.json.in"

    def __init__(self):
        self.projects: List[pathlib.Path] = list()
        self.results: List[TestCase] = list()
        self.repo_info: RepositoryInfo = RepositoryInfo()

    def run(self, path: pathlib.Path, report: pathlib.Path, config_only: bool) -> None:
        """Scan for projects, build each and generate optional report."""

        # Attempt to load submodules configuration. Requires existence of
        # repo.json file, i.e. may only work in meta repository context.
        git_modules: List[GitModuleInfo] = list()
        if self.repo_info.load(tsbuilder_path):
            git_modules = GitModulesReader.load(path=self.repo_info.get_file_path())
            logging.info(f"using meta repo .gitmodules config in '{self.repo_info.get_file_path()}'")

            # Provide user hint at which submodule placeholder paths can be used in file
            # tsconfig.json.in so user does not have to inspect .gitmodules file manually.
            supported_submodule_paths: List[str] = [module.url_short for module in git_modules]
            logging.info(f"supported submodule paths: {supported_submodule_paths}")

        self._lookup_projects(path)

        for p in self.projects:
            filename = p.name
            if filename == Builder.TSCONFIG_TEMPLATE_FILE:
                self._write_config(p, git_modules)

            if not config_only:
                self._build_project(p)

        if report:
            self._create_report(report)
        else:
            logging.info("skipped report, use --report [file] to generate")

    @staticmethod
    def _to_canonical_display_path(path: pathlib.Path) -> str:
        """Format path to json compatible format."""
        return str(path).replace("\\", "/")

    def _lookup_projects(self, path: pathlib.Path) -> None:
        """ Find all directories in path that are TypeScript projects. """

        # Determine repository root path relative to this script
        # file, then append any custom specified path to it.

        if path.is_absolute():
            scan_path = path
        else:
            self_path = pathlib.Path(__file__)
            repo_root_path = pathlib.Path(self_path.parents[2])
            scan_path = repo_root_path.joinpath(path)

        file_candidates = [Builder.TSCONFIG_TEMPLATE_FILE, Builder.TSCONFIG_FILE]
        logging.info(f"scanning '{scan_path}' for TypeScript project files {file_candidates} ...")

        # '**/' pattern -> directories only
        for candidate_dir in scan_path.glob("**/"):

            # Directory may contain a tsconfig.json.in or a tsconfig.json.
            # Prioritize and register the tsconfig.json.in. For each directory
            # only one file needs to be registered.

            for filename in file_candidates:
                file = pathlib.Path(candidate_dir, filename)
                if file.exists():
                    logging.debug(f"found '{file}'")
                    self.projects.append(file)
                    break

        logging.info(f"found {len(self.projects)} TypeScript projects")

    @staticmethod
    def _create_test_name(path: pathlib.Path) -> str:
        """ Create shorter path name from last two elements """
        parts = pathlib.Path(path).parts[-2:]
        combined = '/'.join(parts)
        return combined

    @staticmethod
    def _write_config(path: pathlib.Path, modules: List[GitModuleInfo]) -> None:
        """Open tsconfig.json.in file, replace placeholder paths
        and write tsconfig.json file. Preserves formatting of the
        template file.
        """

        try:
            with path.open(mode="r+") as file:
                data = file.read()

                # Replace "shared configs" dir.
                data = data.replace(
                    "{shared_configs_dir}",
                    Builder._to_canonical_display_path(path=shared_config_path)
                )

                # Replace submodule dirs.
                for module in modules:
                    data = data.replace(
                        "{" + module.url_short + "}",
                        Builder._to_canonical_display_path(module.path)
                    )

                # Save data to result tsconfig.json file.
                tsconfig_file = pathlib.Path(path.parent, Builder.TSCONFIG_FILE)
                with tsconfig_file.open(mode="w+", encoding="utf-8") as outfile:
                    outfile.write(data)
                    logging.info(f"wrote config '{tsconfig_file}'")

        except OSError as e:
            logging.error(f"failed to write config: {e}")

    def _build_project(self, tsconfig_path: pathlib.Path) -> None:
        """Run tsc on a single project, save test result

        :param tsconfig_path: absolute path to tsconfig.json file
        """

        assert tsconfig_path.absolute()

        returncode = 1
        stdout = ""
        stderr = ""

        # Use tsconfig.json containing folder as working directory.
        tsconfig_folder = tsconfig_path.parent

        now = datetime.now()
        try:
            cmd = [Builder.TS_COMPILER, "--build", Builder.TSCONFIG_FILE, "--verbose", "--force"]
            # Running 'tsc' script requires shell
            process = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=tsconfig_folder)
            stdout = process.stdout
            stderr = process.stderr
            returncode = process.returncode
        except OSError as e:
            logging.error(f"failed to build project: {e}")

        duration = (datetime.now() - now).total_seconds()
        Builder._log_build_result(tsconfig_folder, returncode, stdout, duration)
        self._save_test_result(tsconfig_folder, returncode, stdout, stderr, duration)

    @staticmethod
    def _log_build_result(project: pathlib.Path, returncode: int, stdout: str, duration: float) -> None:
        result = "PASSED" if returncode == 0 else "FAILED"
        logging.info(f"[{result}] project='{str(project)}', duration={duration}s")

        # Convenience: show output on failure
        if returncode != 0:
            logging.warning(stdout)

    def _save_test_result(self, project: pathlib.Path, returncode: int, stdout: str, stderr: str,
                          duration: float) -> None:
        """Register testcase result."""
        test_name = Builder._create_test_name(project)
        result_case = TestCase(name=test_name, stdout=stdout, stderr=stderr, elapsed_sec=duration)
        if returncode != 0:
            result_case.add_failure_info(message="build failed")

        self.results.append(result_case)

    def _create_report(self, path: pathlib.Path) -> None:
        testsuite = TestSuite("TypeScript Builds", self.results)
        with path.open("w+", encoding="utf-8") as f:
            f.write(to_xml_report_string([testsuite]))

        logging.info(f"wrote junit report '{path}'")


def main() -> None:
    """Main function, setup logging, scan arguments, call builder."""

    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', level=logging.INFO)
    logging.info(f"TSBuilder v{__version__}, {__copyright__}")

    parser = argparse.ArgumentParser(description='TypeScript projects utility script')
    parser.add_argument('--path', help='Input path to scan for tsconfig files', required=False)
    parser.add_argument('--report', help='Build report xml output file', required=False)
    parser.add_argument('--config-only', action='store_true', help='Generate configs only, no build', required=False)
    args = parser.parse_args()

    input_path = pathlib.Path(os.getcwd())
    if args.path:
        input_path = pathlib.Path(input_path, args.path).resolve()

    report_path = None
    if args.report:
        report_path = pathlib.Path(args.report).resolve()

    config_only = args.config_only

    Builder().run(path=input_path, report=report_path, config_only=config_only)


if __name__ == "__main__":
    main()
