"""Tests for TypeScript DeclarationBuilder class."""

import pathlib
import unittest
from typing import List

# must be relative to project root
from modules.classmodel.model import Model
from modules.classmodel.parser import Parser
from modules.classmodel.elementfilter import ElementFilter
from modules.typescript.builder import DeclarationBuilder, DtsElementFilter

__copyright__ = "Copyright (c) 2026 CCL Software Licensing GmbH"


class DeclarationBuilderTest(unittest.TestCase):
    """ Test DeclarationBuilder class generated TypeScript. """

    TEST_RESOURCE_PATH = "test/resource/typescript/"

    TEST_MODELS = [
        "Cross-platform Security Framework.classModel",
        "Cross-platform System Framework.classModel",
        "Cross-platform GUI Framework.classModel"
    ]

    @staticmethod
    def _load_models(classmodels: List[str]) -> Model:
        """ Read filename classmodel file and return the model data. """
        model = Model()
        element_filter = ElementFilter()
        parser = Parser(model=model, element_filter=element_filter)

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

        model = self._load_models(DeclarationBuilderTest.TEST_MODELS)
        exported_classes = [
            "CCL.AliasParam",
            "CCL.Attributes",
            "CCL.BitmapFilter",
            "CCL.ClassDescription",
            "CCL.ColorParam",
            "Command",  # no namespace in the test fixture: full name is the bare name
            "CCL.CommandParam",
            "CCL.CommandTable",
            "Component",  # not present in the test fixture, kept as a no-op entry
            "CCL.DateTime",
            "EngineHost",  # not present in the test fixture, kept as a no-op entry
            "CCL.FileArchive",
            "CCL.FileResource",
            "CCL.FileTypeRegistry",
            "CCL.FloatParam",
            "CCL.Formatter",
            "CCL.GraphicsHelper",
            "CCL.Image",
            "CCL.ImageProvider",
            "CCL.IntParam",
            "CCL.Iterator",
            "CCL.Key",
            "CCL.KeyEvent",
            "CCL.ListParam",
            "CCL.LocaleManager",
            "CCL.LocaleManager.TranslationTable",
            "MediaTrackFormat",  # not present in the test fixture, kept as a no-op entry
            "CCL.MenuParam",
            "CCL.Message",
            "CCL.Object",
            "CCL.ObjectNode",
            "CCL.ObjectTable",
            "CCL.PackageFile",
            "CCL.ParamContainer",
            "CCL.Parameter",
            "CCL.PlugInCollection",
            "CCL.PlugInManager",
            "CCL.SecurityHost",
            "CCL.Settings",
            "CCL.ScriptGUIHost",
            "CCL.ScriptingHost",
            "CCL.ScriptingHost.Console",
            "CCL.ScriptingHost.InterfaceList",
            "CCL.ScriptingHost.ResultsList",
            "CCL.ScriptingHost.ScriptableIO",
            "CCL.ScriptingHost.Signals",
            "CCL.ServiceManager",
            "CCL.StringParam",
            "CCL.SystemInformation",
            "CCL.TextFile",
            "CCL.UID",
            "CCL.Url",
            "CCL.XmlSettings"
        ]

        exported_objects = [
            ".*"
        ]

        export_filter = DtsElementFilter(classes=exported_classes, objects=exported_objects)

        self.run_test(model=model, namespace="", ref_file="ccl.d.ts", dts_filter=export_filter)

    def test_namespace_collision(self):
        """ A class name reused across namespaces (e.g. a global "Widget" and a
        "Foo::Bar::Widget") must not cause the second definition to be silently dropped,
        and an unqualified same-namespace type reference must resolve to the local class
        rather than the first-registered one.
        """

        model = self._load_models(["Namespace Collision.classModel"])
        export_filter = DtsElementFilter(classes=[".*"], objects=[])

        builder = DeclarationBuilder(model=model, root_namespace="", element_filter=export_filter,
                                     build_comments=False, app_name="TestApp")
        result = builder.run().to_string()

        # Both same-named classes survive registration and are exported.
        self.assertIn("declare class Widget extends CCL.Object", result)
        self.assertIn("export class Widget extends CCL.Object", result)

        # The method inside Foo.Bar.Widget resolves its own bare "Widget" return type to
        # itself, not to the unrelated global class.
        self.assertIn("getWidget: () => Widget;", result)

        # A reference outside both scopes falls back to the first-registered (global) class.
        self.assertIn("widget: Widget;", result)

    def test_full_qualified_name_filter(self):
        """ The classes filter matches by full, namespace-qualified name only. A bare name
        is no longer sufficient to select a namespaced class.
        """

        model = self._load_models(["Namespace Collision.classModel"])
        export_filter = DtsElementFilter(classes=["Foo.Bar.Widget"], objects=[])

        builder = DeclarationBuilder(model=model, root_namespace="", element_filter=export_filter,
                                     build_comments=False, app_name="TestApp")
        result = builder.run().to_string()

        # Only the explicitly qualified class is exported...
        self.assertIn("export class Widget extends CCL.Object", result)

        # ...the global "Widget" and the un-listed "Holder" are excluded.
        self.assertNotIn("declare class Widget", result)
        self.assertNotIn("Holder", result)

    if __name__ == "__main__":
        unittest.main()
