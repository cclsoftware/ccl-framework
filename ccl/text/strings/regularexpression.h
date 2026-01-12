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
// Filename    : ccl/text/strings/regularexpression.h
// Description : Regular Expression
//
//************************************************************************************************

#ifndef _ccl_regularexpression_h
#define _ccl_regularexpression_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/text/iregexp.h"

namespace CCL {

//************************************************************************************************
// RegularExpression
//************************************************************************************************

class RegularExpression: public Unknown,
						 public IRegularExpression
{
public:
	RegularExpression ();
	~RegularExpression ();

	// IRegularExpression
	tresult CCL_API construct (StringRef expression, int options = 0) override;
	tbool CCL_API isFullMatch (StringRef string) const override;
	tbool CCL_API isPartialMatch (StringRef string) const override;
	tbool CCL_API replace (String& string, StringRef format) const override;
	tbool CCL_API replaceAll (String& string, StringRef format) const override;

	CLASS_INTERFACE (IRegularExpression, Unknown)

protected:
	void* reFull;
	void* rePartial;

	void cleanup ();
	bool replace (String& string, StringRef format, bool all) const;
};

} // namespace CCL

#endif // _ccl_regularexpression_h
