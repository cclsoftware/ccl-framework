##########
Workspaces
##########

********
Overview
********

Workspaces allow structuring the UI of an application in a flexible way.

A Workspace contains multiple perspectives that the application can switch between.
A perspective defines an arrangement of nested frames, that can each present the content of a window class.

.. code-block:: xml

  <Workspaces>
    <Workspace name="MyApplication">
      <Perspective name="Perspective1">
        ...
      </Perspective>
      <Perspective name="Perspective2">
        ...
      </Perspective>
    </Workspace>
  </Workspaces>

**************
Window Classes
**************

A window class defines a piece of UI that can appear somewhere in a workspace. It is identified by a name and an optional workspace ID (defaults to application workspace).
The content is described by a form name and the url of a controller that will be used to create the view.
It can have an optional command (specified by category and name) for toggling the visibility (open / close).

The group attribute of a window class determines in which workspace frames of a perspective the content can appear.

.. code-block:: xml

  <WindowClasses>

    <WindowClass name="DocumentView"
      controller="object://hostapp/DocumentManager/ActiveDocument"
      form.name="DocumentView"
      group="MainArea"/>

  </WindowClasses>

The window manager of the application maintains a parameter for each window class that reflects the
open/closed state of the window class. It can be used to toggle it.

.. code-block:: xml

  <Toggle name="://WindowManager/MyClass"/>

***********************
Perspectives and Frames 
***********************

A perspective has a set of nested frames that define the general structure of the UI.
Frames with a "groups" attribute contain the actual content. They can be either empty or occupied by the view of a window class.
The "groups" attribute is a list of group names from window classes, separated by spaces. It specifies which window classes can appear in the frame.

Complex arrangements can be built with horizontal or vertical frame groups, which will create a layout container for their contained frames.
An empty frame (or a frame group with only empty frames) collapses to an empty size, so it doesn't take any space in the perspective.

When the application wants to open a window class

.. code-block:: xml

  <Perspective name="MyPerspective">
    <Frame options="vertical"> <!-- layout group -->
      <Frame name="MyFrame1" groups="group1"/>
      <Frame name="MyFrame2" groups="group2 group3" default="windowClassA"/>
    </Frame>
  </Perspective>

===========
Popup Frame
===========

A frame with the option "popup" opens in a separate floating popup window (not in the frame structure of the perspective view tree).
Workspaces often have a least one "popup multiple" frame as target for all floating window content.
"multiple" here means that each matching window class will open in a new window, instead of replacing content in the existing popup window.

.. code-block:: xml

  <Frame options="popup multiple" name="Popups" groups="Popups"/> <!-- --> 
  <Frame options="popup" name="Song.Popups" groups="Song.Popups"/>

==============
Detached Frame
==============

A frame with the options "popup detached" is a target for detaching a window class.
This can be used for a window class that appears in a frame of the regular perspective view tree by default, but can optionally be detached into a separate window.
The workspace maintains the detached state and uses it to determine if  the window class content should appear in a regular frame or in a detached window.

==============
Embedded Frame
==============

An embedded frame defines a sub tree of frames that can appear in the form of a parent class.
The |xml.class.EmbeddedFrame| element must appear in two places: placed inside the form of a "parent" window class
and placed in the root frame of a perspective with the same name and a parent class. It defines a separate subtree of frames.

When a window class is about to be opened, the workspace system ensures that the parent class, whose view contains the EmbeddeFrame view gets opened first.

.. code-block:: xml

  <!-- inside form of parent window -->
  <Form name="MyParentForm">
    <!-- ... -->
    <EmbeddedFrame name="Embedded"/>
  </Form>

  <!-- in root frame of perspective -->
  <EmbeddedFrame name="Embedded" parent.class="MyParentClass">
    <Frame options="vertical">
      <Frame name="A" groups="a"/>
      <Frame name="B" groups="b"/>
    </Frame>
  </EmbeddedFrame>

=================
Custom Parameters
=================

A perspective can define parameters that can be used in the skin.
This makes sense for controlling some activity in the skin, when the parameter does not need to be known to the application logic.
In a form description, these parameters can be addressed via the "CustomParams" controller of a perspective.

.. code-block:: xml

  <Perspective name="MyPerspective">
	  	<Parameter name="myParam" type="int" range="0,32767"/>
      ...
  </Perspective>

  <!-- address via "CustomParams" controller of a perspective: -->
  <Divider name="://Workspace/myWorkspaceID/MyPerspective/CustomParams/myParam"/>

*******************************
Hosting a Workspace in a Window
*******************************

To make content of a workspace appear in the UI, a special view called "PerspectiveContainer" is provided by the WindowManager.
The view tree created by the currently selected perspective will be inserted into the PerspectiveContainer.

=====================
Application Workspace
=====================

An application skin can have multiple workspaces. If a workspace with the applicationID exists, it used as application workspace that provides the content of the main application window.
The framework then automatically creates a PerspectiveContainer view for it that fills the application window.
To customize this, the application skin can define a form "ApplicationWindow" that must contain a view called "PerspectiveContainer" (and optionally adds other elements like decors).

.. code-block:: xml

		<Form name="ApplicationWindow" style="Workspace" windowstyle="titlebar sizable restoresize restorepos">
		  <View name="PerspectiveContainer" style="PerspectiveContainer" size="0,0,1024,720" attach="all"/>
		</Form>

================
Other Workspaces
================

To show the content of another workspace, we can reference the workspace as controller and let it create its PerspectiveContainer: 

.. code-block:: xml

  <Form name="SomeForm">
    <using controller="://Workspace/MyWorkspace">
      <View name="PerspectiveContainer" size="0,0,1024,720" attach="all"/>
    </using>
  </Form>
