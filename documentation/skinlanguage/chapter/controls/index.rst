.. include:: ../../reference/skin-elements-classmodel.ref.rst

########
Controls
########

The CCL framework provides a rich set of standard control types like buttons, check boxes, text edits, etc. as well as more complex controls like list views and tree views. Please see the reference section for a complete list of supported controls.

The colors, bitmaps, fonts, etc. that make up the appearance of a control on screen can be defined separately as :ref:`visual styles<visual_styles>`. This concept borrows some aspects from CSS used in HTML rendering. A visual style is defined once and can be shared among multiple controls. Additionally, controls provide special options for their behavior on a per-instance basis.

Controls described in the skin XML definition are bound to parameters provided by the underlying component tree. This separates the graphical representation from the controller logic in the code behind. The parameter concept allows multiple controls to connect to the same parameter and it updates all representations automatically without further coding. For example, a parameter can be attached to a |xml.class.textbox| and a |xml.class.selectbox| at the same time.

=================
Control Rendering
=================

In general, controls are updated on screen within the paint interval of a window provided by the operating system. The whole view tree is redrawn in an optimized way, where only areas which have been previously invalidated are actually updated. Invalidation is usually caused by parameter changes emited by the underlying component tree.

There are scenarios where bypassing the regular paint interval results in better performance, e.g. when animated controls need to update very fast. This mechanism is called "direct update". Only the rectangular region of a single control is updated at a time, without checking the whole view tree for invalidated regions. For direct update to work correctly controls must not overlap and they must be able to redraw their background completely opaque. To use alpha-transparency with directly updated controls an option for background composition can be set, where the control takes the background bitmap underneath in the nesting tree to redraw its background.

Some controls can be set to transparent, which means they react on mouse or keyboard input but do not render anything on screen.
