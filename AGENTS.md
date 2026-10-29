# CCL Framework - Development Guide

This document contains useful information for working with the Crystal Class Library (CCL) framework.

**⚠️ Important: Read the "Spacing rules" section in "Code Style" before editing code!**

**Audience tags:** Each section is marked with who it's for — `Everyone`, `Framework maintainers` (editing CCL source itself), or `Component authors` (writing application/plug-in code against CCL's public API). Platform-backend implementation details live in nested guides that only load when you're working in those subtrees: [`ccl/platform/AGENTS.md`](ccl/platform/AGENTS.md) and [`core/platform/AGENTS.md`](core/platform/AGENTS.md). This split mirrors the Sphinx docs: `documentation/cclpublicsdk/` aggregates guides for people building applications/extensions on top of CCL, while `documentation/cclmaintainer/` covers internal tooling (code generation, doc builds) for people editing CCL's own source.

## Architecture

_Audience: Everyone_

The repository has two main library trees: `core/` and `ccl/`. `core/` is the lower-level foundation library; `ccl/` (the full application framework - GUI, networking, security, text, and more, not GUI-only) depends on `core/` and links against it — never the reverse. Historically CCL was one codebase, but in 2012 the lower layers were split out into Core so they could be reused on RTOS-based embedded platforms; `core/portable/` is a small, largely separate application/GUI toolkit for those embedded targets and isn't used by the desktop/mobile CCL stack. Both `core/` and `ccl/` mirror each other's subtree layout (`gui/`, `network/`, `platform/`, `public/`, `system/`, `text/`, `meta/`) — Core's versions are minimal/portable primitives, CCL's are the full-featured frameworks built on top.

The CCL framework uses platform-specific implementations organized under `ccl/platform/{platform}/`:
- `linux/` - Linux/Wayland implementations (X11 is not supported)
- `wasm/` - WebAssembly/Emscripten implementations
- `win/` - Windows implementations
- `cocoa/` - macOS and iOS implementations
- `android/` - Android implementations (JNI bridge to native Activity/View, Vulkan-based rendering)

Each platform implements the same interfaces but uses platform-native APIs. See [`ccl/platform/AGENTS.md`](ccl/platform/AGENTS.md) for guidance on adding or modifying a platform implementation, and [`core/platform/AGENTS.md`](core/platform/AGENTS.md) for the equivalent Core Library layer.

## Code Style

_Audience: Everyone_

Follow the framework code style documented in `documentation/codestyle/`:

**Separator lines:**
- Use `//************************************************************************************************` (96 asterisks) for class declarations and implementations
- Use `//////////////////////////////////////////////////////////////////////////////////////////////////` (96 forward slashes) between method definitions

**Key conventions:**
- Source files: all lowercase
- Include guards: `_filename_h`
- CCL/Core's own headers are included with quoted paths relative to the framework repository (e.g. `#include "ccl/base/singleton.h"`)
- Forward declarations preferred over includes where possible
- TAB-based indentation (size 4)
- **Avoid abbreviations** in variable names - use descriptive full words (e.g., `dragImage` not `dragImg`, `bitmap` not `bmp`)
- **Always initialize variables** - including arrays (use `= {}` for zero-initialization)
- **Avoid anonymous namespaces** - use static functions or private/protected class members instead

**Property macros:**
- `PROPERTY_BOOL`, `PROPERTY_INT`, etc. generate getter/setter methods
- Place property macros in the `public:` section, immediately after the constructor
- These macros manage public/protected scopes internally, so they belong in `public:` even though they generate protected member variables
- Initialize the member variable in the constructor's initialization list

**Generated methods:**
- `PROPERTY_BOOL (member, Method)` generates:
  - `bool isMethod () const` - getter
  - `void setMethod (bool state = true)` - setter
  - `bool member` - protected member variable

Example:
```cpp
class MyClass: public Object
{
public:
    MyClass ();
    
    PROPERTY_BOOL (active, Active)
    PROPERTY_VARIABLE (int, value, Value)
    
    // Other public methods...
};

// Constructor must initialize property members
MyClass::MyClass ()
: active (false),
  value (0)
{}

// Usage
if(isActive ())
    setActive (false);
```

**Namespace usage:**
- **Never use `using namespace CCL` in header files** - always use explicit qualifiers
- In implementation (.cpp) files: add `using namespace CCL;` after the namespace closes, then omit `CCL::` prefixes throughout

Example:
```cpp
// In .cpp file
namespace CCL {
	// class implementations inside namespace
} // namespace CCL

using namespace CCL;

// Now use types without CCL:: prefix
Url* url = unknown_cast<Url> (document.getPath ());  // Not CCL::Url*
```

**Spacing rules:**
- No space after control statement keywords: `if(`, `for(`, `while(`, `switch(`, `catch(`
- Space before opening parenthesis of function calls: `foo (x)`
- No space before opening parenthesis of: `sizeof(`, `typeof(`
- No space before square brackets: `array[i]`
- No space before template angle brackets: `List<int>`

**Type casting:**
- For primitive types: use C-style function cast **without space** before parenthesis: `int(value)`, `float(x)`, `TouchID(identifier)`
- For plain C++ pointer/class casts, use `static_cast` with a space: `static_cast<MyClass*> (ptr)`
- For casting between `IUnknown`-derived interfaces, use `unknown_cast<T>` with a space instead: `unknown_cast<MyClass> (ptr)` - unlike `static_cast`, this performs a runtime COM-style `queryInterface` lookup, not a compile-time reinterpretation
- Prefer `CStringPtr` over `const char*` when casting: `static_cast<CStringPtr> (ptr)` not `static_cast<const char*> (ptr)`
- Examples:
  ```cpp
  // Primitive type casts - NO space
  int x = int(floatValue);
  TouchID id = TouchID(touch.identifier);
  int64 time = int64(timestamp);
  Point where (int(touch.targetX), int(touch.targetY));
  
  // Class/pointer casts - WITH space
  Url* url = unknown_cast<Url> (object);
  int* values = static_cast<int*> (data);
  
  // String pointer casts - use CStringPtr, not const char*
  CStringPtr data = static_cast<CStringPtr> (buffer.getMemoryAddress ());  // Correct
  // NOT: const char* data = static_cast<const char*> (...)  // Wrong!
  ```

**Block formatting:**
- Empty catch blocks: opening brace on next line, empty body
  ```cpp
  catch(e)
  {}
  ```
- Single-line else blocks: omit braces if not required
  ```cpp
  if(condition)
      doSomething ();
  else
      doSomethingElse ();  // No braces for single statement
  ```

**Override and virtual keywords:**
- Use `override` keyword when overriding virtual methods from base classes
- Don't repeat `virtual` keyword when overriding (it's implicit) - just use `override`
- Keep `virtual` only for the base class declaration

**Method naming:**
- Use consistent verb prefixes for lifecycle methods:
  - `initialize*` / `terminate*` for paired setup/teardown operations (e.g., `initializeSelector()` / `terminateSelector()`)
  - `get*` for simple accessors that compute values (e.g., `getElementId()`)

**Comments on overridden methods:**
- Indicate which base class a method is overriding from, not the interface it implements
- Example: If `Application` inherits from `Component`, and overrides `findChild` originally defined in `IObjectNode`, comment should say `// Component` not `// IObjectNode`

**Naming rules** (documented in `documentation/codestyle/namingrules.rst`):
- CamelCase throughout; types and enumerators start uppercase, variables/members start lowercase
- Constants and enumerators are prefixed `k` (e.g. `kResultOk`)
- Interfaces are prefixed `I` (e.g. `IStream`)
- No Hungarian notation, and no `is` prefix on boolean member variables (the `is`-prefixed accessor is generated by the property macro, not the member name itself)
- Acronyms are uppercase in type/method names but lowercase when used as a variable/member name, e.g. `NoteID noteId`
- Always use `nullptr`, never `0` or `NULL`; avoid Yoda conditions (`if(nullptr == ptr)`); a `switch`'s `default` case must not be first, the only case, or empty

**Code reuse in CCL's own source:**
- Prefer refactoring over duplication - when implementing similar functionality, look for existing patterns to reuse or refactor
- Extract common code into shared utilities only when it simplifies the implementation
- Avoid unnecessary abstraction layers - if code is only used in one place, inline it directly
- Platform-specific implementations should follow similar patterns to their counterparts on other platforms
- Share common base class implementations - when multiple platform implementations inherit from the same abstract base and need identical method implementations, extract those methods to a separate cross-platform source file that all platforms can link against

**Public API boundary rule** (`documentation/codestyle/design.rst`) - this is the concrete rule behind the public/internal distinction referenced throughout this document: what actually decides it is the *ABI* boundary, not just API layering - an interface that gets called across a binary boundary (e.g. plug-in/host, scripting bridge, where caller and callee may be built with different compilers) may only use primitive types in its signature, since implementation classes aren't guaranteed layout-stable across binaries. An interface that stays within CCL itself may use those types freely. When deciding whether a new method belongs on a public-facing interface, check whether it needs to cross that ABI boundary, not just whether it "feels" public.

**Memory allocation internals:** all CCL allocations are redirected through `Core::core_malloc`/`core_free` (in `core/malloc/`); the `NEW` macro (used when writing CCL code, see "Memory Management" in Code Patterns) expands to a global `operator new` override that records `__FILE__`/`__LINE__` in debug builds, which is how leaks get traced back to a source location.

**Include minimization:**
- Always use the leanest include list that compiles - if a header's symbols are already reachable transitively through another include, don't add a direct include for it just because the file uses those symbols directly
- Verify by removing includes one at a time and rebuilding; keep the removal whenever the build still succeeds
- Prefer forward declarations in headers; full includes in implementation files

## Debug Logging

_Audience: Everyone_

The CCL framework provides debug logging macros defined in `ccl/public/base/debug.h`.

### Debug Macros

**Enabled in debug builds only (requires `#define DEBUG_LOG 1` before including debug.h):**

- `CCL_PRINT(s)` - Print a string (no newline)
- `CCL_PRINTLN(s)` - Print a string with newline
- `CCL_PRINTF(format, ...)` - Printf-style formatting with variadic arguments

**Always available (even in release builds):**

- `CCL_WARN(format, ...)` - Warning output (printf-style with variadic arguments)

**Important:**
- `CCL_PRINTF` and `CCL_WARN` require variadic arguments. Use `CCL_PRINT` or `CCL_PRINTLN` for messages without format arguments.
- If you need `CCL_WARN` for a message without format arguments, pass a dummy `0` argument: `CCL_WARN ("Message\n", 0)`
- **⚠️ NEVER add a semicolon after CCL_WARN or CCL_PRINTF calls** - these macros already include the semicolon in their expansion. Adding a semicolon will cause compilation errors with if/else statements.
- To enable debug logging, define `DEBUG_LOG` as 1 *before* including any headers that use the macros:

```cpp
#define DEBUG_LOG 1

#include "ccl/public/base/debug.h"
```
- Do this in `.cpp` files only, never in headers - a header-level `#define` would leak the setting into every other file that includes it transitively

**Examples:**

```cpp
// Correct usage
CCL_PRINTLN ("Starting operation...")  // No format args - use PRINTLN
CCL_PRINTF ("Processing %d items\n", count)  // Format args - use PRINTF (no semicolon!)
CCL_WARN ("Invalid value: %d\n", value)  // Warning with format (no semicolon!)
CCL_WARN ("Fatal error\n", 0)  // Warning without format args - pass dummy 0 (no semicolon!)

// Incorrect usage
CCL_PRINTF ("Starting operation...\n")  // ERROR: PRINTF requires variadic args
CCL_WARN ("Message\n")  // ERROR: WARN requires variadic args
CCL_WARN ("Error\n", 0);  // ERROR: Don't add semicolon - macro already has one!
```

## Code Patterns

_Audience: Everyone. Everything below is public API - patterns for writing C++ code that calls into CCL. Framework-internal conventions (the public/internal API boundary rule, code-reuse principles for CCL's own source, generated code, platform-specific rendering internals) live in [Code Style](#code-style), [Code Generation](#code-generation), and [`ccl/platform/AGENTS.md`](ccl/platform/AGENTS.md) instead._

### Interface Implementation

- Use `interface` keyword for abstract interfaces (COM-style interfaces that inherit from `IUnknown`)
- Forward-declare interfaces with `interface` keyword, not `class`:
  ```cpp
  // Correct
  interface IStream;
  
  // Incorrect
  class IStream;
  ```
- Implement `IUnknown` for COM-style interfaces
- Use `unknown_cast<T>` for safe interface casting
- Check result codes: `kResultOk`, `kResultFalse`, etc.

### Memory Management

- `NEW` macro for object creation (instead of `new`) - use for both objects and arrays: `NEW MyClass`, `NEW char[size]` 
- `AutoPtr<T>` for automatic memory management
- `SharedPtr<T>` for reference-counted shared ownership
- Manual `release()` for IUnknown-derived objects
- Use `delete[]` to free arrays allocated with `NEW []`
- **Get Rule:** a `get*` method never transfers ownership of what it returns - if the caller needs to own/free the result, the method should be named `create*` or `copy*` instead
- Constructors should only initialize member state, never do real work (I/O, allocation of other objects, etc.) - follow up with an explicit `initialize()` or `load()` call

### Object to Unknown Reference Conversion

When passing an object as an `IUnknown&` or `Unknown&` reference, use the `asUnknown()` method, which is available on all `Object`-derived classes and returns `IUnknown*`.

### String Handling

Use CCL's `CString`/`String` methods rather than C standard library functions for string operations:

**String types:**
- `String` - Unicode string (UTF-16 internally), owns its data
- `CString` - Thin wrapper around `const char*`, does NOT own data
- `MutableCString` - Mutable C string buffer, owns its data
- `CStringPtr` - Typedef for `const char*`

**Important distinctions:**
- `CString` is just a wrapper - it doesn't allocate or own memory
- Use `MutableCString` when you need to own/modify ASCII/UTF-8 strings
- `String` constructors can accept ASCII strings via `String(const char*)` or with encoding `String(TextEncoding, const char*)`
- Prefer `CStringPtr` over `const char*` throughout (see the "Type casting" rule in Code Style) - the same preference applies here to function signatures and variable declarations, not just casts

**Finding substrings:**
- `.index(char)` - Get first position of ASCII character
- `.index(CStringPtr)` - Get position of substring
- `.lastIndex(char)` - Get last position of character
- `.contains(char)` or `.contains(CStringPtr)` - Check if string contains character/substring

**Extracting substrings:**
- `.subString(start, count)` - Extract substring (if count is -1, extract to end)

**Converting to numbers:**
- `.scanInt()` or `.scanInt(fallback)` - Convert string to integer
- `.scanFloat()` or `.scanFloat(fallback)` - Convert string to float
- `.scanDouble()` or `.scanDouble(fallback)` - Convert string to double
- `.scanLargetInt()` or `.scanLargetInt(fallback)` - Convert string to int64

**Converting between types:**
```cpp
// Unicode String to C string
MutableCString cString (unicodeString, Text::kUTF8);

// C string to Unicode String
String unicodeString (Text::kUTF8, cStringBuffer);

// UrlFullString to C string
UrlFullString urlString (url, true);
MutableCString urlCString (urlString, Text::kUTF8);
navigateToUrl (urlCString);  // Implicit conversion to const char*
```

**Building strings efficiently:**
```cpp
// ❌ DON'T create temporary String objects
String url = String ("http://") + String (protocol) + String ("://") + String (hostname);

// ✅ DO use MutableCString with operator+= for ASCII/UTF-8 buffers
MutableCString url = "http://";
url += protocol;
url += "://";
url += hostname;

// ✅ DO use String::append()/operator<< or appendFormat() when the result needs to stay Unicode
String settingsID (String ("NewDocument.") << documentClass.getFileType ().getExtension ());
String message;
message.appendFormat (XSTR (LoadingElement), args, argCount);
```

**Key points:**
- `MutableCString` has `operator+=` for in-place concatenation
- `MutableCString` does NOT have `operator+` (avoids temporary object creation)
- Always prefer `+=` for building strings incrementally
- When converting `String` (Unicode) to `MutableCString`, always specify encoding: `MutableCString (stringVar, Text::kUTF8)`

**Using `.str()` with variadic functions:**

`MutableCString` and `CString` have implicit conversion to `const char*`, but variadic functions (printf-style) require explicit `.str()` call:

```cpp
MutableCString url = "http://example.com";

// ❌ WRONG - variadic function cannot use implicit conversion
CCL_WARN ("URL: %s\n", url);  // Compiler error: non-trivial type through variadic function

// ✅ CORRECT - explicit .str() for variadic functions
CCL_WARN ("URL: %s\n", url.str ());

// ✅ CORRECT - normal functions use implicit conversion
non_variadic_function (url);  // No .str() needed
```

**Rule:** Use `.str()` with `CCL_WARN`, `CCL_PRINTF`, and any printf-style variadic function. Don't use `.str()` for normal function calls.

**Example:**
```cpp
// Parse comma-separated integers
CString input = "123,456";
int commaPosition = input.index (',');
if(commaPosition >= 0)
{
    int first = input.subString (0, commaPosition).scanInt ();
    int second = input.subString (commaPosition + 1).scanInt ();
}
```

**Do not use:** `sscanf`, `atoi`, `atof`, `strtol`, `snprintf`, `sprintf` - prefer the CCL string methods above.

**Passing strings as parameters:**
- Prefer `StringRef`/`CStringRef` over `const String&`/`const CString&` in function signatures (similarly `StringID`, `UIDRef`, `UrlRef` for their respective types)
- Don't mix `String` and `MutableCString` within the same code path - converting between encodings partway through risks a silent downgrade

**Localization:**
- Wrap all user-facing text in `XSTRING`/`XSTR`, even text that isn't currently localized - this keeps it ready for translation extraction (the `tools/ccl/xstring` tool scans for these macros to produce `.po`/`.xlf` files)

### Geometry Types

**Rect access:**
- Direct member access: `left`, `top`, `right`, `bottom`
- Methods: `getWidth()`, `getHeight()`, `getCenter()`, `getPosition()`

```cpp
Rect size (getSizeInWindow ());
int x = size.left;        // Direct member access
int y = size.top;
int w = size.getWidth (); // Method for computed values
int h = size.getHeight ();
```

### Display Scale Factors (HiDPI Support)

`getContentScaleFactor()` is used internally by custom rendering/platform code, not part of the typical application-facing API surface - application code building UI should generally rely on higher-level layout rather than reading the scale factor directly.

- Use `window->getContentScaleFactor()` to get the current display scaling (1.0, 2.0, 3.0 for HiDPI)
- When creating bitmaps for display, pass the scale factor: ` GraphicsFactory::createBitmap (width, height, format, scaleFactor)`
- The scale factor indicates physical pixels per logical pixel (2.0 means 2x resolution)
- Graphics rendered at scaled resolution must be drawn using proper rendering APIs, not simple pixel copying

### Drag-and-Drop System

The CCL framework provides a cross-platform drag-and-drop system through the public `IDragSession` interface (`ccl/public/gui/framework/idragndrop.h`) and `DragEvent` struct (`ccl/public/gui/framework/guievent.h`). Component authors implement drag targets by overriding `onDragEnter()`/`onDragOver()`/`onDrop()` from `UserControl` (`ccl/app/controls/usercontrol.h`).

**Key concepts:**
- **IDragSession** - Represents an active drag operation, contains the dragged items
- **DragEvent** - Event sent during drag operations (`kDragEnter`, `kDragOver`, `kDragLeave`, `kDrop`); exposes the session via its `session` member
- **Drop effects** - `IDragSession::DropResults` flags indicating allowed operations: `kDropCopyReal`, `kDropCopyShared`, `kDropMove`, `kDropNone`

**IDragSession methods:**
- `addData(mimeType, data)` - Add data to the drag session
- `getItems()` - Get collection of dragged items (as `IUnknown*`)
- `getResult()` / `setResult(effect)` - Get/set the accepted drop effect

**Event handling:**
```cpp
bool MyView::onDragEnter (const DragEvent& event)
{
    // Check what's being dragged
    ForEachUnknown (event.session.getItems (), item)
		UnknownPtr<IUrl> url = item;
        if(url.isValid ())
        {
            // Accept file drops
            if(event.session.getResult () == IDragSession::kDropNone)
                event.session.setResult (IDragSession::kDropCopyReal);
            return true;
        }
    EndFor
    return false;
}

bool MyView::onDrop (const DragEvent& event)
{
    // Handle the dropped items
    ForEachUnknown (event.session.getItems (), item)
        UnknownPtr<IUrl> url = item;
        if(url.isValid ())
            processFile (*url);
    EndFor
    return true;
}
```

**Return values:**
- `onDragEnter()` and `onDragOver()` return `int` with drop effect flags
- The result determines the cursor visual feedback shown to the user
- Return `IDragSession::kDropNone` to reject the drop

### Kernel Initialization

Use `CCL_KERNEL_INIT_LEVEL` for early initialization:

```cpp
CCL_KERNEL_INIT_LEVEL (SystemClasses, kFirstRun)
{
    // Initialization code
    return true;
}
```

Beyond per-component init, every binary has exactly one top-level entry point depending on what it is, and all of them route through `Kernel::instance().initialize()`/`terminate()`:
- GUI apps define `void ccl_app_init ()`, invoked from the framework's `ccl_main_gui`
- Console apps define `int ccl_main (ArgsRef args)`
- Dynamically-loaded plug-in modules export `CCLModuleMain(module, reason)`, handling `kModuleInit`/`kModuleExit`

### Plug-in and Service Registration

Services and plug-ins (e.g. everything under the top-level `services/` directory - bluetooth, sqlite, jsengine, firebase, etc.) register themselves through the same COM-style mechanism as the `IUnknown`/`unknown_cast` pattern above, not a bespoke "service" interface: each exports `CCL_EXPORT IClassFactory* CCL_API CCLGetClassFactory ()`, which calls `registerClass (ClassDesc, createInstance)` for each class it provides, tagged with a category constant (e.g. `PLUG_CATEGORY_DATABASEENGINE`) and a UID.

## Testing

_Audience: Everyone_

CCL uses a custom unit-test framework (`ccl/base/unittest.h`):
- Write tests with `CCL_TEST(SuiteName, TestName) {...}`, or `CCL_TEST_F`/`CCL_TEST_P`/`CCL_TEST_T` for fixture/parameterized/templated variants
- Assert with `CCL_TEST_ASSERT`, `CCL_TEST_ASSERT_EQUAL`, `_NOT_EQUAL`, `_NEAR`, `_FALSE`
- Tests self-register into a global registry - no manual test-list maintenance
- Suite names end in `Test`; source files end in `test.cpp` (e.g. `stringtest.cpp`)
- Follow F.I.R.S.T. principles and Arrange-Act-Assert structure; put shared setup in `setUp()` rather than helper methods
- See `documentation/codestyle/testing.rst` for the full guide

## Skinning and Theming

_Audience: Everyone_

CCL separates an application's business logic (C++ component tree) from its visual appearance, which is defined in an XML "skin language" — see `documentation/skinlanguage/` for the full DSL reference. Three related but distinct concepts are involved:
- **Skin** - an XML package (a folder or `.skin` ZIP) with a root `skin.xml` declaring `Imports`, `Includes`, `Resources`, `Styles`, and `Forms`; loaded at app launch. Implemented in `ccl/gui/skin/` (`SkinRegistry`, `SkinWizard`).
- **Theme** - the runtime object holding *resolved* colors, fonts, metrics, named styles, and renderers once a skin has been parsed; implemented in `ccl/gui/theme/` (`Theme`, `ThemeManager`). Controls query the active theme (`getThemeColor`, `getStyle`, `createRenderer`) when painting.
- **Palette** - unrelated to styling controls; a color/image picker abstraction (`ColorPalette`, `ImagePalette` in `ccl/gui/theme/palette.h`) used by widgets like color swatches.

The top-level `skins/neutralbase` and `skins/neutraldesign` packages are the framework's reference "default look" - `neutraldesign` defines the base style/layout definitions, and `neutralbase` supplies the actual colors/fonts underneath them. Most apps import both.

## Extensions

_Audience: Component authors_

CCL extensions/plug-ins are distributed as digitally-signed `.install` ZIP packages containing `metainfo.xml`, `installdata.xml`, and `package.iconset`. Signing uses RSA developer tokens (JWTs) created via the `cclcrypt` CLI tool; the vendor name declared in `metainfo.xml` must exactly match the vendor in the signing token or the host app's install-time verification fails. See `documentation/extension/overview.rst` for the full packaging format.

## Directory Structure

_Audience: Everyone_

Repository top level:
```
framework/
├── ccl/               # CCL framework - full application framework (GUI, networking, security, text, ...), depends on core/
├── core/              # Core Library - lower-level foundation, no dependency on ccl/
├── classmodels/        # XML schema defs (feed code + doc generators)
├── services/           # Plug-in services (bluetooth, sqlite, jsengine, firebase, ...)
├── skins/              # XML skin packages (neutralbase, neutraldesign = default look)
├── submodules/         # Third-party dependencies (git submodules)
├── tools/              # Build-time CLIs (code generator, packager, doc tools, ...)
├── documentation/      # Sphinx docs - cclpublicsdk/ (app/extension authors), cclmaintainer/ (framework internals)
└── build/              # CMake build output
```

`ccl/` subtree:
```
ccl/
├── platform/           # Platform-specific implementations
│   ├── linux/
│   ├── wasm/
│   ├── win/
│   ├── cocoa/
│   └── android/
├── public/            # Public interfaces
├── gui/               # GUI framework
│   ├── graphics/      # 2D and 3D graphics; image and bitmap handling
│   ├── skin/          # Skin XML loading (see "Skinning and Theming")
│   ├── theme/          # Resolved runtime styles/colors (see "Skinning and Theming")
│   ├── system/        # System integration (cursors, clipboard, etc.)
│   └── windows/       # Window management
├── base/              # Base classes and utilities
├── network/            # Networking (wraps core/network/ sockets)
├── security/           # Licensing, credentials, cryptography
├── test/               # Unit tests (see "Testing")
├── main/               # App/console/module entry points (see "Kernel Initialization")
├── packaging/          # Per-platform installer resources (icons, manifests, NSIS scripts)
├── extras/             # Optional integrations (Firebase, analytics, Node.js, in-app purchases, ...)
└── text/              # Text processing
```

## Code Generation

_Audience: Framework maintainers_

Some source in this repo is generated, not hand-written — never edit it directly:
- `core/meta/` and `ccl/meta/` each hold `models/*.json` + `templates/{lang}/*.in`, rendered by the `cclgenerator` tool into `generated/{lang}/*` (C++, and for `ccl/meta` also TypeScript/JavaScript/Python/Java). Regenerate via each tree's `generate_all.sh`.
- The top-level `classmodels/*.classModel` files are XML schema definitions (for skin elements, meta info, visual styles, etc.) consumed by the Modeller tool (`tools/ccl/modeller/`) to produce both generated code (via `ccl/meta`) and reference documentation (`documentation/skinlanguage/reference/`, `documentation/coreskin/reference/`). Regenerating classmodel output requires rebuilding the Modeller tool first. Most `*.classModel` files are created by the Modeller tool by serializing runtime meta classes, some are hand-authored.

## Compiler Configuration

_Audience: Everyone (applies to any code built against CCL, not just framework internals)_

- C++17 standard
- RTTI disabled on every platform: `-fno-rtti` on Linux/Android/WebAssembly, `/GR-` on Windows (`build/cmake/modules/shared/vendor.win.cmake`), Xcode's `GCC_ENABLE_CPP_RTTI=NO` on Mac/iOS
- CCL's own code avoids exceptions by convention, but this isn't a hard compiler restriction everywhere: Windows explicitly compiles with `/EHa` and Mac/iOS with `GCC_ENABLE_CPP_EXCEPTIONS=YES` (both enable exception handling, needed for SEH/crash-handling interop), and Android re-enables exceptions per-target (`-fexceptions`) for the few libraries that need them
- Uses custom memory allocation (`NEW` macro)

## Build System

_Audience: Everyone_

The CCL framework uses CMake with presets for different platforms and configurations.

**For detailed build instructions, see:** `documentation/cclframework/building.rst`

**CMake Presets:**
The framework provides presets for all supported platforms:
- `win32`, `win64`, `win-arm64` - Windows builds (Visual Studio)
- `mac`, `mac-default-sdk` - macOS builds (Xcode)
- `ios` - iOS builds
- `linux-debug`, `linux-release` - Linux x86_64 builds
- `linux-arm64-debug`, `linux-arm64-release` - Linux ARM64 builds
- `android` - Android builds
- `wasm-debug`, `wasm-release` - WebAssembly builds (Emscripten)

**Quick start:**
```bash
# List available presets
cmake --list-presets

# Configure with a preset
cmake --preset mac

# Build
cd build/mac && cmake --build .
```

**Note:** Platform-specific files only compile for their target platform (e.g., `dragndrop.win.cpp` only compiles for Windows builds)

**Other build-related notes:**
- `cpphint.json` is not a lint/style gate - it configures `cpphint.py`, which generates a Visual Studio IntelliSense `.hint` file so the IDE's parser understands CCL's macros (`PROPERTY_BOOL`, `EndFor`, etc.)
- Custom build-time CLI tools (code generator, packager, doc tools, ...) live under `tools/` and are resolved via `tools/bin/findtool.sh {category} {toolname}`

## URL Handling

_Audience: Everyone writing C++ against CCL_

The CCL framework provides a comprehensive URL handling system through the `Url` class and related utilities, implemented in [ccl/base/storage/url.cpp](ccl/base/storage/url.cpp).

### URL Structure

URLs in CCL follow the format: `protocol://hostname/path?parameters`

**Key components:**
- **Protocol** - Identifies the resource type (e.g., `file`, `memory`, `package`, `resource`)
- **Hostname** - Domain or identifier
- **Path** - Resource location using `/` as separator
- **Type** - `kFile`, `kFolder`, or `kDetect` (auto-determined by trailing `/`)
- **Parameters** - Optional key-value pairs after `?`

### Path Handling

**Native path conversion:**
```cpp
// Windows: Converts / to \, handles UNC paths (\\server\share)
// Linux/macOS: Ensures leading /
char buffer[CORE_STRING_STACK_MAX];
url.toNativePath (buffer, sizeof(buffer));

// From native path
url.fromNativePath (nativePath, Url::kFile);
```

**POSIX path conversion:**
```cpp
char buffer[CORE_STRING_STACK_MAX];
url.toPOSIXPath (buffer, sizeof(buffer));
url.fromPOSIXPath (posixPath, Url::kFile);
```

**Display strings:**
```cpp
String displayString;
url.toDisplayString (displayString, Url::kStringDisplayPath);
url.fromDisplayString (displayString, Url::kFile);
```

### URL Manipulation

**Path navigation:**
```cpp
url.ascend ();              // Move up one directory (changes type to kFolder)
url.descend (name, type);   // Move down to child path

url.isRootPath ();          // Check if at root
url.isAbsolute ();          // Check if absolute path
url.isRelative ();          // Check if relative path
```

### Common Patterns

**Creating file URLs:**
```cpp
// From native path
Url url;
url.fromNativePath ("/path/to/file.txt", Url::kFile);

// From components
Url url ("file", "", "path/to/file.txt", Url::kFile);

// From URL string
Url url ("file:///path/to/file.txt", Url::kFile);
```

**Creating URLs from relative paths:**
```cpp
Url baseUrl ("file:///home/user", Url::kFolder);
Url url;
url.fromRelativePath ("./documents/file.txt", baseUrl, Url::kFile);
```

**Checking URL containment:**
```cpp
Url folder ("file:///home/user/documents", Url::kFolder);
Url file ("file:///home/user/documents/file.txt", Url::kFile);
bool contains = folder.contains (file);  // true
```

**Making unique file names:**
```cpp
url.makeUnique ();  // Appends " 2", " 3", etc. if file exists
url.makeUnique (true);  // Forces suffix even if doesn't exist
```

### Platform-Specific Behavior

**Windows:**
- Handles drive letters (`C:/path`)
- Supports UNC paths (`//server/share`)
- Recognizes UNC prefixes (`\\?\` and `\\.\\`)
- Converts backslashes to forward slashes internally

**Linux/macOS:**
- Absolute paths start with `/`
- Case sensitivity determined by file system

### Best Practices

1. **Never tear a filesystem URL apart with `getPath()`** - use `UrlDisplayString`, `UrlFullString`, or `fromDisplayString()` instead; `getPath()` alone loses protocol/encoding information the rest of the API relies on
2. **Always specify type** - Use `kFile`, `kFolder`, or `kDetect` explicitly
3. **Use forward slashes** - Internal representation always uses `/`
4. **Normalize relative paths** - Call `normalize()` after constructing from user input
5. **Check for empty URLs** - Use `isEmpty()` before operations
6. **Case sensitivity** - Be aware of platform differences
7. **Use helpers** - Leverage `UrlUtils` for common operations
8. **File existence** - URLs don't validate existence; check with file system
9. **Thread safety** - URL objects are not inherently thread-safe

### Common Mistakes

**❌ Don't use `setPath()` alone to create URLs from filesystem paths:**
```cpp
// WRONG - creates URL without protocol
Url* url = NEW Url;
url->setPath ("/tmp/file.txt", IUrl::kFile);  // Missing protocol!
```

**✅ Use `fromNativePath()` or `fromDisplayString()` instead:**
```cpp
// CORRECT - properly sets protocol and path
Url* url = NEW Url;
url->fromNativePath ("/tmp/file.txt", IUrl::kFile);  // Sets file:// protocol
```

**Note:** `setPath()` only sets the path component of an existing URL. It does not set the protocol. When creating a new URL from a filesystem path, always use one of the `from*` methods:
- `fromNativePath()` - For platform filesystem paths
- `fromPOSIXPath()` - For POSIX paths (internally calls `fromNativePath()` on non-Windows)
- `fromDisplayString()` - For display strings (internally calls `fromNativePath()`)

**Implementation detail:** On inspection of url.cpp:
- `fromDisplayString()` is equivalent to `fromNativePath()` (just a wrapper)
- Both properly set the `file://` protocol for filesystem paths
- `setPath()` by contrast only modifies the path member variable

## Related Documentation

- Platform-specific implementations: [`ccl/platform/AGENTS.md`](ccl/platform/AGENTS.md)
- Core-library platform abstraction: [`core/platform/AGENTS.md`](core/platform/AGENTS.md)
- Public interfaces: `ccl/public/`
- Graphics subsystem: `ccl/gui/graphics/`
- Imaging classes: `ccl/gui/graphics/imaging/`
- URL implementation: `ccl/base/storage/url.cpp`
- URL encoding: `ccl/base/storage/urlencoder.h`
- Code style reference (naming, testing, Objective-C, design rules): `documentation/codestyle/`
- Code review checklist: `documentation/codereviewguide/`
- Skin language reference: `documentation/skinlanguage/`
- Extension packaging format: `documentation/extension/`
- Framework introduction / library layering: `documentation/cclframework/introduction.rst`
