set (assimp_LIBRARY_OUTPUT_DEBUG "${assimp_DEPLOYMENT_DIR}/lib/assimp-vc${MSVC_TOOLSET_VERSION}-mtd.lib")
set (assimp_LIBRARY_OUTPUT_RELEASE "${assimp_DEPLOYMENT_DIR}/lib/assimp-vc${MSVC_TOOLSET_VERSION}-mt.lib")

set (assimp_BUILD_BYPRODUCTS "${assimp_LIBRARY_OUTPUT_DEBUG}" "${assimp_LIBRARY_OUTPUT_RELEASE}")