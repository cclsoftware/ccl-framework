#!/usr/bin/python
"""Makedoc actions module."""

import pathlib
import shutil
import subprocess
from abc import abstractmethod, ABC
from typing import Optional, List

from modules.buildenv import BuildEnvironment
from modules.project import BuildActionConfig
from modules.tools import ToolHelper, Tools

__copyright__ = "Copyright (c) 2024 CCL Software Licensing GmbH"


class BuildActionException(Exception):
    """Exception raised for errors in the action implementation."""

    def __init__(self, message: str):
        super().__init__(message)


class IBuildAction(ABC):
    """Build action interface."""

    @abstractmethod
    def run(self) -> bool:
        """Run this action, raise BuildActionException
        on error.

        Returns:
            True on success, False on failure.
        """
        pass


class BuildAction(IBuildAction):
    """Common build action traits and features. Based on build action config,
    has access to build environment."""

    def __init__(self, action_config: BuildActionConfig, build_env: BuildEnvironment,
                 project_path: pathlib.Path) -> None:
        super().__init__()
        self.action_config: BuildActionConfig = action_config
        self.build_env: BuildEnvironment = build_env
        self.project_path: pathlib.Path = project_path

    def run(self) -> bool:
        return False

    def _find_tool(self, name: str, category: str) -> Optional[pathlib.Path]:
        """Find tool, log failure. Final, do not override. Override _find_tool_internal () instead."""

        tool = self._find_tool_internal(name, category)
        if not tool or not tool.is_file():
            raise BuildActionException(f"failed to find tool '{name}'")

        return tool

    def _find_tool_internal(self, name: str, category: str) -> Optional[pathlib.Path]:
        """Internal tool lookup, override this in derived classes."""
        return None

    @staticmethod
    def _make_absolute_path(reference_path: pathlib.Path, path: pathlib.Path) -> pathlib.Path:
        """Combine reference_path and path to an absolute path. Do nothing if path is already absolute."""

        if path.is_absolute():
            return path

        assert reference_path.is_absolute()
        return reference_path.joinpath(path).resolve()

    @staticmethod
    def _run_process(invocation: List[str], verbose: bool, working_dir: pathlib.Path) -> bool:
        """Call subprocess on 'invocation' with optional output.

        Uses project path as working_dir typically as configuration files are specified
        from documentation project perspective (i.e. contained relative paths).
        """

        if verbose:
            result_code = subprocess.call(invocation, cwd=working_dir)
        else:
            result_code = subprocess.call(invocation, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                          cwd=working_dir)
        return result_code == 0

    @staticmethod
    def _delete_files_in_folder(path: pathlib.Path, file_extension: str) -> None:
        pattern = f"*.{file_extension}"
        [f.unlink() for f in path.glob(pattern) if f.is_file()]

    def _process_generic_args(self, args: List[str]) -> List[str]:
        """Give path arguments an absolute path from project path
        perspective, ignore switches etc."""

        result = []
        for arg in args:
            if arg.startswith("-"):
                result.append(arg)
            else:
                result.append(self._make_absolute_path(self.project_path, pathlib.Path(arg)))

        return result


class DoxygenAction(BuildAction):
    """Generate doxygen XML output.

    JSON attributes:
        config: path to doxygen configuration file (.doxy file)
        output_path: path to doxygen xml output (for cleanup)

    Attribute output_path does not impact the output location and is solely
    for cleanup purpose. Configure the output path in the .doxy file instead.

    Doxygen is invoked using the location of the .doxy file as working directory.
    """

    ACTION_ID = "doxygen"  # action identifier
    TOOL_BINARY = "doxygen"  # doxyrest binary tool name, expect as system tool

    ATTR_CONFIG = "config"  # doxy file
    ATTR_OUTPUT_PATH = "output_path"  # doxygen xml output path

    def run(self) -> bool:
        super().run()

        # Step 1: Cleanup any potential previous xml output.
        output_path_attr_value = self.action_config.attrs.get(DoxygenAction.ATTR_OUTPUT_PATH)
        if output_path_attr_value:
            output_path_abs = self._make_absolute_path(self.project_path, pathlib.Path(output_path_attr_value))
            self._delete_files_in_folder(output_path_abs, "*")

        # Step 2: Run doxygen on specified configuration file. Use the location of the .doxy file
        # as the working directory as the doxy file is configured relatively to that location.
        config_attr_value = self.action_config.attrs.get(DoxygenAction.ATTR_CONFIG, "")
        if not config_attr_value:
            raise BuildActionException("config not specified")

        config_path_abs = self._make_absolute_path(self.project_path, pathlib.Path(config_attr_value))
        if not config_path_abs.is_file():
            raise BuildActionException(f"config '{config_path_abs}' does not exist")

        config_folder = config_path_abs.parent
        config_filename_only = config_path_abs.name

        # Legacy, must run in folder that contains the config file.
        invocation = [DoxygenAction.TOOL_BINARY, config_filename_only]
        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=invocation, verbose=verbose, working_dir=config_folder)


class DoxyrestAction(BuildAction):
    """Convert doxygen XML output with doxyrest tool.

     JSON attributes
        config: path to doxyrest config file
        output_path: path to doxyrest RST output
    """

    ACTION_ID = Tools.DOXYREST
    TOOL_BINARY = Tools.DOXYREST

    ATTR_CONFIG = "config"  # doxyrest configuration file path
    ATTR_OUTPUT_PATH = "output_path"  # doxyrest rst file output path

    def run(self) -> bool:
        super().run()

        # Step 1: cleanup any potential previous rst output.
        output_path_attr_value = self.action_config.attrs.get(DoxyrestAction.ATTR_OUTPUT_PATH)
        if output_path_attr_value:
            output_path_abs = self._make_absolute_path(self.project_path, pathlib.Path(output_path_attr_value))
            self._delete_files_in_folder(output_path_abs, "rst")

        doxyrest = self._find_tool(name=DoxyrestAction.TOOL_BINARY, category=Tools.CATEGORY_DOCTOOLS)
        if not doxyrest.is_file():
            raise BuildActionException("doxyrest tool not found")

        config_attr_value = self.action_config.attrs.get(DoxyrestAction.ATTR_CONFIG, "")
        if not config_attr_value:
            raise BuildActionException("config attribute not specified")

        # Step 2: run doxyrest tool with provider configuration file
        config_path_abs = self._make_absolute_path(self.project_path, pathlib.Path(config_attr_value))
        if not config_path_abs.is_file():
            raise BuildActionException(f"config '{config_path_abs}' does not exist")

        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=[doxyrest, "-c", config_path_abs],
                                 verbose=verbose, working_dir=self.project_path)

    def _find_tool_internal(self, name: str, category: str) -> Optional[pathlib.Path]:
        return self.build_env.tool_helper.find_binary_tool(name=name, category=category)


class TypeDocAction(BuildAction):
    """Perform TypeDoc build on a TypeScript project.

    JSON attributes:
        options: TypeDoc options file
        tsconfig: TypeScript project config file
        workdir: TypeDoc invocation working directory

    Use of workdir affects paths to config files.
    """

    ACTION_ID = "typedoc"  # action identifier
    TOOL_BINARY = "typedoc"  # typedoc npm command, expected as system tool

    ATTR_OPTIONS = "options"  # typedoc options file, optional
    ATTR_TSCONFIG = "tsconfig"  # typedoc ts configuration file, required
    ATTR_WORKDIR = "workdir"  # working directory

    def run(self) -> bool:
        super().run()

        workdir = self.project_path
        workdir_attr_value = self.action_config.attrs.get(TypeDocAction.ATTR_WORKDIR, "")
        if workdir_attr_value:
            workdir = self._make_absolute_path(workdir, pathlib.Path(workdir_attr_value))
            if not workdir.exists():
                raise BuildActionException(f"working directory '{workdir}' does not exit")

        config_attr_value = self.action_config.attrs.get(TypeDocAction.ATTR_TSCONFIG, "")
        if not config_attr_value:
            raise BuildActionException("config not specified")

        # Reminder: config path is specified from doc project path perspective, not
        # from working dir perspective
        config_path_abs = self._make_absolute_path(workdir, pathlib.Path(config_attr_value))
        if not config_path_abs.is_file():
            raise BuildActionException(f"config '{config_path_abs}' does not exist")

        # Legacy, must run in folder that contains the config file.
        npm_command = self.build_env.tool_helper.get_npm_command(TypeDocAction.TOOL_BINARY)
        invocation = [npm_command]

        # May have additional options attr
        options_attr_value = self.action_config.attrs.get(TypeDocAction.ATTR_OPTIONS, "")
        if options_attr_value:
            options_path_abs = self._make_absolute_path(workdir, pathlib.Path(options_attr_value))
            if not options_path_abs.is_file():
                raise BuildActionException(f"options '{options_path_abs}' does not exist")

            invocation.extend(["--options", options_path_abs])

        invocation.extend(["--tsconfig", config_path_abs])
        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=invocation, verbose=verbose, working_dir=workdir)


class ModelScanAction(BuildAction):
    """Perform classmodel scan using modeller tool.

    JSON attributes:
        path: sources path to scan
        model: classmodel file to scan for
    """

    ACTION_ID = "modelscan"
    TOOL_BINARY = Tools.CCLMODELLER

    ATTR_PATH = "path"
    ATTR_MODEL = "model"

    def run(self) -> bool:
        super().run()

        modeller = self._find_tool(name=ModelScanAction.TOOL_BINARY, category=Tools.CATEGORY_CCL)
        if not modeller.is_file():
            raise BuildActionException(f"{ModelScanAction.TOOL_BINARY} tool '{modeller}' not found")

        path_attr_value = str(self.action_config.attrs.get(ModelScanAction.ATTR_PATH, []))
        if not path_attr_value:
            raise BuildActionException("scan path not specified")

        scan_path = self._make_absolute_path(self.project_path, pathlib.Path(path_attr_value))
        if not scan_path.exists():
            raise BuildActionException("scan path does not exist")

        model_attr_value = self.action_config.attrs.get(ModelScanAction.ATTR_MODEL, "")
        if not model_attr_value:
            raise BuildActionException("classmodel not specified")

        model_path = self._make_absolute_path(self.project_path, pathlib.Path(model_attr_value))
        if not model_path.is_file():
            raise BuildActionException(f"classmodel '{model_path}' does not exist")

        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=[modeller, "-scan", scan_path, model_path],
                                 verbose=verbose, working_dir=self.project_path)

    def _find_tool_internal(self, name: str, category: str) -> Optional[pathlib.Path]:
        return self.build_env.tool_helper.find_binary_tool(name=name, category=category)


class PythonScriptAction(BuildAction):
    """Run Python script tool.

    JSON attributes:
        args: script invocation arguments, passed to subprocess.call ()
    """

    ACTION_IDS = [
        Tools.CLASSMODELRST,
        Tools.CLASSMODELDTS,
        Tools.CMAKEDOC,
        Tools.CREFDBFIX,
        Tools.TYPEDOCRST,
        Tools.CODESTATS
    ]

    ATTR_ARGS = "args"  # script arguments

    def __init__(self, action_config: BuildActionConfig, build_env: BuildEnvironment, project_path: pathlib.Path,
                 script_name: str):
        super().__init__(action_config, build_env, project_path)
        self.script_name: str = script_name

    def run(self) -> bool:
        super().run()

        args_attr_value = self.action_config.attrs.get(PythonScriptAction.ATTR_ARGS, [])
        if not args_attr_value:
            raise BuildActionException("missing args configuration")

        # Intended as 'doctools' script wrapper so use doctools as category.
        script = self._find_tool(name=self.script_name, category=Tools.CATEGORY_DOCTOOLS)
        if not script.is_file():
            return False

        # Needs python3, running platform may use python3 via 'python' alias (typically Windows).
        python_executable = shutil.which('python') or shutil.which('python3')
        if python_executable is None:
            raise BuildActionException("no suitable Python executable found")

        invocation = [python_executable, script]
        processed_args = self._process_generic_args(args_attr_value)
        for arg in processed_args:
            invocation.append(arg)

        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=invocation, verbose=verbose, working_dir=self.project_path)

    def _find_tool_internal(self, name: str, category: str) -> Optional[pathlib.Path]:
        return self.build_env.tool_helper.find_python_tool(name=self.script_name, category=category)


class CustomToolAction(BuildAction):
    """Run custom binary tool.

    JSON attributes:
        name: name of the tool executable
        args: tool invocation arguments, passed to subprocess.call()
    """

    ACTION_ID = "customtool"

    ATTR_TOOL_NAME = "name"  # name of tool to use
    ATTR_TOOL_CATEGORY = "category"  # tool category, for use with tool lookup, optional
    ATTR_ARGS = "args"  # args required

    def run(self) -> bool:
        super().run()

        name_attr_value = self.action_config.attrs.get(CustomToolAction.ATTR_TOOL_NAME, "")
        if not name_attr_value:
            raise BuildActionException("tool not specified")

        category_attr_value = self.action_config.attrs.get(CustomToolAction.ATTR_TOOL_CATEGORY, "")

        # Handle platform specific binary tool
        tool_path = self._find_tool(name=name_attr_value, category=category_attr_value)
        if not tool_path.is_file():
            raise BuildActionException(f"tool {tool_path} does not exist")

        args_attr_value = self.action_config.attrs.get(CustomToolAction.ATTR_ARGS, [])
        if not args_attr_value:
            raise BuildActionException("args not specified")

        invocation = [tool_path]
        processed_args = self._process_generic_args(args_attr_value)
        for arg in processed_args:
            invocation.append(arg)

        verbose = self.action_config.attrs.get("verbose", False)
        return self._run_process(invocation=invocation, verbose=verbose, working_dir=self.project_path)

    def _find_tool_internal(self, name: str, category: str) -> Optional[pathlib.Path]:
        return self.build_env.tool_helper.find_binary_tool(name=name, category=category)


class ActionFactory:

    @staticmethod
    def create(config: BuildActionConfig, env: BuildEnvironment, project_path: pathlib.Path) -> Optional[IBuildAction]:
        """Attempt to construct build action from given action configuration.

        Returns:
            Action ID pertaining object if the action ID from the configuration is known, None otherwise.
        """

        if config.action == ModelScanAction.ACTION_ID:
            return ModelScanAction(config, env, project_path)
        elif config.action == DoxygenAction.ACTION_ID:
            return DoxygenAction(config, env, project_path)
        elif config.action == DoxyrestAction.ACTION_ID:
            return DoxyrestAction(config, env, project_path)
        elif config.action == TypeDocAction.ACTION_ID:
            return TypeDocAction(config, env, project_path)
        elif config.action == CustomToolAction.ACTION_ID:
            return CustomToolAction(config, env, project_path)
        elif config.action in PythonScriptAction.ACTION_IDS:
            return PythonScriptAction(config, env, project_path, config.action)

        return None
