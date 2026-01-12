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
// Filename    : ccl/gui/controls/popupbox.h
// Description : Popup Box
//
//************************************************************************************************

#ifndef _ccl_popupbox_h
#define _ccl_popupbox_h

#include "ccl/gui/controls/control.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/public/gui/framework/itimer.h"

#include "ccl/base/storage/attributes.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// PopupBox styles (only used in skin description)
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Styles
{
	enum PopupBoxStyles
	{
		kPopupBoxBehaviorSlider	          		= 1 << 0,	///< popup a slider for the param in name attribute
		kPopupBoxBehaviorOverridePosition		= 1 << 1,	///< popup will move itself to preferred position
		kPopupBoxBehaviorWantsFocus      		= 1 << 2,	///< PopupBox will take focus
		kPopupBoxBehaviorKeepMousePos	  		= 1 << 3,	///< for popup slider: don't manipulate mouse pointer position
		kPopupBoxBehaviorWantsDoubleClick		= 1 << 4,	///< double-click opens popup
		kPopupBoxBehaviorNoWheel				= 1 << 5,	///< no mousewheel
		kPopupBoxBehaviorWantsMouseView			= 1 << 6,	///< PopupBox will become mouse view (before child views)
		kPopupBoxBehaviorHasTriggerParameter	= 1 << 7,	///< name attribute specifies a bool parameter that indicates the popup state and can be used to trigger the popup (instead of a popupSelectorClient as object / component)
		kPopupBoxBehaviorWantsMouseInside		= 1 << 8	///< popup opens when mouse enters PopupBox and closes when mouse leaves
	};
}

//************************************************************************************************
// PopupBox
/** Shows a temporary pop-up view when clicked. 
On a mouse click, a PopupBox opens a view in a temporary modal popup window.
The view is created from the skin using the specified "form.name". The "popup" attribute 
specifies the alignment of the popup menu relative to the PopupBox. The popup will close when 
clicked outside of it.

Child views inside the <PopupBox> are displayed as usual, they also get the mouse states "mouse over" 
and "mouse down" when the PopupBox is either hovered or pressed.

\code{.xml}
<!-- Inside the popup form description, the special controller name "source" can be used to 
address the original controller that contains the <PopupBox> -->
<Form name="MyPopup">
  <using controller="source">
  </using>
</Form>  
\endcode
*/
//************************************************************************************************

class PopupBox: public Control,
				public ITimerTask
{
public:
	DECLARE_CLASS (PopupBox, Control)
	DECLARE_METHOD_NAMES (PopupBox)
	DECLARE_STYLEDEF (customStyles)
	
	PopupBox (IPopupSelectorClient* client = nullptr, StringID formName = CString::kEmpty, const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	~PopupBox ();

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)
	PROPERTY_BOOL (wheelEnabled, WheelEnabled)				///< mouse wheel enabled?
	
	// Control
	StringRef getHelpIdentifier () const override;
	View* enterMouse (const MouseEvent& event, View* currentMouseView) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	Attributes& getFormVariables () {return formVariables;}
	IPopupSelectorClient* getClient () const {return client;}
	
	void setPopupVisualStyle (VisualStyle* visualStyle);

	CLASS_INTERFACE (ITimerTask, Control)
	
protected:

	enum PrivateFlags
	{
		kWantsDoubleClick	= 1<<(kLastPrivateFlag + 1)
	};

	PROPERTY_FLAG (privateFlags, kWantsDoubleClick, wantsDoubleClick)

	AutoPtr<PopupSelector> popupSelector;
	IPopupSelectorClient* client;
	Attributes formVariables;

	class ClientTouchHandler;

	void showPopup ();

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;
};

} // namespace CCL

#endif // _ccl_popupbox_h
