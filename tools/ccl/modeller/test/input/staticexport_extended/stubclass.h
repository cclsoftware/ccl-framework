class InterfaceList: public Object
{
public:
	DECLARE_CLASS (InterfaceList, Object)
};

DEFINE_CLASS (InterfaceList, Object)

REGISTER_STUB_CLASS (ISomeInterface, SomeInterfaceStub)
