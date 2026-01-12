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
// Filename    : ccl/app/components/imageselector.cpp
// Description : Image Selector
//
//************************************************************************************************

#include "ccl/app/components/imageselector.h"

#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/utilities/imagebuilder.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/collections/iunknownlist.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("ImageSelector")
	XSTRING (AskScaleImage, "The image size is limited to %(1) x %(2). Do you want to scale the image?")
END_XSTRINGS

//************************************************************************************************
// ImageSelector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ImageSelector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageSelector::ImageSelector (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("ImageSelector") : name),
  provider (nullptr),
  selectorEnabled (true),
  iconSetMode (false),
  iconSetSizeIDList (0)
{
	provider = paramList.addImage (CSTR ("image"), kImage);
	paramList.addParam (CSTR ("selectImage"), kSelectImage);
	paramList.addParam (CSTR ("removeImage"), kRemoveImage)->enable (false);
	paramList.addParam (CSTR ("saveImage"), kSaveImage)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSelector::enable (bool state)
{
	selectorEnabled = state;
	paramList.byTag (kSelectImage)->enable (selectorEnabled);
	paramList.byTag (kRemoveImage)->enable (selectorEnabled && getImage ());
	paramList.byTag (kSaveImage)->enable (selectorEnabled && getImage ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageSelector::setImage (IImage* image)
{
	if(image != provider->getImage ())
	{
		provider->setImage (image);
		enable (selectorEnabled); // update param enabled states
		signal (Message (kPropertyChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageSelector::getImage () const
{
	return provider->getImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageSelector::loadImage (UrlRef path)
{
	AutoPtr<IImage> image = ImageFile::loadImage (path);
	return setImageChecked (image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageSelector::setImageChecked (IImage* _image)
{
	SharedPtr<IImage> image (_image);
	ASSERT (image != nullptr)
	if(image == nullptr)
		return false;

	// check image size
	if(!maxImageSize.isNull ())
	{
		bool fit = image->getWidth () <= maxImageSize.x && image->getHeight () <= maxImageSize.y;
		if(fit == false)
		{
			int result = Alert::ask (String ().appendFormat (XSTR (AskScaleImage), maxImageSize.x, maxImageSize.y), Alert::kYesNo);
			if(result == Alert::kNo)
				return false;

			Rect maxRect (0, 0, maxImageSize.x, maxImageSize.y);
			Rect srcRect (0, 0, image->getWidth (), image->getHeight ());
			Rect dstRect (srcRect);
			dstRect.fitProportionally (maxRect);

			AutoPtr<IImage> image2 = GraphicsFactory::createBitmap (dstRect.getWidth (), dstRect.getHeight ());
			{
				AutoPtr<IGraphics> graphics = GraphicsFactory::createBitmapGraphics (image2);
				graphics->drawImage (image, srcRect, dstRect);
			}

			image = image2;
		}
	}

	// check for icon set
	if(isIconSetMode () == true && image->getType () != IImage::kMultiple)
	{
		AutoPtr<IImage> iconSet = ImageBuilder::createIconSet (image, iconSetSizeIDList);
		image = iconSet; // shared pointer!
	}

	setImage (image);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageSelector::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kRemoveImage :
		{
			setImage (nullptr);
		}
		break;

	case kSelectImage :
		{
			AutoPtr<IFileSelector> selector = ccl_new<IFileSelector> (ClassID::FileSelector);
			ASSERT (selector != nullptr)

			int count = ImageFile::getNumImageFormats ();
			for(int i = 0; i < count; i++)
			{
				const FileType& format = ImageFile::getImageFormat (i);
				selector->addFilter (format);
			}

			if(isIconSetMode () == true) // file type is private, explicitly add it here
				if(const FileType* fileType = ImageFile::getFormatByMimeType (ImageFile::kIconSet))
					selector->addFilter (*fileType);

			Promise p (selector->runAsync (IFileSelector::kOpenFile));
			p.then ([this, selector] (IAsyncOperation& operation) 
			{
				if(operation.getResult () && selector->getPath ())
					loadImage (*selector->getPath ());
			});
		}
		break;

	case kSaveImage :
		{
			AutoPtr<IFileSelector> selector = ccl_new<IFileSelector> (ClassID::FileSelector);
			ASSERT (selector)

			int count = ImageFile::getNumImageFormats ();
			for(int i = 0; i < count; i++)
				selector->addFilter (ImageFile::getImageFormat (i));

			Promise p (selector->runAsync (IFileSelector::kSaveFile));
			p.then ([this, selector] (IAsyncOperation& operation) 
			{
				if(operation.getResult () && selector->getPath ())
				{
					MutableCString mimeType (selector->getPath ()->getFileType ().getMimeType ());
					ImageFile (mimeType, ImageSelector::getImage ()).saveToFile (*selector->getPath ());
				}
			});
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ImageSelector::getObject (StringID name, UIDRef classID)
{
	if(name == "dropTarget")
		return this->asUnknown ();

	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageSelector::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	bool accepted = false;
	UnknownPtr<IUrl> path (data.getFirst ());
	if(path)
		accepted = ImageFile::canLoadImage (*path);
	else
	{
		UnknownPtr<IImage> image (data.getFirst ());
		accepted = image.isValid ();
	}

	if(accepted && session)
		session->setResult (IDragSession::kDropCopyReal);
	return accepted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageSelector::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	bool accepted = false;
	UnknownPtr<IUrl> path (data.getFirst ());
	if(path)
		accepted = loadImage (*path);
	else
	{
		UnknownPtr<IImage> image (data.getFirst ());
		if(image)
			accepted = setImageChecked (image);
	}
	return accepted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageSelector::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasImage")
	{
		var = getImage () != nullptr;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
