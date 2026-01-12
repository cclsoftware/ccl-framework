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
// Filename    : ccl/app/documents/documentassistant.h
// Description : New Document Assistant
//
//************************************************************************************************

#ifndef _ccl_documentassistant_h
#define _ccl_documentassistant_h

#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documenttemplates.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/idialogbuilder.h"

#include "ccl/base/signalsource.h"

namespace CCL {

class DialogBox;
class CustomizationPreset;
class CustomizationPresetMemento;
class CustomizationComponent;

//************************************************************************************************
// NewDocumentAssistant
//************************************************************************************************

class NewDocumentAssistant: public Component,
							public IDialogButtonInterest
{
public:
	DECLARE_CLASS (NewDocumentAssistant, Component)
	DECLARE_METHOD_NAMES (NewDocumentAssistant)

	NewDocumentAssistant ();
	~NewDocumentAssistant ();

	bool run (const FileType* defaultType = nullptr);

	// Public methods for template handler:
	DocumentTemplateList* loadSecondaryTemplates (UrlRef path);
	void selectSecondaryTemplate (int index);
	void setDropFileTypes (const Container& fileTypes);
	void setExclusiveDropFileTypes (const Container& fileTypes);
	void setConfirmEnabled (bool state);
	void closeDialog (bool apply = false);	
	void setDropImportFile (UrlRef url, bool temporaryDocument = false);

	DECLARE_STRINGID_MEMBER (kFilesID)

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	CLASS_INTERFACE (IDialogButtonInterest, Component)

protected:
	static String kDocumentComponent;
	static CCL::Configuration::BoolValue applyCustomization;

	class FileList;

	SignalSink systemInfoSink;
	SignalSink fileSystemSink;
	Url watchedTemplatesFolder;
	bool userTemplateRescanNeeded;
	bool temporaryDocument;
	AutoPtr<DocumentTemplateList> templateList;
	AutoPtr<DocumentTemplate::CategoryFilter> factoryFilter;
	AutoPtr<DocumentTemplateList> secondaryTemplateList;
	AutoPtr<FileTypeFilter> dropFileTypes;
	AutoPtr<FileTypeFilter> exclusiveDropFileTypes;
	AutoPtr<IUrl> dropImportFile;
	DocumentTemplateProvider* templateProvider;
	FileList* fileList;
	DialogBox* currentDialog;
	AutoPtr<Document> currentDocument;
	AutoPtr<Component> documentComponent;
	ViewPtr documentPropertyFrame;
	IComponent* currentTemplateHandler;
	IComponent* previousTemplateHandler;
	CustomizationPresetMemento* previousCustomizationPreset;
	IParameter* confirmButton;
	bool canConfirm;

	void onTemplateSelected (MessageRef msg);
	void onOpenSelectedTemplate (MessageRef msg);
	void updateCurrentTemplate ();
	void applyCustomizationPreset (Document* document);
	
	void replaceDocument (Document* newDocument);
	void replaceDocumentComponent (Component* newComponent);
	IView* createDefaultPropertyView ();
	void updateDocumentPropertyView ();
	bool replaceTemplateHandler (IComponent* newHandler);
	void setPreviousTemplateHandler (IComponent* newHandler);

	DocumentClass* findClassForTemplate (const DocumentTemplate& t) const;
	DocumentTemplate* findDefaultTemplate (const FileType& fileType) const;
	CustomizationPreset* findCustomizationPreset (StringRef presetId, const FileType& documentType) const;

	IDocumentEventHandler* createDocumentEventHandler () const;
	void updateImportFile (const IUrl* path);
	bool isDropFile (UrlRef path) const;
	bool isExclusiveDropFile (UrlRef path) const;

	// Component
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// IDialogButtonInterest
	void CCL_API setDialogButton (IParameter* button, int which) override;
	tbool CCL_API onDialogButtonHit (int which) override;
};

//************************************************************************************************
// IDocumentTemplateHandler
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocumentTemplateHandler: IUnknown
{
	/** Assistant document can be confirmed. */
	virtual tbool CCL_API canConfirm () const = 0;

	/** Handle assistant confirmation. */
	virtual void CCL_API onConfirm (tbool canceled) = 0;

	/** Notify new document component. */
	virtual void CCL_API onDocumentComponentChanged () = 0;

	DECLARE_IID (IDocumentTemplateHandler)
};

//************************************************************************************************
// DocumentTemplateHandler
/** Base class for document template handler. */
//************************************************************************************************

class DocumentTemplateHandler: public Component,
							   public IDocumentTemplateHandler,
							   public IPersistAttributes
{
public:
	DECLARE_CLASS (DocumentTemplateHandler, Component)
	
	// IDocumentTemplateHandler
	tbool CCL_API canConfirm () const override;
	void CCL_API onConfirm (tbool canceled) override;
	void CCL_API onDocumentComponentChanged () override;

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	IObjectNode* CCL_API findChild (StringRef id) const override;

	CLASS_INTERFACE2 (IDocumentTemplateHandler, IPersistAttributes, Component)

protected:
	virtual void getDropFileTypes (Container& fileTypes) const {}
	virtual void getExclusiveDropFileTypes (Container& fileTypes) const {}
};

//************************************************************************************************
// DocumentEventHandler
/** Base class for document event handler. */
//************************************************************************************************

class DocumentEventHandler: public Object,
							public IPersistAttributes,
							public AbstractDocumentEventHandler
{
public:
	DECLARE_CLASS (DocumentEventHandler, Object)

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;

	// IDocumentEventHandler
	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override;

	CLASS_INTERFACE2 (IPersistAttributes, IDocumentEventHandler, Object)

protected:
	UnknownList filesToImport;
};

} // namespace CCL

#endif // _ccl_documentassistant_h
