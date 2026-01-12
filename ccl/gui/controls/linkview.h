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
// Filename    : ccl/gui/controls/linkview.h
// Description : Link View
//
//************************************************************************************************

#ifndef _ccl_linkview_h
#define _ccl_linkview_h

#include "ccl/gui/controls/button.h"

namespace CCL {

class Url;
interface INavigator;

//************************************************************************************************
// LinkView
//************************************************************************************************

class LinkView: public Button
{
public:
	DECLARE_CLASS (LinkView, Button)
	DECLARE_STYLEDEF (customStyles)

	LinkView (const Rect& size = Rect (), Url* url = nullptr, StringRef title = nullptr, StyleRef style = 0);
	LinkView (const Rect& size, IParameter* p, StringRef title = nullptr, StyleRef style = 0);
	~LinkView ();

	const Url* getUrl () const;
	bool getUrl (Url& url) const;
	void setUrl (Url* url);
	void setUrl (UrlRef url);

	INavigator* getNavigator () const;

	// View
	void draw (const UpdateRgn& updateRgn) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	void calcAutoSize (Rect& rect) override;

protected:
	friend class LinkViewMouseHandler;
	Url* url;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

} // namespace CCL

#endif // _ccl_linkview_h
