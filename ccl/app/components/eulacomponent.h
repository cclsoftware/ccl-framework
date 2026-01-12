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
// Filename    : ccl/app/components/eulacomponent.h
// Description : EULA Component
//
//************************************************************************************************

#ifndef _ccl_eulacomponent_h
#define _ccl_eulacomponent_h

#include "ccl/app/component.h"

namespace CCL {

interface IAsyncOperation;

//************************************************************************************************
// EULAComponent
//************************************************************************************************

class EULAComponent: public Component
{
public:
	DECLARE_CLASS (EULAComponent, Component)

	EULAComponent (StringID formName = nullptr);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

	bool startup (const IUrl* defaultPath = nullptr);
	
	enum AgreementType
	{
		kEULA,		///< "End User License Agreement"
		kTOS,		///< "Terms of Service"
		kCustom		///< A custom title is provided
	};

	bool run (StringRef id, UrlRef path, StringRef title, AgreementType type = kEULA);
	IAsyncOperation* runAsync (StringRef id, UrlRef path, StringRef title, AgreementType type = kEULA);

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

protected:
	int runDialog (StringRef title);
	IAsyncOperation* runDialogAsync (StringRef title);

	static CString getAcceptedAttributeID ();
	bool checkAcceptedAndLoadText (StringRef id, UrlRef path);
	String getDialogTitle (StringRef title, AgreementType type) const;
	IView* createDialogView (StringRef title);
};

} // namespace CCL

#endif // _ccl_eulacomponent_h
