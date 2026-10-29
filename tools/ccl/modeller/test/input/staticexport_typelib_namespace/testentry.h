// Reference below omits "Outer::", relying on this using-directive - same pattern as real framework
// code that relies on "using namespace CCL;" to reach a nested namespace's constant.
using namespace Outer;

DEFINE_TYPELIB_ENTRY (TestEntry, "TestEntry", DISAMBIGUATE_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TestEntry)
	ADD_TYPELIB_MEMBER_STRING (Inner::kName)
END_TYPELIB_MEMBERS (TestEntry)
