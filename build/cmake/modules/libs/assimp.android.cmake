set (assimp_LIBRARY_OUTPUT "${assimp_DEPLOYMENT_DIR}/lib/libassimp.a")
set (assimp_BUILD_BYPRODUCTS "${assimp_LIBRARY_OUTPUT}")

list (APPEND assimp_platform_options
	-DCMAKE_C_COMPILE_FEATURES=c_std_99
	-DCMAKE_CXX_ABI_COMPILED=TRUE
	-DOPERATING_SYSTEM=Android
)

set (assimp_c_flags "-Wno-macro-redefined")
set (assimp_cxx_flags "--language=c++ -frtti -fexceptions -Wno-macro-redefined -Wno-register")
