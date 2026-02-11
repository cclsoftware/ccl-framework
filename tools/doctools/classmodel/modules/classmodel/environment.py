"""Environment classes module."""

from abc import abstractmethod, ABC

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class Environment(ABC):

    @abstractmethod
    def get_anchor_prefix(self) -> str:
        pass

    @abstractmethod
    def get_external_files_path(self) -> str:
        pass


class DocEnv(Environment):

    anchor_prefix: str
    external_files_path: str

    def __init__(self, anchor_prefix: str, external_files_path: str) -> None:
        self.anchor_prefix = anchor_prefix
        self.external_files_path = external_files_path

    def get_anchor_prefix(self) -> str:
        return self.anchor_prefix

    def get_external_files_path(self) -> str:
        return self.external_files_path
