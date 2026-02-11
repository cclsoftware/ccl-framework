#!/usr/bin/python
"""Makedoc builder module.
"""

import logging
import os
import pathlib
import shutil
import tarfile
from typing import Optional, List

from modules.buildenv import BuildEnvironment
from modules.actions import ActionFactory, BuildActionException, IBuildAction
from modules.template import SphinxConfigWriter, DoxyrestConfigWriter, DoxygenConfigWriter
from modules.project import ProjectHandler, ProjectConfigInfo
from modules.tools import SphinxTool, PdfTool

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

class InFileHandler:

    def __init__(self, input_path: pathlib.Path, env: BuildEnvironment):
        self.input_path = input_path
        self.env = env

    def run(self):
        for root, dirs, files in os.walk(self.input_path):
            for file in files:
                if not file.endswith(".in"):
                    continue

                file_path = pathlib.Path(root, file)
                self.process_file(file_path)

    def process_file(self, file: pathlib.Path) -> None:
        logging.info(f"processing .in file '{file}'")

        filename = file.name
        writer = None
        if DoxyrestConfigWriter.OUTPUT_FILE in filename:
            writer = DoxyrestConfigWriter(input_file=file, env=self.env)
        elif DoxygenConfigWriter.OUTPUT_FILE in filename:
            writer = DoxygenConfigWriter(input_file=file, env=self.env)

        if writer:
            writer.run()


class BuilderBase:
    LATEX_INDEX_FILE = "index_latex.rst"
    INDEX_FILE = "index.rst"

    def __init__(self, build_env: BuildEnvironment, project_info: ProjectConfigInfo,
                 builder: str, revision: str, verbose: bool, rebuild: bool) -> None:
        self.env = build_env
        self.project_info = project_info  # path + config
        self.builder = builder
        self.revision = revision
        self.verbose = verbose
        self.rebuild = rebuild

    def _setup_output_path(self, output_folder: str) -> pathlib.Path:
        """Build doc output path, remove existing output."""
        joined = pathlib.Path(self.env.output_path, output_folder)
        path = joined.resolve()

        try:
            shutil.rmtree(path)
        except OSError as e:
            logging.debug(f"could not cleanup output directory {path}: {e}")

        return path

    def _allow_build(self, output_folder: str) -> bool:
        """Check if the output directory already exists vs. 'no-rebuild' option."""

        joined = pathlib.Path(self.env.output_path, output_folder)
        path = joined.resolve()

        # Keep simple here and do not check for individual artifact types (html, pdf, ...).
        if path.exists() and not self.rebuild:
            logging.info(f"skipped build for existing output due to use of [-no-rebuild] or [-r] option")
            return False

        return True

    def _zip_html_output(self, output_path: pathlib.Path) -> None:
        """Zip html output folder. The created archive file contains
        all the content of the html/ folder without the html/ folder
        itself.
        """

        html_output_path = pathlib.Path(output_path, "html")
        zip_output_path = pathlib.Path(output_path, self.project_info.config.id)
        shutil.make_archive(str(zip_output_path), 'zip', html_output_path)
        logging.info(f"wrote zip archive {zip_output_path}.zip")

    def _tar_html_output(self, output_path: pathlib.Path) -> None:
        """Tar html/ output folder. The created archive file contains
        all the content of the html/ folder without the html/ folder
        itself.
        """

        html_output_path = pathlib.Path(output_path, "html")
        tar_output_path = pathlib.Path(output_path, f"{self.project_info.config.id}.tar")

        tar = tarfile.open(tar_output_path, "w:gz")
        tar.add(html_output_path, arcname=".")
        tar.close()

        logging.info(f"wrote tar archive {tar_output_path}")

    @staticmethod
    def _extract_vendor_from_id(id: str) -> Optional[str]:
        """Extract vendor name from project id.
        Example: id="dev.ccl.doc.skinlanguage" -> vendor="ccl"
        """

        tokens = id.split(".")
        if tokens and len(tokens) > 1:
            return tokens[1]

        return None

    def _run_build_actions(self, project_info: ProjectConfigInfo) -> None:
        """Run project build actions if specified in makedoc.json file.
        Preprocess all configuration template files as prerequisite.
        """

        # TODO: future improvement, move config file preprocessing to individual steps
        infile_handler = InFileHandler(input_path=project_info.path, env=self.env)
        infile_handler.run()

        for action_config in project_info.config.build:
            action_config.attrs["verbose"] = self.verbose
            action: IBuildAction = ActionFactory.create(action_config, self.env, project_info.path)
            if action is None:
                logging.error(f"unsupported build action '{action_config.action}'")
                continue

            log_prefix = f"[BUILD ACTION] [{action_config.action}]"

            try:
                log = log_prefix
                if action_config.description:
                    log = f"{log} '{action_config.description}'"
                logging.info(log)

                result = action.run()
                if not result:
                    logging.error(f"{log_prefix} failed")

            except BuildActionException as e:
                logging.error(f"{log_prefix} error: {e}")


class MetaProjectBuilder(BuilderBase):
    """Meta project builder."""

    def __init__(self, build_env: BuildEnvironment, project_handler: ProjectHandler, project_info: ProjectConfigInfo,
                 builder: str, revision: str, verbose: bool, rebuild: bool) -> None:

        super().__init__(build_env, project_info, builder, revision, verbose, rebuild)
        self.project_handler = project_handler

    def build(self):
        """Prepare conf.py, copy files and run sphinx build."""

        if self.builder not in ["html", "zip", "tar"]:
            logging.warning(f"meta docs require html, zip or tar build")
            return

        if not self._allow_build(self.project_info.config.id):
            return

        # The project files are copied to the projects/ folder (i.e. one directory up)
        # so it can parent content in any project. Consider this the input directory.

        # Cleanup previous run output
        logging.info(f"cleaning up build folder '{self.env.build_temp_folder}'")
        shutil.rmtree(self.env.build_temp_folder, ignore_errors=True)

        # Detect dependencies from the index.rst.in file.
        index_file_in = pathlib.Path(self.project_info.path, "index.rst.in")
        all_project_ids = self.project_handler.get_all_project_ids()
        required_project_ids = self._get_included_project_ids(index_file_in, all_project_ids)

        ############################################################
        # Run projects setup, copy projects to meta build directory
        ############################################################

        self._preprocess_subprojects(required_project_ids)

        ######################################
        # Create conf.py for meta project
        ######################################

        # Support for index_latex file not needed, meta project not used as PDF.
        # Skip any index_latex.rst files in subprojects as their contained toctree
        # may cause navigation issues in the HTML output.
        exclude_patterns = f"**/{BuilderBase.LATEX_INDEX_FILE}"

        # Attention: this writes to one directory above!
        config_writer = SphinxConfigWriter(project_path=self.project_info.path, output_path=self.env.build_temp_folder,
                                           env=self.env)

        config_writer.write(exclude_patterns=exclude_patterns, latex_index="index",
                            latex_file=self.project_info.config.id,
                            title=self.project_info.config.title, revision=self.revision,
                            vendor=self._extract_vendor_from_id(self.project_info.config.id))

        ####################################
        # Create index.rst for meta project
        ####################################

        index_file_out = pathlib.Path(self.env.build_temp_folder, "index.rst")
        self._build_index_file(index_file_in, index_file_out, required_project_ids)

        ####################
        # Run sphinx build
        ####################

        output_path = self._setup_output_path(self.project_info.config.id)
        SphinxTool.run("html", self.env.build_temp_folder, output_path)

        if self.builder == "zip":
            self._zip_html_output(output_path)
        elif self.builder == "tar":
            self._tar_html_output(output_path)

    def _preprocess_subprojects(self, required_subprojects: List[str]):
        """Run setup script for each included subproject to generate
        potential additional rst files. Does not sphinx-build the
        included projects.
        """

        for project_id in required_subprojects:

            subproject_info = self.project_handler.get_project_info(project_id)
            if subproject_info is None:
                logging.error(f"unknown subproject '{project_id}'")
                continue

            subproject_path = subproject_info.path
            if subproject_path is None:
                continue

            self._run_build_actions(project_info=subproject_info)

            # Copy entire project to meta build folder. At this stage, all
            # additional rst files from class models etc. have been generated
            # from setup.sh.
            subproject_folder = self._build_subproject_folder_name(project_id)
            subproject_build_dir = pathlib.Path(self.env.build_temp_folder, subproject_folder)
            shutil.copytree(subproject_path, subproject_build_dir, symlinks=False, ignore=None, dirs_exist_ok=True)
            logging.info(f"copied '{subproject_path}' to meta build dir '{subproject_build_dir}'")

    @staticmethod
    def _build_subproject_folder_name(project_id: str) -> str:
        """Derive a subfolder name from project_id, example:
        id="dev.ccl.doc.codestyle" -> folder="codestyle"
        """

        tokens = project_id.split(".")
        return tokens[-1]

    @staticmethod
    def _build_project_placeholder(project_id: str) -> str:
        """Convert project id to meta project index.rst.in placeholder format."""
        return "{" + project_id + "}"

    @staticmethod
    def _get_included_project_ids(index_file: pathlib.Path, all_project_ids: List[str]) -> List[str]:
        """Get all required subprojects from a meta project index file.
        Scan for any of the known {project id}s provided by all_project_ids.
        Expect result to have unique project ids. Returns a list over set to
        preserve the order in which the dependencies are found.
        """

        result = list()
        try:
            with index_file.open(mode="r") as f:
                for line in f:
                    stripped_line = line.lstrip()

                    # Ignore one-line comments, for example in toctree directives
                    # where certain subprojects may be temporarily removed. Does
                    # not detect multiline comments where ".. " starts an indented
                    # block of lines.
                    if stripped_line.startswith(".. "):
                        continue

                    # Not performant but index files are typically short.
                    for project_id in all_project_ids:
                        placeholder = MetaProjectBuilder._build_project_placeholder(project_id)
                        if placeholder in stripped_line and project_id not in result:
                            result.append(project_id)

        except OSError as e:
            logging.error(f"could not generate meta index file '{index_file}': {e}")

        return result

    @staticmethod
    def _build_index_file(index_file: pathlib.Path, output_file: pathlib.Path, required_project_ids: List[str]):
        """Generate meta project index.rst file at target location. Replaces
        all required subproject ids with their respective subfolder name.

        Reminder: in the build step, subprojects should be referenced by some folder
        name, not their ID. This avoids project ids in the documentation URLs.
        """

        try:
            with index_file.open(mode="r") as f:
                data = f.read()
            with output_file.open(mode="w+") as f:
                for project_id in required_project_ids:
                    placeholder = MetaProjectBuilder._build_project_placeholder(project_id)
                    data = data.replace(placeholder, MetaProjectBuilder._build_subproject_folder_name(project_id))
                f.write(data)
                logging.info(f"wrote meta index file '{output_file}'")

        except OSError as e:
            logging.error(f"could not open meta index file '{index_file}': {e}")


class ProjectBuilder(BuilderBase):
    """Project builder."""

    def __init__(self, build_env: BuildEnvironment, project_info: ProjectConfigInfo, builder: str,
                 revision: str, verbose: bool, rebuild: bool) -> None:
        super().__init__(build_env, project_info, builder, revision, verbose, rebuild)

    @staticmethod
    def _fix_latex_end_verbatim(latex_file: pathlib.Path) -> None:
        """CCL-1037, CCL-1138: ensure \end{sphinxVerbatim} is on a new line after a closing '}' as
        this can cause an "empty verbatim" error in the FancyVerb package when converting to PDF."""
        with latex_file.open(mode="r+", encoding="utf8") as f:
            data = f.read()
        with latex_file.open(mode="w", encoding="utf8") as f:
            f.write(data.replace("}\end{sphinxVerbatim}", "}\n\end{sphinxVerbatim}"))

    def _determine_latex_index_file(self):
        """Detect index_latex.rst, use as latex build index file if available.
        For use in the sphinx configuration file, strip the file extension.
        """

        latex_index_file = pathlib.Path(self.project_info.path, BuilderBase.LATEX_INDEX_FILE)
        if latex_index_file.is_file():
            logging.info(f"using latex index file '{latex_index_file}'")
            return BuilderBase.LATEX_INDEX_FILE.replace(".rst", "")

        return BuilderBase.INDEX_FILE.replace(".rst", "")

    def _determine_exclude_patterns(self) -> str:
        """ Ignore the optional index_latex.rst file in non-pdf builds.
        The file does not cause severe issues here unlike in meta project
        builds but still results in a otherwise not needed index_latex.html
        file in the HTML output.
        """

        latex_index_file = pathlib.Path(self.project_info.path, BuilderBase.LATEX_INDEX_FILE)
        if latex_index_file.is_file() and self.builder != "pdf":
            # Do not ** wildcard, file should be at project root level
            return BuilderBase.LATEX_INDEX_FILE

        return ""

    def build(self) -> None:
        """Build documentation."""

        if not self._allow_build(self.project_info.config.id):
            return

        output_path = self._setup_output_path(self.project_info.config.id)
        latex_index = self._determine_latex_index_file()
        exclude_patterns = self._determine_exclude_patterns()

        ################
        # Create conf.py
        ################

        config = SphinxConfigWriter(project_path=self.project_info.path, output_path=self.project_info.path,
                                    env=self.env)
        config.write(exclude_patterns=exclude_patterns, latex_index=latex_index, latex_file=self.project_info.config.id,
                     title=self.project_info.config.title, revision=self.revision,
                     vendor=self._extract_vendor_from_id(self.project_info.config.id))

        ##############
        # RUN SETUP
        ##############

        self._run_build_actions(project_info=self.project_info)

        # "pdf" is not a sphinx target, translate to latex
        sphinx_builder = self.builder
        if self.builder == "pdf":
            sphinx_builder = "latex"
        elif self.builder in ["zip", "tar"]:
            sphinx_builder = "html"

        ##############
        # RUN SPHINX
        ##############

        SphinxTool.run(sphinx_builder, self.project_info.path, output_path)

        ##############
        # RUN ZIP/HTML
        ##############

        if self.builder == "zip":
            self._zip_html_output(output_path)
            return
        elif self.builder == "tar":
            self._tar_html_output(output_path)
            return

        ##############
        # RUN PDF
        ##############

        if self.builder != "pdf":
            return

        latex_out_path = pathlib.Path(output_path, "latex")
        latex_file_path = pathlib.Path(latex_out_path, self.project_info.config.id + ".tex")

        # Check that the .tex file exists first.
        if not latex_file_path.is_file():
            logging.error("latex file %s not found" % self.project_info.config.id)
            return

        logging.info(f"fixing verbatim in {latex_file_path}")
        self._fix_latex_end_verbatim(latex_file_path)

        # Build PDF, verify it has been written.
        PdfTool.run(latex_file_path, latex_out_path)

        # Copy PDF file to output/[project_id] so the PDF output file
        # is located at the same output directory level as zip builds.
        # The idea is that a calling script does not need to know about
        # the [project_id]/latex subfolder structure exclusive to pdf builds.
        pdf_file_path = pathlib.Path(latex_out_path, self.project_info.config.id + ".pdf")
        if pdf_file_path.exists():
            pdf_file_path_copied = pathlib.Path(output_path, self.project_info.config.id + ".pdf")
            shutil.move(pdf_file_path, pdf_file_path_copied)
