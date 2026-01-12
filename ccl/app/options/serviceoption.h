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
// Filename    : ccl/app/options/serviceoption.h
// Description : Service Option
//
//************************************************************************************************

#ifndef _ccl_serviceoption_h
#define _ccl_serviceoption_h

#include "ccl/app/options/useroption.h"

namespace CCL {

class ServiceListModel;

//************************************************************************************************
// ServiceOption
//************************************************************************************************

class ServiceOption: public UserOption
{
public:
	DECLARE_CLASS (ServiceOption, UserOption)

	ServiceOption ();
	~ServiceOption ();

	// UserOption
	void CCL_API opened () override;
	void CCL_API closed () override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

protected:
	ServiceListModel* serviceList;
};

} // namespace CCL

#endif // _ccl_serviceoption_h
