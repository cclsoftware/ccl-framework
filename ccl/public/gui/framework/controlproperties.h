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
// Filename    : ccl/public/gui/framework/controlproperties.h
// Description : Control Properties
//
//************************************************************************************************

#ifndef _ccl_controlproperties_h
#define _ccl_controlproperties_h

#include "ccl/public/text/cstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Control Properties
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID (kImageViewBackground, "background") ///< ImageView background [IImage]

DEFINE_STRINGID (kVariantViewTransitionType, "transitionType") ///< VariantView transition type [TransitionType]

DEFINE_STRINGID (kButtonIcon, "icon") ///< Button icon [IImage]
DEFINE_STRINGID (kRadioButtonValue, "value") ///< RadioButton value [int/float]
DEFINE_STRINGID (kToolButtonModeParam, "modeParam") ///< ToolButton mode parameter [IParameter]

DEFINE_STRINGID (kLinkViewUrl, "url") ///< LinkView URL [IUrl]

DEFINE_STRINGID (kWebBrowserViewNavigator, "navigator")		///< INavigator instance of WebBrowserView
DEFINE_STRINGID (kWebBrowserViewIsAvailable, "isAvailable")	///< tells if WebBrowserView is available on current platform; should be considered "true" if the view does not implement this property

} // namespace CCL

#endif // _ccl_controlproperties_h
