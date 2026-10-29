//************************************************************************************************
// DocClass
/** DocClass Description.
	\see DocClass::doSomething
*/
//************************************************************************************************

class DocClass: public Object
{
public:
	DECLARE_CLASS (DocClass, Object)

	BEGIN_METHOD_NAMES (DocClass)
		/** doSomething brief description.
			\code{.cpp}
			DocClass c;
			c.doSomething (42);
			\endcode
		*/
		DEFINE_METHOD_ARGS ("doSomething", "value: int")
	END_METHOD_NAMES (DocClass)

	BEGIN_PROPERTY_NAMES (DocClass)
		/** fooProperty brief description. */
		DEFINE_PROPERTY_NAME ("fooProperty")
	END_PROPERTY_NAMES (DocClass)
};

class DocStringIdClass: public Object
{
public:
	DECLARE_CLASS (DocStringIdClass, Object)

	BEGIN_PROPERTY_NAMES (DocStringIdClass)
		/** stringIdProperty brief description. */
		DEFINE_PROPERTY_NAME (kStringIdPropertyName)
	END_PROPERTY_NAMES (DocStringIdClass)
};

DEFINE_STRINGID_MEMBER (DocStringIdClass, kStringIdPropertyName, "resolvedName")
