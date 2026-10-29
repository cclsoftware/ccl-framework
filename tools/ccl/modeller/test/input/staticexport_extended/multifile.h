namespace TestNamespace {

class MultiFileClass: public Object
{
public:
	DECLARE_CLASS (MultiFileClass, Object)

	BEGIN_PROPERTY_NAMES (MultiFileClass)
		DEFINE_PROPERTY_NAME (kBarPropertyName)
	END_PROPERTY_NAMES (MultiFileClass)
};

DEFINE_CLASS (MultiFileClass, Object)

} // namespace TestNamespace
