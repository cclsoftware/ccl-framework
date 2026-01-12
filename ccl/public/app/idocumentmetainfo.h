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
// Filename    : ccl/public/app/idocumentmetainfo.h
// Description : Document Meta Information
//
//************************************************************************************************

#ifndef _ccl_idocumentmetainfo_h
#define _ccl_idocumentmetainfo_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Document Meta Information
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	/** Document MIME type [String] (e.g. "application/x-document-type")*/
	DEFINE_STRINGID (kDocumentMimeType, "Document:MimeType")

	/** Name of generating application [String] (e.g. "My Application")*/
	DEFINE_STRINGID (kDocumentGenerator, "Document:Generator")

	/** Document format version [int] */
	DEFINE_STRINGID (kDocumentFormatVersion, "Document:FormatVersion")

	/** Name of document creator [String] (e.g. "John Doe")*/
	DEFINE_STRINGID (kDocumentCreator, "Document:Creator")

	/** Document title [String] (e.g. "My Document"). */
	DEFINE_STRINGID (kDocumentTitle, "Document:Title")

	/** Localized document title. */
	DEFINE_STRINGID (kDocumentLocalizedTitle, "Document:LocalizedTitle")

	/** Document description [String] (e.g. "This is my document"). */
	DEFINE_STRINGID (kDocumentDescription, "Document:Description")

	/** Localized document description. */
	DEFINE_STRINGID (kDocumentLocalizedDescription, "Document:LocalizedDescription")

	/** Comma separated document keywords [String] (e.g. "keyword1, keyword2").*/
	DEFINE_STRINGID (kDocumentKeywords, "Document:Keywords")
	
	/** Notes (path to text file) [String]. */
	DEFINE_STRINGID (kDocumentNotes, "Document:Notes")
}

/*
	<MetaInformation>
		<Attribute id="Document:MimeType" value="application/x-document-type"/>
		<Attribute id="Document:Generator" value="My Application/1.0"/>
		<Attribute id="Document:Creator" value="John Doe"/>
	</MetaInformation>
*/

} // namespace CCL

#endif // _ccl_idocumentmetainfo_h
