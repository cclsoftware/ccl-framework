//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/public/corebasicmacros.h
// Description : Basic Macros
//
//************************************************************************************************

#ifndef _corebasicmacros_h
#define _corebasicmacros_h

//************************************************************************************************
// Basic Macros
//************************************************************************************************

/** Lazy concatenation (arguments are evaluated before concatenation) */
#define LAZYCAT(a, b) LAZYCAT_HELP(a, b)
#define LAZYCAT_HELP(a, b) a ## b

/** Stringify numeric definiton. */
#define STRINGIFY(s) STRINGIFY_HELPER(s)
#define STRINGIFY_HELPER(s) #s

/** Generate unique identifier at file scope. */
#define UNIQUE_IDENT(prefix) LAZYCAT (prefix, __LINE__)

/** Count number of items in array. */
#define ARRAY_COUNT(a) int(sizeof(a)/sizeof(a[0]))

/** Define initialization function. */
#define DEFINE_INITIALIZER(Name) \
void __init##Name (); \
struct __##Name { __##Name() { __init##Name (); } }; \
__##Name the##Name##Initializer; \
void __init##Name ()

/** Define terminate function. */
#define DEFINE_TERMINATOR(Name) \
void __terminate##Name (); \
struct __##Name { ~__##Name() { __terminate##Name (); } }; \
__##Name the##Name##Terminator; \
void __terminate##Name ()

/**	Import binary resource linked with the program.
	Symbol names match with framework tools and CMake macros. */
#define IMPORT_BINARY_RESOURCE(SymbolName) \
	extern unsigned int SymbolName##_Size; \
	extern void* SymbolName##_Ptr;

#endif // _corebasicmacros_h
