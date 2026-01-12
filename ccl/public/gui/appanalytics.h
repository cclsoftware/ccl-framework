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
// Filename    : ccl/public/gui/appanalytics.h
// Description : Application Analytics Identifier
//
//************************************************************************************************

#ifndef _ccl_appanalytics_h
#define _ccl_appanalytics_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Analytics Identifier
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AnalyticsID
{
	// Events
	const CStringPtr kFileExported = "FileExported";
	const CStringPtr kFileExportReport = "FileExportReport";
		const CStringPtr kFileExports = "exports";
		const CStringPtr kFileExportContext = "context";
		const CStringPtr kFileExportType = "type";
		const CStringPtr kFileExportCount = "count";

	const CStringPtr kUserInputReport = "UserInputReport";
		const CStringPtr kInputEvents = "events";
		const CStringPtr kInputCount = "count";
		const CStringPtr kInputType = "type";
		const CStringPtr kInputTypeTouch = "touch";
		const CStringPtr kInputTypePen = "pen";
		const CStringPtr kInputTypeMouse = "mouse";
		const CStringPtr kInputTypeDrop = "drop";
		const CStringPtr kInputTypeContextMenu = "contextMenu";
		const CStringPtr kInputTypeKeyCommand = "keyCommand";

	const CStringPtr kViewOpened = "ViewOpened";
	const CStringPtr KViewOpenReport = "ViewOpenReport";
		const CStringPtr kViews = "views";
		const CStringPtr kViewName = "view";
		const CStringPtr kViewOpenCount = "count";

	const CStringPtr kNavigation = "Navigation";
	const CStringPtr kNavigationReport = "NavigationReport";
		const CStringPtr kNavigationPaths = "paths";
		const CStringPtr kNavigationPath = "path";
		const CStringPtr kNavigationCount = "count";

	const CStringPtr kBrowserInteraction = "Browser";
	const CStringPtr kBrowserInteractionReport = "BrowserReport";
		const CStringPtr kBrowsers = "browsers";
		const CStringPtr kBrowserName = "browser";
		const CStringPtr kBrowserInteractionCount = "interactions";
}

} // namespace CCL

#endif // _ccl_appanalytics_h
