# Changelog

**CCL 5.2.0 (2026-10-29)**

*New features and improvements:*

- Support for WebAssembly
- Reusable audio runtime infrastructure
- Text input method support on all platforms
- Extended EditBox now enabled by default on desktop platforms
- JavaScript | QuickJS-NG engine alternative to SpiderMonkey
- Divider | Improved double-click behavior with ScrollViews
- Menu | Additional menu item value attribute
- Accessibility | Parameter value formatting for screen reader
- Accessibility | Expose Toggle as Button ("accessibility.button")
- Accessibility | Override title announced by accessibility tools ("accessibility.title")
- ICredentialManager | Asynchronous credential access
- ISystemShell | Support for system permissions
- ITextLayout | New method to clear formatting
- UrlUtils | Helper method to check for absolute paths
- Extensions | Allow apps to add additional development locations
- Refactoring | AtomRenderer using RenderAtoms for controls
- Refactoring | Generalized parameter definitions
- Refactoring | Generalized 'Skin Elements' and 'Visual Styles' type library macros
- Refactoring | Consolidated CRC16/32 and Hash function in textservices.h
- Refactoring | Consolidated meta class definitions (namespaces/categories)
- Refactoring | Use semantic string types (CStringPtr/UStringPtr) consistently
- CLI Tools | Static class model export via cclmodeller ("-staticexport" option)
- CLI Tools | Format option for cclmakeguid (inline, braces, etc.)
- Core Library | New configuration header file coreconfiguration.h
- Core Library | Title parameter for Button class ("titlename" in JSON)
- Windows | WebView2 integration completed and enabled by default
- Windows | Improved Bluetooth LE robustness
- iOS | Deployment target set to 15.3
- iOS | Migration to UIScene-based Lifecycle
- Linux | Support for Ubuntu 26.04
- Linux | Use cursor-shape protocol for themed mouse cursor
- Linux | Distribution name and version included in system information

*The following issues have been fixed:*

- Network | Potential hang with DNS resolution (c-ares now enabled by default)
- MenuControl | Issues with submenus on mobile platforms
- App with modal dialogs open can crash in Application::requestQuit
- Graphics Layers | Replaced animations not updated correctly
- ProgressDialog | Multi-step progress doesn't resize instantly
- MutableCString | Use-after-clone issue in trimWhitespace() corrupting shared buffers
- Accessibility (Windows) | Narrator navigation not working for groups
- Accessibility (Windows) | Disabled state of accessible items sometimes incorrect
- Accessibility (Windows) | Actions exposed by accessible elements sometimes incorrect
- Windows DirectWrite | Multiline text wraps too early
- Windows Direct2D | Rounded rectangles have bulgy corners in certain situations
- macOS | Context menu missing with CTRL-click
- macOS/iOS | POSIX functions called in NFC normalization rather than NFD
- Skia | Unnecessary line breaks in text layout in certain situations
- Skia/Quartz | Some SVG paths not rendered correctly
- Linux | TLS stream corruption on partial socket writes
- Linux | Initial focus missing in dialogs

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
