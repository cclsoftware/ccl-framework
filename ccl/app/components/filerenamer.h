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
// Filename    : ccl/app/components/filerenamer.h
// Description : File Renamer
//
//************************************************************************************************

#ifndef _ccl_filerenamer_h
#define _ccl_filerenamer_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"

namespace CCL {

interface IAsyncOperation;

//************************************************************************************************
// Renamer
//************************************************************************************************

class Renamer: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (Renamer, Component)

	Renamer (StringRef oldName = nullptr);
	~Renamer ();

	PROPERTY_STRING (alreadyExistsMessage, AlreadyExistsMessage)

	StringRef getOldName () const;	///< old name of item to be renamed (used to check if entered name is different)
	void setOldName (StringRef name, bool updateInitialName = true);

	PROPERTY_STRING (initialName, InitialName) ///< initially offered name in the dialog (optional, default: oldName)
	
	bool runDialog (StringRef title);	///< show rename dialog until succeeded or canceled
	CCL::IAsyncOperation* runDialogAsync (StringRef title);
	bool tryRename (StringRef newName = nullptr);	///< trigger renaming without dialog
	
 	StringRef getNewName () const;
	virtual bool canRenameNow () { return true; }
	
	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	DECLARE_STRINGID_MEMBER (kWarnExists)

protected:
	virtual void makeLegalName (String& name) {}
	virtual bool doesAlreadyExist (StringRef newName) { return false; }
	virtual bool performRename (StringRef newName)    { return true; }
	IParameter& getNameParam ();

private:
	IParameter* nameParam;
	String newName;
	String oldName;
	bool renameSucceeded;
};

//************************************************************************************************
// FileRenamer
//************************************************************************************************

class FileRenamer: public Renamer
{
public:
	DECLARE_CLASS_ABSTRACT (FileRenamer, Renamer)

	FileRenamer (UrlRef oldPath, bool canEditExtension = true);

	PROPERTY_OBJECT (Url, oldPath, OldPath)

	Url* createNewPath ();

	// Renamer
	bool doesAlreadyExist (StringRef newName) override;
	bool performRename (StringRef newName) override;
	tbool CCL_API paramChanged (IParameter* param) override;

	static StringRef strFileAlreadyExists ();
	static StringRef strFolderAlreadyExists ();

protected:
	bool canEditExtension;
	bool editExtension;

	void makeNewPath (Url& newPath, StringRef newName);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringRef Renamer::getNewName () const
{ return newName; }

inline StringRef Renamer::getOldName () const
{ return oldName; }

inline IParameter& Renamer::getNameParam ()
{ return *nameParam; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_filerenamer_h
