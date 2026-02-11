##########
Components
##########

A fundamental building block of the Core library is the :cref:`Core::Portable::Component` class. It follows a Composite pattern using a parent-child hierarchy.

TypedObject
###########

A Component is a :cref:`Core::Portable::TypedObject` supporting Core Run-time Type Information.

Declare core class macro:

.. code-block:: cpp

	// Connecting a type id of a derived class to that of its base class
	DECLARE_CORE_CLASS ('myCp', MyComponent, Component)

You can then cast objects to a class that has used the core class declaration:

.. code-block:: cpp

	if(Component* component = core_cast<Component> (myComponent))
	{
		// component as Component
	}
		
	if(MyComponent* component = core_cast<MyComponent> (myComponent))
	{
		// component as MyComponent
	}

	
Adding Parameters
#################

You can add :cref:`Core::Portable::Parameter` objects of various types using provided macros:

.. code-block:: cpp

	BEGIN_PARAMINFO (ParamList)
		PARAM_TOGGLE (kToggle1, "toggle1", 0, 0, flags),
		PARAM_INT (kIntValue1, "intValue1", kMinValue, kMaxValue, 0, 0, 0, 0, flags),
		PARAM_STRING (kStringValue1, "stringValue1", flags),
		PARAM_LIST (kStringList1, "stringList1", MyStringList, 0, flags),
		PARAM_ALIAS (kParamAlias1, "alias1", flags)
	END_PARAMINFO

This creates an array of :cref:`Core::Portable::ParamInfo` which you can add to the :cref:`Core::Portable::ParamList` owned by your component.

.. code-block:: cpp

	paramList.add (ParamList, ARRAY_COUNT (ParamList));

Parameter Access
################

:cref:`Core::Portable::Component` implements methods to access its parameters as well as parameters of other components.
Examples:

.. code-block:: cpp

	Parameter* param1 = getParameterAt (index); ///< our parameter list
	Parameter* param2 = getParameterByTag (tag); ///< our parameter list
	Parameter* param3 = findParameter ("paramname"); ///< named parameter in our parameter list or overridden to access others
	Parameter* param4 = getRootComponent ()->lookupParameter ("toplevelcomponent/childcomponent/paramname"); ///< any parameter in the component tree


Parameter Observation
#####################

A Component is a :cref:`Core::Portable::IParamObserver`. It automatically observes changes of its own parameters.

.. code-block:: cpp

	void MyComponent::paramChanged (Parameter* p, int msg)
	{
		switch(p->getTag ())
		{
		case toggle1 :
			// do something
			break;
		}
		Component::paramChanged (p, msg);
	}

A :cref:`Core::Portable::Component` can observe parameters of other components by adding your component as an observer to some other parameter.

.. code-block:: cpp

	Parameter* otherParameter = getRootComponent ()->lookupParameter ("toplevelcomponent/childcomponent/paramname");
	otherParameter->addObserver (this);

Persistance
###########

The :cref:`Core::Portable::Component` class implements :cref:`Core::Portable::Component::save` and :cref:`Core::Portable::Component::load` methods to store parameter data to memory section or file system.
Example using a :cref:`Core::Portable::SettingFile` for storage:

.. code-block:: cpp

	// save
	Attributes& a = SettingFile::instance ().getSection (getName ());
	AttributesBuilder builder (a);
	OutputStorage storage (builder);
	save (storage);

.. code-block:: cpp

	// load
	Attributes& a = SettingFile::instance ().getSection (getName ());
	InputStorage storage (a);
	load (storage);
	
Dynamically Adding Components
#############################

You can dynamically create and add child components to your component. Cleanup is handled by the Component base class.

.. code-block:: cpp

	addChild (NEW SubComponent ("mySubComponent");

Usage
#####

You would create a custom :cref:`Core::Portable::Component` class as part of a component tree where it would manage data and functions related to a common purpose. 
Example usage of a component would be a controller for a graphical user interface section. 
In such a case a UI layout may contain several controls that are connected to parameters that are owned by your Component class. Your class can ``load`` and ``save`` the parameter values, perform functions related to user changes, etc.