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
// Filename    : ccl/gui/system/autofill.h
// Description : Autofill Support
//
//************************************************************************************************

#ifndef _ccl_autofill_h
#define _ccl_autofill_h

#include "ccl/base/singleton.h"

#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/controlstyles.h"

namespace CCL {

class View;

//************************************************************************************************
// IAutofillClient
//************************************************************************************************

interface IAutofillClient: IUnknown
{
	virtual int getAutofillClientType () const = 0;

	virtual View* getAutofillClientView () = 0;

	virtual void receiveAutofillText (StringRef text) = 0;

	DECLARE_STYLEDEF (types)

	DECLARE_IID (IAutofillClient)
};

//************************************************************************************************
// AutofillManager
//************************************************************************************************

class AutofillManager: public Object,
					   public ExternalSingleton<AutofillManager>
{
public:
	virtual void addClient (IAutofillClient* client);

	virtual void removeClient (IAutofillClient* client);

	virtual void updateClient (IAutofillClient* client);
};

} // namespace CCL

#endif // _ccl_autofill_h
