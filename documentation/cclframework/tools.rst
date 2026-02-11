.. |cclspy| replace:: **CCL Spy**
.. |ccllocalizer| replace:: **CCL Localizer**
.. |cclskineditor| replace:: **CCL Skin Editor**

#########
CCL Tools
#########


=======
CCL Spy
=======

.. image:: img/cclspy_icon_128.png
  :width: 64

You can use |cclspy| while working with the XML-based UI definitions of your app. Press Ctrl+Alt+O to open the spy or open it from the Debug menu while your application is running. Pressing Ctrl shows the UI element under the mouse in the view hierarchy. "Reload Skin" supports live updates of the application GUI while editing the XML. The |cclspy| includes documentation for all available UI elements. The source code is located in

.. code-block:: rst
  
  services/ccl/cclspy

.. image:: img/cclspy01.png
  :align: center
  :width: 650

.. image:: img/cclspy02.png
  :align: center
  :width: 650


=============
CCL Localizer
=============

.. image:: img/ccllocalizer_icon_128.png
  :width: 64

CCL is using the `GNU Gettext <https://en.wikipedia.org/wiki/Gettext>`_ file format for localization. The translation editor |ccllocalizer| can be downloaded from `ccl.dev <https://ccl.dev>`_.

.. image:: img/ccllocalizer.png
  :align: center
  :width: 650



===============
CCL Skin Editor
===============

.. image:: img/cclskineditor_icon_128.png
  :width: 64

Currently a work in progress started in February 2019 for graphical editing of JSON-based skins and optional C++ export.

.. image:: img/cclskineditor.png
  :align: center
  :width: 650