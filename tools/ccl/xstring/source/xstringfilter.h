//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : xstringfilter.h
// Description : Format Filter
//
//************************************************************************************************

#ifndef _xstringfilter_h
#define _xstringfilter_h

#include "xstringmodel.h"

namespace XString {

//************************************************************************************************
// Filter
//************************************************************************************************

class Filter: public CCL::Object
{
public:
	Filter (Bundle& bundle, UrlRef path);

	virtual bool create () = 0;

protected:
	Bundle& bundle;
	UrlRef path;
};

//************************************************************************************************
// XmlFilter
//************************************************************************************************

class XmlFilter: public Filter
{
public:
	XmlFilter (Bundle& bundle, UrlRef path);

	virtual CCL::XmlNode* createNode () = 0;

	// XmlFilter
	bool create () override;
};

//************************************************************************************************
// ReferenceFilter
//************************************************************************************************

class ReferenceFilter: public XmlFilter
{
public:
	ReferenceFilter (Bundle& bundle, UrlRef path);

	// XmlFilter
	CCL::XmlNode* createNode () override;
};

//************************************************************************************************
// PrototypeFilter
//************************************************************************************************

class PrototypeFilter: public XmlFilter
{
public:
	PrototypeFilter (Bundle& bundle, UrlRef path);

	// XmlFilter
	CCL::XmlNode* createNode () override;
};

} // namespace XString

#endif // _xstringfilter_h
