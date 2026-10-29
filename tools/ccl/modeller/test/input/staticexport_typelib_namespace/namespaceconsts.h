// Two constants sharing a bare name but declared in different namespaces - resolving a reference
// qualified to one of them must not be treated as ambiguous with the other.

namespace Outer
{
	namespace Inner
	{
		const CStringPtr kName = "inner.value";
	}
}

namespace Legacy
{
	const CStringPtr kName = "legacy.value";
}
