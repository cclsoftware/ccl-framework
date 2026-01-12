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
// Filename    : ccl/app/documents/documentmetainfo.h
// Description : Document Meta Information
//
//************************************************************************************************

#ifndef _ccl_documentmetainfo_h
#define _ccl_documentmetainfo_h

#include "ccl/public/storage/metainfo.h"

#include "ccl/public/app/idocumentmetainfo.h"

namespace CCL {

//************************************************************************************************
// DocumentMetaInfo
/** Meta Information saved with documents. */
//************************************************************************************************

class DocumentMetaInfo: public AttributeAccessor
{
public:
	DocumentMetaInfo (IAttributeList& attributes) : AttributeAccessor (attributes) {}

	// Creation Information
	METAINFO_ATTRIBUTE_STRING (MimeType, Meta::kDocumentMimeType)
	METAINFO_ATTRIBUTE_STRING (Generator, Meta::kDocumentGenerator)
	METAINFO_ATTRIBUTE_STRING (Creator, Meta::kDocumentCreator)
	METAINFO_ATTRIBUTE_STRING (Title, Meta::kDocumentTitle)
	METAINFO_ATTRIBUTE_INT (FormatVersion, Meta::kDocumentFormatVersion)

	void resetCreationInfo ()
	{
		setMimeType (nullptr);
		setGenerator (nullptr);
		setCreator (nullptr);
		setTitle (nullptr);
		setFormatVersion (0);
	}

	// Common Attributes
	METAINFO_ATTRIBUTE_STRING (Description, Meta::kDocumentDescription)
	METAINFO_ATTRIBUTE_STRING (Keywords, Meta::kDocumentKeywords)

	// Localized Attributes
	METAINFO_ATTRIBUTE_STRING (LocalizedTitle, Meta::kDocumentLocalizedTitle)
	METAINFO_ATTRIBUTE_STRING (LocalizedDescription, Meta::kDocumentLocalizedDescription)
};

} // namespace CCL

#endif // _ccl_documentmetainfo_h
