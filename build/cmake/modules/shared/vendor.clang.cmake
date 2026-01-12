
# add default compiler options
set (COMPILE_OPTIONS_CXX_C "")
list (APPEND COMPILE_OPTIONS_CXX_C
	# treat warnings as errors
	"-Werror=return-type"
	"-Werror=unicode-whitespace"
	"-Werror=uninitialized"

	# enable warnings
	"-Wshorten-64-to-32"
	"-Winconsistent-missing-override"

	# disable warnings
	"-Wno-multichar"
	"-Wno-parentheses"
	"-Wno-return-type-c-linkage"
	"-Wno-undefined-var-template"
	"-Wno-unused-function"
	"-Wno-reorder"
	"-Wno-switch"
	"-Wno-format-security"
	"-Wno-implicit-const-int-float-conversion"
)

foreach(flag_var ${COMPILE_OPTIONS_CXX_C})
	add_compile_options ($<$<COMPILE_LANGUAGE:CXX,C>:${flag_var}>)
endforeach()
