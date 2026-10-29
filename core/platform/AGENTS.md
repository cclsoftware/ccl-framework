# Core Library Platform Abstraction Guide

_Audience: Framework maintainers implementing or modifying a platform backend under `core/platform/{platform}/`._

This file is loaded automatically alongside the root [`AGENTS.md`](../../AGENTS.md) whenever you're working on files in this subtree — see the root file for code style and other cross-cutting conventions that also apply here. This directory is distinct from [`ccl/platform/`](../../ccl/platform/AGENTS.md), which holds CCL framework platform implementations.

## Platform Abstraction in Core Library

Platform-specific implementations are configured through `core/platform/corefeatures.h`:

**Adding platform-specific implementations:**
1. Create platform implementation in `core/platform/{platform}/feature.{platform}.h`
2. `CORE_PLATFORM_IMPLEMENTATION` is `0`, and an undefined macro evaluates to `0` in a preprocessor `#if`/`#elif` - so a feature with no `CORE_<FEATURENAME>_IMPLEMENTATION` `#define` at all already defaults to the platform-specific implementation. Only add a `#define` in the platform section of `corefeatures.h` when you need to *override* that default (e.g. to a shared POSIX/OpenSSL implementation, or to mark it unimplemented).
3. The feature's `.cpp` file will automatically include the platform header via `CORE_PLATFORM_IMPLEMENTATION_HEADER` macro

**Example - adding a new platform:**
```cpp
// In corefeatures.h under your platform section:
#elif CORE_PLATFORM_MYPLATFORM
    #define CORE_PLATFORM_DIRECTORY myplatform
    // CORE_DEBUG_IMPLEMENTATION needs no #define here - it already defaults to platform-specific
    // Override to a shared implementation only where the platform doesn't have (or need) its own:
    #define CORE_TIME_IMPLEMENTATION CORE_FEATURE_IMPLEMENTATION_POSIX
```

Because `CORE_DEBUG_IMPLEMENTATION` defaults to `CORE_PLATFORM_IMPLEMENTATION`, `CORE_PLATFORM_IMPLEMENTATION_HEADER (coredebug)` expands to include `core/platform/myplatform/coredebug.myplatform.h`, which is expected to implement `Core::Platform::Debug::print()`.

**Available implementation options:**
- `CORE_PLATFORM_IMPLEMENTATION` - Use platform-specific implementation from `core/platform/{platform}/`
- `CORE_FEATURE_IMPLEMENTATION_POSIX` - Use shared POSIX implementation from `core/platform/shared/posix/`
- `CORE_FEATURE_IMPLEMENTATION_OPENSSL` - Use OpenSSL implementation from `core/platform/shared/openssl/`
- `CORE_FEATURE_UNIMPLEMENTED` - Feature not available on this platform

**Note:** Core has its own lower-level plug-in/DLL loading mechanism, separate from CCL's `ClassFactory`/`CCLGetClassFactory` plugin registration (see the root AGENTS.md's "Plugin and Service Registration" section) — don't conflate the two when working at this layer.
