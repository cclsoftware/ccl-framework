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
// Filename    : ccl/public/gui/framework/usertooltip.h
// Description : User Tooltip Popup
//
//************************************************************************************************

#ifndef _ccl_usertooltip_h
#define _ccl_usertooltip_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iobserver.h"
#include "ccl/public/gui/graphics/point.h"

namespace CCL {

interface IView;
interface ITooltipPopup;

//************************************************************************************************
// UserTooltipPopup
/** Helper for managing an individual tooltip popup.
	\ingroup gui_dialog */
//************************************************************************************************

class UserTooltipPopup: public Unknown,
						public IObserver
{
public:
	UserTooltipPopup (IView* view, bool followTooltipSignals = true);
	~UserTooltipPopup ();

	void setTooltip (StringRef text, const Point* position = nullptr);
	void hideTooltip ();

	void setPosition (const Point& position);
	void moveToMouse ();

	void reserve (bool state);
	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IObserver, Unknown)

protected:
	ITooltipPopup* tooltipPopup;
	IView* view;
	const bool followTooltipSignals;
};

} // namespace CCL

#endif // _ccl_usertooltip_h
