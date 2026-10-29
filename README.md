![Crystal Class Library](https://ccl.dev/assets/ccl-logo-readme-wide.png "Crystal Class Library")

## One foundation. Every major platform.

Crystal Class Library&reg; (CCL) is an application framework for building long-lived products and ecosystems across desktop, mobile, embedded, and browser platforms, with a declarative UI and modular architecture.

Over 25 years in continuous development. [Proven in production](https://ccl.dev/about).

### Highlights

- **Scales from embedded to desktop.** The framework is split into two layers: `core/` is a portable foundation that can run anywhere, and `ccl/` is the full application framework built on top of it.
- **Modular by design.** Plug-ins, services, and extensions distributed as signed [.install files](https://cclsoftware.github.io/ccl-documentation/extension/index.html).
- **Declarative UI.** Interfaces are described in the [Skin Definition Language](https://cclsoftware.github.io/ccl-documentation/skinlanguage/index.html) with theming, dark and light modes, Flexbox and anchor layout.
- **Hardware-accelerated graphics.** High-DPI and multi-touch 2D rendering (Direct2D, Skia) and 3D scenes (Direct3D, Metal, Vulkan, OpenGL ES).
- **Scripting and integrations.** JavaScript and TypeScript, Bluetooth LE, USB HID, and reusable audio runtime infrastructure.
- **Internationalization and security.** Unicode, localization, accessibility, encryption, and digital signatures.

See [Features](https://ccl.dev/features) for more.

## Supported Platforms

- Windows (x64, Arm64)
- macOS
- iOS/iPadOS
- Linux (Wayland; x64, Arm64)
- Android
- WebAssembly (browser)
- Embedded and RTOS targets (Zephyr, CMSIS-based)

## Repository Layout

```
build/           Build infrastructure (CMake, Gradle, ...)
ccl/             Application framework: GUI, networking, security, text, ...
classmodels/     Type libraries for tools (e.g. for the Skin Language Server)
core/            Portable foundation library
documentation/   Sphinx documentation sources
services/        Plug-in services (sqlite, bluetooth, jsengine, audio, ...)
skins/           Default look (neutralbase, neutraldesign)
submodules/      Third-party dependencies
tools/           Code generator, packager, documentation tools
translations/    Localization files
```

## Getting Started

This repository contains everything needed to build CCL. For development and target platforms, system requirements, and detailed build instructions, see [Getting started with CCL](https://cclsoftware.github.io/ccl-documentation/cclstartguide).

To explore CCL in action, check out the [CCL Demo](https://github.com/cclsoftware/ccl-demo) application.

## More Information

- [CCL Website](https://ccl.dev)
- [Documentation](https://cclsoftware.github.io/ccl-documentation/)
- [Contributing](CONTRIBUTING.md)
- [Third-party Dependencies](DEPENDENCIES.md)
- [Changelog](CHANGELOG.md)
- [License](LICENSE.md)

Crystal Class Library&reg; is a registered trademark of CCL Software Licensing GmbH.

Made with ♥ in Hamburg, Germany.
