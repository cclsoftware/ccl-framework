namespace TestNamespace {

// Covers DEFINE_CLASS_PERSISTENT: its 3rd argument (the persistent storage name) must be
// exported as the class's name instead of the C++ class name, and any subclass must reference
// it by that same persistent name in its "parent" attribute.

class PersistentClass: public Object
{
public:
	DECLARE_CLASS (PersistentClass, Object)
};

DEFINE_CLASS_PERSISTENT (PersistentClass, Object, "StoredPersistentClass")

class PersistentSubClass: public PersistentClass
{
public:
	DECLARE_CLASS (PersistentSubClass, PersistentClass)
};

DEFINE_CLASS (PersistentSubClass, PersistentClass)

} // namespace TestNamespace
