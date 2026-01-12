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
// Filename    : ccl/public/gui/framework/iview3d.h
// Description : 3D View Interface
//
//************************************************************************************************

#ifndef _ccl_iview3d_h
#define _ccl_iview3d_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface ISceneRenderer3D;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (UserView3D, 0x87ba8c3d, 0x8e1, 0x4e7f, 0xa8, 0x68, 0xae, 0x56, 0x9f, 0x26, 0x85, 0xb5)
	DEFINE_CID (SceneView3D, 0xde8bfddc, 0x1708, 0x4de5, 0x96, 0x98, 0xd3, 0x3c, 0x8c, 0xfc, 0x5f, 0xac)
}

//************************************************************************************************
// IView3D
/** Interface for 3D view.
	\ingroup gui_view */
//************************************************************************************************

interface IView3D: IUnknown
{
	/**	Assign 3D content to view. Depending on the implementing class, this can be either
		IGraphicsContent3D for a UserView3D, or ISceneNode3D for SceneView3D. */
	virtual tresult CCL_API set3DContent (IUnknown* content) = 0;

	DECLARE_IID (IView3D)
};

DEFINE_IID (IView3D, 0x2bafbe7b, 0x51bc, 0x4cba, 0xa7, 0x26, 0x52, 0x27, 0xe7, 0x33, 0x55, 0x14)

//************************************************************************************************
// ISceneView3D
/** Interface for 3D scene view.
	\ingroup gui_view */
//************************************************************************************************

interface ISceneView3D: IView3D
{
	/** Get 3D scene renderer associated with this view. */
	virtual ISceneRenderer3D& CCL_API getSceneRenderer () = 0;

	DECLARE_IID (ISceneView3D)
};

DEFINE_IID (ISceneView3D, 0x2f89aae3, 0x6d84, 0x4316, 0xac, 0xa, 0x9b, 0xec, 0x76, 0x49, 0xfa, 0xd1)

} // namespace CCL

#endif // _ccl_iview3d_h
