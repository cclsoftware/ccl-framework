.. _wasm-implementation:

###################################
WebAssembly Platform Implementation
###################################

The WebAssembly backend (``ccl/platform/wasm``, based on Emscripten) implements CCL's platform interfaces against browser APIs (DOM, Fetch, IndexedDB, Web Crypto, ...) instead of native system calls. This page documents the implementation details and trade-offs behind that.

===================
Cross-origin issues
===================

SharedArrayBuffer and multithreading
-------------------------------------

The Wasm build enables Emscripten pthreads (``-sUSE_PTHREADS=1 -sSHARED_MEMORY=1``, see ``build/cmake/modules/shared/vendor.wasm.cmake``). Threads are implemented on top of ``SharedArrayBuffer``, which browsers only expose to a page that is *cross-origin isolated*, i.e. served with

.. code-block:: rst

  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp

response headers. This is a hosting/serving concern, not something the framework or the build can set - if the application is served without these headers, ``SharedArrayBuffer`` is unavailable and the pthread-based build won't run. The framework checks the resulting state at runtime via ``self.crossOriginIsolated`` (``ccl/platform/wasm/gui/webbrowserview.wasm.cpp``) but does not (and cannot) request the headers itself.

Single-threaded applications can disable pthreads support in order to avoid COOP/COEP restrictions. See "CMake options" below.

WebBrowserView
---------------

There is no native embedded browser control on Wasm; ``WasmWebControl`` (``ccl/platform/wasm/gui/webbrowserview.wasm.cpp``) implements ``CCL::WebBrowserView`` with a plain ``<iframe>`` placed inside the parent's DOM element. This has two consequences:

* When the page is cross-origin isolated (see above, required for threads), the browser's same-origin policy makes ``navigate ()`` refuse to load cross-origin URLs inside the iframe - only ``file:``, protocol-relative, or same-origin URLs are allowed. Anything else is instead handed off to ``CCL::ISystemShell::openUrl`` (opens the URL in a new tab/window). The same restriction can be requested explicitly via the ``kWebBrowserViewBehaviorRestrictToLocal`` custom style.
* Even when a cross-origin page *is* allowed to load, reading it back out (current URL, title, back/forward history) fails silently: ``iframe.contentWindow.location``/``.document`` throw once the iframe content is cross-origin, so ``getUrl ()``/``getTitle ()``/``canGoBack ()`` just return empty/false in that case.

HTTP requests
-------------

``WasmHTTPClient`` (``ccl/platform/wasm/network/httpclient.wasm.cpp``) is implemented on top of the browser ``fetch ()`` API, not a native socket. Ordinary browser CORS rules apply: a cross-origin request to a server that doesn't send the right ``Access-Control-Allow-Origin`` response header fails the same way it would for a plain web page. There is no way to bypass this from C++. The user agent also cannot be overridden (``setUserAgent ()`` returns ``kResultNotImplemented``) since ``fetch ()`` doesn't allow it.

``fetch ()`` and its response stream are main-thread-only JS APIs, so a request issued from a worker thread is proxied to the main thread and the calling thread polls until the request completes.

============================================
IndexedDB file system (vs. host file system)
============================================

``CCL::INativeFileSystem`` on other platforms addresses the real host file system (or what appears to be the host file system in a sandbox). On Wasm, ``WasmNativeFileSystem`` (``ccl/platform/wasm/system/nativefilesystem.wasm.cpp``) is a thin ``PosixNativeFileSystem`` subclass, but the POSIX calls underneath resolve into Emscripten's virtual file system rather than a real disk.

The persistent part of that virtual file system is a browser-side database, not a disk: ``CCL.initFS`` (``ccl/platform/wasm/js/cclsystem.js``) mounts Emscripten's ``IDBFS`` on ``/userdata`` with ``autoPersist: true`` and does a one-time ``FS.syncfs (true, ...)`` on startup to pull any previously persisted data back into memory. From then on, writes are auto-persisted to IndexedDB by Emscripten; there is no explicit periodic sync elsewhere in the framework. Practical implications:

* Data under ``/userdata`` is scoped to the browser's storage for the page's origin - clearing site data/IndexedDB in the browser deletes it, and it is not visible to the real OS file system, or to applications running on other origins. Isolation is per-origin, not per-application: Any other CCL application served from the *same* origin therefore mounts the exact same database and shares the same ``/userdata`` file tree with it.
* There is no file system change notification: ``WasmFileManager::startWatching``/``stopWatching`` (``ccl/platform/wasm/system/filemanager.wasm.cpp``) both return ``kResultNotImplemented``, since IDBFS has no OS-level watch mechanism to hook into. There also is no need for watching file system changes as the file system is expected to be used by the current application exclusively.

Bundled application resources are loaded through the same shared ``FrameworkResourceFileSystem`` abstraction used on other platforms (``ccl/platform/wasm/system/resourceloader.wasm.cpp``); they are not part of the IDBFS-backed ``/userdata`` tree.

============================
File selectors (upload only)
============================

``WasmFileSelector`` (``ccl/platform/wasm/gui/fileselector.wasm.cpp``) implements ``CCL::IFileSelector`` using a hidden HTML ``<input type="file">`` element - the browser's native upload picker. Selected files are copied into a temporary location in the Wasm file system before the rest of the framework sees them as ``CCL::IUrl`` results.

There is no native "Save As" dialog letting the user pick an arbitrary destination path: a "save" operation (``getSaveBehavior ()`` returns ``kSaveNeedsContent``) instead writes the content to a temporary file and triggers a browser *download* of it (via a ``Blob`` and a synthetic anchor click). The user only chooses where the download lands through the browser's own download UI, not through CCL.

Only the asynchronous selector API works on Wasm - the synchronous ``runPlatformSelector ()`` variant is stubbed to always fail, since the browser file picker is inherently asynchronous.

==================================
Folder selectors (not implemented)
==================================

``WasmFolderSelector`` (``ccl/platform/wasm/gui/fileselector.wasm.cpp``) exists to satisfy ``CCL::IFolderSelector``, but both ``runPlatformSelector ()`` and ``runPlatformSelectorAsync ()`` always fail. Browsers have no picker for an arbitrary host folder that a page could then write into, so there is nothing meaningful to wire up; picking/creating a folder inside the IndexedDB-backed file system with a custom in-app UI would be the only viable alternative, but no such UI exists today.

===================
Socket restrictions
===================

General POSIX sockets (``CCL::INetworkService``'s underlying ``core/network`` socket layer) are not implemented for Wasm at all - there is no ``coresocket.wasm.*``/``corenetwork.wasm.*``, unlike other platforms. Only two narrower network primitives are available, both implemented on top of browser APIs:

* HTTP requests via ``fetch ()`` (see "HTTP requests" above), subject to normal CORS rules.
* WebSockets via ``WasmWSClient`` (``ccl/platform/wasm/network/wsclient.wasm.cpp``), wrapping Emscripten's ``emscripten/websocket.h``.

There is also no HTTP *server* support - ``WasmWebEngine::createHTTPServer ()`` returns ``nullptr``.

=================
Thread priorities
=================

Wasm reuses the plain POSIX thread implementation (``core/platform/wasm/corethread.wasm.h`` just aliases ``core/platform/shared/posix/corethread.posix.h``), so setting a thread's priority goes through the same ``pthread_setschedparam``/``SCHED_RR`` calls as on Linux. Emscripten's pthread shim generally ignores scheduling changes in the browser, so a requested priority change is not guaranteed to have any effect.

==============================
Font handling (fallback fonts)
==============================

There is no native font enumeration API available in a browser sandbox, so ``WasmFontManager`` (``ccl/platform/wasm/gui/fontresource.wasm.cpp``) resolves fonts from a fixed ``/fonts`` directory in the Wasm file system via Skia's ``SkFontMgr_New_Custom_Directory``.

Fallback fonts are a small hardcoded set of named buckets (``CCLFallbackJP``, ``CCLFallbackSC``, ``CCLFallbackTC``, ``CCLFallbackKR``, ``CCLFallbackEmoji``, ``CCLFallbackSymbols``, ``CCLFallbackSymbols2``, ``CCLFallbackGeneric``), chosen by BCP-47 language tag or Unicode codepoint range (CJK ideographs are disambiguated using the current system language). The matching bucket's ``.ttf`` isn't bundled up front - it's fetched on demand (``CCL.loadFallbackFont`` in ``ccl/platform/wasm/js/cclgui.js``, which fetches ``./fonts/<name>.ttf`` into the Wasm file system) the first time a character needs it, and the glyph is only available once that async load completes and the view is redrawn.

=============
CMake options
=============

``VENDOR_WASM_THREAD_POOL_SIZE``, if set before ``ccl_configure_app`` runs (``core/cmake/coremacros.wasm.cmake``), is forwarded to Emscripten's ``-sPTHREAD_POOL_SIZE`` link flag, controlling how many worker threads are pre-spawned at startup. If left unset, the pool size defaults to ``navigator.hardwareConcurrency`` (one worker per detected logical CPU). Pre-spawning avoids the cost and deadlock risk of spawning a worker on demand the first time a thread is created; applications with a known, small thread count can set this variable to a lower fixed number to reduce startup overhead.

An app that doesn't need multithreading can avoid the COOP/COEP hosting requirement (see "Cross-origin issues" above) by turning ``VENDOR_WASM_USE_PTHREADS`` (``build/cmake/modules/shared/vendor.wasm.cmake``) ``OFF``.
This drops ``-pthread``/``-sUSE_PTHREADS=1 -sSHARED_MEMORY=1`` and the ``_PTHREADS``/``NVWA_PTHREADS`` compile definitions from every target. ``VENDOR_WASM_THREAD_POOL_SIZE`` then has no effect. Default is ``ON``.

================
Linking plug-ins
================

Plug-ins are linked dynamically, not statically into a single binary: the application is built as an Emscripten ``MAIN_MODULE`` and every plug-in as a ``SIDE_MODULE`` (``build/cmake/modules/shared/vendor.wasm.cmake``), loaded at runtime via ``dlopen ()`` (``WasmExecutableLoader::loadImage``, ``ccl/platform/wasm/system/system.wasm.cpp``), the same mechanism used on other platforms. Plug-in ``.so`` binaries are additionally preloaded into the Wasm file system under ``/Plugins`` at build time.

Before plug-ins are actually scanned and loaded, ``CCL.preloadPlugins`` (``ccl/platform/wasm/js/cclsystem.js``) performs an async ``dlopen ()`` pass over everything under ``/Plugins`` first. This preload is required, not just an optimization: some browsers refuse synchronous instantiation of modules above ~8MB on the main thread, and only a genuine ``dlopen ()``/``dlsync`` round trip updates Emscripten's per-module bookkeeping correctly - which is what lets pthread worker threads later "catch up" on already-loaded modules without rerunning their global constructors on top of already-live shared state. Preloads have to happen strictly sequentially, since Emscripten's dynamic linker takes a process-wide lock during ``dlopen ()``.

================
Credential store
================

``WasmCredentialStore`` (``ccl/platform/wasm/security/credentialmanager.wasm.cpp``) only implements the asynchronous half of ``CCL::Security::CredentialStore``: ``addAsync``/``getAsync`` work (backed by Web Crypto AES-GCM encryption with a non-extractable key, with the encrypted blob stored in local settings), but the synchronous ``add ()``/``get ()`` always fail. Browsers only expose credential/key encryption as Promises, which cannot back a synchronous call. Code that needs to run on Wasm must go through the async credential store API; it cannot fall back to the synchronous one.

Note this is unrelated to any cross-device "sync" of stored credentials - there is none. Credentials are stored per-origin in the browser's local storage the same way other persistent application data is (see "IndexedDB file system" above).
