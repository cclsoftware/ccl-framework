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
// Filename    : ccl/public/gui/framework/iform.h
// Description : Form Interface
//
//************************************************************************************************

#ifndef _ccl_iform_h
#define _ccl_iform_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

struct SizeLimit;
interface IWindow;
interface ISkinElement;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Form class. */
	DEFINE_CID (Form, 0x1528b171, 0xcd36, 0x44d3, 0x81, 0xd7, 0xeb, 0x5f, 0xcd, 0xa8, 0x62, 0x1b);
}

//************************************************************************************************
// IForm
/** Skin-based view class
	\ingroup gui_view
	\ingroup gui_skin */
//************************************************************************************************

interface IForm: IUnknown
{
	/** Get form name (from skin description). */
	virtual StringID CCL_API getFormName () const = 0;

	/** Get window style. */
	virtual StyleRef CCL_API getWindowStyle () const = 0;

	/** Set window style. */
	virtual void CCL_API setWindowStyle (StyleRef style) = 0;

	/** Get associated controller. */
	virtual IUnknown* CCL_API getController () const = 0;

	/** Set controller. */
	virtual tbool CCL_API setController (IUnknown* controller) = 0;

	/** Open window. */
	virtual IWindow* CCL_API openWindow (IWindow* parentWindow = nullptr) = 0;
	
	/** Close window. */
	virtual void CCL_API closeWindow () = 0;

	/** Reload view content. */
	virtual void CCL_API reload () = 0;

	/** Get associated skin element. */
	virtual ISkinElement* CCL_API getISkinElement () const = 0;

	DECLARE_IID (IForm)
};

DEFINE_IID (IForm, 0xe3b93dbd, 0xd0a7, 0x45d4, 0x80, 0x25, 0xfd, 0x7a, 0xa, 0x8e, 0x68, 0xbf)

} // namespace CCL

#endif // _ccl_iform_h
