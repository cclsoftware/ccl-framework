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
// Filename    : ccl/public/gui/framework/iskinmodel.h
// Description : Skin Model Interfaces
//
//************************************************************************************************

#ifndef _ccl_iskinmodel_h
#define _ccl_iskinmodel_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

interface IContainer;
interface IUnknownList;
interface ITypeInfo;
interface IAttributeList;
interface IImage;
interface IVisualStyle;

//************************************************************************************************
// SkinAttributeType
//************************************************************************************************

namespace SkinAttributeTypes
{
	DEFINE_ENUM (Type)
	{
		kStyle,
		kImage,
		kForm,
		kFont,
		kColor,
		kEnum,
		kBool,
		kInteger,
		kFloat,
		kUnspecified // string
	};
};

typedef SkinAttributeTypes::Type SkinAttributeType;

//************************************************************************************************
// CanonicalSkinAttributes
/** Canonical attributes supported by both, JSON and XML-based skins. */
//************************************************************************************************

namespace CanonicalSkinAttributes
{
	DEFINE_STRINGID (kTitle, "title")
	DEFINE_STRINGID (kImage, "image")
	DEFINE_STRINGID (kUrl, "url")
}

//************************************************************************************************
// ISkinModel
/** Skin model interface. 
	\ingroup gui_skin */
//************************************************************************************************

interface ISkinModel: IUnknown
{
	DEFINE_ENUM (ElementType)
	{
		kFontsElement = 'Fnts',
		kStylesElement = 'Stls',
		kImagesElement = 'Imgs',
		kResourcesElement = kImagesElement,
		kFormsElement = 'Frms'
	};

	/** Get container for given element type. */
	virtual IContainer* CCL_API getContainerForType (ElementType which) = 0;

	/** Get paths of imported skin packages. */
	virtual void CCL_API getImportedPaths (IUnknownList& paths) const = 0;

	/** Get submodel (scope) by name. */
	virtual ISkinModel* CCL_API getSubModel (StringID name) = 0;

	DECLARE_IID (ISkinModel)
};

DEFINE_IID (ISkinModel, 0xb33029b7, 0xf1a1, 0x4639, 0x9e, 0x8, 0xe0, 0x9a, 0x84, 0x13, 0x4f, 0xd1)

//************************************************************************************************
// ISkinElement
/** Basic skin element interface, use IContainer to access child elements. 
	\ingroup gui_skin */
//************************************************************************************************

interface ISkinElement: IUnknown
{
	/** Get element name. */
	virtual StringID CCL_API getName () const = 0;

	/** Set element name. */
	virtual void CCL_API setName (StringID name) = 0;

	/** Get optional comment for developers and tool support.
		Note that this is an explicit attribute, not a comment ignored by the XML/JSON parser. */
	virtual void CCL_API getComment (String& comment) const = 0;

	/** Set element comment. */
	virtual void CCL_API setComment (StringRef comment) = 0;

	/** Get info about source code. */
	virtual tbool CCL_API getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl = nullptr) const = 0;

	/** Set source file name. */
	virtual void CCL_API setSourceFile (StringRef fileName) = 0;

	/** Get all element attributes. */
	virtual void CCL_API getAttributes (IAttributeList& attributes) const = 0;

	/** Set all element attributes. */
	virtual void CCL_API setAttributes (const IAttributeList& attributes) = 0;

	/** Get element attribute value. */
	virtual tbool CCL_API getAttributeValue (Variant& value, StringID name) const = 0;

	/** Set element attribute value. */
	virtual void CCL_API setAttributeValue (StringID name, VariantRef value, int index = -1) = 0;

	/** Remove element attribute. */
	virtual tbool CCL_API removeAttribute (StringID name, int* oldIndex = nullptr) = 0;

	/**	Get element class.
		Note that this is different from the type information returned via IObject
		as it corresponds to the public XML or JSON skin type. */
	virtual const ITypeInfo* CCL_API getElementClass () const = 0;

	/** Clone element. */
	virtual void CCL_API clone (ISkinElement*& element) const = 0;

	DECLARE_IID (ISkinElement)
};

DEFINE_IID (ISkinElement, 0xee7ba430, 0xfafe, 0x4bb6, 0x98, 0x77, 0x8b, 0xd8, 0x87, 0x10, 0x70, 0x22)

//************************************************************************************************
// ISkinElementChildren
/** Additonal interface for element containers and elements that support children. */
//************************************************************************************************

interface ISkinElementChildren: IUnknown
{
	/** Add child element. */
	virtual tbool CCL_API addChildElement (ISkinElement* childElement, int index = -1) = 0;

	/** Remove child element. */
	virtual tbool CCL_API removeChildElement (ISkinElement* childElement, int* oldIndex = nullptr) = 0;

	DECLARE_IID (ISkinElementChildren)
};

DEFINE_IID (ISkinElementChildren, 0x8b22c55a, 0x4b3c, 0x4462, 0xab, 0x8c, 0xf1, 0x10, 0x2e, 0x5, 0xcf, 0x32)

//************************************************************************************************
// ISkinImageElement
/** Represents an image defined in skin. */
//************************************************************************************************

interface ISkinImageElement: IUnknown
{
	/** Get image represented by element. */
	virtual IImage* CCL_API getImage () const = 0;

	/** Set image represented by element. */
	virtual void CCL_API setImage (IImage* image) = 0;

	/** Get relative path to image file. */
	virtual StringRef CCL_API getImagePath () const = 0;

	/** Set relative path to image file. */
	virtual void CCL_API setImagePath (StringRef imagePath) = 0;

	DECLARE_IID (ISkinImageElement)
};

DEFINE_IID (ISkinImageElement, 0x292aca73, 0x7150, 0x46ac, 0x83, 0xdd, 0x5b, 0xd5, 0x48, 0x8a, 0xb5, 0xb4)

//************************************************************************************************
// ISkinViewElement
/** View element interface. 
	\ingroup gui_skin */
//************************************************************************************************

interface ISkinViewElement: IUnknown
{
	/** Get element size. */
	virtual RectRef CCL_API getSize () const = 0;

	/** Set element size. */
	virtual void CCL_API setSize (RectRef size) = 0;

	/** Get attribute defined via "data.[id]" in skin XML. */
	virtual tbool CCL_API getDataDefinition (String& string, StringID id) const = 0;

	/** Get standard options defined in skin XML. */
	virtual StyleFlags CCL_API getStandardOptions () const = 0;
	
	DECLARE_IID (ISkinViewElement)
};

DEFINE_IID (ISkinViewElement, 0x65378ec, 0xb202, 0x4e30, 0x89, 0xfa, 0xac, 0xbd, 0x21, 0x50, 0x33, 0xfb)

//************************************************************************************************
// ISkinCreateArgs
/** Skin view creation arguments, passed as data to IViewFactory::createView(). 
	\ingroup gui_skin */
//************************************************************************************************

interface ISkinCreateArgs: IUnknown
{
	/** Get calling element. */
	virtual ISkinViewElement* CCL_API getElement () const = 0;

	/** Get variable from current call stack. */
	virtual tbool CCL_API getVariable (Variant& value, StringID name) const = 0;
	
	/** Get visual style that will be assigned to calling element later. */
	virtual IVisualStyle* CCL_API getVisualStyleForElement () const = 0;

	DECLARE_IID (ISkinCreateArgs)
};

DEFINE_IID (ISkinCreateArgs, 0x26e6c29d, 0x1ab2, 0x4178, 0x98, 0xd2, 0x18, 0xec, 0x91, 0xa6, 0x8e, 0x79)

//************************************************************************************************
// SkinModelAccessor
/** Helper to access skin model elements.
	\ingroup gui_skin */
//************************************************************************************************

class SkinModelAccessor
{
public:
	SkinModelAccessor (ISkinModel& model);

	ISkinElement* findForm (StringID formName) const;
	ISkinElement* findResource (StringID name, StringID typeName = nullptr) const;

protected:
	ISkinModel& model;

	static ISkinElement* find (IContainer* c, StringID name, StringID typeName = nullptr);
};

} // namespace CCL

#endif // _ccl_iskinmodel_h
