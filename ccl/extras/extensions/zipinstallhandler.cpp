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
// Filename    : ccl/extras/extensions/zipinstallhandler.cpp
// Description : ZIP Installation Handler
//
//************************************************************************************************

#include "ccl/extras/extensions/zipinstallhandler.h"
#include "ccl/extras/extensions/extensiondraghandler.h"

#include "ccl/extras/packages/unifiedpackageinstaller.h"

#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// ZipDragHandler
//************************************************************************************************

class ZipDragHandler: public ExtensionDragHandler
{
public:
	ZipDragHandler (IView* view)
	: ExtensionDragHandler (view)
	{}

	// ExtensionDragHandler
	bool matches (const FileType& fileType) const override
	{
		return fileType == FileTypes::Zip ();
	}

	void install (UrlRef path) override
	{
		System::GetSystemShell ().openUrl (path, System::kDeferOpenURL|System::kDoNotOpenExternally);
	}
};

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//************************************************************************************************
// ZipInstallHandler
//************************************************************************************************

IDragHandler* ZipInstallHandler::createDragHandler (const DragEvent& event, IView* view)
{
	AutoPtr<ZipDragHandler> handler (NEW ZipDragHandler (view));
	if(handler->prepare (event.session.getItems (), &event.session))
	{
		event.session.setResult (IDragSession::kDropCopyReal);
		handler->retain ();
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ZipInstallHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ZipInstallHandler::ZipInstallHandler (int installationOrder)
: AbstractFileInstallHandler (installationOrder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZipInstallHandler::toDefaultPath (IUrl& dstPath, UrlRef srcPath) const
{
#if 0 // next to ZIP file
	String packageName;
	srcPath.getName (packageName, false);
	dstPath.assign (srcPath);
	dstPath.ascend ();
	dstPath.descend (packageName, Url::kFolder);

#else // a bit hacky: path selected in installer
	dstPath.assign (Packages::UnifiedPackageInstaller::instance ().getInstallEngine ().getTargetPath ());
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ZipInstallHandler::openFile (UrlRef path)
{
	if(path.isNativePath () && path.getFileType () == FileTypes::Zip ())
	{
		AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (path);
		if(e && canHandlePackage (e->getID ()))
		{
			Url dstPath;
			toDefaultPath (dstPath, path);
			bool result = extractFile (dstPath, *e);
			return result;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ZipInstallHandler::canHandle (IFileDescriptor& descriptor)
{
	Attributes metaInfo;
	descriptor.getMetaInfo (metaInfo);
	String id = metaInfo.getString (Meta::kPackageID);
	return canHandlePackage (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ZipInstallHandler::performInstallation (IFileDescriptor& descriptor, IUrl& path)
{
	if(path.isNativePath () && path.getFileType () == FileTypes::Zip ())
		if(canHandle (descriptor))
		{
			AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (path);
			if(e)
			{
				Url dstPath;
				toDefaultPath (dstPath, path);
				bool result = extractFile (dstPath, *e);
				if(result)
					path.assign (dstPath);
				return result;
			}
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipInstallHandler::extractFile (IUrl& dstPath, ExtensionDescription& description)
{
	UrlRef srcPath = description.getPath ();

	// open package
	AutoPtr<IPackageFile> p = System::GetPackageHandler ().openPackage (srcPath);
	if(p == nullptr)
		return false;

	// determine which part to extract
	Url part;
	if(File* file = description.getManifestEntry ())
	{
		if(!file->getUnpackFolder ().isEmpty ())
			part.setPath (file->getUnpackFolder (), Url::kFolder);
		if(!file->getTargetFolder ().isEmpty ())
			dstPath.descend (file->getTargetFolder (), Url::kFolder);
	}

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	progress->setTitle (description.getTitle ());
	if(UnknownPtr<IProgressDialog> dialog = static_cast<IProgressNotify*> (progress)) 
	{
		dialog->setOpenDelay (1.); // do not open immediately
		dialog->constrainLevels (2, 2); // avoid flicker
	}
	
	ProgressNotifyScope scope (progress);
	if(!part.getPath ().isEmpty ())
		p->extractFolder (part, dstPath, true, nullptr, progress);
	else
		p->extractAll (dstPath, true, nullptr, progress);

	return true;
}
