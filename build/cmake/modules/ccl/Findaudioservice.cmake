find_package (corelib REQUIRED)

ccl_find_path (audio_DIR NAMES "source/plugversion.h" HINTS "${CCL_REPOSITORY_ROOT}/services/audio" DOC "Audio service directory")
mark_as_advanced (audio_DIR)

ccl_import_prebuilt_targets (audioservice PLUGIN)

include ("${audio_DIR}/cmake/audioservice-config.cmake")
