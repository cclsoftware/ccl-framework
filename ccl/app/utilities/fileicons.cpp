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
// Filename    : ccl/app/utilities/fileicons.cpp
// Description : File Icons
//
//************************************************************************************************

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/app/component.h"
#include "ccl/app/utilities/imagebuilder.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// FileIcons
//************************************************************************************************

DEFINE_SINGLETON (FileIcons)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIcons::FileIcons ()
: privateIcons (FileIconsPrivate::instance ())
{
	if(System::IsInMainAppModule () == false)
		appIcons = ccl_new<IFileIcons> (ClassID::FileIcons);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::createIcon (UrlRef url, int flags)
{
	if(IImage* icon = privateIcons.createIcon (url, flags))
		return icon;
	return appIcons ? appIcons->createIcon (url, flags) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::createIcon (const FileType& fileType, int flags)
{
	if(IImage* icon = privateIcons.createIcon (fileType, flags))
		return icon;
	return appIcons ? appIcons->createIcon (fileType, flags) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::createIcon (StringRef fileName, int flags)
{
	if(IImage* icon = privateIcons.createIcon (fileName, flags))
		return icon;
	return appIcons ? appIcons->createIcon (fileName, flags) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::createVolumeIcon (int type, int flags)
{
	if(IImage* icon = privateIcons.createVolumeIcon (type, flags))
		return icon;
	return appIcons ? appIcons->createVolumeIcon (type, flags) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileIcons::setFolderIcon (UrlRef path, IImage* icon)
{
	if(appIcons)
		appIcons->setFolderIcon (path, icon);
	else
		privateIcons.setFolderIcon (path, icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::getDefaultFolderIcon (tbool open) const
{
	if(IImage* icon = privateIcons.getDefaultFolderIcon (open))
		return icon;
	return appIcons ? appIcons->getDefaultFolderIcon (open) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIcons::createFolderPreview (IImage* folderIcon, const IContainer& content, int size) const
{
	if(folderIcon == nullptr)
		return nullptr;
	if(IImage* icon = privateIcons.createFolderPreview (folderIcon, content, size))
		return icon;
	return appIcons ? appIcons->createFolderPreview (folderIcon, content, size) : nullptr;
}

//************************************************************************************************
// FileIconsPrivate
//************************************************************************************************

DEFINE_SINGLETON_CLASS (FileIconsPrivate, Object)
DEFINE_CLASS_UID (FileIconsPrivate, 0x23d1a134, 0xc4bf, 0x472b, 0xb4, 0xe8, 0xa, 0xec, 0x16, 0x73, 0x25, 0x69)
DEFINE_CLASS_NAMESPACE (FileIconsPrivate, "Host")

DEFINE_SINGLETON (FileIconsPrivate)
const char* FileIconsPrivate::kIconNamePrefix  = "FileIcon:";

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIconsPrivate::FileIconsPrivate ()
: theme (RootComponent::instance ().getTheme ()),
  systemSink (*NEW SignalSink (Signals::kSystemInformation))
{
	System::GetSystem ().getLocation (desktopPath, System::kDesktopFolder);
	System::GetSystem ().getLocation (documentsPath, System::kUserDocumentFolder);
	System::GetSystem ().getLocation (musicPath, System::kUserMusicFolder);
	System::GetSystem ().getLocation (contentPath, System::kUserContentFolder);

	systemSink.setObserver (this);
	systemSink.enable (true);

	ASSERT (theme)
	folderIcon		= theme->getImage ("FolderIcon:normal");
	openFolderIcon	= theme->getImage ("FolderIcon:open");
	unknownTypeIcon	= theme->getImage ("FileIcon:unknown");
	desktopIcon 	= theme->getImage ("FolderIcon:Desktop");
	documentsIcon 	= theme->getImage ("FolderIcon:UserDocuments");
	musicIcon		= theme->getImage ("FolderIcon:UserMusic");
	contentIcon 	= theme->getImage ("FolderIcon:UserContent");

	userIcons.objectCleanup (true);
	folderIcons.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIconsPrivate::~FileIconsPrivate ()
{
	systemSink.enable (false);
	delete &systemSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::getDefaultFolderIcon (tbool open) const
{
	return open ? openFolderIcon : folderIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::createFolderPreview (IImage* folderIcon, const IContainer& content, int size) const
{
	if(folderIcon == nullptr)
		return nullptr;
	AutoPtr<IImage> preview = nullptr;
	if(size <= 0)
		preview = ImageBuilder::createBitmapCopy (folderIcon);
	else
		preview = ImageBuilder::createSizedImage (folderIcon, size, size, ImageBuilder::isHighResolutionImageNeeded () ? 2.f : 1.f);
	if(preview)
	{
		AutoPtr<IGraphics> g = GraphicsFactory::createBitmapGraphics (preview);
		ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);

		int contentCount = 0;
		ForEachUnknown (content, unk)
			UnknownPtr<IImage> icon (unk);
			if(icon.isValid ())
				contentCount++;
		EndFor

		static const int kPreviewMargin = 2;
		static const int kPreviewIconScale = 2;
		static const int kPreviewOverlapFactor = 3;
		static const int kMaxPreviewItems = 4;

		Point previewSize (preview->getWidth () / kPreviewIconScale, preview->getHeight () / kPreviewIconScale);
		contentCount = ccl_min (contentCount, 1 + (preview->getWidth () - 2 * kPreviewMargin - previewSize.x) / (previewSize.x / kPreviewOverlapFactor));
		Point fullPreviewSize (previewSize.x + (contentCount - 1) * previewSize.x / kPreviewOverlapFactor, previewSize.y);
		Point offset ((preview->getWidth () - fullPreviewSize.x) / 2, preview->getHeight () - fullPreviewSize.y);
		Rect previewRect (offset.x, offset.y, offset.x + previewSize.x, offset.y + previewSize.y);

		int iconCount = 0;
		ForEachUnknown (content, unk)
			UnknownPtr<IImage> icon (unk);
			if(!icon.isValid ())
				continue;
			Point iconSize (icon->getWidth (), icon->getHeight ());
			Rect dstRect (previewRect);
			if(iconSize.x > iconSize.y)
			{
				dstRect.setHeight (dstRect.getHeight () * iconSize.y / iconSize.x);
				dstRect.offset (0, (previewRect.getHeight () - dstRect.getHeight ()) / 2);
			}
			else if(iconSize.y > iconSize.x)
			{
				dstRect.setWidth (dstRect.getWidth () * iconSize.x / iconSize.y);
				dstRect.offset ((previewRect.getWidth () - dstRect.getWidth ()) / 2, 0);
			}
			g->drawImage (icon, iconSize, dstRect, &mode);
			previewRect.offset (previewSize.x / kPreviewOverlapFactor, 0);
			iconCount++;
			if(iconCount >= contentCount)
				break;
		EndFor
	}
	return preview.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* FileIconsPrivate::getImage (StringID name) const
{
	if(IImage* image = theme->getImage (name))
		return image;

	if(!userIcons.isEmpty ())
		ForEach (userIcons, UserIcon, icon)
			if(icon->getName () == name)
				return icon->getImage ();
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIconsPrivate::setUserIcon (StringID _name, IImage* image)
{
	ASSERT (!_name.isEmpty ())
	if(_name.isEmpty ())
		return;

	MutableCString name (kIconNamePrefix);
	name += _name;
	
	UserIcon* userIcon = nullptr;
	ForEach (userIcons, UserIcon, icon)
		if(icon->getName () == name)
		{
			userIcon = icon;
			break;
		}
	EndFor

	if(image)
	{
		if(userIcon == nullptr)
		{
			userIcon = NEW UserIcon;
			userIcon->setName (name);
			userIcons.add (userIcon);
		}
		userIcon->setImage (image);
	}
	else
	{
		if(userIcon)
			userIcons.remove (userIcon),
			userIcon->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIconsPrivate::setUserIcon (const FileType& fileType, IImage* image)
{
	MutableCString name (fileType.getExtension ());
	setUserIcon (name, image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIconsPrivate::setUserIcons (const StringDictionary& dict, const IImagePalette& palette)
{
	userIcons.removeAll (); // remove old icon associations

	for(int i = 0; i < dict.countEntries (); i++)
	{
		String name = dict.getKeyAt (i);
		String value = dict.getValueAt (i);
		
		IImage* image = nullptr;
		int64 index = -1;
		value.getIntValue (index);
		if(index != -1)
			image = UnknownPtr<IImage> (palette.getAt ((int)index).asUnknown ());

		ASSERT (image != nullptr)
		if(image)
			setUserIcon (MutableCString (name), image);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIconsPrivate::getUserAssignment (StringDictionary& dict, const IImagePalette& palette) const
{
	dict.removeAll ();

	ForEach (userIcons, UserIcon, icon)
		int index = palette.getIndex (icon->getImage ());
		SOFT_ASSERT  (index != -1, MutableCString ().appendFormat ("FileIconsPrivate: user icon not in palette: %s", icon->getName ().str ()).str ())
		if(index != -1)
		{
			MutableCString name = icon->getName ().subString (icon->getName ().index (":") + 1);
			dict.setEntry (String (name), String () << index);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileIconsPrivate::setFolderIcon (UrlRef path, IImage* icon)
{
	FolderIcon* folderIcon = nullptr;
	ForEach (folderIcons, FolderIcon, icon)
		if(icon->getPath ().isEqualUrl (path))
		{
			folderIcon = icon;
			break;
		}
	EndFor

	if(icon)
	{
		if(folderIcon == nullptr)
		{
			folderIcon = NEW FolderIcon;
			folderIcon->setPath (path);
			folderIcons.add (folderIcon);
		}
		folderIcon->setImage (icon);
	}
	else
	{
		if(folderIcon)
			folderIcons.remove (folderIcon),
			folderIcon->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::createIcon (UrlRef url, int flags)
{
	switch(PathClassifier::classify (url))
	{
	case PathClassifier::kFile :
		return createIcon (url.getFileType (), flags);

	case PathClassifier::kNativeRoot :
		return createVolumeIcon (kVolumeList, flags);

	case PathClassifier::kNativeVolume :
		{
			VolumeInfo info;
			info.type = INativeFileSystem::kSuppressSlowVolumeInfo; // suppress details for remote drives, etc.
			System::GetFileSystem ().getVolumeInfo (info, url);
			return createVolumeIcon (info.type, flags);
		}
		break;

	case PathClassifier::kPackageRoot :
		return createVolumeIcon (kPackageList, flags);

	case PathClassifier::kPackageVolume :
		return createVolumeIcon (VolumeInfo::kPackage, flags);
	}

	// check for special folder icons
	if(url.isNativePath ())
	{
		if(!folderIcons.isEmpty ())
			ForEach (folderIcons, FolderIcon, folderIcon)
				if(folderIcon->getPath ().isEqualUrl (url))
					return return_shared<IImage> (folderIcon->getImage ());
			EndFor

		// Desktop
		if(desktopIcon && url.isEqualUrl (desktopPath))
			return return_shared<IImage> (desktopIcon);

		// User Documents
		if(documentsIcon && url.isEqualUrl (documentsPath))
			return return_shared<IImage> (documentsIcon);
		
		// User Music
		if(musicIcon && url.isEqualUrl (musicPath))
			return return_shared<IImage> (musicIcon);

		// User Content
		if(contentIcon && url.isEqualUrl (contentPath))
			return return_shared<IImage> (contentIcon);
	}

	if(flags & kNoDefaultFolderIcon)
		return nullptr;

	return return_shared<IImage> ((flags & kOpen) ? openFolderIcon : folderIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::createIcon (StringRef fileName, int flags)
{
	FileType fileType;
	int index = fileName.lastIndex (CCLSTR ("."));
	if(index != -1)
	{
		String ext = fileName.subString (index + 1);
		fileType.setExtension (ext);
	}
	return createIcon (fileType, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::createIcon (const FileType& fileType, int flags)
{
	IImage* icon = nullptr;

	// try extension ("FileIcon:wav")
	MutableCString extension (fileType.getExtension ());
	extension.toLowercase ();
	MutableCString iconName	(kIconNamePrefix);
	iconName.append (extension);
	if(icon = getImage (iconName))
		return return_shared<IImage> (icon);

	if(!fileType.getMimeType ().isEmpty ())
	{
		// try icon for MIME type ("FileIcon:audio/x-wav")
		MutableCString iconName	(kIconNamePrefix);
		iconName.append (fileType.getMimeType ());
		//todo: slash in image name does not work (interpretet as scope)! 
		//if(icon = getImage (iconName))
		//	return returnIcon (icon);

		// try icon for media type of MIME type ("FileIcon:audio")
		if(int index = iconName.index ('/'))
		{
			iconName.truncate (index);
			if(icon = getImage (iconName))
				return return_shared<IImage> (icon);
		}
	}

	return return_shared<IImage> (unknownTypeIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API FileIconsPrivate::createVolumeIcon (int type, int flags)
{
	MutableCString iconName ("VolumeIcon:");
	
	switch(type)
	{
	case kVolumeList			: iconName += "List"; break;
	case kPackageList			: iconName += "PackageList"; break;
	case VolumeInfo::kUnknown   :
	case VolumeInfo::kLocal     : iconName += "Local"; break;
	case VolumeInfo::kRemote    : iconName += "Remote"; break;
	case VolumeInfo::kOptical   : iconName += "Optical"; break;
	case VolumeInfo::kRemovable : iconName += "Removable"; break;
	case VolumeInfo::kPackage   : iconName += "Package"; break;
	}

	IImage* icon = getImage (iconName);
	if(icon == nullptr)
		icon = (flags & kOpen) ? openFolderIcon : folderIcon;

	return return_shared<IImage> (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileIconsPrivate::notify (ISubject* subject, MessageRef msg)
{
	// update content path
	if(msg == Signals::kContentLocationChanged)
		System::GetSystem ().getLocation (contentPath, System::kUserContentFolder);
}
