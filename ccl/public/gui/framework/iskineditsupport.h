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
// Filename    : ccl/public/gui/framework/iskineditsupport.h
// Description : Skin Editing Support Interfaces
//
//************************************************************************************************

#ifndef _ccl_iskineditsupport_h
#define _ccl_iskineditsupport_h

#include "ccl/public/gui/framework/iskinmodel.h"

#include "ccl/public/base/variant.h"

namespace CCL {

interface ITypeLibrary;
interface IGraphics;
interface IProgressNotify;
interface IFileTypeFilter;
interface IMemoryStream;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** ISkinLoader class for JSON (core) skins, can be created via ccl_new<>. */
	DEFINE_CID (CoreSkinLoader, 0x8683f346, 0x6f53, 0x4f8e, 0x99, 0xd5, 0x9, 0x94, 0x53, 0x7e, 0x48, 0xc7)

	// Pseudo classes used with IClassAllocator from skin edit support:
	DEFINE_CID (FormElement, 0x8244f71f, 0x74f3, 0x4e02, 0x82, 0x0, 0x2b, 0x20, 0x55, 0x8f, 0x33, 0xcf)
	DEFINE_CID (ImageElement, 0xbe10fcf, 0xea14, 0x40d5, 0xaf, 0x3b, 0x3e, 0x1f, 0xa8, 0xd9, 0x4f, 0xd1)
	DEFINE_CID (StyleElement, 0x89128fa8, 0x5f57, 0x44b4, 0xb5, 0x94, 0xeb, 0x47, 0x9d, 0x68, 0x1a, 0xca)
	DEFINE_CID (FontElement, 0xac5d34ba, 0xb621, 0x4687, 0x85, 0x1c, 0x36, 0x52, 0xb5, 0xd1, 0xc1, 0xee)
}

//************************************************************************************************
// ISkinLoader
/*
	TODO:
	- ITheme and IThemeManager basically do the same thing as ISkinLoader (except creating new skins)
*/
//************************************************************************************************

interface ISkinLoader: IUnknown
{
	virtual tbool CCL_API loadSkin (UrlRef path, IProgressNotify* progress = nullptr) = 0;

	virtual tbool CCL_API createSkin (UrlRef path) = 0;

	virtual ISkinModel* CCL_API getISkinModel () = 0;

	DECLARE_IID (ISkinLoader)
};

DEFINE_IID (ISkinLoader, 0x2ec73347, 0x4da3, 0x4e46, 0x82, 0x1d, 0x64, 0x46, 0x1c, 0xd1, 0x81, 0x46)

//************************************************************************************************
// SkinValueChange
//************************************************************************************************

struct SkinValueChange
{
	MutableCString name;
	Variant value;

	SkinValueChange (StringID _name = nullptr, VariantRef _value = Variant ())
	: name (_name),
	  value (_value)
	{
		value.share ();
	}
};

//************************************************************************************************
// ISkinEditSupport
//************************************************************************************************

interface ISkinEditSupport: IClassAllocator
{
	virtual const ITypeLibrary* CCL_API getTypeLibrary () const = 0;

	virtual const ITypeInfo* CCL_API getViewBaseClass () const = 0;
	
	virtual const ITypeInfo* CCL_API getFormClass () const = 0;

	virtual tbool CCL_API suggestSourceFile (String& sourceFile, UIDRef cid, StringRef initialName) const = 0;

	virtual tbool CCL_API suggestAssetFolder (IUrl& folder, UIDRef cid = kNullUID) const = 0;

	virtual tbool CCL_API getSupportedFileTypes (IFileTypeFilter& fileTypes, UIDRef cid) const = 0;

	virtual IImage* CCL_API loadImage (StringRef fileName) const = 0;

	virtual IMemoryStream* CCL_API loadBinaryFile (StringRef fileName) const = 0;

	virtual SkinAttributeType CCL_API getAttributeType (ISkinElement* element, StringID attributeName) const = 0;

	virtual tbool CCL_API isVariantOrTabView (ISkinViewElement* viewElement) const = 0;	
	
	virtual tbool CCL_API canHaveChildViews (ISkinViewElement* viewElement) const = 0;

	virtual ISkinViewElement* CCL_API getReferencedForm (ISkinViewElement* viewElement) const = 0;

	virtual tbool CCL_API getSizeChange (SkinValueChange& valueChange, ISkinViewElement* viewElement, RectRef newSize) const = 0;
	
	virtual tbool CCL_API detectSizeChange (Rect& newSize, ISkinViewElement* viewElement, const SkinValueChange& valueChange) const = 0;

	virtual tbool CCL_API drawFormBackground (IGraphics& graphics, ISkinViewElement* viewElement) const = 0;

	virtual tbool CCL_API drawViewElement (IGraphics& graphics, ISkinViewElement* viewElement) const = 0;
	
	virtual tbool CCL_API getSourceCodeForElement (String& sourceCode, ISkinElement* element) const = 0;

	virtual void CCL_API setModelDirty (ISkinModel::ElementType type, ISkinElement* changedElement = nullptr) = 0;
	
	virtual tbool CCL_API saveModelChanges (IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (ISkinEditSupport)
};

DEFINE_IID (ISkinEditSupport, 0x2f67c4dc, 0xcb27, 0x4a32, 0xa1, 0xe6, 0x7a, 0xf5, 0xe7, 0xa7, 0x36, 0x2)

} // namespace CCL

#endif // _ccl_iskineditsupport_h
