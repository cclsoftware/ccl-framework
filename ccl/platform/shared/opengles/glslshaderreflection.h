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
// Filename    : ccl/platform/shared/opengles/glslshaderreflection.h
// Description : GLSL Shader Reflection
//
//************************************************************************************************

#ifndef _glslshaderreflection_h
#define _glslshaderreflection_h

#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// GLSLShaderReflection
//************************************************************************************************

class GLSLShaderReflection
{
public:
	DECLARE_STRINGID_MEMBER (kAttrUnifiedBuffers)
	DECLARE_STRINGID_MEMBER (kAttrTypes)
	DECLARE_STRINGID_MEMBER (kAttrMembers)
	
	DECLARE_STRINGID_MEMBER (kAttrType)
	DECLARE_STRINGID_MEMBER (kAttrBlockSize)
	DECLARE_STRINGID_MEMBER (kAttrBinding)
	DECLARE_STRINGID_MEMBER (kAttrName)
	DECLARE_STRINGID_MEMBER (kAttrOffset)
	DECLARE_STRINGID_MEMBER (kAttrArray)
	DECLARE_STRINGID_MEMBER (kAttrArrayStride)
	
	static void getBufferTypeInfos (ObjectArray& bufferTypeInfos, UrlRef shaderPath);
};

} // namespace CCL

#endif // _glslshaderreflection_h
