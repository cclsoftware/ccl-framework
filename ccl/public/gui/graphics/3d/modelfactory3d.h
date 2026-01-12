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
// Filename    : ccl/public/gui/graphics/3d/modelfactory3d.h
// Description : 3D Model Factory
//
//************************************************************************************************

#ifndef _ccl_modelfactory3d_h
#define _ccl_modelfactory3d_h

#include "ccl/public/gui/graphics/3d/imodel3d.h"

namespace CCL {

//************************************************************************************************
// ModelFactory3D
//************************************************************************************************

class ModelFactory3D
{
public:
	/** Create solid material with given color. */
	static AutoPtr<IMaterial3D> createSolidColorMaterial (ColorRef color);
	
	/** Create texture material using a Bitmap and an optional background color. */
	static AutoPtr<ITextureMaterial3D> createTextureMaterial (IBitmap* bitmap, ColorRef backgroundColor = Colors::kTransparentBlack);

	/** Create 3D model with data provided by geometry source. */
	static AutoPtr<IModel3D> createModelFromSource (const IGeometrySource3D& source,
											 IMaterial3D* material = nullptr);

	/** Create cube model. */
	static AutoPtr<IModel3D> createUnitCube (IMaterial3D* material = nullptr);

	/** Create grid model. */
	static AutoPtr<IModel3D> createGrid (uint32 gridWidth, uint32 gridHeight, float cellWidth, float cellHeight,
								  IMaterial3D* material = nullptr);

	/** Create sphere model. */
	static AutoPtr<IModel3D> createSphere (float radius, uint32 numberOfParallels, uint32 numberOfMeridians,
									IMaterial3D* material = nullptr);

	/** Create billboard (sprite). */
	static AutoPtr<IModel3D> createBillboard (IMaterial3D* material = nullptr);

private:
	static int getTesselationFlags (IMaterial3D* material);
};

} // namespace CCL

#endif // _ccl_modelfactory3d_h
