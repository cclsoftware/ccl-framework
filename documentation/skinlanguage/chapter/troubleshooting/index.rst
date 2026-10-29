.. include:: ../../reference/skin-elements-classmodel.ref.rst

.. _skin_troubleshooting:

###########################
Tooling and Troubleshooting
###########################

Skin XML is data, not code - there is no compiler built into the format itself. Two complementary layers of tooling catch mistakes instead: an editor extension that checks the skin XML statically while you type, and runtime warnings that catch anything only the running application can know about, such as whether a parameter actually exists.

************************************
CCL Skin Language VS Code Extension
************************************

The `CCL Skin Language Support <https://marketplace.visualstudio.com/items?itemName=cclsoftware.ccl-skin-language>`_ extension adds IntelliSense for Skin XML to Visual Studio Code, and is the fastest way to catch mistakes before ever running the app:

* **Error checking** - squiggles for classes, attributes, or attribute values that aren't defined or have invalid values, validated against the same classmodel schemas (``Skin Elements.classModel``, ``Visual Styles.classModel``) that generate the :ref:`reference<skinsdk_references_index>`.
* **Hover documentation** - hovering an element shows its classmodel documentation; hovering a variable or a relative URL shows its resolved value or absolute path.
* **Autocomplete** - completion for styles, colors, enums, URLs, forms, variables, and attribute names, drawn from the classmodel and from every definition in the currently edited skin pack, not just the current file.
* **Color editor** - concrete color values show a swatch and open a color picker on hover.
* **Go to Definition / Find All References / Rename Symbol** - navigate to and refactor styles, colors, shapes, images, forms, and URLs across the whole skin pack.

Setup
=================================

The extension locates the classmodel files and skin packs it needs via a ``repo.json`` at the project root, defaulting to ``<root>/skins`` and ``<root>/classmodels`` if left empty. In this repository those defaults already match - ``classmodels/Skin Elements.classModel``, ``classmodels/Visual Styles.classModel``, and the top-level ``skins/`` folder - so an empty ``repo.json`` at the repository root is enough. Paths can also be set explicitly via the ``CCLSkin.classmodel.skinElements``, ``CCLSkin.classmodel.visualStyles``, and ``CCLSkin.skinsLocations`` settings.

*****************
Runtime Warnings
*****************

Some mistakes can't be caught statically - most notably whether a ``name`` attribute actually matches a parameter the running application exposes, since that depends on the live component tree rather than anything declared in the skin XML. For these, the skin system resolves names lazily while it builds the UI and reports a warning wherever one doesn't resolve.

Live Editing
=================================

**CCL Spy** is the tool for iterating on a skin without restarting the app - it works in both debug and release builds. Open it with Ctrl+Alt+O (Cmd+Alt+O on macOS), which works in either build type; the **Debug** menu offers the same command, but only exists in debug builds.

CCL Spy shows the full view hierarchy as it exists at runtime; hold Ctrl (Cmd on macOS) and point at any on-screen element to select it in that hierarchy, where it also shows the skin XML file and line it originated from. Its **Reload Skin** command (Shift+F10) re-parses the skin package and rebuilds the current UI in place, so you can keep editing a form without losing your place in the app.

Reading Warnings
=================================

When a name doesn't resolve, the skin system logs a warning of the form:

.. code-block:: text

  [Skin] path/to/file.xml:42 'controlName': Parameter not found: 'value'

The most common messages:

.. list-table::
    :widths: 40 60
    :header-rows: 1

    * - Message
      - Cause
    * - ``Controller not found for Parameter: 'X'``
      - The controller path before the last ``/`` in a ``name`` attribute didn't resolve to any component. See :ref:`Parameter Binding and Addressing<parameter_binding>`.
    * - ``Parameter not found: 'X'``
      - The controller resolved, but it has no parameter with that name.
    * - ``Controller not found for using statement: 'X'``
      - A |xml.class.using| element's ``controller`` path didn't resolve; its children were skipped. Add ``optional="true"`` if that is expected.
    * - ``Color/Image/Style/Gradient Element not found: 'X'``
      - A resource reference (e.g. ``color="@X"``) didn't resolve. The VS Code extension normally catches this for literal references before you run the app; it shows up at runtime mainly for names built dynamically via skin variables.

These warnings always show up in the debugger output of a debug build. To also have them collected as log events (with file and line), turn on skin warnings for the running app - either via **CCL Spy**, or by setting the ``GUI.Skin.skinWarningsEnabled`` framework configuration option to ``true``.

.. note::

  Skin warnings are opt-in and disabled by default, since a large app can accumulate a lot of intentionally-optional references (e.g. an optional |xml.class.using| block). Enable them while actively editing a skin, not as a permanent setting.
