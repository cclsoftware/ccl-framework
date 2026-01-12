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
// Filename    : graphics3dtest.cpp
// Description : UnitTests for 3D graphics classes
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/gui/graphics/3d/transform3d.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (Graphics3DTestSuite, testTransform3DTranspose)
{
    PointF3D p (1.0f, 2.0f, -3.5f);
    Transform3D translation;
    translation.translate (-2.0f, 15.0f, 0.5f);
    
    translation.transform (p);

    CCL_TEST_ASSERT_NEAR (p.x, -1.0f, 1e-5);
    CCL_TEST_ASSERT_NEAR (p.y, 17.0f, 1e-5);
    CCL_TEST_ASSERT_NEAR (p.z, -3.0f, 1e-5);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (Graphics3DTestSuite, testTransform3DDecompose)
{
	Transform3D transform;
	transform.translate (-2.0f, 15.0f, 0.5f);
	transform *= TransformUtils3D::rotateYawPitchRoll (-1.74f, .32f, .95f);
	transform.scale (5.0f, 3.1f, 1.2f);
    
	PointF3D translation;
	PointF3D scale;
	float yaw = 0.f;
	float pitch = 0.f;
	float roll = 0.f;

	transform.getTranslation (translation);
	TransformUtils3D::getYawPitchRollAngles (yaw, pitch, roll, transform);
	transform.getScale (scale);
	
	CCL_TEST_ASSERT_NEAR (translation.x, -2.0f, 1e-5);
	CCL_TEST_ASSERT_NEAR (translation.y, 15.0f, 1e-5);
	CCL_TEST_ASSERT_NEAR (translation.z, 0.5f, 1e-5);
	
	CCL_TEST_ASSERT_NEAR (yaw, -1.74f, 1e-5);
	CCL_TEST_ASSERT_NEAR (pitch, .32f, 1e-5);
	CCL_TEST_ASSERT_NEAR (roll, .95f, 1e-5);
	
	CCL_TEST_ASSERT_NEAR (scale.x, 5.0f, 1e-5);
	CCL_TEST_ASSERT_NEAR (scale.y, 3.1f, 1e-5);
	CCL_TEST_ASSERT_NEAR (scale.z, 1.2f, 1e-5);
}
