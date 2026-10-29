// A second, unrelated type library in the same folder, so 'typelib=PANEL_TYPELIB' can be proven to
// exclude it. 'unresolvableName' is deliberately never #define'd - if the scanner ever tried to
// resolve MARKER_TYPELIB content while exporting PANEL_TYPELIB, this would turn into a hard scan
// error instead of being silently skipped by the library filter.

DEFINE_TYPELIB_ENTRY (Marker, "Marker", MARKER_TYPELIB)
BEGIN_TYPELIB_MEMBERS (Marker)
	ADD_TYPELIB_MEMBER (unresolvableName, "string")
END_TYPELIB_MEMBERS (Marker)
