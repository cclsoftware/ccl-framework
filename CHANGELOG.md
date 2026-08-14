# Changelog

**CCL 5.1.2 (2026-08-14)**

*New features and improvements:*

- Added new "CCLVERSION" translation variable
- Skia | Improved error handling for bitmap decoding
- iOS | Allow asynchronous dialog operations to fail when app terminates
- macOS/iOS StoreKitManager | Improved logging and purchase restoration
- Linux | Exclude bind mounts from volume list
- Linux | Fall back to file copy and delete when rename fails
- Windows | Swallow WM_CHAR for Tab, Enter, and Return on single-line EditBox

*The following issues have been fixed:*

- Linux | Wrong initial DPI scale factor when using OpenGL ES
- Linux | WindowEvent kFullscreenEnter/kFullscreenLeave not triggered
- Linux | Graphics layers flushed before window becomes visible
- Linux | Integer overflow in signal wait time
- Linux | SIGPIPE in socket send calls
- Android | Wrong font selected in certain situations
- Windows | Fractional DPI scaling may cause redraw issues
- Windows | Unintentional scrolling on mouse move in some situations
- Windows | Foreground window claimed even if application is inactive
- iOS | System sharing sheet missing background
- macOS/iOS Quartz | Application fonts not registered correctly

**CCL 5.1.1 (2026-06-29)**

*New features and improvements:*

- Improved "Getting Started" documentation
- Skin XML | Simplified usage of new `<Graphic>` tag

*The following issues have been fixed:*

- Fixes for CCL SDK and CCL Builder on Windows, macOS, and Linux
- Neutral Design | Fixes for Light Mode
- Fix use-after-free in MenuControl::popup()
- Cocoa/Skia | Layer animation fixes

**CCL 5.1.0 (2026-06-12)**

*New features and improvements:*

- Markdown Support (IMarkdownParser/IMarkdownWriter)
- Block-based Content Model and View (IBlockContent, BlockView)
- Skin XML | New `<StyleCondition>` tag and IVisualStyleData interface
- Skin XML | New `<Graphic>` tag for rendering images inside shapes
- Skin XML | New "defines" attribute in `<Import>` tag for import options
- Neutral Design | New monospaced standard font "StandardUI Mono"
- TabView | New kTabViewBehaviorTooltip option
- ItemView | Set header view style from visual style
- ScrollView | Scrolling with mouse wheel leaves other controls unaffected
- HelpViewer | Viewer should not open from modal dialog
- ITextLayout | Set font face for text range
- ISignalHandler | Policy to add/replace deferred messages with same id
- Reusable OpenAI Responses API Client Classes
- New WebSseStream class for HTTP Server-sent Events (SSE)
- Extensions | Support "pluginfilter.xml" in extension root folder
- IHelpManager | Evaluate language variable in Help URL
- EULAComponent | Support for EULA Versioning
- Refactoring | Refactored platform text layout margins
- Refactoring | Shared text formatting definitions (HTML, Markdown, etc.)
- Core Library | Extended unit test classes and macros
- SpiderMonkey libraries updated to 147.0.3
- macOS/iOS | Apple SDKs set to version 26.0
- macOS/iOS | Default font changed to "System Font"
- macOS/iOS | Bluetooth Support for Writes without Response and short CBUUIDs
- macOS Metal | Improved display refresh timing

*The following issues have been fixed:*

- Skin XML | `<DropBox>` doesn't take skin scope into account when model is updated
- Windows | Native text control too small in edit mode
- macOS/iOS Quartz | Rounded rectangles and triangles not working with gradient fills
- macOS/iOS Quartz | Font styles not working correctly
- macOS/iOS Quartz | Text layout vertical alignment broken
- macOS/iOS | Crash when canceling Browser Authentication
- macOS | UnicodeCFString::remove() method negative count causes undefined behavior

**CCL 5.0.2 (2026-03-27)**

*New features and improvements:*

- CCL maintainer documentation added
- Skin XML | Support variables in "attach" attribute
- Windows | Query installed store app information 
- Linux | Improved drag session behavior
- Vulkan | Improved buffer allocation and alignment
- Metal | Improved clear behavior and buffer alignment

*The following issues have been fixed:*

- TreeView | Wrong indentation when used in menus
- Windows | Rendering issues with fractional scaling
- Windows | Rendering issues when DPI awareness disabled
- Windows | Rounding issues in DpiScale functions
- macOS/iOS | Rotation animation not working correctly
- iOS | Alert not shown on disappearing UIViewController
- Linux | Initialize versioned framework modules
- Skia | Graphics path clipping broken in some situations

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
