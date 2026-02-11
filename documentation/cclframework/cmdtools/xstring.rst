.. |custom model| replace:: :ref:`XML model file <custommodels>`
.. |auto formats| replace:: :ref:`auto scan supported format <autoscansupported>`


####################
CCL String Extractor
####################

*This document refers to :ref:`xstring version 1.2.0 <changelog>`*.


Introduction
============

The String Extractor tool parses strings from source and XML files and exports them to a file format suitable for localization.

**Usage:**

.. code-block:: rst

		xstring -[MODE] [INPUT_FOLDER] -[OUTPUT_FORMAT] [OUTPUT_FILE] [MODEL_PATH] -[OPTION]


.. list-table:: Command-line Arguments
   :widths: 25 10 65
   :header-rows: 1

   * - Argument
     - Required
     - Description
   * - MODE
     - No
     - Mode: {**skin**, **menu**, **tutorial**, **metainfo**, **template**, **custom**, **auto**, **code**}, see :ref:`parser modes<parsermodes>`, default: **auto**.
   * - INPUT_FOLDER
     - Yes
     - Input folder to parse, **must be a path to a folder**.
   * - OUTPUT_FORMAT
     - No
     - Output format: {**po**, **xliff**}, default: **po**.
   * - OUTPUT_FILE
     - Yes
     - Path to result file.
   * - MODEL_PATH
     - Yes/No
     - Specify |custom model|, see :ref:`parser modes <parsermodes>` for details.
   * - OPTION
     - No
     - Parser options: {**v** = print debug logs}.


.. _parsermodes:

.. list-table:: Parser Modes
   :widths: 15 35 10 40
   :header-rows: 1

   * - Mode
     - Description
     - Exclusive
     - Use of |custom model|
   * - skin
     - Parse <Skin> XML files.
     - Yes
     - Optional, overwrite <Skin> default.
   * - menu
     - Parse <MenuBar> XML files.
     - Yes
     - Optional, overwrite <MenuBar> default.
   * - tutorial
     - Parse <TutorialHelpCollection> XML files.
     - Yes
     - Optional, overwrite <TutorialHelpCollection> default.
   * - metainfo
     - Parse <MetaInformation> XML files.
     - Yes
     - Optional, overwrite <MetaInformation> default.
   * - template
     - Parse <DocumentTemplate> XML files.
     - Yes
     - Optional, overwrite <DocumentTemplate> default.
   * - custom
     - Parse custom format XML files.
     - Yes
     - **Required**, specify custom XML format.
   * - auto
     - Parse files of any supported content type, see |auto formats|
     - No
     - Optional, add additional formats, overwrite built-in formats.
   * - code
     - Parse source files, supported file extensions: .cpp, .h, .js, .mm.
     - Yes
     - Unsupported.


**Exclusive**: parse the specified content type only, ignore other formats even if supported.

.. tip::

  Run xstring without any arguments to get a usage description.


**Usage examples:**


Parse any supported file format from /path/, create result file /path/output.po in gettext format:

.. code-block:: rst

    xstring /path/ /path/output.po


Parse skin XML files from /path/to/skin, create result file /path/skin.po in gettext format:

.. code-block:: rst

    xstring -skin /path/to/skin -po /path/skin.po


Parse custom XML format from /path/to/custom, create result file /path/custom.po, use model definition from file custom.json, print debug logs:

.. code-block:: rst

    xstring -custom /path/to/custom -po /path/custom.po custom.json -v


.. _autoscansupported:

Auto scan
=========

Scan mode '-auto' allows to read any supported content type from a given input directy without the need to explicitly specify an expected input file format. It enables the user to extract strings from multiple files of different format in a single scan. Auto scan supports the following file formats:

.. list-table::
   :widths: 70 30
   :header-rows: 1

   * - Format Description
     - CCL Defined
   * - <Skin> XML
     - Yes
   * - <MenuBar> XML
     - Yes
   * - <TutorialHelpCollection> XML
     - Yes
   * - <MetaInformation> XML
     - Yes
   * - <DocumentTemplate> XML
     - Yes
   * - Android <resources> XML
     - No
   * - Annotated source code (.cpp, .h, .js, .mm)
     - Yes


.. _custommodels:

XML model format
================

Introduction
------------

The String Extractor tool supports parsing of custom XML based formats. Parser rules (XML models) are defined via a JSON model file. The file format is:

.. code-block:: json

  {
    "root":
    {
      "name": "SomeRootName", // Mandatory, expected root element name.
      "conditions":
      [
        // Optional, root element conditions (file is skipped on mismatch)
      ]
    },
    "extensions": "XML", // Mandatory, expected file extensions.
    "scope":
    {
      // Optional, scope rule applied for all matchers.
    },
    "matchers":
    [
      // List of matchers
    ]
  }

With the matcher structure being:

.. code-block:: json

  {
    // matcher attributes depending on matcher type, see reference below
    "scope":
    {
      // Optional, matcher specific scope rule (overrides model level scope)
    },
    "conditions":
    [
      // Optional, list of conditions, AND-combined
    ]
  }

**Model**

The ``root`` element ``name`` attribute denotes the expected XML file root element name. The String Extractor uses it to match XML files to models. The ``extensions`` value denotes the file extension for this model and causes the String Extractor to not skip this file type when traversing a directory. Note that different XML formats may share the same file extension.

**Matchers**

Matchers define which attributes to read from the XML file. Matchers are evaluated per XML node when the String Extractor traverses the XML tree. Matchers are OR-combined: a single XML node may satisfy multiple matchers resulting in multiple strings being returned from the same node.

**Conditions**

Conditions add additional constraints to a matcher. The parser requires all conditions to be satisfied to export a certain attribute. Conditions are AND-combined.

**Scopes**

Scopes define which string scope is exported. A scope rule can be defined on model or matcher level. Matcher level rules override the model level (global) rule. If a matcher level rule can not return a scope, the parser attempts to fallback to the model level scope.


.. tip::

  Simplified examples for these concepts can be found :ref:`here <modelexamples>`, a production example file can be found :ref:`here <skinmodel>`.


Features
--------

Some model convenience features:

**Combining models**

A single model JSON file may contain multiple models:

.. code-block:: json

  {
    "models":
    [
      {
        // model 1
      },
      {
        // model 2
      }
    ]
  }


**Model inheritance**

A model for which a ``root`` is set typically adds support for this XML format or replaces an existing model for this format entirely. If a built-in model should be reused but for a different file extension, the ``inherit`` attribute can be used over ``root``:

.. code-block:: json

  {
	  "inherit": "DocumentTemplate",
	  "extensions": "apptemplate"
  }

In this example, the parser loads the *DocumentTemplate* built-in format but replaces its associated file extension to *apptemplate*. Note that if ``inherit`` is set, further configuration attributes such as scope rules or matchers will be ignored.

Avoid specifying multiple models for the same format as the parser can register a single model per format only. When the parser loads a model it may overwrite an existing one if the format (root or inherited) was previously registered. Thus, if the same model is to be used for multiple file extensions, specify the extensions as a list but in a single model definition:

.. code-block:: json

  // Correct: both extensions are registered for <DocumentTemplate>
  {
    "inherit": "DocumentTemplate",
    "extensions":
    [
      "apptemplate1",
      "apptemplate2"
    ]
  }

  // Wrong: only apptemplate2 is registered for <DocumentTemplate>
  "models":
  [
    {
      "inherit": "DocumentTemplate",
      "extensions": "apptemplate1"
    },
    {
      "inherit": "DocumentTemplate",
      "extensions": "apptemplate2"
   },
  ]


XML model reference
===================

.. _modeloverview:

Overview
--------

**Matchers**: [:ref:`Attribute<attributematcher>`] [:ref:`Element<elementmatcher>`] **Conditions:** [:ref:`Attribute Sibling<attributecondition>`] [:ref:`Element Name<elementcondition>`] **Scopes:** [:ref:`Static Value<staticscope>`] [:ref:`Parent Element Attribute<parentscope>`]


------------

.. _attributematcher:

Matcher: attribute
------------------

Semantic: parse all attributes of a certain name, irregardless of the element.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "attribute"
* **name** {*string*}: required, name of the attribute to match, must be single value
* **options** {*string, stringlist*}: optional, string processing options

  * split: tokenize comma separated string

* **scope** {*scope handler*}: optional, define scope to export for this string
* **conditions** {*list of conditions*}: optional, matcher constraints

.. code-block:: json
  :caption: Example: parse all 'text' attributes.

  {
    "kind": "attribute",
    "name": "text",
    "options": "split",
    "scope":
    {
      // ...
    },
    "conditions":
    [
      // ...
    ]
  }

-> :ref:`Back to overview<modeloverview>`

------------

.. _elementmatcher:

Matcher: element
----------------

Semantic: Parse all elements of a certain name, read string from the element text or an element attribute.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "element"
* **name** {*string*}: required, name of the element to match
* **text** {*bool*}: optional, try to read string from element text before *attribute* (default: off)
* **attribute** {*string, stringlist*}: required for *text* off: name of the attribute to read string from, priority list, has no effect if *text* option is enabled and already returned a value
* **options** {*string, stringlist*}: optional, string processing options

  * split: tokenize comma separated string

* **scope** {*scope handler*}: optional, define scope to export for this string
* **conditions** {*list of conditions*}: optional, matcher constraints

.. code-block:: json
  :caption: Example: Parse 'text' attribute for all <DemoElement>, use 'name' or 'label' attribute if 'text' attribute does not exist, split value into tokens.

  {
    "kind": "element",
    "name": "DemoElement",
    "attribute":
    [
      "text",
      "name",
      "label"
    ],
    "options": "split",
    "scope":
    {
      // ...
    },
    "conditions":
    [
      // ...
    ]
  }

.. code-block:: json
  :caption: Example: Parse <DemoElement> element text

  {
    "kind": "element",
    "name": "DemoElement",
    "text": true,
    "scope":
    {
      // ...
    },
    "conditions":
    [
      // ...
    ]
  }

-> :ref:`Back to overview<modeloverview>`


------------


.. _attributecondition:

Condition: attribute sibling
----------------------------

Semantic: Current node has another (or same) attribute with a certain value.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "attribute"
* **name** {*string*}: required, name of the attribute to compare to
* **value** {*string*}: required, expected value of the attribute that is compared to
* **operator** {*string*}: optional, comparison type

  * equal
  * notequal


.. code-block:: json
  :caption: Example: node must have a 'type' attribute with value 'string'.

  {
    "kind": "attribute",
    "name": "type",
    "value": "string",
    "operator": "equal"
  }

-> :ref:`Back to overview<modeloverview>`

------------

.. _elementcondition:

Condition: element name
-----------------------

Semantic: Current node must have certain element name.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "element"
* **name** {*string*}: required, element name to match
* **operator** {*string*}: optional, comparison type

  * equal
  * notequal


.. code-block:: json
  :caption: Example: current node must be <Parameter>

  {
    "kind": "element",
    "name": "Parameter",
    "operator": "equal"
  }

-> :ref:`Back to overview<modeloverview>`

------------

.. _staticscope:

Scope: static
-------------

Semantic: constant scope value.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "static"
* **value** {*string*}: required, scope string value to use

.. code-block:: json
  :caption: Example: set scope to "[SomeText]"

  "scope":
  {
    "kind": "static",
    "value": "SomeText"
  }

-> :ref:`Back to overview<modeloverview>`

------------

.. _parentscope:

Scope: parent attribute
-----------------------

Semantic: Use parent element attribute value as scope.

**Attributes**

* **kind** {*string*}: required, object id for internal use, must be "parent"
* **element** {*string, stringlist*}: required, name of the parent element, priority list
* **attribute** {*string, stringlist*}: required, name of parent attribute to use, priority list
* **fallback** {*string*}: optional, constant string value to use as fallback (default value)

.. code-block:: json
  :caption: Example: use 'name' attribute of parent <ParentName>, use 'SomeText' if there is no <ParentName> parent.

  {
    "kind": "parent",
    "element":
    [
      "ParentName",
      "AltParentName"
    ],
    "attribute":
    [
      "name",
      "label"
    ],
    "fallback": "SomeText"
  }

-> :ref:`Back to overview<modeloverview>`

------------


XML model examples
==================

.. _modelexamples:

Here are a few model examples:

Attribute matcher
-----------------

.. code-block:: json
  :caption: Parse all 'text' attributes, irregardless of element name.

  {
    "root":
    {
      "name": "DemoFormat"
    },
    "extensions": "xml",
    "matchers":
    [
      {
        "kind": "attribute",
        "name": "text"
      }
    ]
  }


Element matcher
---------------

.. code-block:: json
  :caption: Parse 'text' attribute from all <DemoElement>.

  {
    "root":
    {
      "name": "DemoFormat"
    },
    "extensions": "xml",
    "matchers":
    [
      {
        "kind": "element",
        "name": "DemoElement",
        "attribute":
        [
          "text"
        ]
      }
    ]
  }


Element condition
-----------------

.. code-block:: json
  :caption: Parse 'text' attribute if element name is not 'Parameter'.

  {
    "root":
    {
      "name": "DemoFormat"
    },
    "extensions": "xml",
    "matchers":
    [
      {
        "kind": "attribute",
        "name": "text",
        "conditions":
        [
          {
            "kind": "element",
            "name": "Parameter",
            "operator": "notequal"
          }
        ]
      }
    ]
  }


Root element filter
-------------------

.. code-block:: json
  :caption: Skip file if root element does not have a 'localization' attribute

  {
    "extensions": "xml",
    "root":
    {
      "name": "DemoFormat",
      "conditions":
      [
        {
          "kind": "attribute",
          "name": "localization",
          "value": "",
          "operator": "notequal"
        }
      ]
    },
    // ...
  }


Static scope
------------

.. code-block:: json
  :caption: Parse all 'text' attributes, irregardless of element name. Export each string with scope 'TestScope'.

  {
    "root":
    {
      "name": "DemoFormat"
    },
    "extensions": "xml",
    "scope":
    {
      "kind": "static",
      "value": "TestScope"
    }
    "matchers":
    [
      {
        "kind": "attribute",
        "name": "text"
      }
    ]
  }


.. _skinmodel:

Skin model
----------

Production example: this is the model definition for the built-in `<Skin>` format, with added comments for illustration purposes:

.. code-block:: json

  {
    "root":
    {
      "name": "Skin"
    },
    "extensions": "xml",
    // For all elements: use the parent element name as scope
    // if a parent named Form, WindowClass or Perspective
    // exists, use 'Skin' otherwise.
    "scope":
    {
      "kind": "parent",
      "element":
      [
        "Form",
        "WindowClass",
        "Perspective"
      ],
      "attribute": "name",
      "fallback": "Skin"
    },
    "matchers":
    [
      // Parse title attribute from any element.
      {
        "kind": "attribute",
        "name": "title"
      },
      // Parse tooltip attribute from any element.
      {
        "kind": "attribute",
        "name": "tooltip"
      },
      // Parse placeholder attribute from any element.
      {
        "kind": "attribute",
        "name": "placeholder"
      },
      // Parse command.name attribute from any element but
      // use "Command" as scope (overrides model level 'parent'
      // scope rule).
      {
        "kind": "attribute",
        "name": "command.name",
        "scope":
        {
          "kind": "static",
          "value": "Command"
        }
      },
      // Parse command.category attribute from any element but
      // use "Command" as scope (overrides model level 'parent'
      // scope rule).
      {
        "kind": "attribute",
        "name": "command.category",
        "scope":
        {
          "kind": "static",
          "value": "Command"
        }
      },
      // Parse range attribute from all <Parameter> elements
      // if the sibling 'type' attribute value is 'string'.
      {
        "kind": "attribute",
        "name": "range",
        "conditions":
        [
          {
            "kind": "element",
            "name": "Parameter",
            "operator": "equal"
          },
          {
            "kind": "attribute",
            "name": "type",
            "value": "string",
            "operator": "equal"
          }
        ]
      },
      // Parse range attribute from all <Parameter> elements
      // if the sibling 'type' attribute value is 'list'.
      // Stringsplit the list value into separate tokens.
      {
        "kind": "attribute",
        "name": "range",
        "options": "split",
        "conditions":
        [
          {
            "kind": "element",
            "name": "Parameter",
            "operator": "equal"
          },
          {
            "kind": "attribute",
            "name": "type",
            "value": "list",
            "operator": "equal"
          }
        ]
      }
    ]
  }


.. _changelog:

Version history
===============

Changelog v1.2.0
----------------

* scan mode '-auto' supports Android resources XML files as built-in format


Changelog v1.1.2
----------------

* change built-in skin parser to scan 'placeholder' attribute on all elements


Changelog v1.1.1
----------------

* parse <MetaInformation> 'Package:Name'


Changelog v1.1.0
----------------

* maintain XML formats as JSON model files
* added *-custom* parser mode: scan custom XML formats
* added *-auto* parser mode: scan any known content type
* added support for built-in *meta information* format
* added support for built-in *document template* format
* improved logging
* added *-v* option to toggle debug logs
