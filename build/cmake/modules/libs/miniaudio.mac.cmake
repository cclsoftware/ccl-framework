# By default miniaudio links CoreAudio/AudioToolbox at runtime via dlopen (), which can cause failures
# during Apple's notarization/App Store review. Link the frameworks directly instead.
# If AudioToolbox is unavailable on older iOS SDKs, replace it with "-framework AudioUnit".
target_compile_definitions (miniaudio INTERFACE MA_NO_RUNTIME_LINKING)
target_link_libraries (miniaudio INTERFACE "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
