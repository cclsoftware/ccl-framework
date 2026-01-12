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
// Filename    : ccl/public/gui/icontroller.h
// Description : Controller Interface
//
//************************************************************************************************

#ifndef _ccl_icontroller_h
#define _ccl_icontroller_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IParameter;

//************************************************************************************************
// IController
/**	A controller implements the backend logic for user interaction.
	It provides parameters and other objects used by GUI widgets. 
	\ingroup gui_skin */
//************************************************************************************************

interface IController: IUnknown
{
	/** Get number of parameters. */
	virtual int CCL_API countParameters () const = 0;

	/** Get parameter by index. */
	virtual IParameter* CCL_API getParameterAt (int index) const = 0;

	/** Get parameter by name. */
	virtual IParameter* CCL_API findParameter (StringID name) const = 0;

	/** Get parameter by tag. */
	virtual IParameter* CCL_API getParameterByTag (int tag) const = 0;

	/** Get object by name and class id. */
	virtual IUnknown* CCL_API getObject (StringID name, UIDRef classID) = 0;

	DECLARE_IID (IController)
};

DEFINE_IID (IController, 0x46ee5c3a, 0x6a49, 0x4ebc, 0xa0, 0x74, 0x4f, 0xc4, 0xc5, 0x92, 0xee, 0xd9)

//************************************************************************************************
// AbstractController
/**
	\ingroup gui_skin */
//************************************************************************************************

class AbstractController: public IController
{
public:
	// IController
	int CCL_API countParameters () const override
	{
		return 0;
	}

	IParameter* CCL_API getParameterAt (int index) const override
	{
		return nullptr;
	}

	IParameter* CCL_API findParameter (StringID name) const override
	{
		return nullptr;
	}

	IParameter* CCL_API getParameterByTag (int tag) const override
	{
		return nullptr;
	}

	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override
	{
		return nullptr;
	}
};


//************************************************************************************************
// IControllerProvider
/**	Get controller interface in cases that do not allow queryInterface() on IController.
	\ingroup gui_skin */
//************************************************************************************************

interface IControllerProvider: IUnknown
{
	/** Get controller. */
	virtual IController* CCL_API getController () const = 0;

	DECLARE_IID (IControllerProvider)
};

DEFINE_IID (IControllerProvider, 0x16ad4d2a, 0x9e14, 0x42e1, 0x9d, 0xa5, 0x72, 0xde, 0x99, 0x2b, 0xf5, 0x16)

} // namespace CCL

#endif // _ccl_icontroller_h
