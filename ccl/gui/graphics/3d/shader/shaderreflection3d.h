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
// Filename    : ccl/gui/graphics/3d/shader/shaderreflection3d.h
// Description : 3D graphics shader reflection
//
//************************************************************************************************

#ifndef _ccl_shaderreflection3d_h
#define _ccl_shaderreflection3d_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/graphics/3d/igraphics3d.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class ShaderVariable3D;
class ShaderBufferWriter3D;

//************************************************************************************************
// ShaderTypeInfo3D
//************************************************************************************************

class ShaderTypeInfo3D: public Object,
						public AbstractTypeInfo
{
public:
	DECLARE_CLASS (ShaderTypeInfo3D, Object)

	ShaderTypeInfo3D ();

	PROPERTY_MUTABLE_CSTRING (structName, StructName)
	PROPERTY_VARIABLE (uint32, structSize, StructSize)
	PROPERTY_VARIABLE (int, bindingIndex, BindingIndex)

	void addVariable (ShaderVariable3D* v);
	const ObjectArray& getVariables () const { return variables; }

	// Object
	int compare (const Object& obj) const override;
	
	// ITypeInfo
	CStringPtr CCL_API getClassName () const override;
	const PropertyDefinition* CCL_API getPropertyNames () const override;

	CLASS_INTERFACE (ITypeInfo, Object)
	
protected:
	ObjectArray variables;
	Vector<PropertyDefinition> variableNames; // null-terminated
};

//************************************************************************************************
// ShaderVariable3D
//************************************************************************************************

class ShaderVariable3D: public Object
{
public:
	DECLARE_CLASS (ShaderVariable3D, Object)

	enum Type
	{
		kUnknown,
		kStruct,
		kFloat,
		kFloat4,
		kFloat4x4,
		kInt
	};

	ShaderVariable3D ();
		
	PROPERTY_VARIABLE (Type, type, Type)
	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_VARIABLE (uint32, offset, Offset)
	PROPERTY_VARIABLE (uint32, size, Size)
	PROPERTY_VARIABLE (uint32, arrayElementCount, ArrayElementCount)
	PROPERTY_VARIABLE (uint32, arrayElementStride, ArrayElementStride)
	PROPERTY_SHARED_AUTO (ShaderTypeInfo3D, structType, StructType)	

	StringID getTypeName () const;
	bool isStruct () const { return type == kStruct; }
	bool isArray () const { return arrayElementCount > 0; }
};

typedef ShaderVariable3D::Type ShaderVariableType3D;

//************************************************************************************************
// ShaderValue3D
//************************************************************************************************

class ShaderValue3D: public Object,
					 public IShaderValue3D
{
public:
	DECLARE_CLASS (ShaderValue3D, Object)

	ShaderValue3D ();

	PROPERTY_POINTER (ShaderVariable3D, variable, Variable)
	PROPERTY_VARIABLE (uint32, arrayElementOffset, ArrayElementOffset)
	PROPERTY_POINTER (ShaderBufferWriter3D, writer, Writer)

	StringID getName () const;
	tresult setValue (const void* srcData, ShaderVariableType3D srcType);
	tresult setValue (float value);
	tresult setValue (int value);

	void removeAll ();

	// IShaderValue3D
	tresult CCL_API setValue (VariantRef value) override;
	tresult CCL_API setValue (Transform3DRef transform) override;
	tresult CCL_API setValue (PointF4DRef point) override;
	tresult CCL_API setValue (ColorFRef color) override;
	IShaderValue3D& CCL_API getMember (StringID name) override;
	IShaderValue3D& CCL_API getElementAt (int index) override;

	CLASS_INTERFACE (IShaderValue3D, Object)

protected:
	static ShaderValue3D kInvalidValue;
	
	AutoPtr<ObjectArray> members;
	AutoPtr<ObjectArray> elements;
};

//************************************************************************************************
// ShaderBufferWriter3D
//************************************************************************************************

class ShaderBufferWriter3D: public Object,
							public IShaderBufferWriter3D
{
public:
	DECLARE_CLASS (ShaderBufferWriter3D, Object)

	ShaderBufferWriter3D ();

	tresult writeValue (const ShaderVariable3D& variable, uint32 arrayElementOffset,
						const void* srcData, ShaderVariableType3D srcType);

	// IShaderBufferWriter3D
	tresult CCL_API setBufferTypeInfo (ITypeInfo* typeInfo) override;
	tresult CCL_API setBuffer (IBufferSegment3D* buffer) override;
	IShaderValue3D& CCL_API asValue () override;

	CLASS_INTERFACE (IShaderBufferWriter3D, ShaderBufferWriter3D)

protected:
	ShaderVariable3D bufferStruct;
	ShaderValue3D bufferValue;
	SharedPtr<IBufferSegment3D> buffer;
	uint8* startAddress;
};

} // namespace CCL

#endif // _ccl_shaderreflection3d_h
