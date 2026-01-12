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
// Filename    : ccl/gui/graphics/3d/shader/shaderreflection3d.cpp
// Description : 3D graphics shader reflection
//
//************************************************************************************************

#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"

#include "ccl/gui/graphics/graphicshelper.h"

using namespace CCL;

//************************************************************************************************
// ShaderTypeInfo3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShaderTypeInfo3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShaderTypeInfo3D::ShaderTypeInfo3D ()
: structSize (0),
  bindingIndex (0)
{
	variables.objectCleanup (true);
	variableNames.add ({}); // add null terminator
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderTypeInfo3D::addVariable (ShaderVariable3D* v)
{
	variables.add (v);

	PropertyDefinition def {};
	def.type = v->isArray () ? kContainer : v->isStruct () ? kObject : kVariant;
	def.name = v->getName ().str ();
	def.typeName = v->getTypeName ().str ();
	def.typeInfo = v->getStructType ();

	ASSERT (!variableNames.isEmpty ())
	variableNames.insertAt (variableNames.count ()-1, def);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ShaderTypeInfo3D::getClassName () const
{
	return structName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ShaderTypeInfo3D::compare (const Object& obj) const
{
	if(const ShaderTypeInfo3D* other = ccl_cast<ShaderTypeInfo3D> (&obj))
		return bindingIndex == other->bindingIndex ? 0 : bindingIndex > other->bindingIndex ? 1 : -1;
	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo::PropertyDefinition* CCL_API ShaderTypeInfo3D::getPropertyNames () const
{
	return variableNames;
}

//************************************************************************************************
// ShaderVariable3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShaderVariable3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShaderVariable3D::ShaderVariable3D ()
: type (kUnknown),
  offset (0),
  size (0),
  arrayElementCount (0),
  arrayElementStride (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ShaderVariable3D::getTypeName () const
{
	if(type == kStruct)
	{
		ASSERT (structType)
		if(structType)
			return structType->getStructName ();
	}
	else switch(type)
	{
	case kFloat : return CSTR ("float");
	case kFloat4 : return CSTR ("float4");
	case kFloat4x4 : return CSTR ("float4x4");
	case kInt : return CSTR ("int");
	}
	return CString::kEmpty;
}

//************************************************************************************************
// ShaderValue3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShaderValue3D, Object)
ShaderValue3D ShaderValue3D::kInvalidValue;

//////////////////////////////////////////////////////////////////////////////////////////////////

ShaderValue3D::ShaderValue3D ()
: variable (nullptr),
  writer (nullptr),
  arrayElementOffset (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderValue3D::setValue (VariantRef value)
{
	auto uiValue = unknown_cast<UIValue> (value.asUnknown ());
	if(uiValue) switch(uiValue->getType ())
	{
	case IUIValue::kTransform3D : return setValue (uiValue->asTransform3DRef ());
	case IUIValue::kPointF4D : return setValue (uiValue->asPointF4DRef ());
	case IUIValue::kColorF : return setValue (uiValue->asColorFRef ());
	}
	if(value.isFloat ())
		return setValue (value.asFloat ());
	if(value.isInt ())
		return setValue (value.asInt ());
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderValue3D::setValue (Transform3DRef transform)
{
	return setValue (&transform, ShaderVariable3D::kFloat4x4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderValue3D::setValue (PointF4DRef point)
{
	return setValue (&point, ShaderVariable3D::kFloat4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderValue3D::setValue (ColorFRef color)
{
	return setValue (&color, ShaderVariable3D::kFloat4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderValue3D& CCL_API ShaderValue3D::getMember (StringID name)
{
	if(!members)
	{
		members = NEW ObjectArray;
		members->objectCleanup (true);

		ASSERT (variable && variable->isStruct ())
		if(variable && variable->isStruct () && variable->getStructType ())
		{
			for(auto memberVariable : iterate_as<ShaderVariable3D> (variable->getStructType ()->getVariables ()))
			{
				auto value = NEW ShaderValue3D;
				value->setVariable (memberVariable);
				value->setArrayElementOffset (arrayElementOffset);
				value->setWriter (writer);
				members->add (value);
			}
		}	
	}

	if(auto value = members->findIf<ShaderValue3D> ([name] (const ShaderValue3D& value) { return value.getName () == name; }))
		return *value;
	else
		return kInvalidValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderValue3D& CCL_API ShaderValue3D::getElementAt (int index)
{
	if(!elements)
	{
		elements = NEW ObjectArray;
		elements->objectCleanup (true);

		ASSERT (variable && variable->isArray ())
		if(variable && variable->isArray ())
		{
			for(uint32 i = 0; i < variable->getArrayElementCount (); i++)
			{
				auto value = NEW ShaderValue3D;
				value->setVariable (variable);
				value->setArrayElementOffset (arrayElementOffset + variable->getArrayElementStride () * i);
				value->setWriter (writer);
				elements->add (value);
			}
		}	
	}

	if(auto value = static_cast<ShaderValue3D*> (elements->at (index)))
		return *value;
	else
		return kInvalidValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderValue3D::removeAll ()
{
	members.release ();
	elements.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ShaderValue3D::getName () const
{
	return variable ? variable->getName () : CString::kEmpty; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShaderValue3D::setValue (float value)
{
	return setValue (&value, ShaderVariable3D::kFloat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShaderValue3D::setValue (int value)
{
	return setValue (&value, ShaderVariable3D::kInt);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShaderValue3D::setValue (const void* srcData, ShaderVariableType3D srcType)
{
	ASSERT (variable && writer)
	if(!variable || !writer)
		return kResultUnexpected;

	return writer->writeValue (*variable, arrayElementOffset, srcData, srcType);
}

//************************************************************************************************
// ShaderBufferWriter3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShaderBufferWriter3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShaderBufferWriter3D::ShaderBufferWriter3D ()
: startAddress (nullptr)
{
	bufferStruct.setType (ShaderVariable3D::kStruct);
	bufferValue.setVariable (&bufferStruct);
	bufferValue.setWriter (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShaderBufferWriter3D::writeValue (const ShaderVariable3D& variable, uint32 arrayElementOffset,
										  const void* srcData, ShaderVariableType3D srcType)
{
	ASSERT (startAddress)
	if(!startAddress)
		return kResultUnexpected;

	ASSERT (srcData)
	if(!srcData)
		return kResultInvalidPointer;

	if(variable.getType () != srcType || srcType == ShaderVariable3D::kUnknown)
		return kResultInvalidArgument;

	auto dst = startAddress + variable.getOffset () + arrayElementOffset;
	::memcpy (dst, srcData, variable.getSize ());
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderBufferWriter3D::setBufferTypeInfo (ITypeInfo* typeInfo)
{
	auto structType = unknown_cast<ShaderTypeInfo3D> (typeInfo);
	bufferStruct.setStructType (structType);
	bufferStruct.setSize (structType ? structType->getStructSize () : 0);
	bufferValue.removeAll ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShaderBufferWriter3D::setBuffer (IBufferSegment3D* newBuffer)
{
	if(buffer != newBuffer)
	{
		if(buffer)
		{
			if(startAddress)
			{
				buffer->getBuffer ()->unmap ();
				startAddress = nullptr;
			}
			buffer.release ();
		}
		buffer = newBuffer;
		if(buffer)
		{
			startAddress = reinterpret_cast<uint8*> (buffer->getBuffer ()->map ());
			ASSERT (startAddress)
			if(!startAddress)
				return kResultAccessDenied;
			
			startAddress += buffer->getOffset ();
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderValue3D& CCL_API ShaderBufferWriter3D::asValue ()
{
	return bufferValue;
}
