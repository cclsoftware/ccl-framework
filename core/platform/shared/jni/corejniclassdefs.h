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
// Filename    : core/platform/shared/jni/corejniclassdefs.h
// Description : JNI class definitions
//
//************************************************************************************************

#ifndef _corejniclassdefs_h
#define _corejniclassdefs_h

#include "core/platform/shared/jni/corejniclass.h"

namespace Core {
namespace Java {

//************************************************************************************************
// java.util.Iterator
//************************************************************************************************

DECLARE_JNI_CLASS (Iterator, "java/util/Iterator")
	DECLARE_JNI_METHOD (bool, hasNext)
	DECLARE_JNI_METHOD (jobject, next)
END_DECLARE_JNI_CLASS (Iterator)

//************************************************************************************************
// java.util.List
//************************************************************************************************

DECLARE_JNI_CLASS (List, "java/util/List")
	DECLARE_JNI_METHOD (int, size)
	DECLARE_JNI_METHOD (jobject, get, int)
END_DECLARE_JNI_CLASS (List)

//************************************************************************************************
// java.util.Map
//************************************************************************************************

DECLARE_JNI_CLASS (Map, "java/util/Map")
	DECLARE_JNI_METHOD (jobject, entrySet)
END_DECLARE_JNI_CLASS (Map)

//************************************************************************************************
// java.util.Map.Entry
//************************************************************************************************

DECLARE_JNI_CLASS (MapEntry, "java/util/Map$Entry")
	DECLARE_JNI_METHOD (jobject, getKey)
	DECLARE_JNI_METHOD (jobject, getValue)
END_DECLARE_JNI_CLASS (MapEntry)

//************************************************************************************************
// java.util.Set
//************************************************************************************************

DECLARE_JNI_CLASS (Set, "java/util/Set")
	DECLARE_JNI_METHOD (jobject, iterator)
END_DECLARE_JNI_CLASS (Set)

//************************************************************************************************
// java.util.UUID
//************************************************************************************************

DECLARE_JNI_CLASS (UUID, "java/util/UUID")
	DECLARE_JNI_STATIC_METHOD (jobject, randomUUID)
	DECLARE_JNI_METHOD (int64, getLeastSignificantBits)
	DECLARE_JNI_METHOD (int64, getMostSignificantBits)
	DECLARE_JNI_METHOD (jstring, toString)
END_DECLARE_JNI_CLASS (UUID)

//************************************************************************************************
// java.io.File
//************************************************************************************************

DECLARE_JNI_CLASS (File, "java/io/File")
	DECLARE_JNI_METHOD (jstring, getAbsolutePath)
END_DECLARE_JNI_CLASS (File)

//************************************************************************************************
// java.lang.Runtime
//************************************************************************************************

DECLARE_JNI_CLASS (Runtime, "java/lang/Runtime")
	DECLARE_JNI_STATIC_METHOD (jobject, getRuntime)
	DECLARE_JNI_METHOD (int64, maxMemory)
	DECLARE_JNI_METHOD (int64, totalMemory)
	DECLARE_JNI_METHOD (int64, freeMemory)
END_DECLARE_JNI_CLASS (Runtime)

//************************************************************************************************
// java.lang.System
//************************************************************************************************

DECLARE_JNI_CLASS (System, "java/lang/System")
	DECLARE_JNI_STATIC_METHOD (void, loadLibrary, jstring)
END_DECLARE_JNI_CLASS (System)

//************************************************************************************************
// java.net.InetAddress
//************************************************************************************************

DECLARE_JNI_CLASS (InetAddress, "java/net/InetAddress")
	DECLARE_JNI_STATIC_METHOD (jobject, getByAddress, jbyteArray)
	DECLARE_JNI_METHOD (jstring, getHostName)
END_DECLARE_JNI_CLASS (InetAddress)

} // namespace Java
} // namespace Core

#endif // _corejniclassdefs_h
