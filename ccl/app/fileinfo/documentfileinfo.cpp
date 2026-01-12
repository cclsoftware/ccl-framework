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
// Filename    : ccl/app/fileinfo/documentfileinfo.cpp
// Description : Document File Information
//
//************************************************************************************************

#include "ccl/app/fileinfo/documentfileinfo.h"

#include "ccl/app/documents/documentmetainfo.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/documenttemplates.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum DocumentInfoTags
	{
		kGenerator   = 'Gene',
		kCreator     = 'Crea',
		kDocument    = 'Docu',
		kDescription = 'Desc',
		kKeywords    = 'Keyw'
	};
	enum DocumentTemplateInfoTags
	{
		kTemplateIcon = 'TIco',
		kTitle        = 'Titl',
		kCategory     = 'Cate'
	};
}

//************************************************************************************************
// DocumentFileInfo
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (DocumentFileInfo, kDirty, "dirty")
DEFINE_CLASS_HIDDEN (DocumentFileInfo, StandardFileInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentFileInfo::DocumentFileInfo (StringRef name, StringID viewName)
: StandardFileInfo (name, viewName),
  dirty (false),
  checkDocumentClass (true)
{
	paramList.addString (CSTR ("generator"), Tag::kGenerator);
	paramList.addString (CSTR ("creator"), Tag::kCreator);
	paramList.addString (CSTR ("document"), Tag::kDocument);
	paramList.addString (CSTR ("description"), Tag::kDescription);
	paramList.addString (CSTR ("keywords"), Tag::kKeywords);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentFileInfo::setFile (UrlRef path)
{
	if(!isLocal (path))
		return false;

	if(checkDocumentClass)  // ignore other package files
		if(!DocumentManager::instance ().findDocumentClass (path.getFileType ()))
			return false;

	PackageInfo info;
	if(!info.loadFromPackage (path))
		return false;

	SuperClass::setFile (path);
	setAttributes (info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentFileInfo::setAttributes (IAttributeList& attributes)
{
	DocumentMetaInfo info (attributes);
	paramList.byTag (Tag::kGenerator)->fromString (info.getGenerator ());
	paramList.byTag (Tag::kCreator)->fromString (info.getCreator ());
	paramList.byTag (Tag::kDocument)->fromString (info.getTitle ());
	paramList.byTag (Tag::kDescription)->fromString (info.getDescription ());
	paramList.byTag (Tag::kKeywords)->fromString (info.getKeywords ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentFileInfo::getAttributes (IAttributeList& attributes) const
{
	DocumentMetaInfo info (attributes);
	info.setGenerator (paramList.byTag (Tag::kGenerator)->getValue ());
	info.setCreator (paramList.byTag (Tag::kCreator)->getValue ());
	info.setTitle (paramList.byTag (Tag::kDocument)->getValue ());
	info.setDescription(paramList.byTag (Tag::kDescription)->getValue ());
	info.setKeywords (paramList.byTag (Tag::kKeywords)->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentFileInfo::paramChanged (IParameter* param)
{
	if(!isDirty ())
	{
		setDirty (true);
		signal (Message (kDirty));
	}
	return SuperClass::paramChanged (param);
}

//************************************************************************************************
// DocumentTemplateFileInfo
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentTemplateFileInfo, StandardFileInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplateFileInfo::DocumentTemplateFileInfo (StringRef name, StringID viewName, const FileType& templateType, DocumentFileInfo* docFileInfo)
: StandardFileInfo (name, viewName),
  templateType (templateType),
  docFileInfo (docFileInfo),
  templateIcon (nullptr)
{
	paramList.addString (CSTR ("title"), Tag::kTitle);
	paramList.addString (CSTR ("category"), Tag::kCategory);
	paramList.addString (CSTR ("description"), Tag::kDescription);

	templateIcon = paramList.addImage (CSTR ("templateIcon"), Tag::kTemplateIcon);

	if(docFileInfo)
		addComponent (docFileInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API DocumentTemplateFileInfo::findChild (StringRef id) const
{
	if(id == "DocumentInfo" && docFileInfo)
		return UnknownPtr<IObjectNode> (docFileInfo->asUnknown ());

	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentTemplateFileInfo::setFile (UrlRef path)
{
	if(templateType.isValid () && path.getFileType () != templateType)
		return false;

	AutoPtr<DocumentTemplate> docTemplate = DocumentTemplate::loadTemplate (path);
	if(docTemplate)
	{
		SuperClass::setFile (path);

		paramList.byTag (Tag::kTitle)->fromString (docTemplate->getTitle ());
		paramList.byTag (Tag::kCategory)->fromString (docTemplate->getCategory ());
		paramList.byTag (Tag::kDescription)->fromString (docTemplate->getDescription ());

		if(docTemplate->getIcon () == nullptr)
			templateIcon->setImage (fileIcon->getImage ());
		else
			templateIcon->setImage (docTemplate->getIcon ());

		if(docFileInfo)
		{
			docFileInfo->setCheckDocumentClass (false);
			docFileInfo->setFile (docTemplate->getDataPath ());
		}

		return true;
	}

	return false;
}

