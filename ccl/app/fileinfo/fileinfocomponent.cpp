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
// Filename    : ccl/app/fileinfo/fileinfocomponent.cpp
// Description : File Info Component
//
//************************************************************************************************

#include "ccl/app/fileinfo/fileinfocomponent.h"
#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/filetype.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/formatter.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileInfo")
	XSTRING (Folder, "Folder")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum StandardFileInfoTags
	{
		kFileIcon = 1,
		kFileName = 10,
		kFullPath,
		kFileType,
		kFileSize,
		kDateModified
	};
}

//************************************************************************************************
// FileInfoComponent
//************************************************************************************************

bool FileInfoComponent::isLocal (UrlRef path)
{
	return System::GetFileSystem ().isLocalFile (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (FileInfoComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileInfoComponent::FileInfoComponent (StringRef name, StringID formName)
: Component (name),
  formName (formName),
  skinNamespace (CSTR ("CCL")),
  explicitSkinNamespace (false)
{
	fileIcon = paramList.addImage (CSTR ("fileIcon"), Tag::kFileIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileInfoComponent::assignSkinNamespace (StringID _skinNamespace)
{
	// allow override only if namespace wasn't explicitely empty before
	if(!getSkinNamespace ().isEmpty () && explicitSkinNamespace == false)
		setSkinNamespace (_skinNamespace);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileInfoComponent::setFile (UrlRef path)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileInfoComponent::isDefault ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileInfoComponent::setDisplayAttributes (IImage* icon, StringRef title)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileInfoComponent::getFileInfoString (String& result, StringID id) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API FileInfoComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FileInfo")
	{
		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)

		MutableCString fullFormName;
		if(!skinNamespace.isEmpty ())
		{
			fullFormName += skinNamespace;
			fullFormName += "/";
		}
		fullFormName += formName;

		IView* view = theme ? theme->createView (fullFormName, this->asUnknown ()) : nullptr;
		ASSERT (view != nullptr)
		return view;
	}
	return nullptr;
}

//************************************************************************************************
// StandardFileInfo
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StandardFileInfo, FileInfoComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardFileInfo::StandardFileInfo (StringRef name, StringID formName)
: FileInfoComponent (name, formName)
{
	paramList.addString (CSTR ("fileName"), Tag::kFileName);
	paramList.addString (CSTR ("fullPath"), Tag::kFullPath);
	paramList.addString (CSTR ("fileType"), Tag::kFileType);
	paramList.addString (CSTR ("fileSize"), Tag::kFileSize);
	paramList.addString (CSTR ("dateModified"), Tag::kDateModified);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardFileInfo::setFile (UrlRef path)
{
	String fileName;
	String fullPath;
	String fileTypeString;
	String fileSize;
	String dateModified;

	path.getName (fileName, false);
	path.toDisplayString (fullPath, IUrl::kStringDisplayPath);

	if(path.isFolder ())
	{
		fileTypeString = XSTR (Folder);
		if(fileName.isEmpty ())
			path.getPathName (fileName);
	}
	else
	{
		fileTypeString = path.getFileType ().getDescription ();

		FileInfo fileInfo;
		if(System::GetFileSystem ().getFileInfo (fileInfo, path))
		{
			if(fileInfo.fileSize != -1) // -1: unknown size
				fileSize = CCL::Format::ByteSize::print (fileInfo.fileSize);
			dateModified = CCL::Format::DateTime::print (fileInfo.modifiedTime);
		}
	}

	fileIcon->setImage (AutoPtr<IImage> (FileIcons::instance ().createIcon (path)));

	paramList.byTag (Tag::kFileName)->fromString (fileName);
	paramList.byTag (Tag::kFullPath)->fromString (fullPath);
	paramList.byTag (Tag::kFileType)->fromString (fileTypeString);
	paramList.byTag (Tag::kFileSize)->fromString (fileSize);
	paramList.byTag (Tag::kDateModified)->fromString (dateModified);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardFileInfo::setDisplayAttributes (IImage* icon, StringRef title)
{
	if(icon)
		fileIcon->setImage (icon);
	if(!title.isEmpty ())
		paramList.byTag (Tag::kFileName)->fromString (title);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardFileInfo::getFileInfoString (String& result, StringID id) const
{
	if(id == kFileInfo1)
	{
		paramList.byTag (Tag::kFileType)->toString (result);
		if(result == XSTR (Folder))
			result.empty ();
		return true;
	}
	return false;
}
