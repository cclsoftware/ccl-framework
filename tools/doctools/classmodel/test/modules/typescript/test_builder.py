"""Tests for TypeScript DeclarationBuilder class."""

import pathlib
import unittest
from typing import List

# must be relative to project root
from modules.classmodel.model import Model
from modules.classmodel.parser import Parser
from modules.classmodel.elementfilter import ElementFilter
from modules.typescript.builder import DeclarationBuilder, DtsElementFilter

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class DeclarationBuilderTest(unittest.TestCase):
    """ Test DeclarationBuilder class generated TypeScript. """

    TEST_RESOURCE_PATH = "test/resource/typescript/"

    TEST_MODELS = [
        "Cross-platform Security Framework.classModel",
        "Cross-platform System Framework.classModel",
        "Cross-platform GUI Framework.classModel"
    ]

    @staticmethod
    def _load_models(classmodels: List[str], ref_prefix: str) -> Model:
        """ Read filename classmodel file and return the model data. """
        model = Model()
        element_filter = ElementFilter()
        parser = Parser(ref_prefix=ref_prefix, external_files_path="", model=model, element_filter=element_filter)

        for classmodel in classmodels:
            filename = f"{DeclarationBuilderTest.TEST_RESOURCE_PATH}{classmodel}"
            parser.parse_file(filename)

        return model

    def run_test(self, model: Model, namespace: str, ref_file: str, dts_filter: DtsElementFilter) -> None:
        """ Run the test for variable model and reference file. """

        builder = DeclarationBuilder(model=model, root_namespace=namespace, element_filter=dts_filter,
                                     build_comments=False, app_name="TestApp")
        dts = builder.run()
        result_string = dts.to_string()

        # Write result to output file.
        output_file_name = f"{ref_file}.out"
        result_file = pathlib.Path(DeclarationBuilderTest.TEST_RESOURCE_PATH, output_file_name)
        with result_file.open(mode='w') as f:
            f.write(result_string)

        # Test verification: compare output to reference file.
        ref_file_path = pathlib.Path(DeclarationBuilderTest.TEST_RESOURCE_PATH, f"{ref_file}.ref")
        with ref_file_path.open(mode='r') as f:
            self.assertEqual(result_string, f.read())

    def test_classmodel_export(self):
        """ Create typings file from multiple classmodels with a classes filter.
        The classes whitelist is a random selection of CCL types occurring in any
        of the input model files.
        """

        model = self._load_models(DeclarationBuilderTest.TEST_MODELS, "ccl-test")
        exported_classes = [
            "AliasParam",
            "Attributes",
            "BitmapFilter",
            "ClassDescription",
            "ColorParam",
            "Command",
            "CommandParam",
            "CommandTable",
            "Component",
            "DateTime",
            "EngineHost",
            "FileArchive",
            "FileResource",
            "FileTypeRegistry",
            "FloatParam",
            "Formatter",
            "GraphicsHelper",
            "Image",
            "ImageProvider",
            "IntParam",
            "Iterator",
            "Key",
            "KeyEvent",
            "ListParam",
            "LocaleManager",
            "LocaleManager.TranslationTable",
            "MediaTrackFormat",
            "MenuParam",
            "Message",
            "Object",
            "ObjectNode",
            "ObjectTable",
            "PackageFile",
            "ParamContainer",
            "Parameter",
            "PlugInCollection",
            "PlugInManager",
            "SecurityHost",
            "Settings",
            "ScriptGUIHost",
            "ScriptingHost",
            "ScriptingHost.Console",
            "ScriptingHost.InterfaceList",
            "ScriptingHost.ResultsList",
            "ScriptingHost.ScriptableIO",
            "ScriptingHost.Signals",
            "ServiceManager",
            "StringParam",
            "SystemInformation",
            "TextFile",
            "UID",
            "Url",
            "XmlSettings"
        ]

        exported_objects = [
            ".*"
        ]

        export_filter = DtsElementFilter(classes=exported_classes, objects=exported_objects)

        self.run_test(model=model, namespace="", ref_file="ccl.d.ts", dts_filter=export_filter)

    if __name__ == "__main__":
        unittest.main()
