//************************************************************************************************
//
// CCL Spy
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
// Filename    : viewproperty.h
// Description :
//
//************************************************************************************************

#ifndef _viewproperty_h
#define _viewproperty_h

#include "objectinfo.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/iusercontrol.h"
#include "ccl/public/text/cstring.h"

namespace CCL {
interface IView; }

using namespace CCL;

namespace Spy {

//************************************************************************************************
// ViewProperty
//************************************************************************************************

class ViewProperty: public PropertyHandler
{
public:
	ViewProperty (CCL::StringID name = nullptr);

	PROPERTY_MUTABLE_CSTRING (name, Name)

	virtual bool getValue (CCL::Variant& var, CCL::IView* view) { return false; }
	virtual CCL::Coord getWidth () const { return name.length () * 8; }
};

//************************************************************************************************
// View Attribute Properties (e.g. IView::kTitle)
//************************************************************************************************

template<int attrID>
struct ViewAttributeProperty: public ViewProperty
{
	bool getValue (Variant& var, IView* view) override
	{
		return view->getViewAttribute (var, attrID) != 0;
	}
};

//************************************************************************************************
// IObject Properties of views
//************************************************************************************************

struct ObjectProperty: public ViewProperty
{
	ObjectProperty (StringID name, StringID propertyId = nullptr)
	: ViewProperty (name),
	  propertyId (propertyId)
	{
		if(propertyId.isEmpty ())
			this->propertyId = name;
	}

	bool getValue (Variant& var, IView* view) override
	{
		UnknownPtr<IObject> object (view);
		return object && object->getProperty (var, propertyId) != 0;
	}

protected:
	MutableCString propertyId;
};

//************************************************************************************************
// IObject Properties of a UserControl
//************************************************************************************************

struct UserControlObjectProperty: public ObjectProperty
{
	UserControlObjectProperty (StringID name, StringID propertyId = nullptr)
	: ObjectProperty (name, propertyId)
	{}

	bool getValue (Variant& var, IView* view) override
	{
		UnknownPtr<IUserControlHost> host (view);
		IUserControl* control = host ? host->getUserControl () : nullptr;
		UnknownPtr<IObject> object (control);
		return object && object->getProperty (var, propertyId) != 0;
	}
};

//************************************************************************************************
// SizeModeProperty
//************************************************************************************************

struct SizeModeProperty: public ViewAttributeProperty<IView::kSizeMode>
{
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// SizeProperty
//************************************************************************************************

struct SizeProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;

protected:
	bool assignSize (Variant& value, RectRef size);
};

//************************************************************************************************
// FlexProperty
//************************************************************************************************

struct FlexProperty: public ViewProperty
{
	FlexProperty (StringID attributeID);

protected:
	CString attributeID;
};

//************************************************************************************************
// FlexItemProperty
//************************************************************************************************

struct FlexItemProperty: public FlexProperty
{
	using FlexProperty::FlexProperty;
	
	// ViewProperty
	bool getValue (Variant& value, IView* view) override;
};

//************************************************************************************************
// FlexContainerProperty
//************************************************************************************************

struct FlexContainerProperty: public FlexProperty
{
	using FlexProperty::FlexProperty;
	
	// ViewProperty
	bool getValue (Variant& value, IView* view) override;
};

//************************************************************************************************
// SizeLimitsProperty
//************************************************************************************************

struct SizeLimitsProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// StyleFlagsProperty
//************************************************************************************************

struct StyleFlagsProperty: public ViewAttributeProperty<IView::kStyleFlags>
{
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// VisualStyleProperty
//************************************************************************************************

struct VisualStyleProperty: public ViewProperty
{
	// ViewProperty
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
	int getEditCapability (VariantRef value) override { return value.asUnknown () ? kObjectLink : kNoEdit; }
	bool edit (VariantRef value, EditContext& context) override { context.objectToInspect.share (value.asUnknown ()); return true; }
};

//************************************************************************************************
// ZoomFactorProperty
//************************************************************************************************

struct ZoomFactorProperty: public ViewProperty
{
	// ViewProperty
	bool getValue (Variant& value, IView* view) override;
};

//************************************************************************************************
// ControllerPathProperty
//************************************************************************************************

struct ControllerPathProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
};

//************************************************************************************************
// FormNameProperty
//************************************************************************************************

struct FormNameProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
};

//************************************************************************************************
// SourceCodeProperty
//************************************************************************************************

struct SourceCodeProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	bool draw  (VariantRef value, const DrawInfo& info) override;
	int getEditCapability (VariantRef value) override { return value.asUnknown () ? kCustomLink : kNoEdit; }
	bool edit (VariantRef value, EditContext& context) override;

private:
	struct SourceInfo: public Object
	{
		String description;
		String fileName;
		int32 line;
		Url packageUrl;
	};
};

//************************************************************************************************
// ParamNameProperty
//************************************************************************************************

struct ParamNameProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// ParamValueProperty
//************************************************************************************************

struct ParamValueProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// ParamCommandProperty
//************************************************************************************************

struct ParamCommandProperty: public ViewProperty
{
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
};

//************************************************************************************************
// SceneNode3DProperty
//************************************************************************************************

struct SceneNode3DProperty: public ViewProperty
{
	// ViewProperty
	bool getValue (Variant& value, IView* view) override;
	void toString (String& string, VariantRef value) override;
	int getEditCapability (VariantRef value) override { return value.asUnknown () ? kObjectLink : kNoEdit; }
	bool edit (VariantRef value, EditContext& context) override { context.objectToInspect.share (value.asUnknown ()); return true; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ViewProperty::ViewProperty (CCL::StringID name)
: name (name) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Spy

#endif // _viewproperty_h
