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
// Filename    : ccl/public/gui/framework/styleids.h
// Description : Style ID Constants
//
//************************************************************************************************

#ifndef _ccl_styleids_h
#define _ccl_styleids_h

namespace CCL {

namespace StyleID
{
	const CStringPtr kPadding = "padding";
	const CStringPtr kPaddingLeft = "padding.left";
	const CStringPtr kPaddingTop = "padding.top";
	const CStringPtr kPaddingRight = "padding.right";
	const CStringPtr kPaddingBottom = "padding.bottom";
	const CStringPtr kMargin = "margin";

	const CStringPtr kMinWidth = "minwidth";
	const CStringPtr kMaxWidth = "maxwidth";
	const CStringPtr kMinHeight = "minheight";
	const CStringPtr kMaxHeight = "maxheight";

	const CStringPtr kTextColor = "textcolor";
	const CStringPtr kTextFont = "textfont";
	const CStringPtr kTextAlign = "textalign";
	const CStringPtr kTextOptions = "textoptions";
	const CStringPtr kStrokeWidth = "strokewidth";
	const CStringPtr kBackground = "background";
	const CStringPtr kForeColor = "forecolor";
	const CStringPtr kBackColor = "backcolor";
	const CStringPtr kHighlightColor = "hilitecolor";

	const CStringPtr kExtendedPopup = "popupextended";

	namespace BIV
	{
		const CStringPtr kIconValueSpacing = "iconvaluespacing";
		const CStringPtr kIconFillFactor = "iconfillfactor";
		const CStringPtr kIconLeading = "iconleading";
		const CStringPtr kIconOverlay = "iconoverlay";
		const CStringPtr kIconAlignment = "iconalign";
		const CStringPtr kValueAlignment = "valuealign";
	}

} // namespace StyleID

} // namespace CCL

#endif // _ccl_styleids_h
