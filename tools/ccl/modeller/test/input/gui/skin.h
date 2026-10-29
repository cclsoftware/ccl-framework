//************************************************************************************************
// TestElement
/** TestElement Description */
//************************************************************************************************

class TestElement: public Object
{
public:
	DECLARE_CLASS (TestElement, Object)
	DECLARE_TYPELIB_CLASS (TestElement)
};

DEFINE_CLASS_HIDDEN (TestElement, Object)
DEFINE_TYPELIB_CLASS (TestElement, "TestElement", SKIN_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TestElement)
	ADD_TYPELIB_MEMBER (rowHeight, "int") ///< Row height in pixels.
/** Optional label text shown for the row.
Spans multiple lines - a leading comment before an ADD_TYPELIB_MEMBER_* shortcut macro, unlike the
trailing comment above, exercises a separate code path in the scanner. */
	ADD_TYPELIB_MEMBER_STRING (rowLabel)
END_TYPELIB_MEMBERS (TestElement)

//************************************************************************************************
// TestElement::Style
//************************************************************************************************

enum class TestElementStyle
{
	kNormal, ///< Normal, unselected row style.
	kHighlighted ///< Highlighted, selected row style.
};

DEFINE_TYPELIB_ENUMERATION (TestElement, Style, TestElementStyle, SKIN_TYPELIB)

BEGIN_STYLEDEF (TestElementStyle)
	{ Normal, kNormal },
	{ Highlighted, kHighlighted }
END_STYLEDEF

//************************************************************************************************
// RelatedDocView
/** RelatedDocView Description */
//************************************************************************************************

class RelatedDocView: public Object
{
public:
	DECLARE_CLASS (RelatedDocView, Object)
};

DEFINE_CLASS_HIDDEN (RelatedDocView, Object)

//************************************************************************************************
// RelatedDocElement
// Carries no doc comment of its own - DEFINE_TYPELIB_RELATED_CLASS points the scanner at
// RelatedDocView's comment instead.
//************************************************************************************************

class RelatedDocElement: public Object
{
public:
	DECLARE_CLASS (RelatedDocElement, Object)
	DECLARE_TYPELIB_CLASS (RelatedDocElement)
};

DEFINE_CLASS_HIDDEN (RelatedDocElement, Object)
DEFINE_TYPELIB_CLASS (RelatedDocElement, "RelatedDocElement", SKIN_TYPELIB)
DEFINE_TYPELIB_RELATED_CLASS (RelatedDocElement, RelatedDocView)

//************************************************************************************************
// QualifiedNestedOuterElement
// Carries no doc comment of its own, and no DEFINE_TYPELIB_RELATED_CLASS either - this is a
// regression test for a class-name-parsing bug, not the related-class feature. An out-of-line
// nested class defined elsewhere with a qualified name ("QualifiedNestedOuterElement::Nested",
// below) must never be mistaken for a same-named top-level class's own documentation.
//************************************************************************************************

class QualifiedNestedOuterElement: public Object
{
public:
	DECLARE_CLASS (QualifiedNestedOuterElement, Object)
	DECLARE_TYPELIB_CLASS (QualifiedNestedOuterElement)

	class Nested;
};

DEFINE_CLASS_HIDDEN (QualifiedNestedOuterElement, Object)
DEFINE_TYPELIB_CLASS (QualifiedNestedOuterElement, "QualifiedNestedOuterElement", SKIN_TYPELIB)

//************************************************************************************************
// QualifiedNestedOuterElement::Nested
/** QualifiedNestedOuterElement::Nested Description */
//************************************************************************************************

class QualifiedNestedOuterElement::Nested: public Object
{
public:
	Nested () {}
};

//************************************************************************************************
// AliasElement::Type
// Enumerators alias constants documented in another header (skinaliasbase.h).
// Their docu must be taken from the aliased constants.
//************************************************************************************************

class AliasElement: public Object
{
public:
	enum Type
	{
		kInteger = kAliasBaseInteger,
		kString = kAliasBaseString
	};
};

DEFINE_TYPELIB_ENUMERATION (AliasElement, Type, AliasElementTypes, SKIN_TYPELIB)

BEGIN_STYLEDEF (AliasElementTypes)
	{ int, AliasElement::kInteger },
	{ string, AliasElement::kString }
END_STYLEDEF
