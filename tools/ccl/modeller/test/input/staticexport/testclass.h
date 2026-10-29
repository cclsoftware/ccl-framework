namespace TestNamespace {

class TestClass: public Object
{
public:
	DECLARE_CLASS (TestClass, Object)

	BEGIN_METHOD_NAMES (TestClass)
		DEFINE_METHOD_ARGS ("doSomething", "value: int")
	END_METHOD_NAMES (TestClass)

	BEGIN_PROPERTY_NAMES (TestClass)
		DEFINE_PROPERTY_NAME (kFooProperty)
	END_PROPERTY_NAMES (TestClass)
};

DEFINE_STRINGID_MEMBER (TestClass, kFooProperty, "foo")

DEFINE_CLASS (TestClass, Object)
DEFINE_CLASS_NAMESPACE (TestClass, "TestNamespace")

} // namespace TestNamespace
