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
// Filename    : core/platform/shared/jni/corejniclassdefs.cpp
// Description : JNI class definitions
//
//************************************************************************************************

#include "core/platform/shared/jni/corejniclassdefs.h"

namespace Core {
namespace Java {

//************************************************************************************************
// java.util.Iterator
//************************************************************************************************

DEFINE_JNI_CLASS (Iterator)
	DEFINE_JNI_METHOD (hasNext, "()Z")
	DEFINE_JNI_METHOD (next, "()Ljava/lang/Object;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.util.List
//************************************************************************************************

DEFINE_JNI_CLASS (List)
	DEFINE_JNI_METHOD (size, "()I")
	DEFINE_JNI_METHOD (get, "(I)Ljava/lang/Object;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.util.Map
//************************************************************************************************

DEFINE_JNI_CLASS (Map)
	DEFINE_JNI_METHOD (entrySet, "()Ljava/util/Set;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.util.Map.Entry
//************************************************************************************************

DEFINE_JNI_CLASS (MapEntry)
	DEFINE_JNI_METHOD (getKey, "()Ljava/lang/Object;")
	DEFINE_JNI_METHOD (getValue, "()Ljava/lang/Object;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.util.Set
//************************************************************************************************

DEFINE_JNI_CLASS (Set)
	DEFINE_JNI_METHOD (iterator, "()Ljava/util/Iterator;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.util.UUID
//************************************************************************************************

DEFINE_JNI_CLASS (UUID)
	DEFINE_JNI_STATIC_METHOD (randomUUID, "()Ljava/util/UUID;")
	DEFINE_JNI_METHOD (getLeastSignificantBits, "()J")
	DEFINE_JNI_METHOD (getMostSignificantBits, "()J")
	DEFINE_JNI_METHOD (toString, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.io.File
//************************************************************************************************

DEFINE_JNI_CLASS (File)
	DEFINE_JNI_METHOD (getAbsolutePath, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.lang.Runtime
//************************************************************************************************

DEFINE_JNI_CLASS (Runtime)
	DEFINE_JNI_STATIC_METHOD (getRuntime, "()Ljava/lang/Runtime;")
	DEFINE_JNI_METHOD (maxMemory, "()J")
	DEFINE_JNI_METHOD (totalMemory, "()J")
	DEFINE_JNI_METHOD (freeMemory, "()J")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.lang.System
//************************************************************************************************

DEFINE_JNI_CLASS (System)
	DEFINE_JNI_STATIC_METHOD (loadLibrary, "(Ljava/lang/String;)V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// java.net.InetAddress
//************************************************************************************************

DEFINE_JNI_CLASS (InetAddress)
	DEFINE_JNI_STATIC_METHOD (getByAddress, "([B)Ljava/net/InetAddress;")
	DEFINE_JNI_METHOD (getHostName, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

} // namespace Java
} // namespace Core
