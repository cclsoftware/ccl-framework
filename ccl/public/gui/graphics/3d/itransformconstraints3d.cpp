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
// Filename    : ccl/public/gui/graphics/3d/itransformconstraints3d.cpp
// Description : 3D Transformation Constraints
//
//************************************************************************************************

#include "ccl/public/gui/graphics/3d/itransformconstraints3d.h"

using namespace CCL;

//************************************************************************************************
// RotationConstraints3D
//************************************************************************************************

RotationConstraints3D::RotationConstraints3D (float minRotationX, float maxRotationX, 
											  float minRotationY, float maxRotationY, 
											  float minRotationZ, float maxRotationZ)
: minRotationX (minRotationX),
  maxRotationX (maxRotationX),
  minRotationY (minRotationY),
  maxRotationY (maxRotationY),
  minRotationZ (minRotationZ),
  maxRotationZ (maxRotationZ)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RotationConstraints3D::isValidTransform (Transform3DRef transform) const
{
	PointF3D rotation;
	transform.getRotation (rotation);

	if(rotation.x < minRotationX)
		return false;
	if(rotation.x > maxRotationX)
		return false;
	if(rotation.y < minRotationY)
		return false;
	if(rotation.y > maxRotationY)
		return false;
	if(rotation.z < minRotationZ)
		return false;
	if(rotation.z > maxRotationZ)
		return false;

	return true;
}

//************************************************************************************************
// TranslationConstraints3D
//************************************************************************************************

TranslationConstraints3D::TranslationConstraints3D (PointF3DRef minTranslation, PointF3DRef maxTranslation)
: minTranslation (minTranslation),
  maxTranslation (maxTranslation)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TranslationConstraints3D::isValidTransform (Transform3DRef transform) const
{
	PointF3D translation;
	transform.getTranslation (translation);

	if(translation.x < minTranslation.x)
		return false;
	if(translation.x > maxTranslation.x)
		return false;
	if(translation.y < minTranslation.y)
		return false;
	if(translation.y > maxTranslation.y)
		return false;
	if(translation.z < minTranslation.z)
		return false;
	if(translation.z > maxTranslation.z)
		return false;

	return true;
}
