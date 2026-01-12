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
// Filename    : ccl/public/gui/graphics/3d/modelfactory3d.cpp
// Description : 3D Model Factory
//
//************************************************************************************************

#include "ccl/public/gui/graphics/3d/modelfactory3d.h"
#include "ccl/public/gui/graphics/3d/itessellator3d.h"
#include "ccl/public/gui/graphics/3d/stockshader3d.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ModelFactory3D
//************************************************************************************************

AutoPtr<IMaterial3D> ModelFactory3D::createSolidColorMaterial (ColorRef color)
{
	ISolidColorMaterial3D* material = ccl_new<ISolidColorMaterial3D> (ClassID::SolidColorMaterial3D);
	material->setMaterialColor (color);
	return material;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<ITextureMaterial3D> ModelFactory3D::createTextureMaterial (IBitmap* bitmap, ColorRef backgroundColor)
{
	AutoPtr<ITextureMaterial3D> material = ccl_new<ITextureMaterial3D> (ClassID::TextureMaterial3D);
	if(material->setTexture (kDiffuseTexture, bitmap) != kResultOk)
		return nullptr;
	UnknownPtr<ISolidColorMaterial3D> solidColorMaterial (material);
	if(solidColorMaterial)
		solidColorMaterial->setMaterialColor (backgroundColor);
	return material.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IModel3D> ModelFactory3D::createModelFromSource (const IGeometrySource3D& source, IMaterial3D* material)
{
	IModel3D* model = ccl_new<IModel3D> (ClassID::Model3D);
	IGeometry3D* geometry = model->createGeometry ();
	geometry->copyFrom (source);
	model->addGeometry (geometry);
	model->setMaterialForGeometries (material);
	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IModel3D> ModelFactory3D::createUnitCube (IMaterial3D* material)
{
	AutoPtr<ICubeTessellator3D> tessellator = ccl_new<ICubeTessellator3D> (ClassID::CubeTessellator3D);
	tessellator->generate (getTesselationFlags (material));

	return createModelFromSource (*tessellator, material);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IModel3D> ModelFactory3D::createGrid (uint32 gridWidth, uint32 gridHeight, float cellWidth, float cellHeight,
											  IMaterial3D* material)
{
	AutoPtr<IGridTessellator3D> tessellator = ccl_new<IGridTessellator3D> (ClassID::GridTessellator3D);
	tessellator->setGridSize (gridWidth, gridHeight);
	tessellator->setCellSize (cellWidth, cellHeight);
	tessellator->generate (getTesselationFlags (material));

	return createModelFromSource (*tessellator, material);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IModel3D> ModelFactory3D::createSphere (float radius, uint32 numberOfParallels, uint32 numberOfMeridians,
												IMaterial3D* material)
{
	AutoPtr<IUVSphereTessellator3D> tessellator = ccl_new<IUVSphereTessellator3D> (ClassID::UVSphereTessellator3D);
	tessellator->setRadius (radius);
	tessellator->setNumberOfMeridians (numberOfMeridians);
	tessellator->setNumberOfParallels (numberOfParallels);
	tessellator->generate (getTesselationFlags (material));

	return createModelFromSource (*tessellator, material);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IModel3D> ModelFactory3D::createBillboard (IMaterial3D* material)
{
	IModel3D* model = ccl_new<IModel3D> (ClassID::Model3D);
	IGeometry3D* geometry = model->createBillboard ();
	model->addGeometry (geometry);
	model->setMaterialForGeometries (material);
	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ModelFactory3D::getTesselationFlags (IMaterial3D* material)
{
	int flags = ITessellator3D::kGenerateNormals;
	if(UnknownPtr<ITextureMaterial3D> (material).isValid ())
		flags |= ITessellator3D::kGenerateTextureCoordinates;
	return flags;
}
