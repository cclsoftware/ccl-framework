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
// Filename    : ccl/gui/popup/popupslider.h
// Description : Popup Slider
//
//************************************************************************************************

#ifndef _ccl_popupslider_h
#define _ccl_popupslider_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/paramlist.h"

namespace CCL {

class Slider;
class View;

//************************************************************************************************
// PopupSlider
//************************************************************************************************

class PopupSlider: public Object,
				   public PopupSelectorClient,
				   public PopupSourceControllerAccess<AbstractNode>,
				   public AbstractController
{
public:
	PopupSlider (IParameter* parameter, StyleRef style);
	~PopupSlider ();
	
	PROPERTY_SHARED_AUTO (IParameter, parameter, Parameter)
	PROPERTY_VARIABLE (StyleFlags, style, Style)
	PROPERTY_BOOL (forceTouch, ForceTouch)
	PROPERTY_MUTABLE_CSTRING (popupFormName, PopupFormName)
	
	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event, IWindow* window) override;
	tbool CCL_API setToDefault () override;
	Result CCL_API onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view) override;
	tbool CCL_API mouseWheelOnSource (const MouseWheelEvent& event, IView* source) override;
	void CCL_API attached (IWindow& popupWindow) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IController
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index) const override;
	IParameter* CCL_API findParameter (StringID name) const override;
	// TODO: getParameterByTag?

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	
	CLASS_INTERFACE3 (IPopupSelectorClient, IController, IObjectNode, Object)

private:
	Slider* findControl (View* parent);
	Point getHandleCenter (Slider* slider);
	void overridePosition (IWindow& parent);
	
	bool isOverridePosition;
	bool hasTouchHandler;
};

} // namespace CCL

#endif // _ccl_popupslider_h
