option (CCL_AUDIO_ENABLE_WEB_AUDIOWORKLETS
	"Use miniaudio's AudioWorklet backend on wasm instead of the deprecated ScriptProcessorNode \
fallback. Requires audioservice to be linked into the main module - AudioWorklet's JS/thread \
bootstrapping only runs when the enabling -s flags reach the main module's own link step, which \
does not happen while audioservice builds as a dynamically loaded (SIDE_MODULE) plugin." OFF)

if (CCL_AUDIO_ENABLE_WEB_AUDIOWORKLETS)
	target_compile_definitions (miniaudio INTERFACE MA_ENABLE_AUDIO_WORKLETS)
	target_link_options (miniaudio INTERFACE "-sAUDIO_WORKLET=1" "-sWASM_WORKERS=1" "-sASYNCIFY")
endif ()
