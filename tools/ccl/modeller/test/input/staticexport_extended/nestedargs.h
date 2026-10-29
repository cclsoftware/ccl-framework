namespace TestNamespace {

class NestedArgsClass: public Object
{
public:
	DECLARE_CLASS (NestedArgsClass, Object)

	BEGIN_PROPERTY_NAMES (NestedArgsClass)
		DEFINE_PROPERTY_METACLASS ("nested", SomeTemplate (Foo, Bar))
	END_PROPERTY_NAMES (NestedArgsClass)
};

DEFINE_CLASS (NestedArgsClass, Object)

} // namespace TestNamespace
