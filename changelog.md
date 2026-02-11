# Changelog

**CCL 5.0.1 (2026-02-11)**

*New features and improvements:*

- Allow tooltips while text control is active and hide when typing
- Skin XML | Add "extended" visual style metric for ToolButton popup menu
- PlugInManager | Store last modifed time of plug-ins as Unix timestamp
- Windows | Update MSVC++ redistributable to 14.50.35719.0
- Tool and documentation sources added

*The following issues have been fixed:*

- Mouse handler not canceled when view is being removed
- DialogBuilder | Focus view not reset when used for multiple runs
- Presets | PresetBrowser shouldn't handle horizontal mouse wheel events
- Presets | PresetDragFilter doesn't accept presets stored with alternative class ids
- Skia macOS | Clipping not handled correctly for sibling NSViews
- Android | Implement IGraphicsLayer::placeAbove()/placeBelow()
- Linux | Repeating key events when losing focus
- Linux | Scale factor not applied immediately when creating window
- Linux/Android | POSIX file copy truncates destination when source doesn't exist
- CMake | Compiler setting fixes for macOS

**CCL 5.0.0 (2026-01-12)**

- Initial dual-licensed (commercial and AGPLv3) source release on Github
