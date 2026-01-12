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
// Filename    : ccl/extras/webfs/downloaddraghandler.cpp
// Description : Download Drag Handler
//
//************************************************************************************************

#include "ccl/extras/webfs/downloaddraghandler.h"
#include "ccl/extras/webfs/webfilemethods.h"

#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("WebFS")
	XSTRING (Download, "Download")
END_XSTRINGS

//************************************************************************************************
// DownloadDragHandler
//************************************************************************************************

DownloadDragHandler* DownloadDragHandler::create (const DragEvent& event, IView* view, const FileType& fileType)
{
	AutoPtr<DownloadDragHandler> handler (NEW DownloadDragHandler (view, fileType));
	if(handler->prepare (event.session.getItems (), &event.session))
	{
		event.session.setResult (IDragSession::kDropCopyReal);
		handler->retain ();
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DownloadDragHandler::DownloadDragHandler (IView* view, const FileType& fileType)
: DragHandler (view),
  fileType (fileType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DownloadDragHandler::drop (const DragEvent& event)
{
	ForEachUnknown (event.session.getItems (), unk)
		UnknownPtr<IDownloadable> sourceInfo (unk);
		UnknownPtr<IFileDescriptor> descriptor (unk);
		if(sourceInfo && descriptor)
			FileMethods ().installFile (sourceInfo->getSourceUrl (), *descriptor);
	EndFor

	return DragHandler::drop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* DownloadDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IDownloadable> sourceInfo (&item);
	UnknownPtr<IFileDescriptor> descriptor (&item);
	if(sourceInfo && descriptor)
	{
		FileType fileType;
		descriptor->getFileType (fileType);
		if(fileType == this->fileType)
		{
			IFileHandler::State state = System::GetFileTypeRegistry ().getHandlers ().getState (*descriptor);
			if(state != IFileHandler::kNotCompatible)
			{
				String fileName;
				descriptor->getFileName (fileName);

				AutoPtr<IImage> icon (FileIcons::instance ().createIcon (fileType));
				spriteBuilder.addItem (icon, fileName);
				
				return sourceInfo.detach ();
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DownloadDragHandler::finishPrepare ()
{
	if(!getData ().isEmpty ())
		spriteBuilder.addHeader (XSTR (Download), -1);
}
