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
// Filename    : ccl/app/components/imageselector.h
// Description : Image Selector
//
//************************************************************************************************

#ifndef _ccl_imageselector_h
#define _ccl_imageselector_h

#include "ccl/app/component.h"

#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/graphics/types.h"

namespace CCL {

//************************************************************************************************
// ImageSelector
//************************************************************************************************

class ImageSelector: public Component,
					 public IDataTarget
{
public:
	DECLARE_CLASS (ImageSelector, Component)

	ImageSelector (StringRef name = nullptr);

	void enable (bool state);

	void setImage (IImage* image);
	IImage* getImage () const;

	PROPERTY_OBJECT (Point, maxImageSize, MaxImageSize)
	PROPERTY_BOOL (iconSetMode, IconSetMode)
	PROPERTY_VARIABLE (int, iconSetSizeIDList, IconSetSizeIDList)

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;

	CLASS_INTERFACE (IDataTarget, Component)

protected:
	enum Tags
	{
		kImage = 100,
		kSelectImage,
		kRemoveImage,
		kSaveImage
	};

	IImageProvider* provider;
	bool selectorEnabled;

	bool loadImage (UrlRef path);
	bool setImageChecked (IImage* image);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_imageselector_h
