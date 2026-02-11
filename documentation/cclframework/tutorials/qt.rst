.. _switching_from_qt:

#################
Switching from Qt
#################


===============================================
Similar concepts and classes between CCL and Qt
===============================================

Signals
=======

* Qt: Signals/Slots
* CCL: Message passing using :cref:`CCL::ISubject::signal` and :cref:`CCL::IObserver::notify`


**Remarks**

No preprocessor involved, synchronous by default. In Qt, signals and slots are the central concept for sending notifications, exchanging data and "connecting" objects. Message passing is a central concept in CCL, too. However, relations between classes are more often determined by explicit composition or aggregation instead of loosely connecting them from the outside. This often results in more readable and better structured code. Instead of using signal/slot definitions as a "protocol" between separate classes, you usually use interfaces in CCL (see the "Interfaces" section below).

In addition to direct message passing, CCL provides SignalSink's and SignalSource's, which can be used to pass messages accross the process via a named atom.


Object
======

* Qt: QObject
* CCL: :cref:`CCL::Object`

**Remarks**

No preprocessor involved. No automatic ownership based on parent pointers. :cref:`CCL::Object` inherits from :cref:`CCL::Unknown`, which implements :cref:`CCL::IUnknown`.


Cast
====

* Qt: qobject_cast
* CCL: :cref:`CCL::ccl_cast`, :cref:`CCL::IUnknown::queryInterface`

**Remarks**

Use ccl_cast for object to object, queryInterface for object to interface or interface to interface.
ccl_cast does not work across module boundaries.

qobject_cast works on QObject only, not on interface pointers.



Scoped pointer
==============

* Qt: QScopedPointer
* CCL: :cref:`CCL::AutoPtr`

**Remarks**

Works only with classes derived from IUnknown


Shared pointer
==============

* Qt: QSharedPointer
* CCL::cref:`CCL::SharedPtr`

**Remarks**

Works only with classes derived from IUnknown.


Strings
=======

* Qt: QString, QByteArray
* CCL: :cref:`CCL::String`, :cref:`CCL::MutableCString`, :cref:`CCL::CString`


Variant
=======

* Qt: QVariant
* CCL: :cref:`CCL::Variant`

**Remarks**

Cannot be extended with something like Q_DECLARE_METATYPE. Takes :cref:`CCL::IUnknown` pointers for objects.


Debug
=====

* Qt: qDebug, qCDebug
* CCL: Debugger::print, CCL_PRINTF

**Remarks**

In Qt, debugging can be configured at runtime by using qCDebug and enabling/disabling specific categories in a config file. In CCL, debugging via CCL_PRINTF can be enabled/disabled at compile time by defining DEBUG_LOG at the top of a .cpp file for this specific file. Alternatively, the Debugger functions can be called directly for debug output in release builds.


String translation
==================

* Qt: tr("text to translate")
* CCL: :cref:`XSTRING` and :cref:`XSTR` macros

**Remarks**

While it is common practice to add translatable strings inline with Qt, these strings are usually defined explicitly in CCL. Search for the :cref:`BEGIN_XSTRINGS` macro to find examples.
Note that you can add translatable strings inline using the :cref:`TRANSLATE` macro if you really need to.


=============
False friends
=============

String references
=================

* Qt: QStringRef
* CCL: :cref:`CCL::StringRef`

**Remarks**

QStringRef is a wrapper around QString holding substrings, :cref:`CCL::StringRef` is typedef'd to a const reference of :cref:`CCL::String`.


Properties
==========

* Qt: Q_PROPERTY
* CCL: :cref:`PROPERTY_VARIABLE`, :cref:`PROPERTY_POINTER`

**Remarks**

Q_PROPERTY adds an attribute to the meta system. It does not add getter/setter/member declaration. CCL macros add getters, setters and an member declaration to a class definition. They don't affect meta classes.


==========
Interfaces
==========

With Qt, interfaces are rarely a thing you care about. When writing plug-ins you just put a Q_DECLARE_INTERFACE and let Qt do some magic. In other contexts you just use abstract classes, which derive from QObject.
If you need to decouple the implementations of two classes which interact with each other, you often just define some signals and slots in both classes and connect them from the outside. This way, none of the classes needs to include headers of the other class.
However, it can be quite difficult to understand the application logic if many connections are made from various places in code. Decoupling classes by using signals and slots can also be error prone (if you change a signal/slot definition, you need to ensure that all definitions in connected classes are changed too).

With CCL, interfaces are much more present. You usually don't use classes implementing certain functionality directly but use the implemented interfaces instead. This way you can use classes compiled into separate modules (.dll, .so, ...) while ensuring ABI compatibility between your module and your dependencies (in the current or future versions and across different compiler ABIs).
Splitting the code into modules sets some hard boundaries which help to structure the code and allow for independent development of application and framework implementations.
Because interfaces are well-defined and rarely (at best never) changed, you can rely on the fact that your application using certain interfaces will work with any future version of the modules implementing those interfaces.

(Note that it is possible to use interfaces in a similar manner in Qt, too. See for example https://www.d34dl0ck.me/2013/06/mastering-interfaces-with-qt.html. It is just not very common.)
However, while Qt ensures ABI compatibility across versions, it does not ensure ABI compatibility between different compiler ABIs (https://stackoverflow.com/questions/35497158/qt-dll-compatibility-between-compilers). CCL does.

The concepts of interfaces and reference counting used in CCL are very similar to COM. There is a central interface IUnknown, which provides methods related to reference counting (used by smart pointers), and provides a queryInterface method to access other interfaces implemented by a certain class.
Pointers to IUnknown are used as a basic type to pass objects around. To actually use an object passed as a IUnknown pointer, you need to either get another interface the object implements using the queryInterface method, or cast the IUnknown pointer to an object using unknown_cast.
queryInterface takes a unique ID of the interface you want to use. This ID can be obtained by the interface directly, as all (public) CCL interfaces declare an __iid attribute.
However, you rarely need to call queryInterface directly and use the helper class UnknownPtr instead.


.. code-block:: cpp
  :caption: UnknownPtr example

  void LoginHandler::onRegistrationCompleted (IAsyncOperation& operation)
  {
    UnknownPtr<ILicenseServerResult> result (operation.getResult ().asUnknown ());
  }


.. code-block:: cpp
  :caption: unknown_cast example

  tbool CCL_API ChannelRecognizer::recognize (IUnknown* object) const
  {
      Engine::AutomationRegionList* regionList = unknown_cast<Engine::AutomationRegionList> (object);
  }


=======================
Model-View-Architecture
=======================

Model-View-Controller (MVC) is a design pattern originating from Smalltalk that is often used when building user interfaces. In Design Patterns, Gamma et al. write:

  *MVC consists of three kinds of objects. The Model is the application object, the View is its screen presentation, and the Controller defines the way the user interface reacts to user input. Before MVC, user interface designs tended to lump these objects together. MVC decouples them to increase flexibility and reuse.*


Data display in Qt
==================

Coming from Qt, you are probably familiar with Qt's model-view architecture.
Using one of the implementations of QAbstractItemView, you already have an implementation of a view. These views display data, which is contained in a QAbstractItemModel.
When drawing data items, the view calls QVariant QAbstractItemModel::data(const QModelIndex &index, int role = Qt::DisplayRole) const for each item.
The index is used to identify the item for which information is needed (usually by row and column). The role determines, what kind of information is needed (display string, tooltip text, background color, ...).


Data display in CCL
===================

Very similar to this concept, there are :cref:`CCL::IItemView` and :cref:`CCL::IItemModel` in CCL. Derived from these interfaces, there are classes like ListViewModel and ListView.
Instead of a data method, :cref:`CCL::IItemModel` provides methods like getItemTitle, getItemIcon, getItemTooltip, which are used by the view to fetch information about data items.
Similar to Qt's QModelIndex, these function take an :cref:`CCL::ItemIndex` as parameter, which identifies a specific item.


Customized item rendering in Qt
===============================

To customize the rendering of specific items, Qt has a concept of delegates. Delegates are classes derived from QAbstractItemDelegate. One can override the paint method to customize rendering.
To assign delegates to a view, one uses the View's setItemDelegate, setItemDelegateForRow or setItemDelegateForColumn methods.


Customized item rendering in CCL
================================

Instead of using delegates, in CCL one can customize item rendering by overriding :cref:`CCL::IItemModel::drawCell`.


Communication between Model and View in Qt
==========================================

Models, views, and delegates communicate with each other using signals and slots:

* Signals from the model inform the view about changes to the data held by the data source.
* Signals from the view provide information about the user's interaction with the items being displayed.
* Signals from the delegate are used during editing to tell the model and view about the state of the editor.


Communication between Model and View in CCL
===========================================

To inform the view about changes to the data, messages like kChanged, kItemAdded or kUpdateColumns are passed from the model to the view. This is similar to Qt's concept of signals and slots.

To inform the model about user interaction and editing, :cref:`CCL::IItemModel` provides several methods. Those include :cref:`CCL::IItemModel::openItem`, :cref:`CCL::IItemModel::onItemFocused`, :cref:`CCL::IItemModel::editCell` and :cref:`CCL::IItemModel::interpretCommand`.


===================
XML UI descriptions
===================

Qt supports creating user interfaces based on XML .ui files. These files are converted to C++ code by Qt's prepressor. The resulting code defines a class in the namespace UI, which contains preconfigured QWidget's as members.
To use the generated user interface, one creates another class (usually a QWindow or at least a QObject), which has an instance of the generated UI class as a member and calls view->setupUi(this) in the constructor (or some other early point of execution).
The user interface widgets are then connected to the window/object via signals/slots. The window class can act as a model and controller, or just connect the relevant signals to separate controller and model classes (note that these models are not neccessarily derived from QAbstractItemModel).
User interface widgets usually don't know about the "parameters" they display or control. They receive values to display via slots and send control changes to the controller or model via signals.

CCL supports user interface XML descriptions ("skins") as well. However, these descriptions are interpreted at runtime. You can even edit and reload skin files without needing to restart the application.
To create a view, a class usually calls :cref:`CCL::ITheme::createView`, passing the name of a form defined in the skin XML file and a controller object (often a this pointer). Such a controller class is usually derived from IViewFactory (Component is a prominent example) and overrides the :cref:`CCL::IViewFactory::createView` method.
Inside the skin XML description, displayed values can be "parameters" or "properties". Both parameters and properties belong to the controller and are accessed by name.
Properties are fetched via :cref:`CCL::IObject::getProperty`. Properties are evaluated only once when a view is created.
Parameters can be modified at any time. They are accessed via :cref:`CCL::IController::findParameter`. When a control changes on user interaction, the corresponding parameter value is set and the controller is notified via :cref:`CCL::IParamObserver::paramChanged`.
While properties have a value only, parameters have a name, a range, a type and other attributes. A control uses these attributes to set it's own range, type, etc.
A skin can not only use the properties and parameters of the single controller that is assigned to it on creation, but refer to other controllers by relative or absolute paths.
In contrast to Qt's XML description, the author of a CCL skin XML file needs to know the names of controllers, parameters and properties the skin is about to use.


=========================================
Keep multiple value presentations in sync
=========================================

The same numeric value is displayed in two windows. One window contains text input and a slider that need to be kept in sync.


CCL code
========

General idea: Use a parameter object, which is shared between multiple controls. The parameter state is an attribute of the parameter object. Controls don't own a parameter, but interact with a parameter object.

Create a Component subclass. In the component's constructor, add a parameter for your numeric value:

.. code-block:: cpp

  paramList.addInteger (CSTR ("numericValue"), Tag::kNumericValue);


Components typically build a tree structure starting at the root component of each module and are addressable from skin XML.

Create skin files for your windows and use the component's parameter:

.. code-block:: xml

  <using controller="object://hostapp/MyComponent">
    <Slider name="numericValue"/>
  </using>


Qt code
=======

General idea: Controls are independent of each other. Controls store their own state. They don't know about the parameters or actions they control. Controls are connected via signals to manipulate and/or display values of a model.

Create a model class. Add an attribute numericValue. Add a slot setNumericValue and a signal numericValueChanged.

Create windows with controls that should display/manipulate numericValue.

Connect the controls to the model, e.g.:

.. code-block:: cpp

  connect(someSlider, sliderMoved, myModel, setNumericValue);
  connect(myModel, numericValueChanged, someSlider, setValue);


===============================
How to: List/Table View Example
===============================

CCL code
========

Basic steps:

* pick model class for your needs (ListViewModel, TreeViewModel), add columns, add items
* pick corresponding control class (example: ListViewModel <-> <ListView> in skin XML)
* model is assigned to control via name in skin, it needs to be made available via getObject() from a Component.

.. code-block:: cpp
  :caption: CCL Table View example

  //************************************************************************
  // DemoTableModel
  //************************************************************************
  
  class DemoTableModel: public ListViewModel
  {
  public:
      DECLARE_STRINGID_MEMBER (kDetailA)
      DECLARE_STRINGID_MEMBER (kDetailB)
  
      DemoTableModel ()
      {
          getColumns ().addColumn (48, 0, kTitleID);
          getColumns ().addColumn (300, 0, kDetailA);
          getColumns ().addColumn (100, 0, kDetailB);
      }
  };
  
  DEFINE_STRINGID_MEMBER_ (DemoTableModel, kDetailA, "detailA")
  DEFINE_STRINGID_MEMBER_ (DemoTableModel, kDetailB, "detailB")
  
  //************************************************************************
  // TableViewDemo
  //************************************************************************
  
  class TableViewDemo: public DemoComponent
  {
  public:
      TableViewDemo ()
      : tableModel (NEW DemoTableModel)
      {
          addObject ("table", *tableModel);
  
          auto item1 = NEW ListViewItem ("Item 1");
          item1->getDetails ().set (DemoTableModel::kDetailA, "someDataA");
          item1->getDetails ().set (DemoTableModel::kDetailB, "someDataB");
          tableModel->addItem (item1);
  
          auto item2 = NEW ListViewItem ("Item 2");
          item2->getDetails ().set (DemoTableModel::kDetailA, "752");
          item2->getDetails ().set (DemoTableModel::kDetailB, "45");
          tableModel->addItem (item2);
  
          auto item3 = NEW ListViewItem ("Item 3");
          item3->getDetails ().set (DemoTableModel::kDetailA, "42");
          item3->getDetails ().set (DemoTableModel::kDetailB, "134");
          tableModel->addItem (item3);
      }
  
  protected:
      AutoPtr<DemoTableModel> tableModel;
  };


Qt code
=======

For a general introduction to Qt MV architecture, see https://doc.qt.io/qt-5/model-view-programming.html


Basic steps:

* pick abstract model class for your needs (list, table, tree), add data, implement all pure virtual methods that are called by the view class (row count, column count, cell data,...)
* pick corresponding view class (example: QAbstractTableModel <-> QTableView)
* assign model to view
* to change data and trigger view updates, use appropriate model methods (e.g. insertRows() or emit dataChanged() signals

Notes:

* model class is not threadsafe, run in UI thread!
* model data must not necessarily be tied to the model

Further concepts:

* delegates: custom cell renderes
* proxy models: sorting, filtering


Example

.. code-block:: cpp
  :caption: Table View Example: Read Only Table

  // mymodel.h
  #include <QAbstractTableModel>
  
  class MyModel: public QAbstractTableModel
  {
  Q_OBJECT
  public:
      MyModel (QObject* parent = nullptr);
  
      // Called by view: the number of rows to display.
      int rowCount (const QModelIndex& parent = QModelIndex ()) const override;
      
      // Called by view: number of columns to display
      int columnCount (const QModelIndex& parent = QModelIndex ()) const override;
  
      // Called by view per table cell represented by 'index'; 
      // role defines the type of cell information (formatting, 
      // tooltips, etc.)
      QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const override;
  };
  
  //////////////////////////////////////////////////////////////////////////
  
  // mymodel.cpp
  #include "mymodel.h"
  
  MyModel::MyModel (QObject* parent)
  : QAbstractTableModel (parent)
  {}
  
  int MyModel::rowCount (const QModelIndex& /*parent*/) const
  {
      return 2;
  }
  
  int MyModel::columnCount (const QModelIndex& /*parent*/) const
  {
      return 3;
  }
  
  QVariant MyModel::data (const QModelIndex& index, int role) const
  {
      // DisplayRole = Text to display
      if (role == Qt::DisplayRole)
        return QString ("Row%1, Column%2")
                    .arg (index.row() + 1)
                    .arg (index.column() +1);
  
      return QVariant();
  }
  
  //////////////////////////////////////////////////////////////////////////
  
  // main.cpp
  #include <QApplication>
  #include <QTableView>
  #include "mymodel.h"
  
  int main (int argc, char *argv[])
  {
      QApplication a (argc, argv);
      QTableView tableView;
      MyModel myModel;
      tableView.setModel (&myModel);
      tableView.show ();
      return a.exec ();
  }

For the full example with additional features, see https://doc.qt.io/qt-5/modelview.html

=========================
How to: Tree View Example
=========================

CCL code
========

Apply the same concepts as shown in the table view example above but for TreeViewModel. Alternatively you can implement a custom AbstractItemModel.

.. code-block:: cpp
  :caption: CCL Tree View Example
  
  //************************************************************************
  // DemoTreeModel
  //************************************************************************
  
  class DemoTreeModel: public Object,
                       public AbstractItemModel
  {
  public:
      class TreeObject: public Object
      {
      public:
          DECLARE_CLASS (TreeObject, Object)
  
          String title;
          ObjectList children;
  
          TreeObject (StringRef title = 0)
          : title (title)
          { children.objectCleanup (true); }
      };
  
      DemoTreeModel ()
      : root (NEW TreeObject (CCLSTR ("root")))
      {
          root->children.add (NEW TreeObject (CCLSTR ("aaa")));
          root->children.add (NEW TreeObject (CCLSTR ("bbb")));
          root->children.add (NEW TreeObject (CCLSTR ("ccc")));
      }
  
      // IItemModel
      tbool CCL_API getRootItem (ItemIndex& index)
      {
          index = ItemIndex (*root);
          return true;
      }
  
      tbool CCL_API canExpandItem (ItemIndexRef index)
      {
          TreeObject* treeObject = unknown_cast<TreeObject> (index.getObject ());
          return treeObject && !treeObject->children.isEmpty ();
      }
  
      tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index)
      {
          TreeObject* treeObject = unknown_cast<TreeObject> (index.getObject ());
          if(!treeObject)
              return false;
  
          ForEach (treeObject->children, TreeObject, child)
              items.add (*child, true);
          EndFor
          return true;
      }
              
      tbool CCL_API getItemTitle (String& title, ItemIndexRef index)
      {
          TreeObject* treeObject = unknown_cast<TreeObject> (index.getObject ());
          if(!treeObject)
              return false;
  
          title = treeObject->title;
          return true;
      }
  
      CLASS_INTERFACE (IItemModel, Object)
  
  protected:
      AutoPtr<TreeObject> root;
  };
  
  DEFINE_CLASS (DemoTreeModel::TreeObject, Object)
  
  //************************************************************************
  // TreeViewDemo
  //************************************************************************
  
  class TreeViewDemo: public DemoComponent
  {
  public:
      TreeViewDemo ()
      : treeModel (NEW DemoTreeModel)
      {
          addObject ("treeModel", *treeModel);
      }
  
  protected:
      AutoPtr<DemoTreeModel> treeModel;
  };


For the full example with additional features, see applications/ccldemo.


Qt code
=======

Apply the same concepts as shown in the table view example above but for QAbstractItemModel. A simple example can be found `here <https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html>`_.