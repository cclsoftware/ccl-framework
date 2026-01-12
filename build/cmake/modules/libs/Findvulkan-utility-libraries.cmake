include_guard (DIRECTORY)

ccl_find_path (vulkan_utility_libraries_INCLUDE_DIR NAMES "vulkan/vk_enum_string_helper.h" HINTS "${CCL_SUBMODULES_DIR}/vulkan-utility-libraries" PATH_SUFFIXES include DOC "Vulkan Utility Libraries include directory.")
mark_as_advanced (vulkan_utility_libraries_INCLUDE_DIR)

if (NOT TARGET vulkan_utility_libraries)
	add_library (vulkan_utility_libraries INTERFACE)

	target_include_directories (vulkan_utility_libraries INTERFACE ${vulkan_utility_libraries_INCLUDE_DIR})
endif ()

set (Vulkan_Utility_Libraries_LIBRARY vulkan_utility_libraries)
