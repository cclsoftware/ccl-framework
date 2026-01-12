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
// Filename    : ccl/app/utilities/fileicons.h
// Description : File Icons
//
//************************************************************************************************

#ifndef _ccl_fileicons_h
#define _ccl_fileicons_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/app/ifileicons.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class FileType;
class SignalSink;
class StringDictionary;
class FileIconsPrivate;
interface ITheme;
interface IImagePalette;

//************************************************************************************************
// FileIcons
/** File icon registry with fallback to application icons, can be used from any module. */
//************************************************************************************************

class FileIcons: public Object,
				 public Singleton<FileIcons>,
				 public IFileIcons
{
public:
	FileIcons ();

	// IFileIcons
	IImage* CCL_API createIcon (UrlRef url, int flags = 0) override;
	IImage* CCL_API createIcon (const FileType& fileType, int flags = 0) override;
	IImage* CCL_API createIcon (StringRef fileName, int flags = 0) override;
	IImage* CCL_API createVolumeIcon (int type, int flags = 0) override;
	void CCL_API setFolderIcon (UrlRef path, IImage* icon) override;
	IImage* CCL_API getDefaultFolderIcon (tbool open = false) const override;
	IImage* CCL_API createFolderPreview (IImage* folderIcon, const IContainer& content, int size = 0) const override;
	
	CLASS_INTERFACE (IFileIcons, Object)

protected:
	FileIconsPrivate& privateIcons;
	AutoPtr<IFileIcons> appIcons;
};

//************************************************************************************************
// FileIconsPrivate
/** File icon registry private to current module. */
//************************************************************************************************

class FileIconsPrivate: public Object,
						public Singleton<FileIconsPrivate>,
						public IFileIcons
{
public:
	DECLARE_CLASS (FileIconsPrivate, Object)

	void setUserIcon (StringID name, IImage* image);
	void setUserIcon (const FileType& fileType, IImage* image);
	void setUserIcons (const StringDictionary& dict, const IImagePalette& palette);
	void getUserAssignment (StringDictionary& dict, const IImagePalette& palette) const;

	// IFileIcons
	IImage* CCL_API createIcon (UrlRef url, int flags = 0) override;
	IImage* CCL_API createIcon (const FileType& fileType, int flags = 0) override;
	IImage* CCL_API createIcon (StringRef fileName, int flags = 0) override;
	IImage* CCL_API createVolumeIcon (int type, int flags = 0) override;
	void CCL_API setFolderIcon (UrlRef path, IImage* icon) override;
	IImage* CCL_API getDefaultFolderIcon (tbool open = false) const override;
	IImage* CCL_API createFolderPreview (IImage* folderIcon, const IContainer& content, int size = 0) const override;
	
	CLASS_INTERFACE (IFileIcons, Object)

protected:
	friend class Singleton<FileIconsPrivate>;
	FileIconsPrivate ();
	~FileIconsPrivate ();

	static const char* kIconNamePrefix;

	class UserIcon: public Object
	{
	public:
		PROPERTY_MUTABLE_CSTRING (name, Name)
		PROPERTY_SHARED_AUTO (IImage, image, Image)
	};

	class FolderIcon: public Object
	{
		PROPERTY_OBJECT (Url, path, Path)
		PROPERTY_SHARED_AUTO (IImage, image, Image)
	};

	ITheme* theme;
	ObjectList userIcons;
	ObjectList folderIcons;

	Url desktopPath;
	Url documentsPath;
	Url musicPath;
	Url contentPath;
	SignalSink& systemSink;

	SharedPtr<IImage> folderIcon;
	SharedPtr<IImage> openFolderIcon;
	SharedPtr<IImage> unknownTypeIcon;
	SharedPtr<IImage> desktopIcon;
	SharedPtr<IImage> documentsIcon;
	SharedPtr<IImage> musicIcon;
	SharedPtr<IImage> contentIcon;

	IImage* getImage (StringID name) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_fileicons_h
