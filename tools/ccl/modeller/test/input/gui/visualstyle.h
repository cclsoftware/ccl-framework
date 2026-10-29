//************************************************************************************************
// TestButton
//************************************************************************************************

// Self-contained stand-in for the real ccl/gui/guitypelibmacros.h shortcut - AliasCollector only
// sees #defines within the folder being scanned (./input/), not the real framework headers.
#define ADD_TYPELIB_MEMBER_COLOR(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kColor)

// Stand-in for a StyleID-style constants header, referenced below by fully-qualified name
// (TestStyleID::...), matching how the real code relies on "using namespace CCL;" to omit
// the outermost namespace.
namespace TestCCL
{
	namespace TestStyleID
	{
		const CStringPtr kSpacing = "spacing";
		const StringID kOutline = CSTR ("outline");
	}
}

DEFINE_TYPELIB_ENTRY (TestButton, "TestButton", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TestButton)
	ADD_TYPELIB_MEMBER_COLOR (highlightColor) ///< Color used to highlight the button.
	ADD_TYPELIB_MEMBER_COLOR (TestStyleID::kSpacing) ///< Spacing between button elements.
	ADD_TYPELIB_MEMBER_COLOR (TestStyleID::kOutline) ///< Outline color of the button.
END_TYPELIB_MEMBERS (TestButton)
