#########
Scripting
#########


========================
Script Method Definition
========================

Methods

.. code-block:: cpp

  DEFINE_METHOD_NAME (name)
  DEFINE_METHOD_ARGS (name, args)
  DEFINE_METHOD_ARGR (name, args, retval)


Typed arguments

.. code-block:: cpp

  // Use "argname: argtype", examples:
  DEFINE_METHOD_ARGS ("setEnabled", "state: bool")
  DEFINE_METHOD_ARGR ("add", "param: Parameter", "Parameter")
  DEFINE_METHOD_ARGS ("setItems", "items: ItemType[]")


Default values

.. code-block:: cpp

  // Use "argname: argtype = defaultvalue"
  DEFINE_METHOD_ARGS ("restore", "force: bool = false")
  DEFINE_METHOD_ARGR ("runDialog", "formName: string, themeID: string = null", "int")


Containers

.. code-block:: cpp

  // Append "[]" to a type to indicate a container with items of that type:
  DEFINE_METHOD_ARGS ("addItems", "items: Item[]")


Composites

.. code-block:: cpp

  // Combine with pipe "|":
  DEFINE_METHOD_ARGR ("selectClass", "cid: UID | string", "bool")


Variadic arguments

.. code-block:: cpp

  // Prefered: set max supported number of args as optional args:
  DEFINE_METHOD_ARGS ("invokeMethod",
    "methodName: string, arg0: string = null, arg1: string = null, arg2: string = null")

  // Fallback: use "vargs" or "...":
  DEFINE_METHOD_ARGS ("invokeMethod", "methodName: string, vargs")
  DEFINE_METHOD_ARGS ("invokeMethod", "methodName: string, ...")


==========================
Script Property Definition
==========================

General

.. code-block:: cpp

  DEFINE_PROPERTY_TYPE (name, type)
  DEFINE_PROPERTY_CLASS (name, className)
  DEFINE_PROPERTY_CLASS_ (name, className, flags)


Typed

.. code-block:: cpp

  // Use type supported by ITypeInfo:
  DEFINE_PROPERTY_TYPE ("title", ITypeInfo::kString)


Read-only

.. code-block:: cpp

  // Combine type with flag ITypeInfo::kReadOnly:
  DEFINE_PROPERTY_TYPE ("title", ITypeInfo::kString | ITypeInfo::kReadOnly)


Classes

.. code-block:: cpp

  DEFINE_PROPERTY_CLASS ("member", "MemberClass")
  // Read-only
  DEFINE_PROPERTY_CLASS_ ("member, "MemberClass", ITypeInfo::kReadOnly)


=============
Type literals
=============

Supported type name literals, see :cref:`CCL::ITypeInfo::DataTypes` for further reference.

.. code-block:: cpp

  kInt = "int";
  kFloat = "float";
  kString = "string";
  kBool = "bool";
  kEnum = "enum";
  kVoid = "void";
  kObject = "object";
  kContainer = "container";
  kVariant = "variant";

