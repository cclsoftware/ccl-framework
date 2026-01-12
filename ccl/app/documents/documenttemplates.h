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
// Filename    : ccl/app/documents/documenttemplates.h
// Description : Document Templates
//
//************************************************************************************************

#ifndef _ccl_documenttemplates_h
#define _ccl_documenttemplates_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class StringList;
class Document;
class DocumentTemplateListModel;
class ImageSelector;
interface IAsyncOperation;

//************************************************************************************************
// DocumentTemplate
//************************************************************************************************

class DocumentTemplate: public StorableObject
{
public:
	DECLARE_CLASS (DocumentTemplate, StorableObject)
	DECLARE_PROPERTY_NAMES (DocumentTemplate)

	DocumentTemplate ();

	class CategoryFilter;

	static DocumentTemplate* loadTemplate (UrlRef path, StringRef packageID = nullptr, bool markAsUserTemplate = false);

	PROPERTY_OBJECT (Url, path, Path)
	void setFileType (const FileType& fileType);
	const FileType& getFileType () const;
	PROPERTY_BOOL (user, User)
	PROPERTY_BOOL (alwaysVisible, AlwaysVisible)

	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (subTitle, SubTitle)
	PROPERTY_STRING (englishTitle, EnglishTitle)
	PROPERTY_STRING (description, Description)
	PROPERTY_STRING (category, Category)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_OBJECT (Url, dataPath, DataPath)
	bool isEmpty () const;

	PROPERTY_STRING (customizationId, CustomizationID) ///< customization preset id (optional)
	PROPERTY_OBJECT (UID, templateHandlerClass, TemplateHandlerClassUID) ///< template handler class uid (optional)
	PROPERTY_OBJECT (UID, documentEventHandlerClass, DocumentEventHandlerClassUID)	///< document event handler class uid (optional)
	PROPERTY_STRING (additionalData, AdditionalData) ///< additional data (optional)
	PROPERTY_STRING (tutorialId, TutorialID) ///< associated tutorial (optional, not automatically loaded)
	PROPERTY_STRING (options, Options) ///< options (optional, application specific)
	PROPERTY_STRING (packageId, PackageID) ///< package this template is provided by (optional)
	
	PROPERTY_VARIABLE (int, menuPriority, MenuPriority)	///< menu priority, lower values first

	// StorableObject
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// DocumentTemplate::CategoryFilter
//************************************************************************************************

class DocumentTemplate::CategoryFilter: public Object,
										public IObjectFilter
{
public:
	enum FilterMode
	{
		kIncludeCategory = 0,
		kExcludeCategory,
		kUserOnly,
		kExcludeUser
	};

	CategoryFilter (StringRef category = nullptr, DocumentTemplate* defaultTemplate = nullptr, FilterMode mode = kIncludeCategory);
	CategoryFilter (StringRef category, const Container& defaultTemplates, FilterMode mode = kIncludeCategory);

	PROPERTY_STRING (category, Category)
	void addDefaultTemplate (DocumentTemplate* t);
	const Container& getDefaultTemplates () const;

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, Object)

protected:
	ObjectArray defaultTemplates;
	FilterMode mode;
};

//************************************************************************************************
// DocumentTemplateList
//************************************************************************************************

class DocumentTemplateList: public Object
{
public:
	DECLARE_CLASS (DocumentTemplateList, Object)
	DECLARE_METHOD_NAMES (DocumentTemplateList)

	DocumentTemplateList ();

	static const String kTemplatesFolder;
	static String getTranslatedTitle ();
	static void getDefaultUserLocation (IUrl& path);
	static void addAdditionalLocation (UrlRef path);

	void addFileType (const FileType& fileType);
	PROPERTY_SHARED_AUTO (IObjectFilter, displayFilter, DisplayFilter)

	int getTemplateCount () const;
	const DocumentTemplate* getTemplate (int index) const;
	int getTemplateIndex (const DocumentTemplate* t) const;

	void addTemplate (DocumentTemplate* t);
	void removeAll ();
	void removeUserTemplates ();

	void scanAppFactoryTemplates ();
	void scanAdditionalLocations ();
	void scanUserTemplates (StringRef folderName);
	void scanUserTemplates (const StringList& folderNames);
	void scanTemplates (UrlRef path);

	void updateDisplayList ();
	void initOptions (StringRef options);

protected:
	static ObjectArray additionalLocations;

	Vector<FileType> fileTypes;
	ObjectArray allTemplates;
	ObjectArray displayList;
	bool scanningUserTemplates;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// DocumentTemplateProperties
//************************************************************************************************

class DocumentTemplateProperties: public Component
{
public:
	DECLARE_CLASS (DocumentTemplateProperties, Component)

	DocumentTemplateProperties (StringRef name = nullptr);
	~DocumentTemplateProperties ();

protected:
	ImageSelector* imageSelector;

	void setProperties (const DocumentTemplate& t);
	void getProperties (DocumentTemplate& t) const;
};

//************************************************************************************************
// DocumentTemplateProvider
//************************************************************************************************

class DocumentTemplateProvider: public DocumentTemplateProperties
{
public:
	DECLARE_CLASS (DocumentTemplateProvider, DocumentTemplateProperties)

	DocumentTemplateProvider (DocumentTemplateList* templateList = nullptr);
	~DocumentTemplateProvider ();

	void setIconColumnWidth (int width);
	void setTitleColumnWidth (int width);
	void setColumnMargin (int margin);

	const DocumentTemplateList* getTemplateList () const;

	void select (const DocumentTemplate* t);
	const DocumentTemplate* getSelected () const;

	void setSecondaryTemplate (const DocumentTemplate* t);
	const DocumentTemplate* getSecondaryTemplate () const;

	void addDisplayFilter (IObjectFilter* filter, StringRef title);
	void filterChanged ();

	void storeSettings (StringRef settingsID) const;
	void restoreSettings (StringRef settingsID);

	DECLARE_STRINGID_MEMBER (kOpenSelected)
	DECLARE_STRINGID_MEMBER (kSecondaryChanged)

	// DocumentTemplateProperties
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	SharedPtr<DocumentTemplateList> templateList;
	Vector< SharedPtr<IObjectFilter> > filterList;
	IParameter* filterParam;
	DocumentTemplateListModel* listModel;
	const DocumentTemplate* selected;
	const DocumentTemplate* secondary;
};

//************************************************************************************************
// DocumentTemplateSaveDialog
//************************************************************************************************

class DocumentTemplateSaveDialog: public DocumentTemplateProperties
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentTemplateSaveDialog, DocumentTemplateProperties)

	DocumentTemplateSaveDialog (Document& document, StringRef folderName, const FileType& fileType);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_OBJECT (Url, location, Location)
	PROPERTY_OBJECT (FileType, fileType, FileType)

	bool run ();
	bool runAsync ();

	// DocumentTemplateProperties
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	Document& document;
	AutoPtr<Url> pathToReplace;

	void getTemplatePath (Url& path) const;
	DocumentTemplate* createTemplate (UrlRef path);
	
	void onAsyncDialogResult (IAsyncOperation&);
};

} // namespace CCL

#endif // _ccl_documenttemplates_h
