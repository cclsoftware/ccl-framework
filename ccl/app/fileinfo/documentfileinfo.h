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
// Filename    : ccl/app/fileinfo/documentfileinfo.h
// Description : Document File Information
//
//************************************************************************************************

#ifndef _ccl_documentfileinfo_h
#define _ccl_documentfileinfo_h

#include "ccl/app/fileinfo/fileinfocomponent.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// DocumentFileInfo
//************************************************************************************************

class DocumentFileInfo: public StandardFileInfo
{
public:
	DECLARE_CLASS (DocumentFileInfo, StandardFileInfo)

	DocumentFileInfo (StringRef name = CCLSTR ("DocumentFileInfo"), StringID viewName = "CCL/DocumentFileInfo");

	DECLARE_STRINGID_MEMBER (kDirty)
	PROPERTY_BOOL (dirty, Dirty)
	PROPERTY_BOOL (checkDocumentClass, CheckDocumentClass)

	virtual void setAttributes (IAttributeList& attributes);
	virtual void getAttributes (IAttributeList& attributes) const;

	// StandardFileInfo
	tbool CCL_API setFile (UrlRef path) override;
	tbool CCL_API paramChanged (IParameter* param) override;
};

//************************************************************************************************
// DocumentTemplateFileInfo
//************************************************************************************************

class DocumentTemplateFileInfo: public StandardFileInfo
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentTemplateFileInfo, StandardFileInfo)

	DocumentTemplateFileInfo (StringRef name, StringID viewName, const FileType& templateType = FileType (), DocumentFileInfo* docFileInfo = nullptr); // takes ownership of docFileInfo 

	// StandardFileInfo
	tbool CCL_API setFile (UrlRef path) override;
	IObjectNode* CCL_API findChild (StringRef id) const override;

protected:
	FileType templateType;
	DocumentFileInfo* docFileInfo;
	IImageProvider* templateIcon;
};

} // namespace CCL

#endif // _ccl_documentfileinfo_h
