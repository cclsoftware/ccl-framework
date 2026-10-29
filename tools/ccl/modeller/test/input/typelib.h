//************************************************************************************************
// TestItem
/** TestItem Description */
//************************************************************************************************

class TestItem: public Object
{
public:
	DECLARE_CLASS (TestItem, Object)
	DECLARE_TYPELIB_CLASS (TestItem)
};

DEFINE_CLASS_HIDDEN (TestItem, Object)
DEFINE_TYPELIB_CLASS (TestItem, "TestItem", TEST_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TestItem)
	ADD_TYPELIB_MEMBER (name, "string") ///< Name of the item.
END_TYPELIB_MEMBERS (TestItem)

//************************************************************************************************
// TestGroup
//************************************************************************************************

// Self-contained stand-in for the real ccl/base/typelibmacros.h shortcut - AliasCollector only
// sees #defines within the folder being scanned (./input/), not the real framework headers.
#define ADD_TYPELIB_MEMBER_STRING(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kString)

DEFINE_TYPELIB_ENTRY (TestGroup, "TestGroup", TEST_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TestGroup)
	ADD_TYPELIB_MEMBER_STRING (label) ///< Display label for the group.
/** Icon shown next to the label.
Spans multiple lines - a leading comment before an entry's ADD_TYPELIB_MEMBER_* shortcut, unlike
the trailing comment above, exercises a separate code path in the scanner. */
	ADD_TYPELIB_MEMBER_STRING (icon)
END_TYPELIB_MEMBERS (TestGroup)

//************************************************************************************************
// TestItem::Mode
//************************************************************************************************

enum class TestItemMode
{
	kNormal,
	kAdvanced
};

BEGIN_ENUMINFO (TestItemMode)
	{"normal", kNormal}, ///< Normal operating mode.
	{"advanced", kAdvanced} ///< Advanced operating mode.
END_ENUMINFO

DEFINE_TYPELIB_ENUMERATION (TestItem, Mode, TestItemMode, TEST_TYPELIB)
