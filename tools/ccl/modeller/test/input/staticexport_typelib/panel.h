// Type library fixture for '-staticexport ... typelib=PANEL_TYPELIB'. Covers: a class-backed root
// entry (abstract, docgroup), a plain DEFINE_TYPELIB_ENTRY_WITH_PARENT entry, member macros (built-in
// shortcuts, WITH_TYPE, and a local custom alias), attribute space-join on a duplicate key, an
// enumeration with local enumerator names, and a parent-chained enumeration (no local enumerators).

#define TAG_PANEL "Panel"
#define TAG_LABEL "Label"
#define ATTR_ENABLED "enabled"
#define ATTR_OPACITY "opacity"
#define ATTR_LABEL "label"
#define ATTR_HIGHLIGHTCOLOR "highlightColor"
#define ATTR_ALIGNMENT "alignment"

// Local stand-in for a project-specific ADD_TYPELIB_MEMBER_* shortcut (e.g. guitypelib.h's
// ADD_TYPELIB_MEMBER_COLOR) - alias discovery only sees #defines within the folder being scanned,
// not the real framework headers.
#define ADD_TYPELIB_MEMBER_COLOR(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, "color")

DEFINE_TYPELIB_CLASS (Panel, "Panel", PANEL_TYPELIB)
DEFINE_TYPELIB_ABSTRACT (Panel)
DEFINE_TYPELIB_DOCGROUP (Panel, "Controls")
BEGIN_TYPELIB_MEMBERS (Panel)
	ADD_TYPELIB_MEMBER_BOOL (ATTR_ENABLED)
	ADD_TYPELIB_MEMBER_FLOAT (ATTR_OPACITY)
END_TYPELIB_MEMBERS (Panel)
BEGIN_TYPELIB_ATTRIBUTES (Panel)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE ("Panel")
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE ("Control")
	ADD_TYPELIB_CHILDGROUP_ATTRIBUTE ("Panel")
END_TYPELIB_ATTRIBUTES (Panel)

DEFINE_TYPELIB_ENTRY_WITH_PARENT (Label, Panel, "Label", PANEL_TYPELIB)
BEGIN_TYPELIB_MEMBERS (Label)
	ADD_TYPELIB_MEMBER_STRING (ATTR_LABEL)
	ADD_TYPELIB_MEMBER_COLOR (ATTR_HIGHLIGHTCOLOR)
END_TYPELIB_MEMBERS (Label)

BEGIN_ENUMINFO (PanelAlignment)
	{"left", 0},
	{"center", 1},
	{"right", 2}
END_ENUMINFO

DEFINE_TYPELIB_ENUMERATION (TAG_PANEL, ATTR_ALIGNMENT, PanelAlignment, PANEL_TYPELIB)
DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_LABEL, ATTR_ALIGNMENT, nullptr, TAG_PANEL, ATTR_ALIGNMENT, PANEL_TYPELIB)
