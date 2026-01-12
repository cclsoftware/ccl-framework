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
// Filename    : core/portable/gui/corealertbox.h
// Description : Alert box
//
//************************************************************************************************

#ifndef _corealertbox_h
#define _corealertbox_h

#include "core/public/coremacros.h"
#include "core/public/corestringbuffer.h"
#include "core/portable/gui/coregraphics.h"

namespace Core {
namespace Portable {

class RootView;

typedef int AlertID;

//************************************************************************************************
// IAlertCompletionHandler
/** \ingroup core_gui */
//************************************************************************************************

struct IAlertCompletionHandler
{
	virtual void onAlertCompleted (AlertID alertId, int result) = 0;
};

//************************************************************************************************
// AlertDescription
/** \ingroup core_gui */
//************************************************************************************************

class AlertDescription
{
public:
	enum Result
	{
		kYes,
		kNo,
		kOk,
		kCancel,
		
		kFirst = kYes,
		kUndefined = -1
	};

	AlertDescription ()
	: firstResult (kUndefined),
	  secondResult (kUndefined)
	{}

	PROPERTY_CSTRING_BUFFER (GraphicsRenderer::kMaxMultilineStringLength, text, Text)
	PROPERTY_CSTRING_BUFFER (256, secondaryText, SecondaryText)
	PROPERTY_CSTRING_BUFFER (32, firstButton, FirstButton)
	PROPERTY_CSTRING_BUFFER (32, secondButton, SecondButton)
	PROPERTY_VARIABLE (int, firstResult, FirstResult)
	PROPERTY_VARIABLE (int, secondResult, SecondResult)
};

//************************************************************************************************
// AlertBox
/** \ingroup core_gui */
//************************************************************************************************

class AlertBox: public AlertDescription
{
public:
	static void setRootView (RootView* rootView);
	static CStringPtr getButtonTitle (int result);
		
	static void showOk (CStringPtr text, CStringPtr secondaryText, AlertID id = 0, IAlertCompletionHandler* handler = nullptr);
	static void showOkCancel (CStringPtr text, CStringPtr secondaryText, AlertID id = 0, IAlertCompletionHandler* handler = nullptr);
	static void showYesNo (CStringPtr text, CStringPtr secondaryText, AlertID id = 0, IAlertCompletionHandler* handler = nullptr);

	void initButtons (int firstResult, int secondResult = kUndefined);
	void runAsync (AlertID id = 0, IAlertCompletionHandler* handler = nullptr);

protected:
	static RootView* theRootView;
};

} // namespace Portable
} // namespace Core

#endif // _corealertbox_h
