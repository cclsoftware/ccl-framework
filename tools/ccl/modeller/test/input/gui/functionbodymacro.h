//************************************************************************************************
// Function body macros
// DEFINE_VECTOR_COMPARE, DEFINE_VECTOR_COMPARE_OBJECT and DEFINE_ARRAY_COMPARE open a function
// body that the caller closes with '}'. "MacroView" must still resolve to TestMacro::MacroView,
// not the decoy declared first.
//************************************************************************************************

namespace TestMacroDecoy {

//************************************************************************************************
// TestMacroDecoy::MacroView
/** Decoy MacroView Description */
//************************************************************************************************

class MacroView
{
};

} // namespace TestMacroDecoy

namespace TestMacro {

//************************************************************************************************
// TestMacro::MacroView
/** MacroView Description */
//************************************************************************************************

class MacroView: public Object
{
public:
	DECLARE_CLASS (MacroView, Object)
};

// Namespace scope: the closing '}' must not end the enclosing namespaces early.
namespace MacroSorter
{
	DEFINE_ARRAY_COMPARE (sortByName, Object, lhs, rhs)
		return 0;
	}

	DEFINE_VECTOR_COMPARE (sortByType, Object, lhs, rhs)
		return 0;
	}
}

// Class scope inside a function body.
void sortMacroViews ()
{
	struct Sorter
	{
		static DEFINE_VECTOR_COMPARE_OBJECT (compare, AutoPtr<MacroView>, lhs, rhs)
			return 0;
		}
	};
}

//************************************************************************************************
// MacroElement
// No doc comment of its own, uses the related class.
//************************************************************************************************

class MacroElement: public Object
{
public:
	DECLARE_CLASS (MacroElement, Object)
	DECLARE_TYPELIB_CLASS (MacroElement)
};

DEFINE_CLASS_HIDDEN (MacroElement, Object)
DEFINE_TYPELIB_CLASS (MacroElement, "MacroElement", SKIN_TYPELIB)
DEFINE_TYPELIB_RELATED_CLASS (MacroElement, MacroView)

} // namespace TestMacro
