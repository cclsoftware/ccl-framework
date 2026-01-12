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
// Filename    : ccl/app/documents/documentdialog.h
// Description : New Document Dialog
//
//************************************************************************************************

#ifndef _ccl_documentdialog_h
#define _ccl_documentdialog_h

#include "ccl/app/component.h"

namespace CCL {

class Document;
class DocumentClass;
class DocumentTemplate;
class DocumentTemplateProvider;
class DialogBox;
interface IAsyncOperation;

//************************************************************************************************
// AsyncNewDocumentDialog
//************************************************************************************************

class AsyncNewDocumentDialog: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (AsyncNewDocumentDialog, Component)

	AsyncNewDocumentDialog (StringRef name, StringID formName, DocumentClass& documentClass, 
							StringRef initialDocumentName = nullptr);
	~AsyncNewDocumentDialog ();

	static AsyncNewDocumentDialog* fromArguments (const Attributes* args); ///< get instance from arguments
	static void addToArguments (Attributes& args, Component* dialog);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	void setTemplateProvider (DocumentTemplateProvider* provider);

	void runAsync (); ///< run asynchronously

	virtual void applyTo (Document& document) const; ///< apply options to document

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

protected:
	DocumentClass& documentClass;
	IParameter* pathParam;
	IParameter* nameParam;
	DocumentTemplateProvider* templateProvider;
	DialogBox* currentDialog;
	bool creating;

	Attributes& getSettings () const;
	Url getDefaultFolder () const;
	void setDocumentFolder (UrlRef folder);
	void getDocumentFolder (Url& folder) const;
	void getDocumentLocation (Url& path) const;
	void onDialogCompleted (IAsyncOperation&);
	void createDocument ();

	void closeDialog (int result);
	void applyAndClose ();

	const DocumentTemplate* getSelectedTemplate () const;
	const DocumentTemplate* copySelectedTemplate (Document& document); ///< copies template data file to document path
	void triggerTemplateSelected ();
	virtual void onTemplateSelected ();

private:
	const DocumentTemplate* lastTemplate;

	void storeSettings () const;
	void restoreSettings ();
};

//************************************************************************************************
// NewDocumentDialog
//************************************************************************************************

class NewDocumentDialog: public AsyncNewDocumentDialog
{
public:
	DECLARE_CLASS_ABSTRACT (NewDocumentDialog, AsyncNewDocumentDialog)

	NewDocumentDialog (StringRef name, StringID formName, Document& document);

	static NewDocumentDialog* createForDocument (Document& document, StringID contextId = ""); ///< create instance via document class
	static NewDocumentDialog* shareFromArguments (const Attributes* args); ///< share if available, must be released by caller
	
	bool run (); ///< run dialog

	virtual void apply ();	///< apply options to document

	// AsyncNewDocumentDialog
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	Document& document;

	Url getFolder (IParameter* folderParam) const;
	bool selectFolder (IParameter* folderParam);
};

} // namespace CCL

#endif // _ccl_documentdialog_h
