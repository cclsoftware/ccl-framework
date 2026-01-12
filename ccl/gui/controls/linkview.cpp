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
// Filename    : ccl/gui/controls/linkview.cpp
// Description : Link View
//
//************************************************************************************************

#include "ccl/gui/controls/linkview.h"

#include "ccl/base/storage/url.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/systemshell.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/inavigator.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// LinkViewMouseHandler
//************************************************************************************************

class LinkViewMouseHandler: public MouseHandler
{
public:
	LinkViewMouseHandler (LinkView* view = nullptr)
	: MouseHandler (view)
	{}

	void onRelease (bool canceled) override
	{ 
		if(canceled)
			return;

		if(!view->isInsideClient (current.where))
			return;

		LinkView* linkView = (LinkView*)view;
		Url url;
		if(linkView->getUrl (url))
		{
			INavigator* navigator = linkView->getNavigator ();
			if(navigator)
				navigator->navigateDeferred (url);
			else
			{
				if(url.isFolder () && System::GetFileSystem ().isLocalFile (url))
					System::GetSystemShell ().showFile (url);
				else
					System::GetSystemShell ().openUrl (url);
			}
		}
		else
			linkView->push ();
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// LinkView
//************************************************************************************************

BEGIN_STYLEDEF (LinkView::customStyles)
	{"button",	 Styles::kLinkViewAppearanceButton},
	{"fittitle", Styles::kLinkViewAppearanceFitTitle},
	{"urltitle", Styles::kLinkViewAppearanceTitleAsUrl},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (LinkView, Button)
DEFINE_CLASS_UID (LinkView, 0x3804ac91, 0xd0fb, 0x4fec, 0xb1, 0xa, 0xb, 0x86, 0xae, 0xd9, 0x3b, 0x5f)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinkView::LinkView (const Rect& size, Url* _url, StringRef title, StyleRef style)
: Button (size, nullptr/*no parameter*/, 0, title),
  url (nullptr)
{
	this->style = style;
	setParameter (nullptr); // delete default parameter
	enable (true);
	setUrl (_url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinkView::LinkView (const Rect& size, IParameter* p, StringRef title, StyleRef style)
: Button (size, p, 0, title),
  url (nullptr)
{
	this->style = style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinkView::~LinkView ()
{
	if(url)
		url->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkView::setUrl (Url* _url)
{
	take_shared<Url> (url, _url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkView::setUrl (UrlRef _url)
{
	if(url)
		url->release ();
	url = NEW Url (_url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* LinkView::getUrl () const 
{ 
	return url; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkView::getUrl (Url& result) const
{
	if(url)
	{
		result = *url;
		return true;
	}

	if(param && param->getType () == IParameter::kString)
	{
		String string;
		param->toString (string);
		if(string.isEmpty ())
			return false;

		result.setUrl (string);
		if(result.getProtocol ().isEmpty ())
			result.setProtocol (CCLSTR ("http"));
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INavigator* LinkView::getNavigator () const
{
	const View* v = this;
	while(v)
	{
		IUnknown* c = v->getController ();
		UnknownPtr<INavigator> navigator (c);
		if(navigator)
			return navigator;

		v = v->getParent ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkView::draw (const UpdateRgn& updateRgn)
{
	if(style.isCustomStyle (Styles::kLinkViewAppearanceButton))
	{
		SuperClass::draw (updateRgn);
		return;
	}

	if(style.isOpaque ())
	{
		String title (this->title);
		
		if(style.isCustomStyle (Styles::kLinkViewAppearanceTitleAsUrl))
		{
			if(title.isEmpty () && param)
				if(param->getType () == IParameter::kString)
					param->toString (title);
		}
		
		if(title.isEmpty ())
			return View::draw (updateRgn);
		
		GraphicsPort port (this);
		Rect r;
		getClientRect (r);

		const IVisualStyle& vs = getVisualStyle ();
		Font font (vs.getTextFont ());
		font.isUnderline (getMouseState () != kMouseNone);

		if(style.isCustomStyle (Styles::kLinkViewAppearanceFitTitle))
			Font::collapseString (title, r.getWidth (), font);

		Alignment textAlign (vs.getTextAlignment ());
		Color color = vs.getColor ("linkcolor", getTheme ().getThemeColor (ThemeElements::kHyperlinkColor));
		port.drawString (r, title, font, SolidBrush (color), textAlign);
	}

	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* LinkView::createMouseHandler (const MouseEvent& event)
{
	if(!url &&  !param)
		return nullptr; // used only for decoration, input handled somewhere else

	return NEW LinkViewMouseHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkView::onMouseEnter (const MouseEvent& event)
{
	//if(!style.isCustomStyle (Styles::kLinkViewButton)) keep pointhand cursor for image links!
	{
		bool emptyUrl = false; // do not show point hand if provided url is empty
		if(param && param->getType () == IParameter::kString)
		{
			String string;
			param->toString (string);
			emptyUrl = string.isEmpty ();
		}

		if(emptyUrl == false)
			setCursor (unknown_cast<MouseCursor> (getTheme ().getThemeCursor (ThemeElements::kPointhandCursor)));
	}

	return SuperClass::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkView::onMouseLeave (const MouseEvent& event)
{
	return SuperClass::onMouseLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkView::calcAutoSize (Rect& rect)
{
	if(title.isEmpty () && !views.isEmpty ())
		View::calcAutoSize (rect);
	else
		SuperClass::calcAutoSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinkView::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kLinkViewUrl)
	{
		var = (IUrl*)getUrl ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinkView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kLinkViewUrl)
	{
		UnknownPtr<IUrl> url (var);
		if(url)
			setUrl (*url);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}
