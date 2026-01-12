//************************************************************************************************
//
// CCL Spy
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
// Filename    : doceditor.h
// Description : Documentation Editor
//
//************************************************************************************************

#ifndef _doceditor_h
#define _doceditor_h

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/app/component.h"

namespace CCL {
class XmlNode; }

namespace Spy {

//************************************************************************************************
// DocumentationFile
//************************************************************************************************

class DocumentationFile: public CCL::TextResource
{
public:
	DECLARE_CLASS (DocumentationFile, TextResource)

	PROPERTY_OBJECT (CCL::Url, path, Path)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (summary, Summary)

	// TextResource
	CCL::tbool CCL_API load (CCL::IStream& stream) override;

protected:
	void summarize (const CCL::XmlNode& parent);
};

//************************************************************************************************
// DocumentationEditor
//************************************************************************************************

class DocumentationEditor: public CCL::Component
{
public:
	DocumentationEditor ();

	void setFile (DocumentationFile* file);
};

} // namespace Spy

#endif // _doceditor_h
