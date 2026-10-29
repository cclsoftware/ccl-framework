############
Introduction
############

The CCL Cross-platform Framework provides an XML-based user interface definition language to describe the appearance of an application.

A skin package is used to deploy the relevant assets. It can either be a folder or a ZIP archive with the filename extension .skin, containing XML files, images (in formats like JPEG, PNG, WebP, SVG), fonts, etc. When the application launches, it looks for a skin package at a predefined location, parses its XML content and loads the primary form "ApplicationWindow".

Conceptually, the skin system separates the application's business logic from its graphical representation on screen. The application code and the GUI can address and connect to each other based on a naming scheme. The application code provides a tree of named components with parameters and properties. The skin data model in turn provides resources like images, styles, forms, etc. also associated with names.

Because the two sides only ever connect by name, the UI can be laid out, styled, and reworded entirely in XML - without touching or recompiling the underlying code - and changes can be reloaded into a running app to see them take effect immediately (see :ref:`Tooling and Troubleshooting<skin_troubleshooting>`).

********
Concepts
********

The following terms are used throughout this guide:

Skin package
    The unit of deployment: a folder or ``.skin`` archive containing a ``skin.xml`` and everything it references (images, fonts, other XML files). See :doc:`chapter/package/index`.

Form
    A named group of controls, either a top-level window or a reusable sub-part of another form. See :doc:`chapter/forms/index`.

Control
    A single interactive or display element inside a form, such as a button, text box, or label. See :doc:`chapter/controls/index`.

Style
    A named, reusable set of appearance properties (colors, fonts, images, ...) that controls reference for their look. See :ref:`Styles<visual_styles>`.

Resource
    A named asset - color, image, gradient, or style - declared in a skin package and referenced by name elsewhere in the skin.

Controller
    The component in the application's tree that a form (or a part of it) is currently bound to; names in skin XML are resolved relative to it. See :ref:`Parameter Binding and Addressing<parameter_binding>`.

Parameter
    A named value owned by a component in the application's tree (e.g. a numeric value, an on/off state, text, etc.) that a control's ``name`` attribute binds to. See :ref:`Parameter Binding and Addressing<parameter_binding>`.

Workspace
    The application-level arrangement of windows and panels (perspectives, frames, window classes) that hosts one or more forms at runtime. See :doc:`chapter/workspace/index`.
