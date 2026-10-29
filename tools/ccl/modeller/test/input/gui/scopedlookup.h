//************************************************************************************************
// Scoped class name lookup
// "ScopedView" must resolve to TestCCL::ScopedView, not the decoy declared first.
//************************************************************************************************

namespace TestCore {
namespace Portable {

//************************************************************************************************
// TestCore::Portable::ScopedView
/** Decoy ScopedView Description */
//************************************************************************************************

class ScopedView
{
};

} // namespace Portable
} // namespace TestCore

namespace TestCCL {

//************************************************************************************************
// TestCCL::ScopedView
/** ScopedView Description */
//************************************************************************************************

class ScopedView: public Object
{
public:
	DECLARE_CLASS (ScopedView, Object)
};

//************************************************************************************************
// ScopedElement
// No doc comment of its own, uses the related class.
//************************************************************************************************

class ScopedElement: public Object
{
public:
	DECLARE_CLASS (ScopedElement, Object)
	DECLARE_TYPELIB_CLASS (ScopedElement)

	bool isEnabled () const;
};

// Must not end the enclosing namespace early.
bool ScopedElement::isEnabled () const
{
	return true;
}

DEFINE_CLASS_HIDDEN (ScopedElement, Object)
DEFINE_TYPELIB_CLASS (ScopedElement, "ScopedElement", SKIN_TYPELIB)
DEFINE_TYPELIB_RELATED_CLASS (ScopedElement, ScopedView)

} // namespace TestCCL

//************************************************************************************************
// UsingScopedElement
// Global scope, resolves via using-directive.
//************************************************************************************************

using namespace TestCCL;

class UsingScopedElement: public Object
{
public:
	DECLARE_CLASS (UsingScopedElement, Object)
	DECLARE_TYPELIB_CLASS (UsingScopedElement)
};

DEFINE_CLASS_HIDDEN (UsingScopedElement, Object)
DEFINE_TYPELIB_CLASS (UsingScopedElement, "UsingScopedElement", SKIN_TYPELIB)
DEFINE_TYPELIB_RELATED_CLASS (UsingScopedElement, ScopedView)
