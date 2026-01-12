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
// Filename    : ccl/app/documents/documentassistant.cpp
// Description : New Document Assistant
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/documents/documentassistant.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/documentdialog.h"

#include "ccl/app/options/customization.h"

#include "ccl/app/params.h"
#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/controls/listviewmodel.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/collections/arraybox.h"

#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// NewDocumentAssistant::FileList
//************************************************************************************************

class NewDocumentAssistant::FileList: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (FileList, ListViewModel)

	FileList (NewDocumentAssistant& assistant);

	void clear ();
	void getPaths (Container& paths) const;
	const IUrl* getImportFilePath () const;

	// ListViewModel
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;

protected:
	NewDocumentAssistant& assistant;

	ListViewItem* prepareItem (UrlRef path) const;
	void removeExclusiveItems ();
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum NewDocumentAssistantTags
	{
		kDocumentType = 100,
		kDocumentTypeIcon,

		kSupportsCustomization,
		kHasCustomization,
		kCustomizationPreset,
		kCustomizationChecked,
		kCustomizationUserPreset,

		kSelectFiles,
		kClearFiles
	};
}

//************************************************************************************************
// NewDocumentAssistant::FileList
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NewDocumentAssistant::FileList, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

NewDocumentAssistant::FileList::FileList (NewDocumentAssistant& assistant)
: assistant (assistant)
{
	getColumns ().addColumn (20, nullptr, kIconID);
	getColumns ().addColumn (300, nullptr, kTitleID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::FileList::clear ()
{
	if(!isEmpty ())
	{
		removeAll ();
		signal (Message (kChanged));
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::FileList::getPaths (Container& paths) const
{
	ASSERT (paths.isObjectCleanup ())
	ForEach (items, ListViewItem, item)
		if(auto url = item->getDetails ().getObject<Url> ("path"))
			if(assistant.isDropFile (*url))
				paths.add (return_shared (url));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* NewDocumentAssistant::FileList::getImportFilePath () const
{
	ForEach (items, ListViewItem, item)
		if(auto url = item->getDetails ().getObject<Url> ("path"))
			if(assistant.isExclusiveDropFile (*url))
				return url;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::FileList::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	ForEachUnknown (data, unk)
		if(UnknownPtr<IUrl> path = unk)
			if(assistant.isDropFile (*path) || assistant.isExclusiveDropFile (*path))
			{
				if(session)
					session->setResult (IDragSession::kDropCopyShared);
				return true;
			}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::FileList::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	bool result = false;
	ForEachUnknown (data, unk)
		if(UnknownPtr<IUrl> path = unk)
		{
			if(assistant.isDropFile (*path))
			{
				addItem (prepareItem (*path));
				result = true;
			}
			else if(assistant.isExclusiveDropFile (*path))
			{
				removeExclusiveItems ();
				addItem (prepareItem (*path));
				result = true;
			}
		}
	EndFor
	if(result)
		signal (Message (kChanged));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* NewDocumentAssistant::FileList::prepareItem (UrlRef path) const
{
	auto item = NEW ListViewItem;
	item->setTitle (UrlDisplayString (path));
	item->getDetails ().set ("path", NEW Url (path), Attributes::kOwns);
	AutoPtr<IImage> icon = FileIcons::instance ().createIcon (path);
	item->setIcon (icon);

	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::FileList::removeExclusiveItems ()
{
	ForEachReverse (items, ListViewItem, item)
		if(auto url = item->getDetails ().getObject<Url> ("path"))
			if(assistant.isExclusiveDropFile (*url))
			{
				removeItem (item);
				item->release ();
			}
	EndFor
}

//************************************************************************************************
// NewDocumentAssistant
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NewDocumentAssistant, Component)
String NewDocumentAssistant::kDocumentComponent ("DocumentComponent");
DEFINE_STRINGID_MEMBER_ (NewDocumentAssistant, kFilesID, "files")
Configuration::BoolValue NewDocumentAssistant::applyCustomization ("Application.NewDocumentAssistant", "applyCustomization", true);

//////////////////////////////////////////////////////////////////////////////////////////////////

NewDocumentAssistant::NewDocumentAssistant ()
: Component ("NewDocumentAssistant"),
  systemInfoSink (Signals::kSystemInformation),
  fileSystemSink (Signals::kFileSystem),
  userTemplateRescanNeeded (false),
  temporaryDocument (false),
  templateList (NEW DocumentTemplateList),
  secondaryTemplateList (NEW DocumentTemplateList),
  dropFileTypes (NEW FileTypeFilter),
  exclusiveDropFileTypes (NEW FileTypeFilter),
  templateProvider (nullptr),
  fileList (nullptr),
  currentDialog (nullptr),
  currentTemplateHandler (nullptr),
  previousTemplateHandler (nullptr),
  previousCustomizationPreset (nullptr),
  confirmButton (nullptr),
  canConfirm (true)
{
	systemInfoSink.setObserver (this);
	fileSystemSink.setObserver (this);

	addComponent (templateProvider = NEW DocumentTemplateProvider (templateList));

	paramList.addString ("documentType", Tag::kDocumentType);
	paramList.addImage ("documentTypeIcon", Tag::kDocumentTypeIcon);

	paramList.addParam ("supportsCustomization", Tag::kSupportsCustomization)->setReadOnly (true);
	paramList.addParam ("hasCustomization", Tag::kHasCustomization)->setReadOnly (true);
	paramList.addString ("customizationPreset", Tag::kCustomizationPreset);
	paramList.addParam ("customizationChecked", Tag::kCustomizationChecked);
	paramList.addAlias ("customizationUserPreset", Tag::kCustomizationUserPreset);

	fileList = NEW FileList (*this);
	addObject ("fileList", fileList);
	fileList->addObserver (this);

	paramList.addParam ("selectFiles", Tag::kSelectFiles);
	paramList.addParam ("clearFiles", Tag::kClearFiles);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NewDocumentAssistant::~NewDocumentAssistant ()
{
	fileList->removeObserver (this);
	fileList->release ();

	ASSERT (currentTemplateHandler == nullptr)
	ASSERT (previousTemplateHandler == nullptr)
	ASSERT (previousCustomizationPreset == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NewDocumentAssistant::initialize (IUnknown* context)
{
	StringList folderNames;
	ObjectArray defaultTemplates;
	for(auto docClass : iterate_as<DocumentClass> (DocumentManager::instance ().getDocumentClasses ()))
		if(docClass->isNative ())
		{
			if(auto defaultTemplate = docClass->createDefaultTemplate ())
			{
				templateList->addTemplate (defaultTemplate);
				defaultTemplates.add (defaultTemplate);
			}
			
			const auto& templateType = docClass->getTemplateType ();
			if(templateType.isValid ())
			{
				templateList->addFileType (templateType);
				secondaryTemplateList->addFileType (templateType);

				docClass->getUserTemplateFolders (folderNames);
			}
		}

	templateList->scanAppFactoryTemplates ();
	templateList->scanAdditionalLocations ();
	templateList->scanUserTemplates (folderNames);

	// watch top-level user templates folder
	DocumentTemplateList::getDefaultUserLocation (watchedTemplatesFolder);
	System::GetFileManager ().addWatchedLocation (watchedTemplatesFolder, IFileManager::kDeep);
	fileSystemSink.enable (true);
	systemInfoSink.enable (true);
	
	// filters for factory/user templates
	factoryFilter = NEW DocumentTemplate::CategoryFilter (nullptr, defaultTemplates, DocumentTemplate::CategoryFilter::kExcludeUser);
	templateProvider->addDisplayFilter (factoryFilter, nullptr);
	templateProvider->addDisplayFilter (AutoPtr<IObjectFilter> (NEW DocumentTemplate::CategoryFilter (nullptr, defaultTemplates, DocumentTemplate::CategoryFilter::kUserOnly)), nullptr);
	templateProvider->restoreSettings ("NewDocumentAssistant.Templates");

	signalSlots.advise (templateProvider, kPropertyChanged, this, &NewDocumentAssistant::onTemplateSelected);
	signalSlots.advise (templateProvider, DocumentTemplateProvider::kOpenSelected, this, &NewDocumentAssistant::onOpenSelectedTemplate);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NewDocumentAssistant::terminate ()
{
	systemInfoSink.enable (false);
	fileSystemSink.enable (false);

	System::GetFileManager ().removeWatchedLocation (watchedTemplatesFolder);
	
	templateProvider->storeSettings ("NewDocumentAssistant.Templates");

	signalSlots.unadvise (templateProvider);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NewDocumentAssistant::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kContentLocationChanged)
	{
		// Templates folder may not yet exist, watch attempt may fail.
		System::GetFileManager ().removeWatchedLocation (watchedTemplatesFolder);
		DocumentTemplateList::getDefaultUserLocation (watchedTemplatesFolder);
		System::GetFileManager ().addWatchedLocation (watchedTemplatesFolder, IFileManager::kDeep);

		userTemplateRescanNeeded = true;
	}
	else if(msg == Signals::kFileCreated || msg == Signals::kFileChanged)
	{
		// Check 'created' signal to pick up file changes in unmonitored templates folder.
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		if(path && watchedTemplatesFolder.contains (*path))
			userTemplateRescanNeeded = true;
	}
	else if((msg == kChanged) && isEqualUnknown (subject, fileList->asUnknown ()))
	{
		propertyChanged ("itemCount");
		updateImportFile (fileList->getImportFilePath ());
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API NewDocumentAssistant::findChild (StringRef id) const
{
	if(id == kDocumentComponent)
		return documentComponent;
	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API NewDocumentAssistant::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "DocumentPropertyFrame")
	{
		ViewBox view (ClassID::View, bounds);
		documentPropertyFrame = view;
		return view;
	}
	else if(name == "DefaultPropertyView")
	{
		return createDefaultPropertyView ();
	}
	else if(name == "DropZone")
	{
		return getTheme ()->createView ("CCL/NewDocumentAssistant.DropZone", this->asUnknown ());
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kSelectFiles :
		{
			AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
			for(auto& ft : dropFileTypes->getContent ())
				fs->addFilter (ft);
			for(auto& ft : exclusiveDropFileTypes->getContent ())
				fs->addFilter (ft);
			
			if(fs->run (IFileSelector::kOpenMultipleFiles))
			{
				UnknownList data;
				for(int i = 0; i < fs->countPaths (); i++)
					data.add (fs->getPath (i), true);
				fileList->insertData (0, 0, data, nullptr);
			}
		}
		break;
	case Tag::kCustomizationChecked:
		applyCustomization.setValue (param->getValue ().asBool ());
		break;
	case Tag::kClearFiles :
		fileList->clear ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "itemCount")
	{
		var = const_cast<FileList*> (fileList)->countFlatItems ();
		return true;
	}
	else if(propertyId == "importFileAvailable")
	{
		var = dropImportFile != nullptr;
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NewDocumentAssistant::setDialogButton (IParameter* button, int which)
{
	if(which != DialogResult::kOkay)
		return;

	confirmButton = button;
	if(confirmButton)
		confirmButton->enable (canConfirm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::onDialogButtonHit (int which)
{
	// Reminder: do not use this to trigger template handler actions.
	// This event occurs within dialog lifecycle and may not trigger
	// actions that require the dialog to be closed.

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentAssistant::run (const FileType* defaultType)
{
	// check for pending rescan
	if(userTemplateRescanNeeded)
	{
		StringList folderNames;
		for(auto docClass : iterate_as<DocumentClass> (DocumentManager::instance ().getDocumentClasses ()))
			if(docClass->isNative () && docClass->getTemplateType ().isValid ())
				docClass->getUserTemplateFolders (folderNames);

		templateList->removeUserTemplates ();
		templateList->scanUserTemplates (folderNames);

		templateProvider->filterChanged ();

		userTemplateRescanNeeded = false;
	}

	// check if caller wants a specifc template type
	if(defaultType)
		if(auto t = findDefaultTemplate (*defaultType))
			templateProvider->select (t);

	auto view = getTheme ()->createView ("CCL/NewDocumentAssistant", this->asUnknown ());
	ASSERT (view)
	if(!view)
		return false;

	DocumentManager::DirtySuspender dirtySuspender;
	ASSERT (DocumentManager::instance ().isMultipleDocuments ()) // we don't close open documents
		
	updateCurrentTemplate ();
	safe_release (previousCustomizationPreset);

	bool result = false;
	{
		DialogBox dialogBox;
		ScopedVar<DialogBox*> scope (currentDialog, &dialogBox);
		result = dialogBox->runDialog (view) == DialogResult::kOkay;
	}

	// Allow handler to perform post-dialog state changes or actions.
	if(UnknownPtr<IDocumentTemplateHandler> handler = currentTemplateHandler)
		handler->onConfirm (!result);

	if(result)
	{
		// Import an existing document.
		if(dropImportFile)
		{
			Attributes args;
			if(auto eventHandler = createDocumentEventHandler ())
				args.set (IDocumentManager::kEventHandler, eventHandler, Attributes::kOwns);

			int flags = IDocumentManager::kHidden;
			if(temporaryDocument)
				flags |= IDocumentManager::kOpenTemporary;

			auto& manager = DocumentManager::instance ();
			auto document = unknown_cast<Document> (manager.openDocument (*dropImportFile, flags, &args));
			if(document)
			{
				applyCustomizationPreset (document);
				manager.showDocument (document);
			}

			// additional cleanup if not done by document manager
			if(!document || !document->getEventHandler ())
				if(UnknownPtr<IDocumentEventHandler> handler = args.getUnknown (IDocumentManager::kEventHandler))
				{
					args.remove (IDocumentManager::kEventHandler);
					ccl_release (handler.detach ()); // must be the only existing reference
				}

			// DocumentManager::openDocument () creates new document. Ensure abandoned
			// currentDocument is terminated in replaceDocument (nullptr) below.
			ASSERT (currentDocument != nullptr)
		}
		// Create a new document.
		else if(currentDocument)
		{
			ASSERT (dropImportFile == nullptr)
			Attributes args;
			NewDocumentDialog::addToArguments (args, documentComponent);
			if(currentDocument->prepare (&args))
			{
				if(auto eventHandler = createDocumentEventHandler ())
				{
					currentDocument->setEventHandler (eventHandler);
					eventHandler->release (); // document takes ownerhip
				}

				auto primaryTemplate = templateProvider->getSelected ();
				if(primaryTemplate)
				{
					String templateId;
					if(primaryTemplate->isUser ())
						templateId = CCLSTR ("User");
					else
					{
						templateId = primaryTemplate->getEnglishTitle ();
						if(templateId.isEmpty ())
							templateId = primaryTemplate->getTitle ();

						auto secondaryTemplate = templateProvider->getSecondaryTemplate ();
						if(secondaryTemplate && !secondaryTemplate->getEnglishTitle ().isEmpty ())
							templateId.append (":").append (secondaryTemplate->getEnglishTitle ());
					}

					templateId << "." << primaryTemplate->getPath ().getFileType ().getExtension ();
					currentDocument->setSourceTemplateID (templateId);
				}

				auto& manager = DocumentManager::instance ();
				manager.addDocument (currentDocument);
				applyCustomizationPreset (currentDocument);
				manager.showDocument (currentDocument);
			}

			// DocumentManager::addDocument () takes currentDocument ownership. Release
			// pointer so replaceDocument (nullptr) below does not terminate it.
			currentDocument.release ();
			ASSERT (currentDocument == nullptr)
		}
		replaceDocumentComponent (nullptr);
	}

	replaceDocument (nullptr);
	replaceTemplateHandler (nullptr);
	setPreviousTemplateHandler (nullptr);

	if(previousCustomizationPreset)
	{
		previousCustomizationPreset->confirmCustomization ();
		previousCustomizationPreset->release ();
		previousCustomizationPreset = nullptr;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::onTemplateSelected (MessageRef msg)
{
	if(currentDialog != nullptr) // only update when dialog shown
		updateCurrentTemplate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::onOpenSelectedTemplate (MessageRef msg)
{
	if(currentDialog != nullptr)
	{
		(*currentDialog)->setDialogResult (DialogResult::kOkay);
		(*currentDialog)->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::updateCurrentTemplate ()
{
	auto t = templateProvider->getSelected ();
	if(!t)
		return;

	auto newClass = findClassForTemplate (*t);
	ASSERT (newClass)
	if(!newClass)
		return;

	CCL_PRINTF ("Updating current template: new document class is %s\n", newClass->myClass ().getPersistentName ())
	auto oldClass = currentDocument ? currentDocument->getDocumentClass () : nullptr;
	bool docClassChanged = newClass != oldClass;

	// detach and reset secondary template, etc.
	if(docClassChanged)
		if(auto newDialog = ccl_cast<NewDocumentDialog> (documentComponent))
			newDialog->setTemplateProvider (nullptr);
	templateProvider->setSecondaryTemplate (nullptr); 	

	dropImportFile = nullptr;
	fileList->clear ();
	temporaryDocument = false;

	// Template handler
	bool handlerCanConfirm = true;
	IComponent* newHandler = nullptr;
	if(t->getTemplateHandlerClassUID ().isValid ())
	{
		newHandler = ccl_new<IComponent> (t->getTemplateHandlerClassUID ());
		if(newHandler)
			newHandler->initialize (this->asUnknown ());

		// Initial 'allow confirmation' state, note that dialog may not be available yet.
		if(UnknownPtr<IDocumentTemplateHandler> handler = newHandler)
			handlerCanConfirm = handler->canConfirm ();
	}
	setConfirmEnabled (handlerCanConfirm);

	ASSERT (newHandler || !t->getTemplateHandlerClassUID ().isValid ())
	bool handlerChanged = replaceTemplateHandler (newHandler);

	// Document
	if(docClassChanged)
	{
		auto newDoc = newClass->createDocument ();
		ASSERT (newDoc)
		if(newDoc)
			replaceDocument (newDoc);
	}

	// force view update if template handler changed
	if(handlerChanged && !docClassChanged)
		updateDocumentPropertyView ();

	// deferred release after view update
	setPreviousTemplateHandler (nullptr);

	// Document type and icon
	auto& documentType = newClass->getFileType ();
	String documentTypeString = documentType.getDescription ();
	paramList.byTag (Tag::kDocumentType)->fromString (documentTypeString);

	AutoPtr<IImage> documentTypeIcon = FileIcons::instance ().createIcon (documentType);
	paramList.byTag<ImageProvider> (Tag::kDocumentTypeIcon)->setImage (documentTypeIcon);

	// Customization
	auto cp = findCustomizationPreset (t->getCustomizationID (), documentType);
	bool hasPreset = cp != nullptr;
	ASSERT (t->getCustomizationID ().isEmpty () || hasPreset)
	paramList.byTag (Tag::kCustomizationPreset)->fromString (cp ? cp->getName () : String::kEmpty);
	paramList.byTag (Tag::kHasCustomization)->setValue (hasPreset);
	paramList.byTag (Tag::kCustomizationPreset)->enable (hasPreset);
	paramList.byTag (Tag::kCustomizationChecked)->enable (hasPreset);

	CustomizationComponent* customizationComponent = CustomizationComponent::findCustomizationComponent (documentType);
	paramList.byTag (Tag::kSupportsCustomization)->setValue (customizationComponent != nullptr);

	// customization preset list param (presents the last preset selected by a user)
	IParameter* presetListParam = nullptr;
	if(!hasPreset && customizationComponent)
		presetListParam = customizationComponent->getUserSelectedPresetParameter ();

	UnknownPtr<IAliasParameter> (paramList.byTag (Tag::kCustomizationUserPreset))->setOriginal (presetListParam);
	paramList.byTag (Tag::kCustomizationUserPreset)->enable (presetListParam != nullptr);

	// Rule: always enable if template provides a customization preset, do
	// not auto-enable if user previously disabled the option
	paramList.byTag (Tag::kCustomizationChecked)->setValue (hasPreset && applyCustomization.getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::applyCustomizationPreset (Document* document)
{
	bool checked = paramList.byTag (Tag::kCustomizationChecked)->getValue ();
	if(checked)
	{
		// apply preset provided by template
		String presetId;
		if(auto t = templateProvider->getSelected ())
			presetId = t->getCustomizationID ();

		if(presetId.isEmpty () || !document)
			return;

		for(auto c : iterate_as<CustomizationComponent> (CustomizationComponent::getInstances ()))
			if(c->matchesDocument (*document))
			{
				if(auto cp = c->getPresetByID (presetId))
				{
					safe_release (previousCustomizationPreset);
					previousCustomizationPreset = NEW CustomizationPresetMemento (c);

					c->selectPreset (*cp, false);
				}
				break;
			}
	}
	else
	{
		// apply user-selected preset
		CustomizationComponent* customizationComponent = CustomizationComponent::findCustomizationComponent (document->getPath ().getFileType ());
		UnknownPtr<IListParameter> presetList (paramList.byTag (Tag::kCustomizationUserPreset));
		if(customizationComponent && presetList)
			if(auto preset = unknown_cast<CustomizationPreset> (presetList->getSelectedValue ()))
				customizationComponent->selectPreset (*preset, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::replaceDocument (Document* newDocument)
{
	if(currentDocument != newDocument)
	{
		CCL_PRINTF ("Replacing document: new = %p...\n", newDocument)				
		replaceDocumentComponent (nullptr);
		
		if(currentDocument)
			currentDocument->terminate ();
		currentDocument = newDocument;
		if(currentDocument)
			currentDocument->initialize ();

		// rebuild dialog component for document
		if(currentDocument)
		{
			auto docClass = currentDocument->getDocumentClass ();
			ASSERT (docClass)
			auto newComponent = docClass->createNewDialog (*currentDocument, "assistant");
			ASSERT (newComponent)

			// inject template provider
			if(auto newDialog = ccl_cast<NewDocumentDialog> (newComponent))
				newDialog->setTemplateProvider (templateProvider);

			replaceDocumentComponent (newComponent);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::replaceDocumentComponent (Component* newComponent)
{
	if(documentComponent != newComponent)
	{
		CCL_PRINTF ("Replacing document component: new = %p\n", newComponent)
		if(documentComponent)
			documentComponent->terminate ();
		documentComponent = newComponent;
		if(documentComponent)
			documentComponent->initialize (this->asUnknown ());
			
		if(UnknownPtr<IDocumentTemplateHandler> docTemplateHandler = currentTemplateHandler)
			docTemplateHandler->onDocumentComponentChanged ();

		updateDocumentPropertyView ();
		signalHasChild (kDocumentComponent);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* NewDocumentAssistant::createDefaultPropertyView ()
{
	if(auto newDialog = ccl_cast<NewDocumentDialog> (documentComponent))
		return getTheme ()->createView (newDialog->getFormName (), newDialog->asUnknown ());
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::updateDocumentPropertyView ()
{
	if(documentPropertyFrame)
	{
		CCL_PRINTLN ("Updating property view...")
		documentPropertyFrame->getChildren ().removeAll ();

		if(!documentComponent) // we are in the middle of changing something
		{
			CCL_PRINTLN ("View update suppressed.")
			return;
		}
		
		AutoPtr<IView> childView;
		bool useDefault = true;

		// check if template handler wants to create a view
		if(currentTemplateHandler)
		{
			MutableCString formName, packageId;
			if(auto handlerClass = ccl_classof (currentTemplateHandler))
			{
				Variant v1;
				handlerClass->getClassAttribute (v1, "formName");
				formName = v1.asString ();

				Variant v2;
				if(UnknownPtr<ICodeResource> codeResource = const_cast<IClassDescription*> (handlerClass))
				   if(const IAttributeList* metaInfo = codeResource->getMetaInfo ())
					   metaInfo->getAttribute (v2, Meta::kPackageID);
				packageId = v2.asString ();
			}

			if(!formName.isEmpty ())
			{
				CCL_PRINTF ("Form name is '%s'.\n", formName.str ())
				useDefault = false; // no view to indicate error if something fails here

				auto theme = packageId.isEmpty () ? System::GetThemeManager ().getApplicationTheme () : 
							 System::GetThemeManager ().getTheme (packageId);
				ASSERT (theme)
				if(theme)
					childView = theme->createView (formName, currentTemplateHandler);
			}
		}
		
		if(useDefault)
		{
			CCL_PRINTLN ("Using default property view.")
			childView = createDefaultPropertyView ();
		}

		if(childView)
		{
			Rect r;
			ViewBox (documentPropertyFrame).getClientRect (r);
			SizeLimit limits = ViewBox (documentPropertyFrame).getSizeLimits ();

			ViewBox (childView).setSizeMode (IView::kAttachAll);
			childView->setSizeLimits (limits);
			childView->setSize (r);

			documentPropertyFrame->getChildren ().add (childView.detach ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentAssistant::replaceTemplateHandler (IComponent* newHandler)
{
	if(currentTemplateHandler != newHandler)
	{
		CCL_PRINTF ("Replacing template handler: new = %p\n", newHandler)
		setPreviousTemplateHandler (currentTemplateHandler); // defer release of old handler
		currentTemplateHandler = newHandler;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::setPreviousTemplateHandler (IComponent* newHandler)
{
	if(previousTemplateHandler)
		previousTemplateHandler->terminate (),
		ccl_release (previousTemplateHandler);

	previousTemplateHandler = newHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass* NewDocumentAssistant::findClassForTemplate (const DocumentTemplate& t) const
{
	for(auto docClass : iterate_as<DocumentClass> (DocumentManager::instance ().getDocumentClasses ()))
		if(docClass->isNative ())
		{
			if(docClass->getTemplateType ().isValid () && docClass->getTemplateType () == t.getFileType ())
				return docClass;
			if(docClass->getFileType () == t.getFileType ())
				return docClass;
		}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplate* NewDocumentAssistant::findDefaultTemplate (const FileType& fileType) const
{
	ASSERT (factoryFilter)
	if(factoryFilter)
		for(auto t : iterate_as<DocumentTemplate> (factoryFilter->getDefaultTemplates ()))
			if(t->getFileType () == fileType)
				return t;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* NewDocumentAssistant::findCustomizationPreset (StringRef presetId, const FileType& documentType) const
{
	if(!presetId.isEmpty ())
		if(CustomizationComponent* c = CustomizationComponent::findCustomizationComponent (documentType))
			return c->getPresetByID (presetId);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentEventHandler* NewDocumentAssistant::createDocumentEventHandler () const
{
	if(auto t = templateProvider->getSelected ())
		if(t->getDocumentEventHandlerClassUID ().isValid ())
			if(IDocumentEventHandler* eventHandler = ccl_new<IDocumentEventHandler> (t->getDocumentEventHandlerClassUID ()))
			{
				AutoPtr<Attributes> data = NEW Attributes; // might be retained

				ObjectArray paths;
				paths.objectCleanup (true);
				fileList->getPaths (paths);
				if(!paths.isEmpty ())
					data->queue (kFilesID, paths, Attributes::kShare);

				// Collect template handler data
				if(UnknownPtr<IPersistAttributes> templateHandlerPa = currentTemplateHandler)
					templateHandlerPa->storeValues (*data);

				// Pass data to document event handler
				if(!data->isEmpty ())
					if(UnknownPtr<IPersistAttributes> eventHandlerPa = eventHandler)
						eventHandlerPa->restoreValues (*data);

				return eventHandler;
			}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::updateImportFile (const IUrl* path)
{
	// UI updates only required when transitioning from "no file" to "some file" and vice versa,
	// not when switching files. Hide DefaultPropertyView if import file available. Import file
	// may provide song settings (tempo, ...).
	bool refreshView = false;

	if(path == nullptr)
	{
		refreshView = dropImportFile != nullptr;
		dropImportFile.release ();
	}
	else
	{
		refreshView = dropImportFile == nullptr;
		dropImportFile = NEW Url (*path);
	}

	if(refreshView)
		updateDocumentPropertyView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentAssistant::isDropFile (UrlRef path) const
{
	// File type filters for exclusive and non-exclusive files may overlap.
	// Use document manager to detect exclusive 'document' files.
	bool document = DocumentManager::instance ().canOpenDocument (path);
	return dropFileTypes->matches (path) && !document;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NewDocumentAssistant::isExclusiveDropFile (UrlRef path) const
{
	// File type filters for exclusive and non-exclusive files may overlap.
	// Use document manager to detect exclusive 'document' files.
	bool document = DocumentManager::instance ().canOpenDocument (path);
	return exclusiveDropFileTypes->matches (path) && document;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateList* NewDocumentAssistant::loadSecondaryTemplates (UrlRef path)
{
	secondaryTemplateList->removeAll ();
	secondaryTemplateList->scanTemplates (path);
	secondaryTemplateList->updateDisplayList ();

	// copy options of main template to secondary templates if they do not have own options
	if(auto t = templateProvider->getSelected ())
		if(t->getOptions ().isEmpty () == false)
			secondaryTemplateList->initOptions (t->getOptions ());

	return secondaryTemplateList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::selectSecondaryTemplate (int index)
{
	const DocumentTemplate* t = index >= 0 ? secondaryTemplateList->getTemplate (index) : nullptr;
	templateProvider->setSecondaryTemplate (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::setDropFileTypes (const Container& fileTypes)
{
	dropFileTypes->setContent (fileTypes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::setExclusiveDropFileTypes (const Container& fileTypes)
{
	exclusiveDropFileTypes->setContent (fileTypes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::setConfirmEnabled (bool state)
{
	canConfirm = state;
	if(confirmButton)
		confirmButton->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::closeDialog (bool apply)
{
	if(currentDialog)
	{
		(*currentDialog)->setDialogResult (apply ? DialogResult::kOkay : DialogResult::kCancel);
		(*currentDialog)->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NewDocumentAssistant::setDropImportFile (UrlRef url, bool temporary)
{
	dropImportFile = NEW Url (url);
	temporaryDocument = temporary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (NewDocumentAssistant)
	DEFINE_METHOD_ARGR ("loadSecondaryTemplates", "path: Url", "DocumentTemplateList")
	DEFINE_METHOD_ARGR ("selectSecondaryTemplate", "index: int", "")
	DEFINE_METHOD_ARGR ("setDropFileTypes", "fileTypes: Container", "")
	DEFINE_METHOD_ARGR ("setExclusiveDropFileTypes", "fileTypes: Container", "")
	DEFINE_METHOD_ARGR ("getClassIcon", "className: string", "Object") // <-- TODO: find a better place...
	DEFINE_METHOD_ARGS ("setDropImportFile", "path: Url, temporaryDocument: bool = false")
	DEFINE_METHOD_ARGS ("setConfirmEnabled", "state: bool")
	DEFINE_METHOD_ARGS ("closeDialog", "apply: bool = false")	
END_METHOD_NAMES (NewDocumentAssistant)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewDocumentAssistant::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "loadSecondaryTemplates")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		if(path)
			returnValue.takeShared (loadSecondaryTemplates (*path)->asUnknown ());
		return true;
	}
	else if(msg == "selectSecondaryTemplate")
	{
		selectSecondaryTemplate (msg[0].asInt ());
		return true;
	}
	else if(msg == "setDropFileTypes")
	{
		if(AutoPtr<Container> c = ArrayBox::convert (msg[0]))
			setDropFileTypes (*c);
		return true;
	}
	else if(msg == "setExclusiveDropFileTypes")
	{
		if(AutoPtr<Container> c = ArrayBox::convert (msg[0]))
			setExclusiveDropFileTypes (*c);
		return true;
	}
	else if(msg == "getClassIcon")
	{
		String className (msg[0].asString ());
		if(!className.isEmpty ())
		{
			PlugInClass plugClass;
			plugClass.parseClassName (className);
			returnValue.takeShared (plugClass.getExactIcon ());
		}
		return true;
	}
	else if(msg == "setDropImportFile")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		bool temporary = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		if(path)
			setDropImportFile (*path, temporary);
		return true;
	}
	else if(msg == "setConfirmEnabled")
	{
		setConfirmEnabled (msg[0]);
		return true;
	}
	else if(msg == "closeDialog")
	{
		bool apply = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		closeDialog (apply);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// IDocumentTemplateHandler
//************************************************************************************************

DEFINE_IID_ (IDocumentTemplateHandler, 0xadda9d8b, 0x1a08, 0x40cc, 0x87, 0xcb, 0x4a, 0x6c, 0xda, 0x92, 0x34, 0x64)

//************************************************************************************************
// DocumentTemplateHandler
//************************************************************************************************

DEFINE_CLASS (DocumentTemplateHandler, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateHandler::canConfirm () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentTemplateHandler::onConfirm (tbool canceled)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentTemplateHandler::onDocumentComponentChanged ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentTemplateHandler::storeValues (IAttributeList& values) const
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentTemplateHandler::restoreValues (const IAttributeList& values)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentTemplateHandler::initialize (IUnknown* context)
{
	auto assistant = unknown_cast<NewDocumentAssistant> (context);
	ASSERT (assistant)
	if(!assistant)
		return kResultInvalidArgument;

	ObjectArray fileTypes;
	fileTypes.objectCleanup (true);
	getDropFileTypes (fileTypes);
	assistant->setDropFileTypes (fileTypes);

	ObjectArray exclusiveFileTypes;
	exclusiveFileTypes.objectCleanup (true);
	getExclusiveDropFileTypes (exclusiveFileTypes);
	assistant->setExclusiveDropFileTypes (exclusiveFileTypes);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API DocumentTemplateHandler::findChild (StringRef id) const
{
	if(id == "context")
		return UnknownPtr<IObjectNode> (getContext ());
	return SuperClass::findChild (id);
}

//************************************************************************************************
// DocumentEventHandler
//************************************************************************************************

DEFINE_CLASS (DocumentEventHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentEventHandler::storeValues (IAttributeList& values) const
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentEventHandler::restoreValues (const IAttributeList& values)
{
	filesToImport.removeAll ();
	AttributeReadAccessor reader (values);
	IterForEachUnknown (reader.newUnknownIterator (NewDocumentAssistant::kFilesID), unk)
		if(UnknownPtr<IAttribute> a = unk)
			if(UnknownPtr<IUrl> path = a->getValue ().asUnknown ())
				filesToImport.add (path, true);
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentEventHandler::onDocumentEvent (IDocument& document, int eventCode)
{}

//************************************************************************************************
// DocumentTemplateHandlerStub
//************************************************************************************************

class DocumentTemplateHandlerStub: public StubObject,
								   public IDocumentTemplateHandler
{
public:
	DECLARE_STUB_METHODS (IDocumentTemplateHandler, DocumentTemplateHandlerStub)

	// IDocumentTemplateHandler
	tbool CCL_API canConfirm () const override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("canConfirm"));
		return returnValue;
	}

	void CCL_API onConfirm (tbool canceled) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onConfirm", canceled));
	}

	void CCL_API onDocumentComponentChanged () override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onDocumentComponentChanged"));
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stub registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (DocumentTemplateHandlerStub, kFirstRun)
{
	REGISTER_STUB_CLASS (IDocumentTemplateHandler, DocumentTemplateHandlerStub)
	return true;
}
