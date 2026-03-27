.. _cclscripttools:

################
CCL Script Tools
################

.. |makedoctool| replace:: ``makedoc.py``

=======
Makedoc
=======


Introduction
============

The |makedoctool| script is a CCL script tool that serves as a Sphinx wrapper designed to build Sphinx documentation projects by transforming reStructuredText (RST) files into HTML or PDF formats. In addition to Sphinx core functionality, |makedoctool| extends Sphinx capabilities by enabling more flexible project structures and integrating content from sources beyond RST files, like `Doxygen <https://www.doxygen.nl/>`_ output or CCL classmodel files.

**Usage:**

#. (install required dependencies, see :ref:`setup section <makedocsetup>`)
#. from ``doctools/makedoc/`` run |makedoctool| as follows:

.. code-block:: rst

  $ python makedoc.py PROJECT_ID BUILDER [REVISION] [OPTIONS]

Arguments:

* **PROJECT_ID**, required: id of a documentation project, example: *dev.ccl.doc.skinlanguage*
* **BUILDER**, required: output format, example: *HTML*, *PDF*
* **REVISION**, optional: revision number to appear in the document, example *54234*
* **OPTIONS**, optional: any of [-h], [-v], [-r]

For argument details, available project ids, and output format builders use the help *[-h]* option:

.. code-block:: rst

  $ python makedoc.py -h


**Examples:**

.. code-block:: rst

  # HTML output
  $ python makedoc.py dev.ccl.doc.skinlanguage html

  # HTML, with added revision number
  $ python makedoc.py dev.ccl.doc.skinlanguage html 12345

  # PDF output, verbose logging
  $ python makedoc.py -v dev.ccl.doc.skinlanguage pdf


**Output**

|makedoctool| writes build output to ``makedoc/output/[projectid]``, see |makedoctool| console log for exact location.


Projects
========

|makedoctool| identifies a folder as a documentation project if the folder contains a ``makedoc.json`` file. A ``makedoc.json`` file provides documentation project meta info and build steps. The most important attribute is the project id which uniquely identifies the documentation project in current (meta) repository context.

.. code-block:: rst
  :caption: Example ``makedoc.json`` file

  {
    "id": "dev.ccl.doc.skinlanguage",
    "title": "CCL Skin Definition Language",
    "build":
    [
      {
        "action": "classmodelrst",
        "description": "Convert classmodels to RST",
        "args":
        [
          "classmodel-config.json"
        ]
      }
    ]
  }

On start, |makedoctool| scans the repository for documentation projects in each ``documentation`` folder provided by the meta repo ``repo.json`` file. In CCL framework repository only context, the default assumed location is ``documentation/``.

Reminder: |makedoctool| prints a list of all found project ids when invoked with the help option *[-h]*:

.. code-block:: rst

  $ python makedoc.py -h


.. _makedocsetup:

Setup
=====

To run |makedoctool|, some dependencies need to be installed first:

--------
Required
--------

|makedoctool| requires Python 3.9, Sphinx and a few additional Python modules to run, thus:

* setup Python 3.9 or later, download installer from `python.org <https://www.python.org/>`_
* important, required for next step: configure Python installer to update environment variables (PATH) so Python and pip are accessible from any working directory
* run ``makedoc/setup.sh`` to install required Python modules (includes Sphinx), requires pip in PATH

--------
Optional
--------

Some |makedoctool| build steps require external tools. For example, adding `Doxygen <https://www.doxygen.nl/>`_ source code documentation requires a local installation of `doxygen <https://www.doxygen.nl/>`_. Build steps vary with the project. To inspect a project for potential external tool dependencies, refer to the ``build`` section of the project ``makedoc.json`` file. However, most build step required tools (*doctools*) are already shipped with |makedoctool| and do not need to be installed separately.

In addition to build step related tools, using the |makedoctool| PDF output format option requires `pdflatex` which is part of the `MiKTeX <https://miktex.org/>`_ LaTeX distribution.


.. note::

  If |makedoctool| fails to find an external tool required for building a project, it logs a message to the console. Thus, always check the output for errors.


Doxygen
-------

Required for projects that embed `Doxygen <https://www.doxygen.nl/>`_ C++ reference output. Download and install from `doxygen.nl <https://www.doxygen.nl/>`_. Add the ``doxygen`` binary to the system or user environment path. To test the installation, run the following command from terminal:

.. code-block:: rst

  $ doxygen


TypeDoc
-------

Required for projects that use/generate JavaScript API output. Get TypeDoc from `typedoc.org <https://typedoc.org/>`_, requires `Node.js <https://nodejs.org/>`_. To test the installation, run the following command from terminal:

.. code-block:: rst

  $ typedoc


pdfLaTeX
--------

Required for PDF output format. Install `MiKTeX <https://miktex.org/>`_ from `mixtex.org <https://miktex.org/>`_. To test the installation, run the following command from terminal:

.. code-block:: rst

  $ pdflatex

The Sphinx generated .tex files may reference additional LaTeX packages that are not installed with `MiKTeX <https://miktex.org/>`_ per default. To install them, attempt a PDF build via:

.. code-block:: rst

  python makedoc.py [PROJECT_ID] pdf


This should cause `MiKTeX <https://miktex.org/>`_ to download missing LaTeX packages. It either auto-downloads them in the background or asks for confirmation. The download can take a while and may cause the PDF build to freeze or fail. If so, retry it.


====================
TypeScript Generator
====================

Introduction
============

The TypeScript conversion script ``classmodeldts.py`` can auto-generate TypeScript script typings from .classmodel files.


**Usage:**

.. code-block:: rst

  python classmodeldts.py [CONFIG]


Due to the number of processing options a configuration file is used to control the script input and output parameters.


Config format
=============

The configuration file is a JSON file. It has the following structure:

  .. code-block:: rst
    :caption: Example config.json

    {
      "output": "test.d.ts",
      "template": "testframe.in",
      "comments": false,
      "app": "ApplicationName",
      "models":
      [
        "TestFile1.classModel",
        "TestFile2.classModel"
      ],
      "classes":
      [
        "TestClass1",
        "TestClass2"
      ],
      "objects":
      [
        ".*"
      ]
    }


**Attributes:**

* **output**: path to output .d.ts file
* **models**: list of .classmodel files to process
* **template**: `jinja <https://jinja.palletsprojects.com/en/3.1.x/>`_ template file to insert generated typings into
* **comments**: generate type comments on/off
* **app**: application name (certain application specific typings may be generated)
* **classes**: classes to include in output (use ".*" for all)
* **objects**: objects to include in output (use ".*" for all)


Frame file format
=================

The frame file can be of any content but ensure it does not conflict with the generated typings. The classmodeldts.py script uses `jinja <https://jinja.palletsprojects.com/en/3.1.x/>`_ for templating. Generated typings are inserted into a ``{{typings}}`` placeholder. Ensure this placeholder is contained in the frame file.

.. code-block:: py
  :caption: Frame example file

  ////////////////////////////////
  // A test frame file
  // Copyright (c) 2024
  ////////////////////////////////

  // Generated typings:

  {{typings}}


.. note::

  Use different configs with different frame files and class/object filters to customize output to specific needs.

