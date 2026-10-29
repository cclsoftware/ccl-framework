.. _wasm-platform:

############################
WebAssembly Platform Support
############################

Using the WebAssembly backend (based on Emscripten), CCL applications can run inside a browser sandbox rather than on top of a native OS. Most platform interfaces are implemented against browser APIs (DOM, Fetch, IndexedDB, Web Crypto, ...) instead of native system calls, which brings a number of restrictions that don't exist on the other platforms. This page summarizes the ones that are easy to trip over.

===================
Cross-origin issues
===================

Multithreading requires the page to be served with cross-origin isolation headers (``Cross-Origin-Opener-Policy: same-origin`` and ``Cross-Origin-Embedder-Policy: require-corp``), since threads rely on ``SharedArrayBuffer``, which browsers only expose to a cross-origin isolated page. This is a hosting/serving concern your deployment needs to take care of. Apps that don't need multithreading can instead disable pthreads support at build time to avoid the requirement - see "CMake options" below.

WebView is backed by a plain ``<iframe>``. Once the page is cross-origin isolated, it can only navigate to same-origin/local content - cross-origin URLs are bounced out to a new browser tab instead. Reading back the current URL/title/history of a cross-origin page silently returns empty/false due to the browser's same-origin policy.

HTTP requests go through the browser's ``fetch ()`` API and are subject to normal CORS rules - there is no way to bypass CORS from C++, and the user agent cannot be overridden.

============================================
IndexedDB file system (vs. host file system)
============================================

:cref:`CCL::INativeFileSystem` addresses a real host file system on other platforms, but on Wasm it addresses a browser-side IndexedDB-backed virtual file system instead. Data is scoped to the browser's storage for the page's origin - clearing site data/IndexedDB deletes it, and it isn't visible to the real OS file system, or to applications running on other origins. It is however shared with any other CCL application served from the *same* origin. File system change notifications are not supported.

============================
File selectors (upload only)
============================

:cref:`CCL::IFileSelector` uses the browser's native file upload picker, which is inherently asynchronous - only the async selector API works on Wasm. There is no native "Save As" dialog: saving instead triggers a browser file *download*, with the destination chosen through the browser's own download UI, not through CCL.

==================================
Folder selectors (not implemented)
==================================

:cref:`CCL::IFolderSelector` has no working implementation on Wasm - browsers have no picker for an arbitrary host folder a page could write into, so any call always fails.

===================
Socket restrictions
===================

General POSIX sockets are not available on Wasm at all. Only HTTP requests (via ``fetch ()``) and WebSockets are available as network primitives; there is also no HTTP server support.

=================
Thread priorities
=================

Thread priority changes are not guaranteed to have any effect - Emscripten's pthread shim generally ignores OS-level scheduling changes in the browser.

==============================
Font handling (fallback fonts)
==============================

There is no native font enumeration API in a browser sandbox. Fallback glyphs (CJK, emoji, symbols, ...) are resolved from a small set of bundled fallback fonts, fetched on demand the first time a character needs one.

=============
CMake options
=============

``VENDOR_WASM_THREAD_POOL_SIZE`` controls how many worker threads Emscripten pre-spawns at startup (``-sPTHREAD_POOL_SIZE``); it defaults to one per detected logical CPU. Lowering it can reduce startup overhead for apps with a known, small thread count.

``VENDOR_WASM_USE_PTHREADS`` (default ``ON``) can be turned ``OFF`` to disable pthreads support entirely, avoiding the cross-origin isolation requirement for single-threaded apps; ``VENDOR_WASM_THREAD_POOL_SIZE`` then has no effect.

================
Credential store
================

:cref:`CCL::Security::ICredentialManager`'s synchronous methods ``addCredential ()``, ``addPassword ()``, ``getCredential ()`` always fail on Wasm - only the asynchronous API is implemented, since browsers only expose the underlying credential encryption as Promises. This is unrelated to any cross-device credential sync, which doesn't exist here; credentials are stored per-origin in the browser's local storage, like other persistent application data.
