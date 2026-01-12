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
// Filename    : ccl/gui/skin/skinparser.h
// Description : Skin XML Parser
//
//************************************************************************************************

#ifndef _ccl_skinparser_h
#define _ccl_skinparser_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/xmlcontentparser.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

#include "ccl/base/storage/xmlpihandler.h"

namespace CCL {

class SkinModel;
class ISkinContext;

namespace SkinElements {
class Element; }

//************************************************************************************************
// SkinParser
//************************************************************************************************

class SkinParser: public XmlContentParser,
				  public XmlProcessingInstructionHandler
{
public:
	SkinParser (ISkinContext* context);
	~SkinParser ();

	SkinModel* parseSkin (UrlRef url);
	SkinModel* parseSkin (IStream& stream);
	SkinModel* getModel ();

	bool getFirstError (String& message) const;
	void setFileName (CStringRef fileName);

	// XmlContentParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API processingInstruction (StringRef target, StringRef data) override;

protected:
	SkinModel* model;
	bool firstTag;
	SkinElements::Element* current;
	MutableCString fileName;   // for error reporting
};

} // namespace CCL

#endif // _ccl_skinparser_h
