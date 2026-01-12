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
// Filename    : ccl/public/gui/iparamobserver.h
// Description : Parameter Observer
//
//************************************************************************************************

#ifndef _ccl_iparamobserver_h
#define _ccl_iparamobserver_h

#include "ccl/public/base/variant.h"

namespace CCL {

interface IParameter;

//************************************************************************************************
// ParamPreviewEvent
/** Parameter preview event.
	\ingroup gui_param */
//************************************************************************************************

struct ParamPreviewEvent
{
	DEFINE_ENUM (Type)
	{
		kFocus,
		kUnfocus,
		kChange,
		kCancel,
		kPrepareMenu,
		kCleanupMenu,
		kSetMode, ///< can be a generic key state string or application-specific
		kOptionClick,
		kResetClick,
		kDoubleTap
	};

	Type type;
	Variant value;
	Variant handlerData;

	ParamPreviewEvent ()
	: type (kChange)
	{}
};

//************************************************************************************************
// IParamObserver
/** Notification interface for parameter changes.
	\ingroup gui_param */
//************************************************************************************************

interface IParamObserver: IUnknown
{
	/** The given parameter changed its value, usually caused by user interaction. */
	virtual tbool CCL_API paramChanged (IParameter* param) = 0;

	/** The user started or ended editing the value. */
	virtual void CCL_API paramEdit (IParameter* param, tbool begin) = 0;

	DECLARE_IID (IParamObserver)
};

DEFINE_IID (IParamObserver, 0x31971683, 0x812b, 0x4ece, 0x91, 0xec, 0x79, 0xfb, 0xbd, 0x1c, 0xea, 0xf0)

//************************************************************************************************
// IParamPreviewHandler
/** Notification interface for parameter preview.
	\ingroup gui_param */
//************************************************************************************************

interface IParamPreviewHandler: IUnknown
{
	/** The given parameter invoked a preview event. */
	virtual void CCL_API paramPreview (IParameter* param, ParamPreviewEvent& e) = 0;

	DECLARE_IID (IParamPreviewHandler)
};

DEFINE_IID (IParamPreviewHandler, 0xb17e3f80, 0xe16, 0x4fc8, 0x85, 0x2d, 0x30, 0xf2, 0xf8, 0x2a, 0xc8, 0xf9)

} // namespace CCL

#endif // _ccl_iparamobserver_h
