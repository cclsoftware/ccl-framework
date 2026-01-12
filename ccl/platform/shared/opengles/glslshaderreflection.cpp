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
// Filename    : ccl/platform/shared/opengles/glslshaderreflection.cpp
// Description : GLSL Shader Reflection
//
//************************************************************************************************

#include "ccl/platform/shared/opengles/glslshaderreflection.h"

#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/jsonarchive.h"

using namespace CCL;

//************************************************************************************************
// GLSLShaderReflection
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrUnifiedBuffers, "ubos")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrTypes, "types")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrMembers, "members")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrType, "type")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrBlockSize, "block_size")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrBinding, "binding")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrName, "name")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrOffset, "offset")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrArray, "array")
DEFINE_STRINGID_MEMBER_ (GLSLShaderReflection, kAttrArrayStride, "array_stride")

//////////////////////////////////////////////////////////////////////////////////////////////////

void GLSLShaderReflection::getBufferTypeInfos (ObjectArray& bufferTypeInfos, UrlRef path)
{
	// load reflection info from a json file
	Url reflectionPath (path);
	reflectionPath.setFileType (FileTypes::Json ());
	AutoPtr<IMemoryStream> stream = File::loadBinaryFile (reflectionPath);
	if(stream)
	{
		JsonArchive archive (*stream);
		Attributes a;
		if(archive.loadAttributes (nullptr, a))
		{
			IterForEach (a.newQueueIterator (kAttrUnifiedBuffers, ccl_typeid<Attributes> ()), Attributes, unifiedBufferInfo)
				CString bufferTypeName = unifiedBufferInfo->getCString (kAttrType);
				uint32 bufferSize = unifiedBufferInfo->getInt (kAttrBlockSize);
				if(bufferTypeName.isEmpty () || bufferSize <= 0)
					continue;
				
				int binding = unifiedBufferInfo->getInt (kAttrBinding);
			
				auto bufferTypeInfo = NEW ShaderTypeInfo3D;
				bufferTypeInfo->setBindingIndex (binding);
				bufferTypeInfos.addSorted (bufferTypeInfo);
				bufferTypeInfo->setStructSize (bufferSize);
				bufferTypeInfo->setStructName (bufferTypeName);
				
				Attributes* types = a.getAttributes (kAttrTypes);
						
				const auto addTypeInfoRecursive = [&] (CStringRef structTypeName) -> void
				{
					auto addTypeInfo = [&] (CStringRef structTypeName, ShaderTypeInfo3D* typeInfo, ShaderVariable3D* parent, auto& recurse) mutable -> void
					{
						Variant value;
						if(types && types->contains (structTypeName))
						{
							types->getAttribute (value, structTypeName);
							Attributes* typeInfoAttributes = unknown_cast<Attributes> (value);
							if(typeInfoAttributes)
							{
								IterForEach (typeInfoAttributes->newQueueIterator (kAttrMembers, ccl_typeid<Attributes> ()), Attributes, member)
									int arraySize = 0;
									if(member->unqueueAttribute (kAttrArray, value))
										arraySize = value.asInt ();
									int arrayStride = member->getInt (kAttrArrayStride);
								
									AutoPtr<ShaderVariable3D> v = NEW ShaderVariable3D;
									CString name = member->getCString (kAttrName);
									v->setName (name);

									v->setOffset (member->getInt (kAttrOffset) + (parent ? parent->getOffset () : 0));
									v->setSize (arrayStride);
									CString typeName = member->getCString (kAttrType);
									if(typeName == "float")
									{
										v->setSize (sizeof(float));
										v->setType (ShaderVariable3D::kFloat);
									}
									else if(typeName == "vec4")
									{
										v->setSize (sizeof(float) * 4);
										v->setType (ShaderVariable3D::kFloat4);
									}
									else if(typeName == "mat4")
									{
										v->setSize (sizeof(float) * 4 * 4);
										v->setType (ShaderVariable3D::kFloat4x4);
									}
									else if(typeName == "int")
									{
										v->setSize (sizeof(int32));
										v->setType (ShaderVariable3D::kInt);
									}
									else if(types->contains (typeName))
									{
										v->setType (ShaderVariableType3D::kStruct);
										AutoPtr<ShaderTypeInfo3D> structTypeInfo = NEW ShaderTypeInfo3D;
										structTypeInfo->setStructName (typeName);
										v->setStructType (structTypeInfo);
										recurse (typeName, structTypeInfo, v, recurse);
									}
									else
									{
										ASSERT (0)
										v->setSize (0);
										v->setType (ShaderVariable3D::kUnknown);
									}

									v->setArrayElementCount (arraySize);
									v->setArrayElementStride (arrayStride);

									if(typeInfo)
										typeInfo->addVariable (v.detach ());
								EndFor
							}
						}
					};
					addTypeInfo (structTypeName, bufferTypeInfo, nullptr, addTypeInfo);
				};
				
				addTypeInfoRecursive (bufferTypeName);
			EndFor
		}
	}
};

