set (webp_LIBRARY_OUTPUT "${webp_DEPLOYMENT_DIR}/${CMAKE_INSTALL_LIBDIR}/libwebp.a")
set (sharpyuv_LIBRARY_OUTPUT "${webp_DEPLOYMENT_DIR}/${CMAKE_INSTALL_LIBDIR}/libsharpyuv.a")

set (webp_BUILD_BYPRODUCTS "${webp_LIBRARY_OUTPUT}" "${sharpyuv_LIBRARY_OUTPUT}")

set (webp_c_flags "-DWEBP_EXTERN= -fvisibility=hidden")
