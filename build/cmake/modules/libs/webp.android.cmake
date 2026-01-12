set (webp_LIBRARY_OUTPUT "${webp_DEPLOYMENT_DIR}/lib/libwebp.a")
set (sharpyuv_LIBRARY_OUTPUT "${webp_DEPLOYMENT_DIR}/lib/libsharpyuv.a")

set (webp_BUILD_BYPRODUCTS "${webp_LIBRARY_OUTPUT}" "${sharpyuv_LIBRARY_OUTPUT}")

list (APPEND webp_platform_options 
	-DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}
    -DANDROID_NDK=${CMAKE_ANDROID_NDK}
    -DCMAKE_C_COMPILE_FEATURES=c_std_99
)

set (webp_c_flags "-DWEBP_EXTERN= -fvisibility=hidden -Wno-macro-redefined")
