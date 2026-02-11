#########################
Run-time type information
#########################

The Core Library provides its own run-time type information (RTTI) mechanism that can be used in place of the one that C++ offers. This can be useful in cases where RTTI isn't properly supported by particular toolchain or when RTTI is disabled due to other constraints. The RTTI implementation in Core allows to perform safe typecasts using the :cref:`Core::Portable::core_cast` template function and supporting macros.

Using RTTI
==========

To use RTTI with your own objects, your class needs to derive from :cref:`Core::Portable::ITypeInfo` and implement the :cref:`Core::Portable::ITypeInfo::castTo` function. This is usually done by using the :cref:`DECLARE_CORE_CLASS` macro in the class declaration. The following example demonstrates how to add type information to a class.

.. code-block:: cpp

  #include "core/portable/coretypeinfo.h"

  class Parameter: public TypedObject
  {
  public:
  	DECLARE_CORE_CLASS ('Para', Parameter, TypedObject)
  
  	Parameter (const ParamInfo& info, bool ownsInfo = false);
  	~Parameter ();
  
  	// ... other declarations following
  };

In this example the :cref:`Core::Portable::Parameter` class derives from :cref:`Core::Portable::TypeInfo`. The ``TypeInfo`` class again derives directly from ``ITypeInfo`` and provides a simple implementation of ``castTo`` that always returns ``nullptr``. The :cref:`DECLARE_CORE_CLASS` macro then assigns the type identifier ``'Para'`` to the class and also overrides the ``castTo`` function which returns a pointer to the object itself when the type identifier matches or passes the call up to the derived class. Other classes deriving from :cref:`Core::Portable::Parameter` (like :cref:`Core::Portable::NumericParam` or :cref:`Core::Portable::ListParam`) do the same to implement RTTI.

In order to perform a dynamic typecast on an object, use the :cref:`Core::Portable::core_cast` function as shown as in the following code snippet.

.. code-block:: cpp

  Parameter* param = ...
  
  NumericParam* numParam = core_cast<NumericParam> (param);
  if(numParam != nullptr)
  {
  	// This is a NumericParam object and its safe to call it's functions.
  }
  else
  {
  	// Not a NumericParam
  }

If you must obtain the type identifier for a particular class use the :cref:`Core::Portable::core_typeid` template function.

.. code-block:: cpp

  ITypedObject::TypeId id = core_typeid<NumericParam> ();
  // id equals 'NPar'