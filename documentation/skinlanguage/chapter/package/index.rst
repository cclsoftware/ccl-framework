.. include:: ../../reference/skin-elements-classmodel.ref.rst

############
Skin Package
############

A *skin package* has to contain a *skin.xml* file on root-level. This file is used to include other XML files inside the same package with the Include tag, or import other packages with the Import tag.

**Example**

Here is an example *skin.xml* file. Its basic structure consists of a |xml.class.skin| root element that parents the five main sections |xml.class.imports|, |xml.class.includes|, |xml.class.resources|, |xml.class.styles| and |xml.class.forms|:

.. code-block:: xml

    <Skin>
      <Imports>
        <Import name="@package-to-import"/>
      </Imports>

      <Includes>
        <Include name="" url="path/to/file.xml"/>
      </Includes>

      <Resources>
        <Image name="InternalName" url="path/to/file.png"/>
      </Resources>

      <Styles>
        <Style name="MyStyle">
            <Color name="backcolor" color="black"/>
        </Style>
      </Styles>

      <Forms>
        <Form name="ApplicationWindow">
            <Button name="quit" title="Quit"/>
        </Form>	
      </Forms>		
    </Skin>

.. _neutralbase_and_neutraldesign:

================
Neutral Design
================

*neutraldesign* and *neutralbase* are packages that can be imported to provide "Standard" control :ref:`styles<visual_styles>` for a neutral cross-platform design.

While *neutraldesign* defines the basic "Neutral" styles, *neutralbase* is used to customize the look by defining base colors and fonts for these styles. Inside *neutralbase* you will also find the mapping of "Neutral" styles to the "Standard" ones that will eventually be used by the Framework. Through this separation, a vendor-specific base package can be used to customize the default style appearance, by overriding the *neutraldesign* styles.

.. code-block:: xml

    <Skin>
      <Imports>		
		<!-- neutralbase: colors and fonts for style definitions in neutraldesign 
			defining "Standard" styles inheriting from these "Natural" styles -->
		<Import name="@neutralbase"/>
		<!-- neutraldesign: "Natural" parent style definitions using basecolors and fonts -->
        <Import name="@neutraldesign"/>
      </Imports>
    
        ...

    </Skin>


When building a new app, it is good practice to import these two packages to ensure that all controls receive a consistent default appearance. The default styles are applied to controls that do not have an explicitly assigned style.

In addition to custom styles defined in the application skin, the default styles can also be redefined.
For example, if a customized default style for buttons is required, the default button style "Standard.Button" can be overridden within the application skin.

.. code-block:: xml

		<Style name="Standard.Button" inherit="Neutral.Button" appstyle="true" override="true">
			<Color name="textcolor" color="green"/>
		</Style>
