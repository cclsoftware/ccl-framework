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
// Filename    : ccl/gui/controls/control.h
// Description : Control class
//
//************************************************************************************************

#ifndef _ccl_control_h
#define _ccl_control_h

#include "ccl/gui/views/view.h"

namespace CCL {

interface IParameter;
interface IParamObserver;
interface IParamPreviewHandler;
class ThemeRenderer;

//************************************************************************************************
// Control
//************************************************************************************************

class Control: public View,
			   public IControl
{
public:
	DECLARE_CLASS (Control, View)

	Control (const Rect& size = Rect (), IParameter* param = nullptr,
			 StyleRef style = 0, StringRef title = nullptr);
	~Control ();

	void connect (IParamObserver* owner, int tag);
	int getVisualState () const;
	IParamPreviewHandler* getPreviewHandler () const;

	virtual ThemeRenderer* getRenderer ();
	void setRenderer (ThemeRenderer* renderer);	/// control owns renderer

	String makeEditTooltip ();
	
	PROPERTY_BOOL (wheelEnabled, WheelEnabled)				///< mouse wheel enabled?
	PROPERTY_BOOL (contextMenuEnabled, ContextMenuEnabled)	///< context menu enabled?

	virtual bool canHandleDoubleTap () const;
	virtual void performReset ();
	static bool isResetClick (const MouseEvent& event);
	static bool handleMouseWheel (const MouseWheelEvent& event, IParameter* param, bool inverse = false);

	template <class T> class PhaseProperty;
	
	// IControl
	IParameter* CCL_API getParameter () const override;
	void CCL_API setParameter (IParameter* param) override;
		
	// View
	const IVisualStyle& CCL_API getVisualStyle () const override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;
	void updateClient () override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	bool onDrop (const DragEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool setHelpIdentifier (StringRef id) override;
	StringRef getHelpIdentifier () const override;
			
	CLASS_INTERFACE (IControl, View)

protected:
	IParameter* param;
	ThemeRenderer* renderer;
	String helpId;

	bool tryRecognizeParam (const DragEvent& event);
	bool tryWheelParam (const MouseWheelEvent& event, bool inverse = false);
	virtual void paramChanged ();
	bool contextMenuForParam (const ContextMenuEvent& event, IParameter* param);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// Control::PhaseProperty
//************************************************************************************************

template <class T>
class Control::PhaseProperty
{
public:
	PhaseProperty ()
	: phase (0.f)
	{}

	void setPhase (float _phase)
	{
		if(phase != _phase)
		{
			phase = _phase;
			static_cast<T*> (this)->invalidate ();
		}
	}

	float getPhase () const
	{
		return phase;
	}

protected:
	float phase;

	bool setPhaseProperty (StringID propertyId, const Variant& var)
	{
		if(propertyId == "phase")
		{
			setPhase (var);
			return true;
		}
		return false;
	}
};

} // namespace CCL

#endif // _ccl_control_h
