set (cmark_LIBRARY_OUTPUT "${cmark_DEPLOYMENT_DIR}/lib/libcmark.a")
set (cmark_BUILD_BYPRODUCTS "${cmark_LIBRARY_OUTPUT}")

list (APPEND cmark_platform_options
	-DCMAKE_C_COMPILE_FEATURES=c_std_99
	-DCMAKE_CXX_ABI_COMPILED=TRUE
	-DOPERATING_SYSTEM=Android
)

set (cmark_c_flags "-Wno-macro-redefined")
set (cmark_cxx_flags "--language=c++ -frtti -fexceptions -Wno-macro-redefined -Wno-register")
