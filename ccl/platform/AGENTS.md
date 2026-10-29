# CCL Platform Implementation Guide

_Audience: Framework maintainers implementing or modifying a platform backend under `ccl/platform/{platform}/`._

This file is loaded automatically alongside the root [`AGENTS.md`](../../AGENTS.md) whenever you're working on files in this subtree — see the root file for code style, string handling, and other cross-cutting conventions that also apply here.

## Platform Implementation Guidelines

When implementing platform-specific code in `ccl/platform/{platform}/`:

**File organization:**
- Use naming convention: `classname.{platform}.cpp`/`.mm` (e.g., `window.win.cpp`, `webbrowserview.cocoa.mm`, `system.linux.cpp`)
- Place platform-specific helper functions in a platform namespace (e.g., `CCL::Win`) - never in an anonymous namespace (see the root style guide)
- Keep the public class implementation in the global or `CCL` namespace

**Objective-C/++ files** (macOS and iOS backends only, `documentation/codestyle/objectivec.rst`):
- Use the `.mm` extension for Objective-C++ source
- Add a space before each selector/type in method declarations, but no space inside nested `[ ]` message-send brackets

**Cross-platform consistency:**
- Follow the same patterns as other platform implementations for similar functionality
- Use consistent naming conventions within a platform (e.g., Windows classes use a `Windows` prefix - `WindowsMouseCursor`, `WindowsTooltip`, `WindowsCursorFactory` - follow the same pattern for a new platform)
- Return types should be consistent: use `bool` for success/failure, `tresult` for CCL API methods

**Error handling:**
- Platform-specific operations should fail gracefully
- Return appropriate error codes (`kResultFailed`, `false`, `nullptr`) rather than crashing

## Vendored Libraries Backing Platform Code

Several `submodules/` dependencies are wired into specific platform trees, not used generically:
- **Skia** (2D rendering) - used on Mac, Linux, and WebAssembly only (`ccl/platform/shared/skia/`, plus `linux/skia/`, `cocoa/skia/`), with Vulkan interop under `shared/vulkan/`; Windows uses Direct2D (`ccl/platform/win/direct2d/`) and Android has its own native graphics backend instead

When adding a new platform backend for one of these subsystems (rendering, embedded browser, compositor), check whether the corresponding submodule already has a platform abstraction you should follow rather than introducing a new one.
