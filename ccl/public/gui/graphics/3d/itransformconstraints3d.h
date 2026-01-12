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
// Filename    : ccl/public/gui/graphics/3d/itransformconstraints3d.h
// Description : 3D Transformation Constraints
//
//************************************************************************************************

#ifndef _ccl_transformconstraints3d_h
#define _ccl_transformconstraints3d_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/gui/graphics/3d/transform3d.h"

namespace CCL {

//************************************************************************************************
// ITransformConstraints3D
/** 3D Transformation Constraints
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ITransformConstraints3D: IUnknown
{
	/** Check if a given transform matches constraints. */
	virtual tbool CCL_API isValidTransform (Transform3DRef transform) const = 0;

	DECLARE_IID (ITransformConstraints3D)
};

DEFINE_IID (ITransformConstraints3D, 0x949f99e8, 0x881d, 0x4019, 0x85, 0x24, 0x20, 0xaf, 0x81, 0x82, 0xf2, 0xae)

//************************************************************************************************
// TransformConstraints3D
//************************************************************************************************

class TransformConstraints3D: public Unknown,
							  public ITransformConstraints3D
{
public:
	CLASS_INTERFACE (ITransformConstraints3D, Unknown)
};

//************************************************************************************************
// RotationConstraints3D
/** 3D Rotation Constraints
	\ingroup gui_graphics3d */
//************************************************************************************************

class RotationConstraints3D: public TransformConstraints3D
{
public:
	RotationConstraints3D (float minRotationX = 0.f, float maxRotationX = 0.f, float minRotationY = 0.f, float maxRotationY = 0.f, float minRotationZ = 0.f, float maxRotationZ = 0.f);

	// TransformConstraints3D
	tbool CCL_API isValidTransform (Transform3DRef transform) const override;

protected:
	float minRotationX;
	float maxRotationX;
	float minRotationY;
	float maxRotationY;
	float minRotationZ;
	float maxRotationZ;
};

//************************************************************************************************
// TranslationConstraints3D
/** 3D Translation Constraints
	\ingroup gui_graphics3d */
//************************************************************************************************

class TranslationConstraints3D: public TransformConstraints3D
{
public:
	TranslationConstraints3D (PointF3DRef minTranslation, PointF3DRef maxTranslation);

	// TransformConstraints3D
	tbool CCL_API isValidTransform (Transform3DRef transform) const override;

protected:
	PointF3D minTranslation;
	PointF3D maxTranslation;
};

} // namespace CCL

#endif // _ccl_transformconstraints3d_h
