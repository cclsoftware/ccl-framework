#############
CCL Generator
#############

*This document refers to :ref:`tool version 1.1.0 <changelog_generator>`*.

============
Introduction
============

The *CCL Generator Tool* is a command line based utility tool that can generate source code files for different programming languages from different input files, utilizing a meda model input format. A meta model can be passed in directly or be partially generated from classmodel or source files.

=====
Usage
=====

.. code-block:: rst

	$ cclgenerator [mode] <arg1> <arg2> [-part] [-ref] [-v]


**Arguments:**

* **[mode]**: one of [-g, -p]: 

	* -g (generate): create a source file from meta model or classmodel file
	* -p (parse): create a meta model file from source file

* **[input]**: input file (meta model, classmodel, source file)
* **[output]**: output file to write
* **[template]**: template file to apply model data to (parse mode only)
* **[-v]**: print debug logs (optional)

Run the tool without arguments to get usage instructions.


=================
Meta Model Format
=================

The meta model allows specification of constants, enumerations, definitions and groups in JSON format. Groups may contain constants, definitions, enumerations and other groups recursively.


Root structure
==============

.. code-block:: json

	{
		"constants":
		[
		]
		"definitions":
		[
		],
		"enums":
		[
		],
		"groups":
		[
		]
	}

Constant
========

.. code-block:: json

	{
		"name": "democonstant",
		"value": "democonstant_value",
		"type": "string",
		"brief": "brief comment",
		"details": "details comment"
	}


Definition
==========

.. code-block:: json

	{
		"name": "demodefinition",
		"value": "123",
		"type": "int",
		"brief": "brief comment",
		"details": "details comment"
	}


Enumeration
===========

.. code-block:: json

	{
		"name": "demoenum",
		"brief": "brief comment",
		"details": "details comment",
		"enumerators":
		[
			{
				"name": "first",
				"value": "0",
				"type": "int",
				"brief": "brief comment",
				"details": "details comment"
			},
			{
				"name": "second",
				"value": "1<<1",
				"type": "int",
				"brief": "brief comment",
				"details": "details comment"
			}
		]
	}


Functions
=========

Values for constants, definitions, and enumerators may be defined as function calls. Example:

.. code-block:: rst

			{
				"name": "first",
				"type": "int",
				"function":
				{
					"name": "fourcc",
					"args": ["eain"]
				}
			}


Note that the "value" attribute is not set here as the CCL generator tool calculates it automatically. Do not forget to set the "type" attribute to the expected function return type. For readability, the CCL Generator adds a function processing hint to the "brief" attribute, indicating the original function input arguments.

**Supported functions**

* **fourcc**

	* arg 0 (*string*): four-character string, example: "eain"
	* result (*int*): four-character code (FourCC) for arg 0



Attribute notes:
================

* **expression**: (string values only): set to true to unquote string type values in the output

	.. code-block:: rst

			{
				"name": "democonstant",
				"value": "somefunction()",
				"type": "string",
				"expression": true
			}

	For "true", cppvalue (...) export "somefunction()" without quotes. For false, "somefunction()" is exported in quotes (being a string value).


* **autoValue** (Enumeration): set to true to auto-value enum enumerators

	.. code-block:: rst

		{
			"name": "demoenum",
			"autoValue": true,
			"enumerators":
			[
				{ "name": "first" },
				{ "name": "second" },
			]
		}

	Enumerators get values assigned based on their list position index, starting at zero:

	* first = 0
	* second = 1

* **type**: needs to be one of

	* bool
	* int
	* float
	* double
	* string


=====================
String Template Usage
=====================

The CCL Generator tool uses the CCL StringTemplate class to generate source files. The StringTemplate class renders a string from an Attributes container, supporting a series of commands to insert data from the container into the string. Supported commands are:

* {{ variable }}
* {% for %} ... {% endfor %}
* {% include %}
* {% if %} {% else %} {% endif %}


Variables
=========

Use {{ variable }} to include a variable in the string template output.

.. code-block:: rst

	The variable value is {{ variable }}.


The variable (attribute) needs to exist in the Attributes input data container.


For loops
=========

Loop over an attributes list from the Attributes container, creating output for each list element:

.. code-block:: rst

	{% for constant in constants %}
	Constant name is {{ constant.name }}
	{% endfor %}

In the example, the attributes container is required to contain a "constants" list attribute.

Inside a loop statemtent, use the **loop** variable to check for certain loop attributes:

* **loop.last**: current iteration is last list entry
* **loop.index**: iterator index, starting at 0

.. code-block:: rst

	{% for constant in constants %}
	Element index: {{ loop.index }}
	Constant name is {{ constant.name }}
	{% if loop.last %}
	{{ constant.name }} is the last element
	{% endif %}
	{% endfor %}


If-else
=======

Render or skip certain string sections based on a condition:

.. code-block:: rst

	{% if variable.name %}
		Variable has a name!
	{% else %}
		Variable has no name!
	{% endif %}


.. note::
	The current implementation supports a truthness check for string values only.


Includes
========

Include another template inside a template:

.. code-block:: rst

	{% include sometemplate.in %}


Included templates are expected in the same folder as the template file passed in as argument.


Filters
=======

The string template supports variable filters which can be applied to a {{ variable }} statement. The syntax is:

.. code-block:: rst

	{{ variable | filter1 | filter2 | ... }}

Each filter modifies the value of variable. Filters are applied from left to right. The following filters are available, affecting string values:

* **upper**: convert string to uppercase
* **lower**: convert string to lowercase
* **capitalize**: convert first string character to uppercase
* **decapitalize**: convert first string character to lowercase
* **cpptype, javatype, tstype, jstype**: convert meta model type string to language specific type (expects one of supported built-in meta model types)
* **cppvalue, javavalue, tsvalue, jsvalue**: convert meta model value to language specific value
* **sentence**: format string as a sentence (start with capital letter, end with period)
* **identifier**: format string as C++  (and most other languages) compatible identifier
* **deconstify** remove leading 'k' in names that qualify as CCL constants (e.g. "kFontStyleNormal")


.. _changelog_generator:

===============
Version history
===============

Changelog v1.1.0
================

* introduce meta model format
* generate output via string templates


Changelog v1.0.0
================

* initial version
