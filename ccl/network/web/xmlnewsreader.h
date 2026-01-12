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
// Filename    : ccl/network/web/xmlnewsreader.h
// Description : Atom/RSS Reader
//
//************************************************************************************************

#ifndef _ccl_xmlnewsreader_h
#define _ccl_xmlnewsreader_h

#include "ccl/network/web/webnewsreader.h"

namespace CCL {
class XmlNode;

namespace Web {

//************************************************************************************************
// XmlNewsReader
//************************************************************************************************

class XmlNewsReader: public WebNewsReader
{
public:
	DECLARE_CLASS (XmlNewsReader, WebNewsReader)

protected:
	void setItemAttribute (WebNewsItem& item, StringID id, XmlNode* parent, StringID tagName);

	bool parseAtom (XmlNode* root);
	void updateAtomItem (WebNewsItem& item, XmlNode* parent);

	bool parseRSS (XmlNode* root);
	void updateRSSItem (WebNewsItem& item, XmlNode* parent);

	// WebNewsReader
	bool parseFeed (IStream& stream) override;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_xmlnewsreader_h
