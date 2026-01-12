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
// Filename    : ccl/app/documents/documentrenamer.h
// Description : Document Renamer
//
//************************************************************************************************

#ifndef _ccl_documentrenamer_h
#define _ccl_documentrenamer_h

#include "ccl/app/components/filerenamer.h"

#include "ccl/base/storage/url.h"

namespace CCL {

class Document;

//************************************************************************************************
// DocumentRenamer
//************************************************************************************************

class DocumentRenamer: public Renamer
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentRenamer, Renamer)

	DocumentRenamer (Document& document);

	// Renamer
	void makeLegalName (String& name) override;
	bool doesAlreadyExist (StringRef newName) override;
	bool performRename (StringRef newName) override;

protected:
	Document& document;
	Url oldFile;
	Url oldFolder;
	FileType fileType;
	bool hasOwnFolder;	///< does the document reside in a folder with the same name?

	virtual bool renameFolder (UrlRef newFolder);

	static StringRef strRenameFileFailed ();
	static StringRef strRenameFolderFailed ();
	void showErrorMessage (StringRef pattern, UrlRef path);	
};

//************************************************************************************************
// DocumentPathHelper
//************************************************************************************************

class DocumentPathHelper
{
public:
	DocumentPathHelper (UrlRef documentFile);

	StringRef getFileName () const;
	UrlRef getParentFolder () const;

	bool getDedicatedFolder (Url& folder) const;
	bool hasDedicatedFolder () const;

protected:
	String fileName;
	Url parentFolder;
	bool hasOwnFolder = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// DocumentPathHelper inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringRef DocumentPathHelper::getFileName () const
{ return fileName; }

inline UrlRef DocumentPathHelper::getParentFolder () const
{ return parentFolder; }

inline bool DocumentPathHelper::hasDedicatedFolder () const
{ return hasOwnFolder; }

} // namespace CCL

#endif // _ccl_documentrenamer_h
