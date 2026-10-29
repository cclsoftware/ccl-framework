.. include:: ../../reference/skin-elements-classmodel.ref.rst

########
Controls
########

The CCL framework provides a rich set of standard control types like buttons, check boxes, text edits, etc. as well as more complex controls like list views and tree views. Please see the reference section for a complete list of supported controls.

The colors, images, fonts, etc. that make up the appearance of a control on screen can be defined separately as :ref:`visual styles<visual_styles>`, covered in the next chapter. Additionally, controls provide special options for their behavior on a per-instance basis.

Controls described in the skin XML definition are bound to parameters provided by the underlying component tree. This separates the graphical representation from the controller logic in the code behind. The parameter concept allows multiple controls to connect to the same parameter and it updates all representations automatically without further coding. For example, a parameter can be attached to a |xml.class.textbox| and a |xml.class.selectbox| at the same time.

====================
Putting It Together
====================

The pieces covered so far - forms, controls, parameter binding, and styles - come together like this:

.. code-block:: xml

  <Style name="SettingsCheckBox" inherit="Standard.CheckBox">
    <Color name="textcolor" color="black"/>
  </Style>

  <Form name="SettingsPanel">
    <Vertical attach="all">
      <Label title="Settings"/>
      <CheckBox name="enabled" title="Enable feature" style="SettingsCheckBox"/>
    </Vertical>
  </Form>

* ``<Style name="SettingsCheckBox" inherit="Standard.CheckBox">`` - a custom style that inherits from the default check box style and overrides its text color, see :ref:`Styles<visual_styles>`.
* |xml.class.form| - the top-level container for this piece of UI, see :doc:`../forms/index`.
* ``<Vertical attach="all">`` - a layout container that fills its parent and stacks its children top to bottom, see :doc:`../layouts/index`.
* ``<Label title="Settings"/>`` - static text; there is nothing to bind, since it doesn't represent a value.
* ``<CheckBox name="enabled" title="Enable feature" style="SettingsCheckBox"/>`` - a control bound to a parameter called "enabled" on the form's current controller (see :ref:`Parameter Binding and Addressing<parameter_binding>`), labeled via ``title``, and styled by the custom style defined above.

If the current controller doesn't actually expose an "enabled" parameter, the check box still renders - it just won't do anything, and a warning shows up in the ways described in :ref:`Tooling and Troubleshooting<skin_troubleshooting>`.

=================
Control Rendering
=================

In general, controls are updated on screen within the paint interval of a window provided by the operating system. The whole view tree is redrawn in an optimized way, where only areas which have been previously invalidated are actually updated. Invalidation is usually caused by parameter changes emitted by the underlying component tree.

There are scenarios where bypassing the regular paint interval results in better performance, e.g. when animated controls need to update very fast. This mechanism is called "direct update". Only the rectangular region of a single control is updated at a time, without checking the whole view tree for invalidated regions. For direct update to work correctly controls must not overlap and they must be able to redraw their background completely opaque. To use alpha-transparency with directly updated controls an option for background composition can be set, where the control takes the background bitmap underneath in the nesting tree to redraw its background.

Some controls can be set to transparent, which means they react on mouse or keyboard input but do not render anything on screen.
