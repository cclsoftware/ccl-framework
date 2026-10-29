#define kCycleA kCycleB
#define kCycleB kCycleA

class CyclicClass: public Object
{
public:
	DECLARE_CLASS (CyclicClass, Object)

	BEGIN_PROPERTY_NAMES (CyclicClass)
		DEFINE_PROPERTY_NAME (kCycleA)
	END_PROPERTY_NAMES (CyclicClass)
};

DEFINE_CLASS (CyclicClass, Object)
