.. include:: ../../reference/skin-elements-classmodel.ref.rst

#####
Forms
#####

A |xml.class.form| describes a group of controls which can either appear directly as a window on screen or can be a sub-part of another form. Forms can reference each other without special knowledge of the application to break complex user interfaces into smaller pieces.

When a form is created by the application, it is associated with a component at a certain point in the component tree. To reference parameters of other components inside the form, they can be addressed either relative or absolute, see :ref:`Parameter Binding and Addressing<parameter_binding>` below.

An example |xml.class.form| could be structured like this (simplified):

.. code-block:: xml

  <Form>
    <!-- row 1, include a predefined view -->
    <Horizontal>
      <View name="OtherView" attach="all"/>
    </Horizontal>

    <!-- row 2, textbox with button -->
    <Horizontal>
      <TextBox name="parameter"/>
      <Button title="Button in second row"/>
    </Horizontal>

    <!-- row 3, label with button -->
    <Horizontal>
      <Label title="A label"/>
      <Button title="Button in bottom row"/>
    </Horizontal>
  </Form>

.. _parameter_binding:

********************************
Parameter Binding and Addressing
********************************

Most controls display and change a value owned by the application. A control's ``name`` attribute doubles as the name of the parameter it binds to, looked up on the component the enclosing |xml.class.form| was created for (its *current controller*).

.. code-block:: xml

  <EditBox name="value"/>

This binds the edit box to a parameter called "value" on the current controller. If no such parameter exists, the control silently displays nothing and ignores input - see :ref:`Tooling and Troubleshooting<skin_troubleshooting>` for how to be warned about that instead of guessing.

Controller Paths
=================================

To reach a parameter on a different component than the current controller, prefix the parameter name with a *controller path*, separated from it by the last ``/`` in the ``name`` attribute:

.. code-block:: xml

  <EditBox name="componentA/value"/>

Everything before the last ``/`` is resolved as a controller path; everything after it is the parameter name looked up on whatever component that path resolves to. A controller path is either:

* **Relative** - a plain child name, or a slash-separated chain of them, resolved by walking down child components starting at the current controller (as in the example above: "componentA" is a child of the current controller).

* **Absolute** - a path starting with ``object://`` is resolved from the application root instead of the current controller. The segment between ``://`` and the next ``/`` names a *root object* registered with the application (e.g. ``hostapp``, ``WindowManager``, ``Workspace``); everything after it is a relative child-path from there, exactly as above.

  .. code-block:: xml

    <Toggle name="object://WindowManager/MyWindowClass"/>
    <TextBox name="object://hostapp/DocumentManager/ActiveDocument/title"/>

Narrowing Scope with "using"
=================================

When several controls in a row need the same controller path, wrap them in a |xml.class.using| element instead of repeating the path on every control. It doesn't render anything itself - it only changes which controller its children resolve names against, and accepts the same relative or absolute paths described above.

.. code-block:: xml

  <using controller="componentA">
    <TextBox name="value"/>
    <CheckBox name="enabled"/>
  </using>

If the controller path might legitimately not resolve (e.g. an optional sub-component that not every instance has), set ``optional="true"`` - otherwise a missing controller is reported as a warning. Either way, the ``using`` block's children are skipped when the controller can't be found.

.. code-block:: xml

  <using controller="componentB" optional="true">
    <TextBox name="value"/>
  </using>
