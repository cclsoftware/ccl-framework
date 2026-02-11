"""ClassModel data to-string converter."""

from typing import Dict

from modules.classmodel.elements import BaseElement
from modules.classmodel.model import Model
from modules.rstprinter import *
from modules.util import *

__copyright__ = "Copyright (c) 2023 CCL Software Licensing GmbH"


class ClassModelPrinter:
    """
    Convert model data to RST conform string.
    """

    @staticmethod
    def to_rst_string(elements: Dict[str, BaseElement], list_title: str, pref: str) -> str:
        """
        Convert all elements of a specific elements list to RST. Generate an anchor for each element.
        """

        def element_to_string(element_key: str, print_hr: bool) -> str:
            """ Print the element for 'element_key', also print a hr for 'print_hr = true'. """
            result = ""
            e = elements.get(element_key, None)
            if e is None:
                raise ValueError

            # add horizontal line above the element
            if print_hr:
                result += RstPrinter.hr()
            result += RstPrinter.anchor(
                ElementUID.create(pref, e.kind, e.name))
            result += "%s\n\n\n" % e.to_rst()

            return result

        if elements is None or len(elements.values()) <= 0:
            return ""

        result_str = RstPrinter.h2(list_title)

        # print all elements, add a hr on top of each, but only for element 2, ... , n
        # to avoid a sphinx warning about document ending with a transition (i.e. horizontal rule)
        is_first_element = True
        for key in sorted(elements.keys()):
            if is_first_element:
                result_str += element_to_string(key, False)
                is_first_element = False
            else:
                # Disabled hr for now. Structural elements should be avoided for
                # PDF output. TODO: use ".. only::" directive?
                result_str += element_to_string(key, False)

        return result_str

    @staticmethod
    def model_data_to_string(model: Model, anchor: str) -> str:

        # print all groups, classes, enums, objects; skips empty containers
        out = RstPrinter.h1("%s Reference" % model.root_name)
        for entry_list, title in zip(
                [model.doc_groups, model.displayed_schema_groups, model.classes, model.enumerations,
                 model.objects_hierarchy],
                ["Groups", "Schema Groups", "Classes", "Enums", "Objects"]):
            out += ClassModelPrinter.to_rst_string(entry_list, title, anchor)

        out += RstPrinter.latex_clear_page()
        return out


class ClassModelRefPrinter:
    """
    Print classmodel top level elements as links.
    """

    @staticmethod
    def _to_rst_string(elements: Dict[str, BaseElement], pref: str) -> str:
        """
        Convert all elements of a specific elements list to RST. Generate an anchor for each element.
        """

        def element_to_ref_string(element_key: str) -> str:
            """
            Create shortcut reference for element.
            """

            result = ""
            e = elements.get(element_key, None)
            if e is None:
                raise ValueError

            placeholder = str("%s.%s.%s" % ("xml", e.kind, e.name)).lower()
            replacement = RstPrinter.hyperlink(e.name,
                                               ElementUID.create(pref, e.kind, e.name))
            result += RstPrinter.replace(placeholder, "<%s>" % replacement)
            return result

        if elements is None or len(elements.values()) <= 0:
            return ""

        result_str = ""
        for key in sorted(elements.keys()):
            result_str += element_to_ref_string(key)

        return result_str

    @staticmethod
    def model_data_to_string(model: Model, anchor: str) -> str:
        """
        Generate links for all groups, classes, enums, objects, skip empty containers.
        """

        out = RstPrinter.comment("%s Reference" % model.root_name)
        for entry_list, title in zip(
                [model.doc_groups, model.classes, model.enumerations, model.objects_hierarchy],
                ["Groups", "Classes", "Enums", "Objects"]):
            out += ClassModelRefPrinter._to_rst_string(entry_list, anchor)

        return out


class ClassModelHtmlRefPrinter:
    """
    Utility class for generating HTML hyperlinks to reference elements in documentation.
    Provides static methods to convert model elements into HTML-formatted hyperlinks.
    """

    @staticmethod
    def _to_rst_string(elements: Dict[str, BaseElement], pref: str) -> str:
        def element_to_ref_string(element_key: str) -> str:
            """ Create hyperlink string for given element, i.e. <a href .... />. """

            result = ""
            e = elements.get(element_key, None)
            if e is None:
                raise ValueError

            # sphinx build converts rst anchors to lower case and replaces "." with "-".
            # Manually adjust the anchors here, so they match their HTML output counterparts.

            href_target = ElementUID.create(pref, e.kind, e.name).lower().replace(".", "-")
            result += f"<a href=\"documentation://{href_target}\">{e.name}</a> ({e.kind})<br>\n"
            return result

        if elements is None or len(elements.values()) <= 0:
            return ""

        result_str = ""
        for key in sorted(elements.keys()):
            result_str += element_to_ref_string(key)

        return result_str

    @staticmethod
    def model_data_to_string(model: Model, anchor: str) -> str:
        out = "<html><body>\n"
        for entry_list, title in zip(
                [model.doc_groups, model.classes, model.enumerations, model.objects_hierarchy],
                ["Groups", "Classes", "Enums", "Objects"]):
            out += ClassModelHtmlRefPrinter._to_rst_string(entry_list, anchor)

        out += "</body></html>"
        return out


class ClassModelIndexPrinter:
    """
    Print class model top level elements as an index file which can be
    used as a base file for setting up an exclusion list.
    """

    @staticmethod
    def _to_rst_string(elements):
        """
        Convert all elements to a list of top-level elements:

        class.Foo
        enumeration.Bar
        etc.
        """

        def element_to_string(element_key: str) -> str:
            """
            Print existing element as type.name.
            """

            e = elements.get(element_key, None)
            if e is None:
                raise ValueError

            return "%s.%s\n" % (e.kind, e.name)

        if elements is None or len(elements.values()) <= 0:
            return ""

        result_str = ""
        for key in sorted(elements.keys()):
            result_str += element_to_string(key)

        return result_str

    @staticmethod
    def model_data_to_string(model: Model) -> str:
        """
        Generate index for all groups, classes, enums and objects.
        """

        out = "# Index: %s\n\n" % model.get_root_name()
        for entry_list, title in zip(
                [model.get_doc_groups(), model.get_classes(), model.get_enumerations(), model.get_objects_hierarchy()],
                ["Groups", "Classes", "Enums", "Objects"]):
            out += "###################\n"
            out += "# %s\n" % title
            out += "###################\n\n"
            out += ClassModelIndexPrinter._to_rst_string(entry_list)
            out += "\n"

        return out
