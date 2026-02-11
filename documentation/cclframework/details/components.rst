#################
Application Model
#################

************************
Event-Driven Programming
************************
	
As usual on platforms with graphical user interfaces, the control flow of a CCL application is triggered by UI events
like mouse, keyboard or touch input. The CCL framework internally runs an `event loop <https://en.wikipedia.org/wiki/Event-driven_programming>`_
that receives these events from the OS and dispatches them e.g. to a window of the application.

For example, when the user presses a mouse button, the framework receives an OS event and passes it as a :cref:`CCL::MouseEvent<CCL::MouseEvent>`
to the active window, which in turn passes it through its view tree until a view handles it.

*****************
Application Types
*****************

CCL applications can run on desktop (Windows, macOS, Linux) and mobile (iOS, Android) platforms.

Typically, the application code and the UI decription in XML can be completely platform-independent,
as platform details are handled under the hood by the framework.
However, there are some differences between desktop and mobile platforms that you should be aware of.
	
Asynchronous Event Handling
===========================
	
Some UI interactions like modal dialogs are internally handled differently on desktop and mobile platforms:

* On most desktop platforms, running a modal dialog is implemented as a "modal loop", that blocks the main event loop while the dialog is open.
  On such platforms, a call like :cref:`DialogBox::runDialog ()<CCL::DialogBox>` returns after the dialog closed
  (which was convinient as the return value, that indicates the clicked button, can be interpreteted directly after the ``runDialog ()`` call).

* On mobile platforms, ``runDialog ()`` returns immediately, while the dialog is still open.
  To handle the result, the asynchronous call :cref:`DialogBox::runDialogAsync<CCL::DialogBox>` must be used.
  It returns immediately, but allows to add a lambda function (with the help of the :cref:`Promise<CCL::Promise>` class) that will be called when the dialog is closed. 

Example:

.. code-block:: cpp

    Promise promise (DialogBox ()->runDialogAsync (view, Styles::kWindowCombinedStyleDialog, Styles::kOkayButton|Styles::kCancelButton));
    promise.then ([] (IAsyncOperation& operation)
    {
        // lambda is called when dialog is closed, result specifies the clicked button
        if(operation.getResult ().asInt () == DialogResult::kOkay)
        {
            //...
        }
    });

Other UI interactions with a similar asynchronous behavior are
:cref:`AlertBox::runAsync ()<CCL::AlertBox>`,
:cref:`IPopupSelector::popupAsync ()<CCL::IPopupSelector>`,
:cref:`IFileSelector::runAsync ()<CCL::IFileSelector>`,
:cref:`IDragSession::dragAsync ()<CCL::IDragSession>`.

In general, it's recommended to use the asynchronous version of these methods.
This way you are on the safe side when some code originally written for desktop will later also run on a mobile platform.


UI Element Sizes
================

UI controls on mobile platforms often need to be larger than on desktop, as they are typically used with touch input.

The *neutraldesign* skin package already takes care of such decisions by defining many standard styles with e.g. different font sizes for mobile and desktop applications.
By importing *neutraldesign* into your applicaiton skin, you might not need further adjustments.

If you still need to distinguish between mobile and desktop in a skin, this can be done by using the XML processing instructions
``<?defined> mobileap?>`` and ``<?defined desktopapp?>``. They mark sections of XML to be only valid for the given application type.

Example:

.. code-block:: xml

    <Style name="MyButton" inherit="Standard.Button">
        <?defined mobileapp?>
            <Font  name="textfont"	themeid="StandardUI" size="14"/>
        <?defined desktopapp?>
            <Font  name="textfont"	themeid="StandardUI" size=12/>
        <?defined?>
    </Style>


**********
Components
**********

Components are reusable building blocks of an application.
They can be nested: a component has access to child and parent components.
Each module of an application has a global component tree that starts with the :cref:`RootComponent<CCL::RootComponent>`.

A component (including child components) an also exist outside this global tree, e.g.

* A component for a modal dialog might be created only for the lifetime of the dialog.
* In document-based applications, each document has its own document tree starting with the document controller.

Components can be seen as the backend of the UI. The base class :cref:`Component<CCL::Component>` offers functionality like managing parameters, handling commands and context menus.

Components and their parameters typically present aspects of an underlying data model so that views like UI controls can connect to them.
They implement the application logic for managing the UI by reacting to changes in both the data model and the views.
This is similar to the `Presentation Model <https://martinfowler.com/eaaDev/PresentationModel.html>`_ design pattern
or the ``View model`` in the `MVVM <https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93viewmodel>`_ pattern.

**********
Parameters
**********

A parameter is an object that represents a "value". Standard parameter types are *Bool*, *Integer*, *Float*, *String*, *List*.
It can be identified by name or tag (integer) and is often used as “backend” for UI Controls like :cref:`CCL::Button`, :cref:`CCL::Toggle`, :cref:`CCL::TextBox`, :cref:`CCL::Slider`, etc.

Values are abstracted as :cref:`Variant<CCL::Variant>` in the parameter interface.
Numerical parameters (:cref:`IntParam<CCL::IntParam>`, :cref:`FloatParam<CCL::FloatParam>`) have minimum and maximum value and a default value.
Based on this range, the value can be translated to / from a *normalized* scale between 0.1 and 1.0 .

.. code-block:: cpp

    AutoPtr<Parameter> param (NEW IntParam (0, 100, "level"));
    param->setDefaultValue (50);

    param->setValue (25);
    param->setNormalized (0.25f);

    String string;
    param->toString (string);

Parameter notifications
=======================

A parameter has one :cref:`IParamObserver<CCL::IParamObserver>`, called “controller” (typically the owning component)
and can have multiple observers (e.g. UI Controls) through the :cref:`ISubject<CCL::ISubject>` / :cref:`IObserver<CCL::IObserver>` mechanism.

.. code-block:: cpp

    param->connect (this, Tag::kLevel); // sets controller & tag

When the value changes

* the controller is notified via a :cref:`IParamObserver::paramChanged ()<CCL::IParamObserver::paramChanged>` call
* the observers are informed via :cref:`kChanged<CCL::kChanged>` message

Methods that change the value have an ``update`` argument.
Setting the parameter value with ``update == true`` leads to a ``paramChanged ()`` call of the controller (in addition the ``kChanged`` message for the observers).

.. code-block:: cpp

    void CCL_API setValue (VariantRef v, tbool update = false);
    void CCL_API setNormalized (float v, tbool update = false);
    void CCL_API fromString (StringRef s, tbool update = false);

The typical use is:

* A UI control connected to the parameter sets the value with ``update == true`` to inform the controller.
* The owning component usually sets the value (e.g. from the data model) with ``update == false``, because he doesn't need a notification for the value he already knows.
  
Parameters in a Component
=========================

The :cref:`Component` base class has a ``paramList`` that helps managing parameters.
It makes these parameters available by implementing the :cref:`IController<CCL::IController>` interface.

A derived component class typically

* adds parameters to the ``paramList``
* sets parameter values according to data model or some internal state
* implements :cref:`IParamObserver::paramChanged ()<CCL::IParamObserver::paramChanged>` to handle parameter changes

.. code-block:: cpp

    MyComponent::MyComponent ()
    : Component ("MyComponent")
    {
        paramList.addInteger (0, 100, "level", Tag::kLevel);
        paramList.addString ("name", Tag::kName);
        paramList.add (NEW MyParameter ("format"), Tag::kFormat);
        
        paramList.byTag (Tag::kLevel)->setValue (30);
    }

    ///////////////////////////////////////////////////////////////////

    tbool CCL_API MyComponent::paramChanged (IParameter* param)
    {
        switch(param->getTag ())
        {
        case Tag::kLevel:
            // level param has changed, do something with the new value
            applyLevel (param->getValue ().asInt ());
            return true;
        }
        return SuperClass::paramChanged (param);
    }

**************
Component Tree
**************

Components can have a name and child components. This makes it possible to address them in the component tree via a path.
Similar, paths can also be created for the parameters of a component.

.. code-block:: cpp

    // add a MyComponent instance to the RootComponent
    auto* myComponent = NEW MyComponent;
    myComponent->setName ("MyComponent");
    RootComponent::instance ().addComponent (myComponent);

    // add a ChildComponent to MyComponent
    auto* childComponent = NEW MyChildComponent;
    childComponent->setName ("MyChildComponent");
    myComponent->addComponent (childComponent);

    // Component path of MyChildComponent
    "object://hostapp/MyComponent/MyChildComponent"

    // Parameter path of the "level" parameter in MyComponent
    "hostapp/MyComponent/level"

********
Commands
********

Commands are “One-Shot” actions, identified by category and name.
Example: *“Edit” - “Delete”*

This classification resembles to the menu structure of common desktop applications with menus like "File" and "Edit".
Category and name are not only used as internal identifiers, but also appear in the (english) UI, e.g. in menu items.

Commands can be triggered by various means:

* Menu items
* Keyboard shortcuts
* Hardware buttons on a device
* Programmatically
  
    .. code-block:: cpp

        System::GetCommandTable ().performCommand (CommandMsg ("Edit", "Delete"));

Handling Commands in a Component
================================

Commands are handled via the :cref:`ICommandHandler<CCL::ICommandHandler>` interface. The :cref:`Component<CCL::Component>` base class already implements it (by passing the command to child components).

.. code-block:: cpp

    interface ICommandHandler: IUnknown
    {
        virtual tbool CCL_API checkCommandCategory (CStringRef category) const = 0;

        virtual tbool CCL_API interpretCommand (const CommandMsg& msg) = 0;
    };

To handle command, a component can override :cref:`ICommandHandler::interpretCommand<CCL::ICommandHandler::interpretCommand>`, compare ``msg.category`` and ``msg.name`` with the commands it wants to handle
and execute code if there is a match.

This can be simplified using the :cref:`CommandDispatcher<CCL::CommandDispatcher>` template and related helper macros that manage the dispatching of category and name.

.. code-block:: cpp

    class MyComponent: public CCL::Component
                       public CCL::CommandDispatcher<MyComponent>
    {
    public:
        DECLARE_CLASS (MyComponent, Component)

        MyComponent ();

        DECLARE_COMMANDS (MyComponent) // declares interpretCommand and dispatch table
        DECLARE_COMMAND_CATEGORY ("Edit", Component) // implements checkCommandCategory

    private:
        // Command methods
        bool onEditDelete (CCL::CmdArgs args);
    };

.. code-block:: cpp

    // these macros create a dispatch-table and an implementation of interpretCommand
    // that automatically calls the given method when category and name match

    BEGIN_COMMANDS (MyComponent)
        DEFINE_COMMAND ("Edit", "Delete", MyComponent::onEditDelete)
    END_COMMANDS (MyComponent)

    IMPLEMENT_COMMANDS (MyComponent, Component)

    //////////////////////////////////////////////////////////////////////////////////////////////////

    bool MyComponent::onEditDelete (CmdArgs args)
    {
        if(args.checkOnly ())
            return true; // checkOnly flag: can the command be executed?
        else
        {
            // execute the command (delete something)
        }
        return true;
    }

In most situations, the framework delivers a command message twice:
first with the ``kCheckOnly`` flag set to check if the command can be executed (e.g. for greying out unavailable commands in a menu)
and then without the flag for the actual execution. So it's very important to check for ``args.checkOnly ()`` in a command handling method: **only execute the command if the flag is not set**.

*********************
Context menu handling
*********************

When a context menu is opened (e.g. on right click with a mouse or long press gesture on touch screen),
the controllers of all views under the mouse can contribute to the menu if they implement the :cref:`IContextMenuHandler<CCL::IContextMenuHandler>` interface (already done by the :cref:`Component<CCL::Component>` base class).

A derived component class can override :cref:`IContextMenuHandler::appendContextMenu<CCL::IContextMenuHandler::appendContextMenu>` and append commands to the menu (typically commands it handles). 
Returning ``kResultTrue`` stops building the menu.

.. code-block:: cpp

    tresult CCL_API MyComponent::appendContextMenu (IContextMenu& contextMenu)
    {
        contextMenu.addCommandItem (XSTR (Delete), "Edit", "Delete", this);
        contextMenu.addSeparatorItem ();
        contextMenu.addCommandItem (XSTR (Paste), "Edit", "Paste", this);

        return SuperClass::appendContextMenu (contextMenu);
    }

****************
Application Menu
****************

An application can have a classical menu bar or an inplace application menu that can be invoked with a button in the UI.
Both are defined in XML, as a tree of nested submenus and command items.
The framework looks for the menu definitions as resources named ``menubar.xml`` or ``appmenu.xml``.

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <MenuBar>

        <Menu name="File">
            <MenuItem name="New" follow="1"/>
            <MenuItem name="Open" follow="1"/>
            <MenuItem name="Close"/>

            <MenuSeparator/>

            <MenuItem name="Save"/>

            <MenuSeparator/>

            <MenuItem name="Quit"/>
        </Menu>

        ...
    </MenuBar>

*****
Views
*****

A view represents a rectangular area on the screen. It has a size and position (relative to the parent view).
Views can be nested. A Window is typically the root of a view tree.
 
A view

* can draw itself
* can handle user input: mouse / key / touch / drag & drop
* can react to other events: onSize (), attached (), detached ()


View classes in cclgui
======================

The Base class CCL::View implements the nesting and has the interfaces IView, IViewChildren.

The cclgui framework has a lot of derived view classes. Some examples are:

* Controls like ``Button``, ``Toggle``, ``Slider``, ``Knob``, ``Label``, ``TextBox``, ``EditBox``
* ``LayoutView`` for arranging child view in various ways
* ``ScrollView`` for presenting a larger target view in a smaller viewport

Views are only created in the cclgui module. In other modules (e.g. the application module), it’s not allowed to derive from view classes.
An application can work with views via the :cref:`IView<CCL::IView>` interface and the :cref:`ViewBox<CCL::ViewBox>` helper class.

UserControls
============

To implement custom views, application code can derive from :cref:`UserControl<CCL::UserControl>`.
A user control is an application object that cooperates with a special view of class :cref:`UserControlHost<CCL::UserControlHost>`.

A ``UserControlHost`` is part of the view tree like any other view. It delegates all relevant UI events to the ``UserControl``
They communicate via interfaces :cref:`IUserControlHost<CCL::IUserControlHost>` (and ``IView``) and :cref:`IUserControl<CCL::IUserControl>`.
All this is managed by the base classes. A derived ``UserControl`` just overrides the methods it needs.

Example: Deriving from ``UserControl``

.. code-block:: cpp

    class MyCustomView: public CCL::UserControl
    {
    public:
        MyCustomView (CCL::RectRef size);

        // draw contents
        void draw (const CCL::DrawEvent& event) override;

        // react to various events
        void onSize (CCL::PointRef delta) override;
        void attached (CCL::IView* parent) override;
        void removed (CCL::IView* parent) override;

        // handle user input events
        bool onKeyDown (const CCL::KeyEvent& event) override;
        bool onKeyUp (const CCL::KeyEvent& event) override;
        bool onMouseWheel (const CCL::MouseWheelEvent& event) override;

        // handle user input transactions
        CCL::IMouseHandler* CCL_API createMouseHandler(const CCL::MouseEvent & event) override;
        CCL::ITouchHandler* CCL_API createTouchHandler (const CCL::TouchEvent& event) override;
        CCL::IDragHandler* CCL_API createDragHandler (const CCL::DragEvent& event) override;

        // ...
    };

Creating Views in a Component
=============================

The Component base class inherits the IViewFactory interface. A derived class can implement it by

* creating a view defined as ``<Form>`` in the application skin
* creating a UserControl (implemented in the application module)
* create a framework view (implemented in cclgui)

Example:

.. code-block:: cpp

    IView* CCL_API MyComponent::createView (StringID name, VariantRef data, const Rect& bounds)
    {
        if(name == "MyEditView")
        {
            // a) create a view defined as <Form> in skin, pass "this" as controller (e.g. to provide parameters)
            ITheme* theme = getTheme ();
            if(theme)
                return theme->createView ("EditorView", this->asUnknown ());
        }
        else if(name == "MyUserControl")
        {
            // b) create a UserControl (implemented in the appplication module)
            auto* myView = NEW MyUserControl (bounds, this);
            return *myView;
        }
        else if(name == "MyView")
        {
            // c) create a framework view (implemented in cclgui)
            ViewBox view (ClassID::View, bounds);
            return view;
        }
        return SuperClass::createView (name, data, bounds);
    }
