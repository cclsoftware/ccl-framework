//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/tooltip.wasm.cpp
// Description : WebAssembly Tooltips
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/tooltip.h"
#include "ccl/gui/views/view.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, updateTooltipElement, (int x, int y, CStringPtr text, CStringPtr background, CStringPtr textColor), 
{
	let tooltip = document.getElementById ('tooltip');
	if(!tooltip) 
	{
		tooltip = document.createElement ('div');
		tooltip.id = 'tooltip';
		tooltip.style.position = 'fixed';
		tooltip.style.background = UTF8ToString (background);
		tooltip.style.color = UTF8ToString (textColor);
		document.body.appendChild (tooltip);
	}

	tooltip.innerText = UTF8ToString (text);
	tooltip.style.left = x + 'px';
	tooltip.style.top = y + 'px';
	tooltip.style.display = 'block';
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removeTooltipElement, (),
{
	var tooltip = document.getElementById ('tooltip');
	if(tooltip)
		tooltip.remove ();
});

} // namespace Wasm

//************************************************************************************************
// WasmTooltip
//************************************************************************************************

class WasmTooltip: public TooltipPopup
{
public:
	DECLARE_CLASS (WasmTooltip, TooltipPopup)

	WasmTooltip ();
	~WasmTooltip ();

	// TooltipPopup
	void setBackColor (Color color) override;
	void setTextColor (Color color) override;
	void CCL_API construct (IView* view) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API setPosition (PointRef pos, IView* view = nullptr) override;
	void CCL_API setText (StringRef text) override;

private:
	Color backColor;
	Color textColor;

	void updateTooltip ();
};

//************************************************************************************************
// WasmTooltipFactory
//************************************************************************************************

class WasmTooltipFactory: public TooltipFactory
{
public:
	// TooltipFactory
	ITooltipPopup* createTooltipPopup () override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipFactory::linkTooltipFactory () {} // force linkage of this file

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (WasmTooltip, kFrameworkLevelFirst)
{
	static WasmTooltipFactory theFactory;
	TooltipPopup::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// WasmTooltipFactory
//************************************************************************************************

ITooltipPopup* WasmTooltipFactory::createTooltipPopup ()
{
	return NEW WasmTooltip;
}

//************************************************************************************************
// WindowsTooltip
//************************************************************************************************

DEFINE_CLASS (WasmTooltip, TooltipPopup)
DEFINE_CLASS_UID (WasmTooltip, 0xA077C193, 0x3A76, 0x4834, 0xB2, 0x34, 0x05, 0x78, 0xF1, 0x13, 0xAA, 0x32) // ClassID::TooltipPopup

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmTooltip::WasmTooltip ()
: backColor (Colors::kWhite),
  textColor (Colors::kBlack)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmTooltip::~WasmTooltip ()
{
	removeTooltipElement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTooltip::updateTooltip ()
{
	char background[10] = "";
	char foreground[10] = "";
	Colors::toCString (backColor, background, ARRAY_COUNT (background));
	Colors::toCString (textColor, foreground, ARRAY_COUNT (foreground));

	updateTooltipElement (savedPosition.x, savedPosition.y, MutableCString (savedText, Text::kUTF8), background, foreground);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmTooltip::construct (IView* iview)
{
	View* view = unknown_cast<View> (iview);
	initColors (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTooltip::setBackColor (Color color)
{
	if(color != backColor)
	{
		backColor = color;
		updateTooltip ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmTooltip::setTextColor (Color color)
{
	if(color != textColor)
	{
		textColor = color;
		updateTooltip ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmTooltip::show ()
{
	updateTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmTooltip::hide ()
{
	removeTooltipElement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmTooltip::setPosition (PointRef pos, IView* view)
{
	CCL_PRINTF ("position: %d, %d\n", pos.x, pos.y)
	Point p (pos);
	if(view)
		view->clientToScreen (p);
	CCL_PRINTF ("screen position: %d, %d\n", p.x, p.y)

	if(p != savedPosition)
	{
		savedPosition = p;
		updateTooltip ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmTooltip::setText (StringRef text)
{
	if(text != savedText)
	{
		savedText = text;
		updateTooltip ();
	}
}
