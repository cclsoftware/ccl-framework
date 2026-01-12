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
// Filename    : ccl/network/web/webrequest.h
// Description : Web Request
//
//************************************************************************************************

#ifndef _ccl_webrequest_h
#define _ccl_webrequest_h

#include "ccl/base/object.h"

#include "ccl/public/network/web/iwebrequest.h"

#include "ccl/base/collections/stringdictionary.h"

namespace CCL {
namespace Web {

class WebResponse;

//************************************************************************************************
// WebHeaderCollection
//************************************************************************************************

class WebHeaderCollection: public CStringDictionary,
						   public IWebHeaderCollection
{
public:
	DECLARE_CLASS (WebHeaderCollection, CStringDictionary)

	WebHeaderCollection ();

	// IWebHeaderCollection
	ICStringDictionary& getEntries () override;
	tbool CCL_API parseFileName (String& fileName) const override;
	tbool CCL_API parseDate (DateTime& date) const override;
	tbool CCL_API isChunkedTransfer () const override;
	tbool CCL_API setRangeBytes (int64 start, int64 end = 0) override;

	CLASS_INTERFACE (IWebHeaderCollection, CStringDictionary)
};

//************************************************************************************************
// WebRequest
//************************************************************************************************

class WebRequest: public Object,
				  public IWebRequest
{
public:
	DECLARE_CLASS (WebRequest, Object)

	WebRequest (IStream* stream = nullptr);
	~WebRequest ();

	void setStream (IStream* stream);

	// IWebRequest
	IStream* CCL_API getStream () override;
	IWebResponse* CCL_API getWebResponse () override;
	IWebHeaderCollection* CCL_API getWebHeaders () override;

	CLASS_INTERFACE (IWebRequest, Object)

protected:
	IStream* stream;
	WebResponse* response;
	WebHeaderCollection* headers;
};

//************************************************************************************************
// WebResponse
//************************************************************************************************

class WebResponse: public Object,
				   public IWebResponse
{
public:
	DECLARE_CLASS (WebResponse, Object)

	WebResponse (IStream* stream = nullptr);
	~WebResponse ();

	void setStream (IStream* stream);

	// IWebResponse
	IStream* CCL_API getStream () override;
	IWebHeaderCollection* CCL_API getWebHeaders () override;

	CLASS_INTERFACE (IWebResponse, Object)

protected:
	IStream* stream;
	WebHeaderCollection* headers;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webrequest_h
