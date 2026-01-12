//************************************************************************************************
//
// TUIO Support
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
// Filename    : tuioservice.h
// Description : TUIO Service
//
//************************************************************************************************

#ifndef _tuiouseroption_h
#define _tuiouseroption_h

#include "ccl/app/options/useroption.h"

#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/gui/framework/iview.h"

namespace CCL {

//************************************************************************************************
// RemoteUserOption
//************************************************************************************************

class TUIOUserOption: public UserOption,
					  public PluginInstance
{
public:
	DECLARE_CLASS (TUIOUserOption, UserOption)

	TUIOUserOption ();
	
	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	
	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	CLASS_INTERFACE (IPluginInstance, UserOption)
	
protected:
	ViewPtr optionView;
	IParameter& monitorNumber;
	IParameter& monitorCount;
};

} // namespace CCL

#endif // _tuiouseroption_h
