#!/usr/bin/python
"""Makedoc project module.
"""

import json
import logging
import os
import pathlib
import sys
from dataclasses import dataclass
from json import JSONDecodeError
from typing import List, Optional, Dict

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"

from modules.buildenv import BuildEnvironment

sys.path.append(os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "../../../../../ccl/extras/python"))

from tools.repoinfo import RepositoryInfo


@dataclass
class BuildActionConfig:
    action: str  # name of the action, must be known to action factory
    description: str  # display text
    attrs: Dict[str, any]  # all attributes


@dataclass
class ProjectConfig:
    """Documentation project configuration, provided as makedoc.json file."""

    id: str  # unique project identifier, example: "dev.ccl.doc.someproject"
    title: str  # document title
    meta: bool  # is a meta document combining multiple projects
    build: List[BuildActionConfig]  # list of additional build steps to perform


@dataclass
class ProjectConfigInfo:
    """Group ProjectConfig data with path to project."""

    config: ProjectConfig  # data from makedoc.json
    path: pathlib.Path  # path to documentation project, folder that contains makedoc.json


class ProjectHandler:
    """ Manage projects. Collects all project configs found in a
     repository structure and caches them. Provides the path to
     project via ProjectConfigInfo.
     """

    CONFIG_FILE = "makedoc.json"

    def __init__(self, env: BuildEnvironment):
        self.env = env
        self.projects: Dict[str, ProjectConfigInfo] = dict()
        self._collect_configs()

    def get_project_config(self, project_id: str) -> Optional[ProjectConfig]:
        """Lookup project config by project id."""

        info = self.projects.get(project_id, None)
        if info is None:
            return None

        return info.config

    def get_project_info(self, project_id: str) -> Optional[ProjectConfigInfo]:
        """Lookup project info by project id."""

        return self.projects.get(project_id, None)

    def get_project_path(self, project_id: str) -> Optional[pathlib.Path]:
        """Lookup project path by project id."""

        info = self.projects.get(project_id, None)
        if info is None:
            return None

        return info.path

    def list_projects(self) -> None:
        """Print all known projects by id and path."""

        if not self.projects:
            self._collect_configs()

        for project_id, project_info in self.projects.items():
            logging.info(f"id='{project_id}', path='{project_info.path}'")

    def get_all_project_ids(self) -> List[str]:
        """Get all known project ids as a flat list."""

        return list(self.projects.keys())

    def _collect_configs(self) -> None:
        """Lookup all or a specific documentation projects with respect
        to current repository structure. This should be called once
        during init () only.
        """

        self.projects.clear()
        for doc_path in ProjectHandler._get_documentation_paths(self.env):
            for subdir, dirs, files in os.walk(doc_path):

                # Attempt to load makedoc.json file.
                project_path = pathlib.Path(doc_path, subdir).resolve()
                project_config = ProjectHandler._load_project_config(project_path)
                if project_config is None:
                    continue

                # Add to projects database.
                info = ProjectConfigInfo(config=project_config, path=project_path)
                self.projects[project_config.id] = info

    @staticmethod
    def _load_project_config(path: pathlib.Path) -> Optional[ProjectConfig]:
        filename = pathlib.Path(path, ProjectHandler.CONFIG_FILE)
        try:
            with filename.open(mode="r") as f:
                data = json.load(f)

                build_steps_raw = data.get("build", list())
                build_steps_imported: List[BuildActionConfig] = list()
                for step_def in build_steps_raw:
                    build_steps_imported.append(
                        BuildActionConfig(
                            action=step_def.get("action"),
                            description=step_def.get("description"),
                            attrs=step_def
                        )
                    )

            return ProjectConfig(
                id=data.get("id", ""),
                title=data.get("title", ""),
                meta=data.get("meta", False),
                build=build_steps_imported
            )

        except OSError:
            # This function may be used to probe directories for existence
            # of a makedoc.json file. Ignore this 'file-not-found' type
            # error. Expect caller to check for None result.
            pass
        except JSONDecodeError as e:
            logging.error(f"malformed makedoc.json file '{filename}': {e}")

        return None

    @staticmethod
    def _get_documentation_paths(env: BuildEnvironment) -> List[pathlib.Path]:
        """Lookup 'documentation' folders based on repository context:
        - CCL framework repo context: use CCL framework repo 'documentations' folder
        - meta repo context: use all 'documentation' folders provided by RepositoryInfo
        """

        # CCL framework repo context.
        doc_paths = [RepositoryInfo.get_ccl_documentation_path()]

        # Optional: meta repo context.
        if env.repo_info:
            doc_paths = env.repo_info.get_paths(RepositoryInfo.CATEGORY_DOCUMENTATION)

        return doc_paths
