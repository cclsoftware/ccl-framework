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
// Filename    : ccl/public/app/inavigationserver.h
// Description : Navigation Server Interface
//
//************************************************************************************************

#ifndef _ccl_inavigationserver_h
#define _ccl_inavigationserver_h

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IView;
interface INavigator;

//************************************************************************************************
// NavigateArgs
/** Navigation arguments.
	\ingroup app_inter */
//************************************************************************************************

struct NavigateArgs
{
	INavigator& navigator;
	UrlRef url;
	IView& contentFrame;
	IUnknown* contentComponent;
	MutableCString errorDocumentName;

	NavigateArgs (INavigator& _navigator,
				  UrlRef _url,
				  IView& _contentFrame,
				  IUnknown* contentComponent = nullptr)
	: navigator (_navigator),
	  url (_url),
	  contentFrame (_contentFrame),
	  contentComponent (contentComponent)
	{}
};

//************************************************************************************************
// INavigationServer
/** Navigation server interface.
	\ingroup app_inter */
//************************************************************************************************

interface INavigationServer: IUnknown
{
	/** Navigate to new location. */
	virtual tresult CCL_API navigateTo (NavigateArgs& args) = 0;

	DECLARE_IID (INavigationServer)
};

DEFINE_IID (INavigationServer, 0x824d471a, 0xe6e, 0x44ae, 0x8b, 0x93, 0x23, 0x5d, 0x65, 0xb0, 0x7f, 0x1e)

} // namespace CCL

#endif // _ccl_inavigationserver_h
