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
// Filename    : ccl/app/documents/documentdialog.cpp
// Description : New Document Dialog
//
//************************************************************************************************

#include "ccl/app/documents/documentdialog.h"

#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/documenttemplates.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (NoFileName, "Please supply a filename.")
	XSTRING (ResetFolder, "Reset Folder")
END_XSTRINGS

//************************************************************************************************
// AsyncNewDocumentDialog
//************************************************************************************************

AsyncNewDocumentDialog* AsyncNewDocumentDialog::fromArguments (const Attributes* args)
{
	return args ? args->getObject<AsyncNewDocumentDialog> ("dialog") : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::addToArguments (Attributes& args, Component* dialog)
{
	args.set ("dialog", dialog->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AsyncNewDocumentDialog, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncNewDocumentDialog::AsyncNewDocumentDialog (StringRef name, StringID formName, DocumentClass& documentClass,
												StringRef initialDocumentName)
: Component (name),
  formName (formName),
  documentClass (documentClass),
  pathParam (nullptr),
  nameParam (nullptr),
  templateProvider (nullptr),
  currentDialog (nullptr),
  creating (false),
  lastTemplate (nullptr)
{
	pathParam = paramList.addString (CSTR ("documentPath"));
	nameParam = paramList.addString (CSTR ("documentName"));

	setDocumentFolder (getDefaultFolder ());
	restoreSettings ();

	String documentName = initialDocumentName;
	if(documentName.isEmpty ())
		documentName = documentClass.makeTitle ();

	// make unique folder name
	Url documentPath;
	getDocumentFolder (documentPath);
	documentPath.descend (documentName, Url::kFolder);
	documentPath.makeUnique ();
	documentPath.getName (documentName);

	nameParam->fromString (documentName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncNewDocumentDialog::~AsyncNewDocumentDialog  ()
{
	ASSERT (currentDialog == nullptr)

	setTemplateProvider (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::setTemplateProvider (DocumentTemplateProvider* provider)
{
	share_and_observe (this, templateProvider, provider);
	
	if(templateProvider) // initial update
		triggerTemplateSelected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& AsyncNewDocumentDialog::getSettings () const
{
	String settingsID (String ("NewDocument.") << documentClass.getFileType ().getExtension ());
	return Settings::instance ().getAttributes (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::storeSettings () const
{
	String pathString;
	pathParam->toString (pathString);

	getSettings ().set ("documentPath", pathString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::restoreSettings ()
{
	String pathString;
	getSettings ().getString (pathString, "documentPath");

	// only apply if folder still exists
	if(!pathString.isEmpty ())
	{
		Url folder;
		folder.fromDisplayString (pathString, IUrl::kFolder);
		if(System::GetFileSystem ().fileExists (folder))
			pathParam->fromString (pathString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncNewDocumentDialog::notify (ISubject* subject, MessageRef msg)
{
	if(templateProvider && subject == templateProvider)
	{
		if(msg == DocumentTemplateProvider::kOpenSelected)
		{
			applyAndClose ();
		}
		else
		{
			if(msg == kPropertyChanged || msg == DocumentTemplateProvider::kSecondaryChanged)
				triggerTemplateSelected ();
			signal (msg);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* AsyncNewDocumentDialog::getSelectedTemplate () const
{
	if(templateProvider)
	{
		if(templateProvider->getSecondaryTemplate ())
			return templateProvider->getSecondaryTemplate ();
		else
			return templateProvider->getSelected ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* AsyncNewDocumentDialog::copySelectedTemplate (Document& document)
{
	const DocumentTemplate* t = getSelectedTemplate ();
	if(t && !t->isEmpty ())
	{
		// copy template
		bool templateCopied = System::GetFileSystem ().copyFile (document.getPath (), t->getDataPath ());
		if(!templateCopied)
		{
			// 2nd try with trimmed template filename (could have been created in previous versions, but whitespace can easily get lost when transferring files)
			Url trimmedUrl (t->getDataPath ());
			String fileName;
			trimmedUrl.getName (fileName, false);
			FileType fileType (trimmedUrl.getFileType ());

			fileName.trimWhitespace ();
			trimmedUrl.setName (fileName, Url::kFile);
			trimmedUrl.setFileType (fileType, true);
			if(trimmedUrl != t->getDataPath ())
				templateCopied = System::GetFileSystem ().copyFile (document.getPath (), trimmedUrl);
		}

		if(templateCopied)
			return t;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::triggerTemplateSelected ()
{
	auto newTemplate = getSelectedTemplate ();
	if(lastTemplate != newTemplate) // avoid duplicate updates
	{
		lastTemplate = newTemplate;
		onTemplateSelected ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::onTemplateSelected ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AsyncNewDocumentDialog::getProperty (Variant& var, MemberID propertyId) const
{
	if(SuperClass::getProperty (var, propertyId)) // "hasChild", etc.
		return true;

	if(templateProvider && templateProvider->getProperty (var, propertyId))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url AsyncNewDocumentDialog::getDefaultFolder () const
{
	Url folder (DocumentManager::instance ().getDocumentFolder ());
	if(!documentClass.getSubFolder ().isEmpty ())
		folder.descend (documentClass.getSubFolder (), Url::kFolder);

	return folder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::setDocumentFolder (UrlRef folder)
{
	pathParam->fromString (UrlDisplayString (folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::getDocumentFolder (Url& folder) const
{
	String pathString;
	pathParam->toString (pathString);
	folder.fromDisplayString (pathString, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::getDocumentLocation (Url& path) const
{
	String documentName;
	nameParam->toString (documentName);
	documentName.trimWhitespace ();
	documentName = LegalFileName (documentName);

	// make unique folder name
	getDocumentFolder (path);
	path.descend (LegalFolderName (documentName), IUrl::kFolder);
	path.makeUnique ();
	path.getName (documentName);

	path.descend (documentName, IUrl::kFile);
	const FileType& fileType = documentClass.getFileType ();
	path.setExtension (fileType.getExtension (), false); // name may contain a dot
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::applyTo (Document& document) const
{
	// set document path to allow save without file selector
	Url path;
	getDocumentLocation (path);
	document.setPath (path);

	// create folder for unique naming
	Url folder (path);
	folder.ascend ();
	if(System::GetFileSystem ().createFolder (folder))
		document.setCreatedFolder (folder);

	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::runAsync ()
{
	IView* view = getTheme ()->createView (formName, this->asUnknown ());
	ASSERT (view != nullptr)
	if(view == nullptr)
		return;

	retain ();
	ASSERT (currentDialog == nullptr)
	currentDialog = NEW DialogBox;

	Promise p ((*currentDialog)->runDialogAsync (view, Styles::kWindowCombinedStyleDialog, Styles::kDialogOkCancel));
	p.then (this, &AsyncNewDocumentDialog::onDialogCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::closeDialog (int result)
{
	if(currentDialog)
	{
		(*currentDialog)->setDialogResult (result);
		(*currentDialog)->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::applyAndClose ()
{
	closeDialog (DialogResult::kOkay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::createDocument ()
{
	if(creating)
		return;
	
	SharedPtr<Unknown> lifeGuard (this); // to keep creating variable alife
	ScopedVar<bool> guard (creating, true);
	Attributes args;
	addToArguments (args, this);
	DocumentManager::instance ().createDocument (&documentClass.getFileType (), 0, &args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncNewDocumentDialog::onDialogCompleted (IAsyncOperation& operation)
{
	if(operation.getResult ().asInt () == DialogResult::kOkay)
		createDocument ();
	
	delete currentDialog;
	currentDialog = nullptr;

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AsyncNewDocumentDialog::appendContextMenu (IContextMenu& contextMenu)
{
	if(contextMenu.getContextID ().startsWith ("documentPath"))
	{
		auto handler = makeCommandDelegate ([this] (const CommandMsg& msg, VariantRef data)
		{
			if(!msg.checkOnly ())
				setDocumentFolder (getDefaultFolder ());

			return true;
		}, nullptr);

		contextMenu.addCommandItem (CommandWithTitle (CSTR ("File"), CSTR ("Reset Folder"), XSTR (ResetFolder)), handler);
	}
	return SuperClass::appendContextMenu (contextMenu);
}

//************************************************************************************************
// NewDocumentDialog
//************************************************************************************************

NewDocumentDialog* NewDocumentDialog::createForDocument (Document& document, StringID contextId)
{
	if(auto docClass = document.getDocumentClass ())
		if(AutoPtr<Component> c = docClass->createNewDialog (document, contextId))
			return return_shared (ccl_cast<NewDocumentDialog> (c));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NewDocumentDialog* NewDocumentDialog::shareFromArguments (const Attributes* args)
{
	if(auto dialog = fromArguments (args))
		return return_shared (ccl_cast<NewDocumentDialog> (dialog));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (NewDocumentDialog, AsyncNewDocumentDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

NewDocumentDialog::NewDocumentDialog (StringRef name, StringID formName, Document& document)
: AsyncNewDocumentDialog (name, formName, *document.getDocumentClass (), document.getTitle ()),
  document (document)
{	
	paramList.addParam (CSTR ("changePath"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url NewDocumentDialog::getFolder (IParameter* folderParam) const
{
	String pathString;
	folderParam->toString (pathString);

	Url path;
	path.fromDisplayString (pathString, IUrl::kFolder);
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentDialog::selectFolder (IParameter* folderParam)
{
	AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
	fs->setPath (getFolder (folderParam));
	if(fs->run ())
	{
		String pathString;
		fs->getPath ().toDisplayString (pathString);
		folderParam->fromString (pathString);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentDialog::apply ()
{
	applyTo (document);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentDialog::run ()
{
	int result = DialogResult::kCancel;
	AutoPtr<IView> view = getTheme ()->createView (formName, this->asUnknown ());
	if(view)
	{
		while(1)
		{
			{
				DialogBox dialog;
				ScopedVar<DialogBox*> scope (currentDialog, &dialog);
				result = dialog->runDialog (return_shared<IView> (view), Styles::kWindowCombinedStyleDialog, Styles::kDialogOkCancel);
			}

			if(result != DialogResult::kOkay)
				break;

			String name;
			nameParam->toString (name);
			if(name.isEmpty () == false)
				break;

			Alert::error (XSTR (NoFileName));
		}
	}
	return result == DialogResult::kOkay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentDialog::paramChanged (IParameter* param)
{
	if(param->getName () == "changePath")
		selectFolder (pathParam);

	return true;
}

