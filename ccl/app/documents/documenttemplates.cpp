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
// Filename    : ccl/app/documents/documenttemplates.cpp
// Description : Document Templates
//
//************************************************************************************************

#include "ccl/app/documents/documenttemplates.h"
#include "ccl/app/documents/documentmetainfo.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentmanager.h"

#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/shellcommand.h"
#include "ccl/app/components/imageselector.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/filefilter.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// DocumentTemplateListModel
//************************************************************************************************

class DocumentTemplateListModel: public ItemModel,
                                 public ICommandHandler
{
public:
	DocumentTemplateListModel (DocumentTemplateProvider& provider);
	~DocumentTemplateListModel ();

	enum Columns
	{
		kLeftMarginColumn,
		kIconColumn,
		kMiddleMarginColumn,
		kTitleColumn
	};

	PROPERTY_VARIABLE (int, iconColumnWidth, IconColumnWidth)
	PROPERTY_VARIABLE (int, titleColumnWidth, TitleColumnWidth)
	PROPERTY_VARIABLE (int, columnMargin, ColumnMargin)

	void syncViewSelection ();
	void cancelSelectMessages ();

	// IItemModel
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	void CCL_API viewAttached (IItemView* itemView) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (ICommandHandler, ItemModel)

protected:
	DocumentTemplateProvider& provider;

	const DocumentTemplate* resolve (ItemIndexRef index) const;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (Templates, "Templates")
	XSTRING (AskReplaceExisiting, "%(1) already exists.\nDo you want to replace it?")
	XSTRING (SaveTemplateFailed, "The template could not be saved!")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum DocumentTemplateTags
	{
		kTemplateTitle = 100,
		kTemplateSubTitle,
		kTemplateDescription
	};

	enum DocumentTemplateSaveTags
	{
		kReplaceTemplate = 200
	};
}

//************************************************************************************************
// DocumentTemplate
//************************************************************************************************

DocumentTemplate* DocumentTemplate::loadTemplate (UrlRef path, StringRef packageId, bool markAsUserTemplate)
{
	AutoPtr<DocumentTemplate> t = NEW DocumentTemplate;
	t->setPath (path);
	t->setPackageID (packageId);

	if(markAsUserTemplate)
		t->setUser (true);
	if(!t->loadFromFile (path))
		return nullptr;
	if(t->isEmpty ())
		return nullptr;

	if(t->getTitle ().isEmpty ())
	{
		String title;
		path.getName (title, false);
		t->setTitle (title);
	}

	return return_shared<DocumentTemplate> (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DocumentTemplate, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplate::DocumentTemplate ()
: user (false),
  alwaysVisible (false),
  menuPriority (1000)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplate::setFileType (const FileType& fileType)
{
	path.setFileType (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& DocumentTemplate::getFileType () const
{
	ASSERT (path.getFileType ().isValid ())
	return path.getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentTemplate::isEmpty () const
{
	return dataPath.isEmpty () != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentTemplate::compare (const Object& obj) const
{
	const DocumentTemplate& other = (const DocumentTemplate&)obj;
	int prioDiff = menuPriority - other.getMenuPriority ();
	if(prioDiff)
		return prioDiff;

	return title.compareWithOptions (other.title, Text::kIgnoreCase|Text::kCompareNumerically);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentTemplate::load (const Storage& storage)
{
	PersistentAttributes attr;
	if(!attr.load (storage))
		return false;

	DocumentMetaInfo metaInfo (attr);
	
	String tableId = attr.getString (Meta::kTranslationTableID);
	if(tableId.isEmpty ())
		tableId = getPackageID ();
	ITranslationTable* stringTable = nullptr;
	if(!tableId.isEmpty ())
		stringTable = System::GetLocaleManager ().getStrings (MutableCString (tableId));
	
	setEnglishTitle (metaInfo.getTitle ());
	String title;
	if(stringTable)
		stringTable->getStringWithUnicodeKey (title, nullptr, getEnglishTitle ());
	if(title.isEmpty ())
		title = metaInfo.getLocalizedTitle ();
	if(title.isEmpty ())
		title = getEnglishTitle ();
	setTitle (title);

	String description;
	if(stringTable)
		stringTable->getStringWithUnicodeKey (description, nullptr, metaInfo.getDescription ());
	if(description.isEmpty ())
		description = metaInfo.getLocalizedDescription ();
	if(description.isEmpty ())
		description = metaInfo.getDescription ();
	setDescription (description);
	
	String subTitle;
	if(stringTable)
		stringTable->getStringWithUnicodeKey (subTitle, nullptr, attr.getString ("Document:SubTitle"));
	if(subTitle.isEmpty ())
		subTitle = attr.getString ("Document:LocalizedSubTitle");
	if(subTitle.isEmpty ())
		subTitle = attr.getString ("Document:SubTitle");
	setSubTitle (subTitle);

	setCategory (attr.getString ("Document:Category"));

	Url basePath (path);
	basePath.ascend ();
	ASSERT (!basePath.isEmpty ())

	String iconName (attr.getString ("Document:Icon"));
	if(!iconName.isEmpty ())
	{
		Url iconPath;
		iconPath.setName (iconName);
		iconPath.makeAbsolute (basePath);

		AutoPtr<IImage> icon = ImageFile::loadImage (iconPath);
		ASSERT (icon != nullptr)
		if(icon && !isUser ())
		{
			UnknownPtr<IObject> (icon)->setProperty (IImage::kIsAdaptive, true);
			//UnknownPtr<IObject> (icon)->setProperty (IImage::kIsTemplate, true);
		}
		setIcon (icon);
	}

	String dataName (attr.getString ("Document:Template"));
	Url dataPath;
	dataPath.setName (dataName);
	dataPath.makeAbsolute (basePath);
	ASSERT (!dataPath.isEmpty ())
	setDataPath (dataPath);

	customizationId = attr.getString ("Document:CustomizationID");
	templateHandlerClass.fromString (attr.getString ("Document:TemplateHandler"));
	documentEventHandlerClass.fromString (attr.getString ("Document:DocumentEventHandler"));
	additionalData = attr.getString ("Document:AdditionalData");
	tutorialId = attr.getString ("Document:TutorialID");
	options = attr.getString ("Document:Options");
	menuPriority = attr.getInt ("Document:MenuPriority");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentTemplate::save (const Storage& storage) const
{
	PersistentAttributes attr;

	DocumentMetaInfo metaInfo (attr);
	metaInfo.setTitle (title);
	metaInfo.setDescription (description);

	if(!subTitle.isEmpty ())
		attr.set ("Document:SubTitle", subTitle);

	// Note: category, excludedApps, includedApps, menuPriority aren't saved for user templates!

	if(icon)
	{
		ASSERT (!path.isEmpty ())
		Url iconPath (path);
		iconPath.setExtension ("png");
		bool result = ImageFile (ImageFile::kPNG, icon).saveToFile (iconPath);
		ASSERT (result == true)

		String iconName;
		iconPath.getName (iconName);
		attr.set ("Document:Icon", iconName);
	}

	ASSERT (!dataPath.isEmpty ())
	String dataName;
	dataPath.getName (dataName);
	attr.set ("Document:Template", dataName);

	if(!customizationId.isEmpty ())
		attr.set ("Document:CustomizationID", customizationId);
	if(templateHandlerClass.isValid ())
		attr.set ("Document:TemplateHandler", UIDString (templateHandlerClass));
	if(documentEventHandlerClass.isValid ())
		attr.set ("Document:DocumentEventHandler", UIDString (documentEventHandlerClass));
	if(!additionalData.isEmpty ())
		attr.set ("Document:AdditionalData", additionalData);
	if(!tutorialId.isEmpty ())
		attr.set ("Document:TutorialID", tutorialId);
	if(!options.isEmpty ())
		attr.set ("Document:Options", options);

	return attr.save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (DocumentTemplate)
	DEFINE_PROPERTY_NAME ("title")
	DEFINE_PROPERTY_NAME ("subTitle")
	DEFINE_PROPERTY_NAME ("description")
	DEFINE_PROPERTY_NAME ("additionalData")
END_PROPERTY_NAMES (DocumentTemplate)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplate::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	else if(propertyId == "subTitle")
	{
		var = getSubTitle ();
		return true;
	}
	else if(propertyId == "description")
	{
		var = getDescription ();
		return true;
	}
	else if(propertyId == "additionalData")
	{
		var = getAdditionalData ();
		return true;
	}
	else if(propertyId == "tutorialId")
	{
		var = getTutorialID ();
		return true;
	}
	else if(propertyId == "icon")
	{
		var.takeShared (const_cast<IImage*> (getIcon ()));
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DocumentTemplate::CategoryFilter
//************************************************************************************************

DocumentTemplate::CategoryFilter::CategoryFilter (StringRef category, DocumentTemplate* defaultTemplate, FilterMode mode)
: category (category),
  mode (mode)
{
	if(defaultTemplate)
		defaultTemplates.add (defaultTemplate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplate::CategoryFilter::CategoryFilter (StringRef category, const Container& _defaultTemplates, FilterMode mode)
: category (category),
  mode (mode)
{
	defaultTemplates.add (_defaultTemplates);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplate::CategoryFilter::addDefaultTemplate (DocumentTemplate* t)
{
	defaultTemplates.add (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& DocumentTemplate::CategoryFilter::getDefaultTemplates () const
{
	return defaultTemplates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplate::CategoryFilter::matches (IUnknown* object) const
{
	if(DocumentTemplate* t = unknown_cast<DocumentTemplate> (object))
	{
		if(t->isAlwaysVisible ())
			return true;

		switch(mode)
		{
		case kIncludeCategory:
			if(category.isEmpty () || t->getCategory () == category)
				return true;
			break;
		
		case kExcludeCategory:
			if(category.isEmpty () || t->getCategory () != category)
				return true;
			break;
			
		case kUserOnly:
			if(t->isUser ())
				return true;
			break;
		
		case kExcludeUser:
			if(!t->isUser ())
				return true;
			break;

		default:
			return true;
		}
	}
	
	return false;
}

//************************************************************************************************
// DocumentTemplateList
//************************************************************************************************

const String DocumentTemplateList::kTemplatesFolder ("templates");
String DocumentTemplateList::getTranslatedTitle () { return XSTR (Templates); }
void DocumentTemplateList::getDefaultUserLocation (IUrl& path)
{
	System::GetSystem ().getLocation (path, System::kUserContentFolder);
	path.descend (kTemplatesFolder, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray DocumentTemplateList::additionalLocations;
void DocumentTemplateList::addAdditionalLocation (UrlRef path)
{
	additionalLocations.objectCleanup (true);
	additionalLocations.add (NEW Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DocumentTemplateList, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateList::DocumentTemplateList ()
: scanningUserTemplates (false)
{
	allTemplates.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::addFileType (const FileType& fileType)
{
	fileTypes.add (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentTemplateList::getTemplateCount () const
{
	return displayList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* DocumentTemplateList::getTemplate (int index) const
{
	return (DocumentTemplate*)displayList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentTemplateList::getTemplateIndex (const DocumentTemplate* t) const
{
	return displayList.index (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::addTemplate (DocumentTemplate* t)
{
	if(t->getIcon () == nullptr)
	{
		// search for explicit template icon
		MutableCString iconName ("TemplateIcon:");
		iconName.append (t->getFileType ().getExtension ());
		t->setIcon (RootComponent::instance ().getTheme ()->getImage (iconName));

		if(t->getIcon () == nullptr) // fall back to file type icon
		{
			AutoPtr<IImage> icon = FileIcons::instance ().createIcon (t->getFileType ());
			t->setIcon (icon);
		}
	}

	allTemplates.addSorted (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::removeAll ()
{
	displayList.removeAll ();
	allTemplates.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::removeUserTemplates ()
{
	bool changed = false;
	ForEachReverse (allTemplates, DocumentTemplate, t)
		if(t->isUser ())
		{
			allTemplates.remove (t);
			t->release ();
			changed = true;
		}
	EndFor	
	
	if(changed)
		updateDisplayList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::scanAppFactoryTemplates ()
{
	Url path;
	System::GetSystem ().getLocation (path, System::kAppDeploymentFolder); // can differ in debug builds
	path.descend (kTemplatesFolder, Url::kFolder);
	scanTemplates (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::scanAdditionalLocations ()
{
	for(auto path : iterate_as<Url> (additionalLocations))
		scanTemplates (*path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::scanUserTemplates (StringRef folderName)
{
	ScopedVar<bool> scope (scanningUserTemplates, true);
	Url path;
	System::GetSystem ().getLocation (path, System::kUserContentFolder);
	path.descend (folderName, Url::kFolder);
	scanTemplates (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::scanUserTemplates (const StringList& folderNames)
{
	for(auto folderName : folderNames)
		scanUserTemplates (*folderName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::scanTemplates (UrlRef path)
{
	ASSERT (!fileTypes.isEmpty ())

	String packageId = UrlUtils::extractPackageID (path);

	FileFilter filter (path);
	ForEachFile (System::GetFileSystem ().newIterator (path), p)
		if(fileTypes.contains (p->getFileType ()))
		{
			if(!filter.matches (*p))
				continue;

			if(DocumentTemplate* t = DocumentTemplate::loadTemplate (*p, packageId, scanningUserTemplates))
				addTemplate (t);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::updateDisplayList ()
{
	displayList.removeAll ();
	ForEach (allTemplates, DocumentTemplate, t)
		if(displayFilter && !displayFilter->matches (t->asUnknown ()))
			continue;
		displayList.add (t);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateList::initOptions (StringRef options)
{
	ForEach (allTemplates, DocumentTemplate, t)
		if(t->getOptions ().isEmpty ())
			t->setOptions (options);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (DocumentTemplateList)
	DEFINE_METHOD_ARGR ("getTemplateCount", "", "int")
	DEFINE_METHOD_ARGR ("getTemplate", "index: int", "DocumentTemplate")
END_METHOD_NAMES (DocumentTemplateList)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateList::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getTemplateCount")
	{
		returnValue = getTemplateCount ();
		return true;
	}
	else if(msg == "getTemplate")
	{
		if(auto t = getTemplate (msg[0]))
			returnValue.takeShared (ccl_as_unknown (t));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// DocumentTemplateProperties
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentTemplateProperties, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateProperties::DocumentTemplateProperties (StringRef name)
: Component (name),
  imageSelector (nullptr)
{
	paramList.addString (CSTR ("title"), Tag::kTemplateTitle);
	paramList.addString (CSTR ("subTitle"), Tag::kTemplateSubTitle);
	paramList.addString (CSTR ("description"), Tag::kTemplateDescription);

	addComponent (imageSelector = NEW ImageSelector ("icon"));
	imageSelector->setMaxImageSize (Point (512, 512));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateProperties::~DocumentTemplateProperties ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProperties::setProperties (const DocumentTemplate& t)
{
	paramList.byTag (Tag::kTemplateTitle)->fromString (t.getTitle ());
	paramList.byTag (Tag::kTemplateSubTitle)->fromString (t.getSubTitle ());
	paramList.byTag (Tag::kTemplateDescription)->fromString (t.getDescription ());
	imageSelector->setImage (t.getIcon ());

	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProperties::getProperties (DocumentTemplate& t) const
{
	String title;
	paramList.byTag (Tag::kTemplateTitle)->toString (title);

	String subTitle;
	paramList.byTag (Tag::kTemplateSubTitle)->toString (subTitle);

	String description;
	paramList.byTag (Tag::kTemplateDescription)->toString (description);

	SharedPtr<IImage> icon = imageSelector->getImage ();

	t.setTitle (title);
	t.setSubTitle (subTitle);
	t.setDescription (description);
	t.setIcon (icon);
}

//************************************************************************************************
// DocumentTemplateProvider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentTemplateProvider, Component)
DEFINE_STRINGID_MEMBER_ (DocumentTemplateProvider, kOpenSelected, "openSelected")
DEFINE_STRINGID_MEMBER_ (DocumentTemplateProvider, kSecondaryChanged, "secondaryChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateProvider::DocumentTemplateProvider (DocumentTemplateList* templateList)
: DocumentTemplateProperties (CCLSTR ("Templates")),
  templateList (templateList),
  listModel (nullptr),
  selected (nullptr),
  secondary (nullptr),
  filterParam (nullptr)
{
	listModel = NEW DocumentTemplateListModel (*this);

	filterParam = paramList.addList ("filterList");
	filterParam->setStorable (true);

	// select first template, etc.
	filterChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateProvider::~DocumentTemplateProvider ()
{
	listModel->cancelSelectMessages (); // kill a pending "select" message, listModel is not yet deleted by the release below (and has a reference to us)
	listModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::setIconColumnWidth (int width)
{
	listModel->setIconColumnWidth (width);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::setTitleColumnWidth (int width)
{
	listModel->setTitleColumnWidth (width);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::setColumnMargin (int margin)
{
	listModel->setColumnMargin (margin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::filterChanged ()
{
	const DocumentTemplate* first = nullptr;

	int index = filterParam->getValue ().asInt ();
	IObjectFilter* filter = index < filterList.count () ? filterList.at (index) : nullptr;
	if(templateList)
	{
		templateList->setDisplayFilter (filter);
		templateList->updateDisplayList ();
		first = templateList->getTemplate (0);
	}

	select (first);

	listModel->signal (Message (kChanged));
	listModel->syncViewSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::addDisplayFilter (IObjectFilter* filter, StringRef title)
{
	filterList.add (filter);
	UnknownPtr<IListParameter> (filterParam)->appendString (title);

	if(filterList.count () == 1) // initial update
		filterChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::storeSettings (StringRef settingsID) const
{
	paramList.storeSettings (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::restoreSettings (StringRef settingsID)
{
	paramList.restoreSettings (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplateList* DocumentTemplateProvider::getTemplateList () const
{
	return templateList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::select (const DocumentTemplate* t)
{
	if(t == selected)
		return;

	selected = t;

	if(selected)
		setProperties (*selected);
	else
		setProperties (DocumentTemplate ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateProvider::setSecondaryTemplate (const DocumentTemplate* t)
{
	if(secondary != t)
	{
		secondary = t;
		signal (Message (kSecondaryChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* DocumentTemplateProvider::getSecondaryTemplate () const
{
	return secondary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* DocumentTemplateProvider::getSelected () const
{
	return selected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateProvider::paramChanged (IParameter* param)
{
	if(param == filterParam)
	{
		filterChanged ();
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API DocumentTemplateProvider::getObject (StringID name, UIDRef classID)
{
	if(name == "templates")
		return listModel->asUnknown ();

	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateProvider::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isTemplateSelected")
	{
		var = selected && !selected->isEmpty ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DocumentTemplateListModel
//************************************************************************************************

DocumentTemplateListModel::DocumentTemplateListModel (DocumentTemplateProvider& provider)
: provider (provider),
  iconColumnWidth (42),
  titleColumnWidth (100),
  columnMargin (3)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateListModel::~DocumentTemplateListModel ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DocumentTemplate* DocumentTemplateListModel::resolve (ItemIndexRef index) const
{
	if(const DocumentTemplateList* templateList = provider.getTemplateList ())
		return templateList->getTemplate (index.getIndex ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (columnMargin);		// kLeftMarginColumn
	list.addColumn (iconColumnWidth);  	// kIconColumn
	list.addColumn (columnMargin);		// kMiddleMarginColumn
	list.addColumn (titleColumnWidth); 	// kTitleColumn
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DocumentTemplateListModel::countFlatItems ()
{
	if(const DocumentTemplateList* templateList = provider.getTemplateList ())
		return templateList->getTemplateCount ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::getItemTitle (String& title, ItemIndexRef index)
{
	const DocumentTemplate* t = resolve (index);
	if(t == nullptr)
		return false;

	title = t->getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API DocumentTemplateListModel::getItemIcon (ItemIndexRef index)
{
	const DocumentTemplate* t = resolve (index);
	if(t == nullptr)
		return nullptr;

	return t->getIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::getItemTooltip (String& tooltip, ItemIndexRef index, int column)
{
	tooltip.empty ();
	if(const DocumentTemplate* t = resolve (index))
		if(column == 3 && !t->getSubTitle ().isEmpty ())
		{
			// show tooltip only if the sub title is truncated
			if(IItemView* itemView = getItemView ())
			{
				Rect itemRect;
				itemView->getItemRect (itemRect, index, column);
				Font font = ViewBox (itemView).getVisualStyle ().getTextFont ();
				Coord width = Font::getStringWidth (t->getSubTitle (), font);
				if(width > itemRect.getWidth ())
				{
					tooltip = t->getSubTitle ();
					return true;
				}
			}
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::onItemFocused (ItemIndexRef index)
{
	const DocumentTemplate* t = resolve (index);
	if(t)
		provider.select (t);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentTemplateListModel::viewAttached (IItemView* itemView)
{
	ItemModel::viewAttached (itemView);

	syncViewSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateListModel::syncViewSelection ()
{
	if(IItemView* itemView = getItemView ())
		if(const DocumentTemplate* selected = provider.getSelected ())
			if(const DocumentTemplateList* templateList = provider.getTemplateList ())
				(NEW Message ("select", itemView, templateList->getTemplateIndex (selected)))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateListModel::cancelSelectMessages ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentTemplateListModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "select")
	{
		UnknownPtr<IItemView> itemView (msg[0]);
		ASSERT (itemView)
		int index = msg[1].asInt ();
		itemView->setFocusItem (index);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	const DocumentTemplate* t = resolve (index);
	if(t == nullptr)
		return false;

	const IVisualStyle& vs = info.view->getVisualStyle ();

	switch(column)
	{
	case kIconColumn :
		{		
			drawIcon (info, t->getIcon (), true, true, columnMargin);
		}
		break;
		
	case kTitleColumn :
		StringRef subTitle = t->getSubTitle ().isEmpty () ? t->getDescription () : t->getSubTitle ();
		if(subTitle.isEmpty () == false)
			drawTitleWithSubtitle (info, t->getTitle (), subTitle, true, 0, 0);
		else
			drawTitle (info, t->getTitle (), true, vs.getMetric<bool> ("noBold", false) ? 0 : Font::kBold);
		break;
	}
	
	// draw bottom separator
	Color separatorColor (vs.getColor ("separatorcolor", Colors::kTransparentBlack));
	if(separatorColor.getAlphaF () != 0)
	{
		Coord y = info.rect.bottom - 1;
		info.graphics.drawLine (Point (info.rect.left, y), Point (info.rect.right, y), Pen (separatorColor));
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	const DocumentTemplate* t = resolve (item);
	if(t)
	{
		menu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), CSTR ("File"), CSTR ("Show in Explorer/Finder"), this);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::checkCommandCategory (CStringRef category) const
{
	return category == "File";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::interpretCommand (const CommandMsg& msg)
{
	if(IItemView* itemView = getItemView ())
	{
		ItemIndex index;
		if(itemView->getFocusItem (index))
		{
			if(const DocumentTemplate* t = resolve (index))
			{
				if(msg.category == "File" && msg.name == "Show in Explorer/Finder")
				{
					if(!t->getPath ().isEmpty ())
						return ShellCommand::showFileInSystem (t->getPath (), msg.checkOnly ());
				}
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateListModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	const DocumentTemplate* t = resolve (index);
	provider.select (t);
	provider.deferSignal (NEW Message (DocumentTemplateProvider::kOpenSelected));
	return true;
}

//************************************************************************************************
// DocumentTemplateSaveDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentTemplateSaveDialog, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////


DocumentTemplateSaveDialog::DocumentTemplateSaveDialog (Document& document, StringRef folderName, const FileType& fileType)
: DocumentTemplateProperties (CCLSTR ("TemplateSaver")),
  formName (CSTR ("CCL/DocumentTemplateSaveDialog")),
  document (document),
  fileType (fileType)
{
	System::GetSystem ().getLocation (location, System::kUserContentFolder);
	location.descend (folderName, Url::kFolder);

	paramList.byTag (Tag::kTemplateTitle)->fromString (document.getTitle ());

	paramList.addParam (CSTR ("replace"), Tag::kReplaceTemplate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentTemplateSaveDialog::run ()
{
	Attributes arguments;
	arguments.set ("fileType", fileType.getExtension ());

	ITheme* theme = getTheme ();
	AutoPtr<IView> view = theme ? theme->createView (formName, this->asUnknown (), &arguments) : nullptr;
	ASSERT (view != nullptr)
	if(view == nullptr)
		return false;

	Url path;
	while(1)
	{
		int dialogResult = DialogBox ()->runDialog (return_shared<IView> (view));
		if(dialogResult != DialogResult::kOkay)
			return false;

		getTemplatePath (path);
		if(pathToReplace == nullptr && System::GetFileSystem ().fileExists (path))
		{
			String fileName;
			path.getName (fileName);
			int alertResult = Alert::ask (String ().appendFormat (XSTR (AskReplaceExisiting), fileName), Alert::kYesNo);
			if(alertResult == Alert::kNo)
				continue;
		}

		break;
	}

	AutoPtr<DocumentTemplate> t = createTemplate (path);
	if(t == nullptr)
		Alert::error (XSTR (SaveTemplateFailed));
	return t != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentTemplateSaveDialog::runAsync ()
{
	Attributes arguments;
	arguments.set ("fileType", fileType.getExtension ());

	ITheme* theme = getTheme ();
	AutoPtr<IView> view = theme ? theme->createView (formName, this->asUnknown (), &arguments) : nullptr;
	ASSERT (view != nullptr)
	if(view == nullptr)
		return false;

	retain ();
	Promise p (DialogBox ()->runDialogAsync (return_shared<IView> (view)));
	p.then (this, &DocumentTemplateSaveDialog::onAsyncDialogResult);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateSaveDialog::onAsyncDialogResult (IAsyncOperation& op)
{
	AutoPtr<Object> thisCleanup (this); // release this afterwards

	if(op.getResult ().asInt () == DialogResult::kOkay)
	{
		Url path;
		getTemplatePath (path);
		AutoPtr<DocumentTemplate> t = createTemplate (path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentTemplateSaveDialog::getTemplatePath (Url& path) const
{
	if(pathToReplace)
		path = *pathToReplace;
	else
	{
		path = location;

		String fileName;
		paramList.byTag (Tag::kTemplateTitle)->toString (fileName);
		fileName = LegalFileName (fileName);
		fileName.trimWhitespace ();

		path.descend (fileName);
		path.setExtension (fileType.getExtension (), false); // name may contain a dot
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplate* DocumentTemplateSaveDialog::createTemplate (UrlRef path)
{
	AutoPtr<DocumentTemplate> t = NEW DocumentTemplate;
	getProperties (*t);
	if(t->getTitle ().isEmpty ())
		return nullptr;

	t->setPath (path);

	Url dataPath (path);
	dataPath.setExtension ("data");
	t->setDataPath (dataPath);

	// create data file
	Url oldPath (document.getPath ());
	bool dirty = document.isDirty () != 0;
	DocumentManager::instance ().signalDocumentEvent (document, Document::kBeforeAutoSave);
	bool saved = document.saveAs (dataPath);
	DocumentManager::instance ().signalDocumentEvent (document, Document::kAutoSaveFinished);
	document.setPath (oldPath);
	document.setDirty (dirty);
	if(saved == false)
		return nullptr;

	// create template
	saved = t->saveToFile (path);
	if(saved == false)
		return nullptr;

	IUrl* url = const_cast<IUrl*> (&path);
	SignalSource (Signals::kFileSystem).signal (Message (Signals::kFileCreated, url));

	return return_shared<DocumentTemplate> (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateSaveDialog::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kReplaceTemplate)
	{
		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
		fs->addFilter (fileType);
		fs->setFolder (location);
		if(fs->run (IFileSelector::kOpenFile))
		{
			UrlRef path = *fs->getPath ();
			AutoPtr<DocumentTemplate> t = DocumentTemplate::loadTemplate (path);
			if(t)
			{
				// check if we're about to overwrite an existing template with an older document format
				PackageInfo packageInfo;
				if(packageInfo.loadFromPackage (t->getDataPath ()))
				{
					DocumentClass* documentClass = document.getDocumentClass ();
					ASSERT (documentClass)
					int currentFormatVersion = documentClass ? documentClass->getFormatVersion () : 0;
					int existingFormatVersion = DocumentMetaInfo (packageInfo).getFormatVersion ();
					if(currentFormatVersion > existingFormatVersion)
					{
						// show warning, user must confirm
						int result = Alert::ask (DocumentStrings::OldDocumentFormatWarning (), Alert::kYesNo);
						if(result != Alert::kYes)
							return true;
					}
				}

				setProperties (*t);
				pathToReplace = NEW Url (path);
			}
		}
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}
