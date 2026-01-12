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
// Filename    : ccl/gui/windows/tooltip.h
// Description : Tooltips
//
//************************************************************************************************

#ifndef _ccl_tooltip_h
#define _ccl_tooltip_h

#include "ccl/base/object.h"
#include "ccl/base/singleton.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/gui/framework/itooltip.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

class View;
class Window;
interface IParameter;
interface ICommandParameter;

//************************************************************************************************
// TooltipFactory
//************************************************************************************************

class TooltipFactory
{
public:
	virtual ITooltipPopup* createTooltipPopup () = 0;

	static void linkTooltipFactory ();
};

//************************************************************************************************
// TooltipPopup
//************************************************************************************************

class TooltipPopup: public Object,
					public ITooltipPopup
{
public:
	DECLARE_CLASS_ABSTRACT (TooltipPopup, Object)

	TooltipPopup ();

	static void setFactory (TooltipFactory* factory); ///< platform-specific!
	static ITooltipPopup* createTooltipPopup (View* view);

	virtual void setBackColor (Color color) = 0;
	virtual void setTextColor (Color color) = 0;

	// ITooltipPopup
	void CCL_API setDuration (int64 ticks = kDefaultDuration) override;
	void CCL_API moveToMouse () override;
	int64 CCL_API getTimeToHide () override;
	StringRef CCL_API getText () const override;
	
	tbool CCL_API isReserved () const override {return exclusiveMode;}
	void CCL_API reserve (tbool state) override {exclusiveMode = (state != 0);}

	CLASS_INTERFACE (ITooltipPopup, Object)

protected:
	static TooltipFactory* factory;

	int64 timeToHide;
	String savedText;
	Point savedPosition;
	bool exclusiveMode;

	enum { kTooltipDuration = 5000 };

	void initColors (View* view);
};

//************************************************************************************************
// TooltipWindow
//************************************************************************************************

class TooltipWindow: public TooltipPopup,
					 public IdleClient
{
public:
	DECLARE_CLASS (TooltipWindow, TooltipPopup)
	
	TooltipWindow ();
	~TooltipWindow ();

	static TooltipFactory& getFactory ();
	
	// TooltipPopup
	void setBackColor (Color color) override;
	void setTextColor (Color color) override;
	void CCL_API construct (IView* view) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API setPosition (PointRef pos, IView* view = nullptr) override;
	void CCL_API setText (StringRef text) override;
	
	// IdleClient
	void onIdleTimer () override;
	
	CLASS_INTERFACE (ITimerTask, TooltipPopup)
	
protected:
	View* view;
	Window* tooltipWindow;
	View* tooltipView;
	Color backColor;
	Color textColor;
	
	bool needsRefresh;
	int64 lastRefresh;

	class WindowImpl;
    void constrainPosition (Point& pos);
	void updateWindow ();
};
	
//************************************************************************************************
// ComposedTooltip
/**
	Resolves variables in the tooltip string, encoded as @keyword[argument]

	Supported keywords:
	- "cmd": key binding for a command, either specified as argument [category|name] or taken from the the control's command parameter
	- "cmd.title": like cmd, but starting with the command title
	- "key": translated, platform specific key name for given key idientifier
	- "value": value of the control's parameter
	- "property": resolves argument as property path, with the view as anchor

	Examples:
		tooltip="Save @cmd[File|Save]" 
		tooltip="Save @cmd[]"
		tooltip="@cmd.title[]"
		tooltip="Volume (@value[])"
		tooltip="Enable @property[parent.title]"
		tooltip="Press @key[option] and @key[shift]"
. */
//************************************************************************************************

class ComposedTooltip: public String
{
public:
	ComposedTooltip (View* view);

private:
	View* view;

	void resolve (String& text);
	String resolveVariable (StringRef identifier, StringRef argument);
	String resolveCommandKey (StringID category, StringID name);
	String resolveCommandTitle (StringID category, StringID name);
	IParameter* getParameter ();
	ICommandParameter* getCommandParameter ();
};

} // namespace CCL

#endif // _ccl_tooltip_h
