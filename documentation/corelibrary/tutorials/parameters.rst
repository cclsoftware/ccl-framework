.. _parameters:

##########
Parameters
##########

The Core Library supports an architectural pattern that separates the internal representation of information from the user interface. This is accomplished using parameters, components, and views, similar to the established Model-View-Controller (MVC) pattern. There are two flavors of parameters in the library. The :cref:`Core::Portable::Parameter` class and it's derived classes can be used when heap memory is available, whereas parameters used with the more lightweight :cref:`Core::Portable::RootValueController` class are intended for use in embedded systems without a heap. This tutorial is focused on value controllers.

About Value Controllers
#######################

As the name suggests, a :cref:`Core::Portable::RootValueController` manages a set of values of different types. These can be booleans, integers, floats, and strings, all with a numeric identifier that is used to store and retrieve values. Values can have additional properties depending on the nature of the parameter they represent. For instance, integers and floats may be limited in range, have a default value, and stepping distance.

The value controller usually hosts the parameters and ensures the values stay within their defined ranges. Whenever a value changes it notifies one or more observers about the change. The GUI toolkit from the Core Library allows to connect many of its UI elements to parameters from the :cref:`Core::Portable::RootValueController`.

Using Value Controllers
#######################

Your custom controller class inherits from :cref:`Core::Portable::RootValueController` and implements ``getModelValue``,  ``setModelValue``, and ``getModelString``. The :cref:`Core::Portable::RootValueController` base class manages parameters and their definitions, however, since it is designed to be used in resource contrained environemnts it does not allocate actual memory to hold the values. Therefore, you need to list a member variable for each parameter in the class definition.

The following code block shows how to create a basic controller class with two parameters.

.. code-block:: cpp

  class MyController: Core::Portable::RootValueController
  {
  public:
  	MyController ();
  
  protected:
  	// RootValueController
  	Value getModelValue (int paramTag) const override;
  	bool setModelValue (int paramTag, Value value, int flags) override;
  
  private:
  	float firstValue;
  	float secondValue;
  };

Aside from the class definition, you also need to define a set of integers identifiers used to access the values. It makes sense to define an enumeration type that represents the volume parameters from the previous example.

.. code-block:: cpp

  enum MyParameters
  {
  	kFirstValue,
  	kSecondValue
  };

You also need to define parameter properties by creating a list of :cref:`Core::Portable::ParamInfo` objects that is passed to the :cref:`Core::Portable::RootValueController` constructor. The Core Library provides a set of macros to simplify the list definition. The following example uses the :cref:`BEGIN_PARAMINFO`, :cref:`END_PARAMINFO`, and :cref:`PARAM_FLOAT` macros to define the properties of the parameters.

.. code-block:: cpp

  BEGIN_PARAMINFO (kMyParamList)
  	PARAM_FLOAT (kFirstValue, "firstValue", 0.0f, 20.0f, 5.0f, 0, 0, 0, 0.0f, 0),
  	PARAM_FLOAT (kSecondValue, "secondValue", 0.0f, 20.0f, 5.0f, 0, 0, 0, 0.0f, 0)
  END_PARAMINFO

The :cref:`BEGIN_PARAMINFO` starts the declaration of an array of :cref:`Core::Portable::ParamInfo` objects using the identifier ``kMyParamList``. Each parameter is declared using the :cref:`PARAM_FLOAT` macro. Other macros are also available for the declaration of boolean, integer, string, or list parameters. The second parameter contains the name string of the parameter. It is used to allow to obtain parameter values by their name instead of using the numeric identifier. The following arguments describe the minimum, maximum, and default values. The remaining arguments are usually just set to zero. The :cref:`END_PARAMINFO` macro terminates the parameter definition.

The constructor of the ``MyController`` class can now pass the parameter information to the :cref:`Core::Portable::RootValueController` class.

.. code-block:: cpp

  MyController::MyController ()
  : RootValueController (kMyParamList, ARRAY_COUNT (kMyParamList)),
    firstValue (0.0f),
    secondValue (0.0f)
  {
  }

The base class uses the ``getModelValue`` and ``setModelValue`` functions to access the variables in the derived class. The implementation of these fuctions is straightforward.

.. code-block:: cpp

  Value MyController::getModelValue (int paramTag)
  {
  	switch(paramTag)
  	{
  	case kFirstValue:
  		return firstValue;
  
  	case kSecondValue:
  		return secondValue;
  
  	default
  		return Value ();
  	}
  }
  
  bool MyController::setModelValue (int paramTag, Value value, int flags)
  {
  	switch(paramTag)
  	{
  	case kFirstValue:
  		firstValue = value.asFloat ();
  		return true;
  
  	case kSecondValue:
  		firstValue = value.asFloat ();
  		return true;
  
  	default:
  		return false;
  	}
  }

To get and set parameter values use the ``getValue`` and ``setValue`` functions from the :cref:`Core::Portable::ValueController` class. This class also provides a number of other functions to access parameters and their properties. The following example demonstrates how to increase both values simultaneously.

.. code-block:: cpp

  MyController controller;
  
  bool increaseValues ()
  {
  	Value firstValue = controller.getValue (kFirstValue);
  	controller.setFloatValue (kFirstValue, firstValue.asFloat () + 1.0f);
  
  	Value secondValue = controller.getValue (kSecondValue);
  	controller.setFloatValue (kSecondValue, secondValue.asFloat () + 1.0f);
  }