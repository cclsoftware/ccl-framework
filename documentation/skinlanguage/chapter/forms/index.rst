.. include:: ../../reference/skin-elements-classmodel.ref.rst

#####
Forms
#####

A |xml.class.form| describes a group of controls which can either appear directly as a window on screen or can be a sub-part of another form. Forms can reference each other without special knowledge of the application to break complex user interfaces into smaller pieces.

When a form is created by the application, it is associated with a component at a certain point in the component tree. To reference parameters of other components inside the form, they can be addressed either relative or absolute with the using-statement.

An example |xml.class.form| could be structured like this (simplified):

.. code-block:: xml

  <Form>
    <!-- row 1, include a predefined view -->
    <Horizontal>
      <View name="BrowserTemplate" attach="all"/>
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
