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
// Filename    : ccl/app/documents/packagedocument.h
// Description : Package Document
//
//************************************************************************************************

#ifndef _ccl_packagedocument_h
#define _ccl_packagedocument_h

#include "ccl/app/documents/document.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class PackageInfo;
class ArchiveHandler;
class Settings;
interface IPackageFile;
interface IProgressNotify;

//************************************************************************************************
// PackageDocument
//************************************************************************************************

class PackageDocument: public Document,
					   public IStorageRegistry
{
public:
	DECLARE_CLASS (PackageDocument, Document)

	PackageDocument (DocumentClass* documentClass = nullptr);
	~PackageDocument ();

	virtual PackageInfo& getPackageInfo (bool update = false);
	void resetDocumentMetaInfo ();

	Settings& getDocumentSettings ();

	virtual bool load (IProgressNotify* progress);
	virtual bool save (IProgressNotify* progress);
	bool saveTo (UrlRef path, IProgressNotify* progress);

	// IStorageRegistry
	void CCL_API registerHandler (IStorageHandler* handler) override;
	void CCL_API unregisterHandler (IStorageHandler* handler) override;

	// Document
	bool load () override;
	bool save () override;
	IUnknown* CCL_API getMetaInfo () const override;

	CLASS_INTERFACE (IStorageRegistry, Document)

protected:
	static IPackageFile* createPackageForSave (UrlRef path);

	PackageInfo* packageInfo;
	LinkedList<IStorageHandler*> handlerList;
	Settings* settings;

	String makeProgressText (StringRef elementName, bool isSave);

	virtual bool checkCompatibility ();
	virtual bool loadContent (ArchiveHandler& archiveHandler);
	virtual bool saveContent (ArchiveHandler& archiveHandler);
};

} // namespace CCL

#endif // _ccl_packagedocument_h
