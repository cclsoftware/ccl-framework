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
// Filename    : ccl/app/documents/packagedocument.cpp
// Description : Package Document
//
//************************************************************************************************

#define DEBUG_LOG 0 // enable for load/save profiling

#include "ccl/app/documents/packagedocument.h"
#include "ccl/app/documents/documentmetainfo.h"

#include "ccl/app/component.h"

#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// DocumentGenerator
//************************************************************************************************

class DocumentGenerator
{
public:
	DocumentGenerator (DocumentClass* documentClass);

	static bool extract (String& name, VersionNumber& version, StringRef generator);

	enum CheckResult { kSameFormat, kOlderFormat, kNewerFormat };
	CheckResult checkCompatibility (const DocumentMetaInfo& metaInfo, bool warn = true);

protected:
	DocumentClass* documentClass;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (Saving, "Saving %(1)...")
	XSTRING (Loading, "Loading %(1)...")
	XSTRING (SavingElement, "Saving: %(1)...")
	XSTRING (LoadingElement, "Loading: %(1)...")
	XSTRING (CompatibilityWarning, "This file is not compatible because it has been created with a newer version of $APPNAME.\n\nThis version: %(1)\nGenerator: %(2)")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

static const Configuration::BoolValue delayProgressDialog ("PackageDocument", "delayProgressDialog", false);
static const Configuration::BoolValue useMemoryBinForSave ("PackageDocument", "useMemoryBinForSave", false);
static const Configuration::FloatValue packageCompressionLevel ("PackageDocument", "compressionLevel", 0.5);

//************************************************************************************************
// DocumentGenerator
//************************************************************************************************

bool DocumentGenerator::extract (String& name, VersionNumber& version, StringRef generator)
{
	name.empty ();
	version = VersionNumber ();

	int index = generator.lastIndex (CCLSTR ("/"));
	if(index != -1)
	{
		name = generator.subString (0, index);
		version.scan (generator.subString (index+1));
	}
	else
		name = generator;

	return !name.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentGenerator::DocumentGenerator (DocumentClass* documentClass)
: documentClass (documentClass)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentGenerator::CheckResult DocumentGenerator::checkCompatibility (const DocumentMetaInfo& metaInfo, bool warn)
{
	String otherGenerator = metaInfo.getGenerator ();
	ASSERT (!otherGenerator.isEmpty ())

	String currentGenerator (RootComponent::instance ().getGeneratorName ());

	// 1) check for explicit document format version
	ASSERT (documentClass != nullptr)
	int thisFormatVersion = documentClass ? documentClass->getFormatVersion () : 0;
	int otherFormatVersion = metaInfo.getFormatVersion ();
	if(thisFormatVersion > 0 && otherFormatVersion > 0)
	{
		if(thisFormatVersion == otherFormatVersion)
			return kSameFormat;
		if(otherFormatVersion < thisFormatVersion)
			return kOlderFormat;

		if(warn)
			Alert::warn (String ().appendFormat (XSTR (CompatibilityWarning), currentGenerator, otherGenerator));

		return kNewerFormat;
	}

	// 2) check generator version
	if(otherGenerator.isEmpty ())
		return kSameFormat;

	String currentName, otherName;
	VersionNumber currentVersion, otherVersion;
	extract (currentName, currentVersion, currentGenerator);
	extract (otherName, otherVersion, otherGenerator);

	VersionNumber v1 (currentVersion), v2 (otherVersion);
	v1.revision = v2.revision = v1.build = v2.build = 0; // ignore revision + build number
	CheckResult result = v1.compare (v2) > 0 ? kNewerFormat : v1.compare (v2) < 0 ? kOlderFormat : kSameFormat;

	if(result == kNewerFormat && warn)
		Alert::warn (String ().appendFormat (XSTR (CompatibilityWarning), currentGenerator, otherGenerator));

	return result;
}

//************************************************************************************************
// PackageDocument
//************************************************************************************************

IPackageFile* PackageDocument::createPackageForSave (UrlRef path)
{
	return System::GetPackageHandler ().createPackage (path, ClassID::ZipFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PackageDocument, Document)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageDocument::PackageDocument (DocumentClass* documentClass)
: Document (documentClass),
  packageInfo (NEW PackageInfo),
  settings (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageDocument::~PackageDocument ()
{
	packageInfo->release ();
	if(settings)
		settings->release ();

	ASSERT (handlerList.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageInfo& PackageDocument::getPackageInfo (bool update)
{
	ASSERT (packageInfo != nullptr)
	if(update == true)
	{
		DocumentMetaInfo documentInfo (*packageInfo);

		ASSERT (documentClass != nullptr)
		if(documentInfo.getMimeType ().isEmpty ())
			documentInfo.setMimeType (documentClass->getFileType ().getMimeType ());

		if(documentClass->getFormatVersion () > 0)
			documentInfo.setFormatVersion (documentClass->getFormatVersion ());

		if(documentInfo.getTitle ().isEmpty ())
		{
			// don't include description (from a restored docuemnt version) in metainfo title
			String title (getTitle ());
			String descriptionSuffix;
			descriptionSuffix << " (" << documentInfo.getDescription () << ")";

			int index = title.lastIndex (descriptionSuffix);
			if(index >= 0 && index == (title.length () - descriptionSuffix.length ()))
				title.truncate (index);

			documentInfo.setTitle (title);
		}

		if(documentInfo.getCreator ().isEmpty ())
			documentInfo.setCreator (RootComponent::instance ().getCreatorName ());

		if(documentInfo.getGenerator ().isEmpty ())
			documentInfo.setGenerator (RootComponent::instance ().getGeneratorName ());
	}
	return *packageInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageDocument::resetDocumentMetaInfo ()
{
	DocumentMetaInfo documentInfo (getPackageInfo ());
	documentInfo.resetCreationInfo ();

	isOlderFormat (false); // clear old format flag
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PackageDocument::getMetaInfo () const
{
	return packageInfo->asUnknown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& PackageDocument::getDocumentSettings ()
{
	if(!settings)
		settings = NEW Settings (CCLSTR ("Settings"));
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageDocument::registerHandler (IStorageHandler* handler)
{
	handlerList.append (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageDocument::unregisterHandler (IStorageHandler* handler)
{
	handlerList.remove (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::load ()
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	String progressTitle;
	progressTitle.appendFormat (XSTR (Loading), getTitle ());
	progress->setTitle (progressTitle);
	progress->setProgressText (progressTitle);
	if(getPreviewMode ().isEmpty () == false || delayProgressDialog)
		UnknownPtr<IProgressDialog> (progress)->setOpenDelay (1., isSilentPreview () == false); // do not open immediately

	return load (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::load (IProgressNotify* progress)
{
	CCL_PROFILE_START (PackageDocument_load)

	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (getPath (), IPackageHandler::kNestedPackageSupported);
	ASSERT (packageFile != nullptr)
	if(!packageFile)
		return false;

	IPackageFile::Closer packageFileCloser (*packageFile);
	IFileSystem* fileSystem = packageFile->getFileSystem ();
	ASSERT (fileSystem != nullptr)

	{
		ProgressNotifyScope progressScope (progress);

		ArchiveHandler archiveHandler (*fileSystem);
		archiveHandler.setProgress (progress);

		CancelGuard cancelGuard (*this, progress);
		if(!loadContent (archiveHandler))
			return false;
	}

	CCL_PROFILE_STOP (PackageDocument_load)

	return SuperClass::load (); // clear dirty state
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::checkCompatibility ()
{
	ASSERT (documentClass != nullptr)
	DocumentGenerator generator (documentClass);
	DocumentMetaInfo metaInfo (getPackageInfo ());
	DocumentGenerator::CheckResult checkResult = generator.checkCompatibility (metaInfo, !isSilent ());
	if(checkResult == DocumentGenerator::kOlderFormat)
	{
		isOlderFormat (true);
	}
	else if(checkResult == DocumentGenerator::kNewerFormat)
	{
		isCanceled (true);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::loadContent (ArchiveHandler& archiveHandler)
{
	// load meta info
	if(!getPackageInfo (false).loadFromHandler (archiveHandler))
		return false;

	// check compatibility
	if(checkCompatibility () == false)
		return false;

	archiveHandler.loadItem (CCLSTR ("settings.xml"), "Settings", getDocumentSettings ());

	// load handlers
	ListForEach (handlerList, IStorageHandler*, handler)
		bool result = handler->loadContent (archiveHandler.getFileSystem (), Variant (), archiveHandler.getProgress ()) != 0;
		ASSERT (result == true)
		if(!result)
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::save ()
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	if(isAutoSave () || delayProgressDialog)
	{
		UnknownPtr<IProgressDialog> dialog (progress);
		dialog->setOpenDelay (.5);
		dialog->setTranslucentAppearance (true);
	}
	String progressTitle;
	progressTitle.appendFormat (XSTR (Saving), getTitle ());
	progress->setTitle (progressTitle);
	progress->setProgressText (progressTitle);
	progress->setCancelEnabled (false);

	return save (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::save (IProgressNotify* progress)
{
	CCL_PROFILE_START (PackageDocument_save)

	// prepare URL for temporary file used during save
	Url nativeTempPath (getPath ());
	String tempFileName;
	nativeTempPath.getName (tempFileName);
	tempFileName << ".temp";
	nativeTempPath.setName (tempFileName);
	nativeTempPath.makeUnique ();

	Url tempPath;
	if(useMemoryBinForSave)
	{
		tempPath.setUrl (CCLSTR ("memory://PackageDocument/save"));
		tempPath.makeUnique ();
	}
	else
		tempPath = nativeTempPath;

	bool result = saveTo (tempPath, progress);

	if(useMemoryBinForSave)
	{
		// copy from memory bin to temporary file
		if(result)
			result = System::GetFileSystem ().copyFile (nativeTempPath, tempPath);

		System::GetFileSystem ().removeFile (tempPath);
	}

	// overwrite original document with temporary file
	if(result)
		result = System::GetFileSystem ().moveFile (getPath (), nativeTempPath, INativeFileSystem::kDisableWriteProtection);

	if(!result)
		System::GetFileSystem ().removeFile (nativeTempPath);

	CCL_PROFILE_STOP (PackageDocument_save)

	return result ? SuperClass::save () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::saveTo (UrlRef path, IProgressNotify* progress)
{
	AutoPtr<IPackageFile> packageFile = createPackageForSave (path);
	ASSERT (packageFile != nullptr)
	double compressionLevel = packageCompressionLevel.getValue ();
	if(compressionLevel > 0.)
	{
		packageFile->setOption (PackageOption::kCompressed, true);
		packageFile->setOption (PackageOption::kCompressionLevel, compressionLevel);
	}
	packageFile->setOption (PackageOption::kFailOnInvalidFile, true);
	if(!packageFile->create ())
		return false;

	bool result = false;

	// Close file scope
	{
		IFileSystem* fileSystem = packageFile->getFileSystem ();
		ASSERT (fileSystem != nullptr)

		ProgressNotifyScope progressScope (progress);

		ArchiveHandler archiveHandler (*fileSystem);
		archiveHandler.setProgress (progress);

		IPackageFile::Closer packageFileCloser (*packageFile);

	    #if (0 && DEBUG) // allow reuse of previous package content
		AutoPtr<IPackageFile> oldPackageFile = System::GetPackageHandler ().openPackage (previousPath);
		archiveHandler.setSourcePackage (oldPackageFile);
		#endif

		if(saveContent (archiveHandler))
		{
			//ProgressNotifyScope flushScope (progressScope);
			result = packageFile->flush (progress);
		}
	}

	if(!result)
		System::GetFileSystem ().removeFile (path);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageDocument::saveContent (ArchiveHandler& archiveHandler)
{
	// save (updated) meta info
	DocumentMetaInfo (getPackageInfo ()).resetCreationInfo (); // reset first!
	if(!getPackageInfo (true).saveWithHandler (archiveHandler))
		return false;

	if(settings && !settings->isEmpty ())
		archiveHandler.addSaveTask (CCLSTR ("settings.xml"), "Settings", *settings);

	// save handlers
	ListForEach (handlerList, IStorageHandler*, handler)
		bool result = handler->saveContent (archiveHandler.getFileSystem (), Variant (), archiveHandler.getProgress ()) != 0;
		ASSERT (result == true)
		if(!result)
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageDocument::makeProgressText (StringRef elementName, bool isSave)
{
	String result;
	Variant args[1] = {elementName};
	result.appendFormat (isSave ? XSTR (SavingElement) : XSTR (LoadingElement), args, 1);
	return result;
}

