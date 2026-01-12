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
// Filename    : ccl/gui/graphics/3d/model/tessellator3d.cpp
// Description : 3D tessellators for geometric primitives
//
//************************************************************************************************

#include "ccl/gui/graphics/3d/model/tessellator3d.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace CCL;

//************************************************************************************************
// CubeTessellator3D
//************************************************************************************************

DEFINE_CLASS (CubeTessellator3D, Object)
DEFINE_CLASS_UID (CubeTessellator3D, 0xabd9356a, 0x7c95, 0x4cd8, 0x8f, 0xb1, 0x42, 0xd8, 0x4b, 0xf4, 0xce, 0xf3)

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D CubeTessellator3D::kPositions[kVertexCount] =
{
	{-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, // top
	{-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, // front
	{ 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f}, // right
	{ 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, // back
	{-0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, // left
	{-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, // bottom
};

const PointF3D CubeTessellator3D::kNormals[kVertexCount] =
{
	{ 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0}, // top
	{ 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, // front
	{ 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0}, // right
	{ 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1}, // back
	{-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, // left
	{ 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, // bottom
};

const PointF3D CubeTessellator3D::kInverseNormals[kVertexCount] =
{
	{ 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, { 0, -1,  0}, // top
	{ 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1}, { 0,  0,  1}, // front
	{-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, {-1,  0,  0}, // right
	{ 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, { 0,  0, -1}, // back
	{ 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0}, { 1,  0,  0}, // left
	{ 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0}, { 0,  1,  0}, // bottom
};

const uint32 CubeTessellator3D::kIndicesCW[kIndexCount] =
{
	0, 3, 1, 3, 2, 1,
	4, 7, 5, 7, 6, 5,
	8, 11, 9, 11, 10, 9,
	12, 15, 13, 15, 14, 13,
	16, 19, 17, 19, 18, 17,
	20, 23, 21, 23, 22, 21,
};

const uint32 CubeTessellator3D::kIndicesCCW[kIndexCount] =
{
	0, 1, 3, 3, 1, 2,
	4, 5, 7, 7, 5, 6,
	8, 9, 11, 11, 9, 10,
	12, 13, 15, 15, 13, 14,
	16, 17, 19, 19, 17, 18,
	20, 21, 23, 23, 21, 22,
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF CubeTessellator3D::kTextureCoords[kVertexCount] =
{
#if 0
	{.5f, 0.f}, {.25f, 0.f}, {.25f, .34f}, {.5f, .34f}, // top
	{.75f, .67f}, {1.f, .67f}, {1.f, .34f}, {.75f, .34f}, // front
	{0.f, .67f}, {.25f, .67f}, {.25f, .34f}, {0.f, .34f}, // right
	{.25f, .67f}, {.5f, .67f}, {.5f, .34f}, {.25f, .34f}, // back
	{.5f, .67f}, {.75f, .67f}, {.75f, .34f}, {.5f, .34f}, // left
	{.5f, .67f}, {.25f, .67f}, {.25f, 1.f}, {.5f, 1.f}, // bottom
#else
	{1.f, 0.f}, {0.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}, // top
	{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}, // front
	{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}, // right
	{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}, // back
	{0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}, {0.f, 0.f}, // left
	{1.f, 0.f}, {0.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}, // bottom
#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CubeTessellator3D::CubeTessellator3D ()
: flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CubeTessellator3D::generate (uint32 tesselatorFlags)
{
	flags = 0;

	windingOrderCW (get_flag<uint32> (tesselatorFlags, kWindingOrderCW));

	if(get_flag<uint32> (tesselatorFlags, kGenerateNormals))
		inverseNormals (false);
	else if(get_flag<uint32> (tesselatorFlags, kGenerateInverseNormals))
		inverseNormals (true);
	
	if(get_flag<uint32> (tesselatorFlags, kGenerateTextureCoordinates))
		useTextureCoordinates (true);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API CubeTessellator3D::getPositions () const
{
	return kPositions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API CubeTessellator3D::getNormals () const
{
	return inverseNormals () ? kInverseNormals : kNormals;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF* CCL_API CubeTessellator3D::getTextureCoords () const
{
	return useTextureCoordinates () ? kTextureCoords : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API CubeTessellator3D::getVertexCount () const
{
	return kVertexCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uint32* CCL_API CubeTessellator3D::getIndices () const
{
	return windingOrderCW () ? kIndicesCW : kIndicesCCW;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API CubeTessellator3D::getIndexCount () const
{
	return kIndexCount;
}

//************************************************************************************************
// GridTessellator3D
//************************************************************************************************

DEFINE_CLASS (GridTessellator3D, Object)
DEFINE_CLASS_UID (GridTessellator3D, 0x51bfe9c6, 0x5485, 0x4c09, 0x94, 0x87, 0xb4, 0x64, 0x7b, 0x4f, 0xa6, 0x6d)

//////////////////////////////////////////////////////////////////////////////////////////////////

GridTessellator3D::GridTessellator3D ()
: gridWidth (0),
  gridHeight (0),
  cellWidth (0.f),
  cellHeight (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GridTessellator3D::generate (uint32 flags)
{
	if(gridWidth == 0 || gridHeight == 0 || cellWidth <= 0.f || cellHeight <= 0.f)
		return kResultFailed;

	generatePositions ();
	generateIndices (get_flag<uint32> (flags, kWindingOrderCW));

	if(get_flag<uint32> (flags, kGenerateNormals))
		generateNormals (false);
	else if(get_flag<uint32> (flags, kGenerateInverseNormals))
		generateNormals (true);

	if(get_flag<uint32> (flags, kGenerateTextureCoordinates))
		generateTextureCoords ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API GridTessellator3D::getPositions () const
{
	return positions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API GridTessellator3D::getNormals () const
{
	return normals;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF* CCL_API GridTessellator3D::getTextureCoords () const
{
	return textureCoords;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API GridTessellator3D::getVertexCount () const
{
	return positions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uint32* CCL_API GridTessellator3D::getIndices () const
{
	return indices;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API GridTessellator3D::getIndexCount () const
{
	return indices.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GridTessellator3D::setGridSize (uint32 width, uint32 height)
{
	if(width == 0 || height == 0)
		return kResultInvalidArgument;

	gridWidth = width;
	gridHeight = height;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GridTessellator3D::setCellSize (float width, float height)
{
	if(width <= 0.f || height <= 0.f)
		return kResultInvalidArgument;

	cellWidth = width;
	cellHeight = height;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GridTessellator3D::generatePositions ()
{
	positions.resize ((gridWidth + 1) * (gridHeight + 1));

	for(uint32 z = 0; z <= gridHeight; z++)
	{
		for(uint32 x = 0; x <= gridWidth; x++)
			positions.add ({ x * cellWidth - (gridWidth * cellWidth / 2.f), 0.f, z * cellHeight - (gridHeight * cellHeight / 2.f)});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GridTessellator3D::generateNormals (bool inverse)
{
	uint32 count = (gridWidth + 1) * (gridHeight + 1);
	normals.resize (count);

	if(inverse)
	{
		for(uint32 i = 0; i < count; i++)
			normals.add ({ 0.f, -1.f, 0.f });
	}
	else
	{
		for(uint32 i = 0; i < count; i++)
			normals.add ({ 0.f, 1.f, 0.f });
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GridTessellator3D::generateTextureCoords ()
{
	uint32 count = (gridWidth + 1) * (gridHeight + 1);
	textureCoords.resize (count);

	for(uint32 z = 0; z <= gridHeight; z++)
	{
		for(uint32 x = 0; x <= gridWidth; x++)
			textureCoords.add ({ float (x) / gridWidth, float (z) / gridHeight });
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GridTessellator3D::generateIndices (bool windingOrderCW)
{
	indices.resize (gridWidth * gridHeight * 6);

	for(uint32 z = 0; z < gridHeight; z++)
	{
		uint32 k1 = z * (gridWidth + 1);
		uint32 k2 = k1 + gridWidth + 1;

		for(uint32 x = 0; x < gridWidth; x++, k1++, k2++)
		{
			if(windingOrderCW)
			{
				indices.add (k2);
				indices.add (k2 + 1);
				indices.add (k1);

				indices.add (k2 + 1);
				indices.add (k1 + 1);
				indices.add (k1);
			}
			else
			{
				indices.add (k2);
				indices.add (k1);
				indices.add (k2 + 1);

				indices.add (k2 + 1);
				indices.add (k1);
				indices.add (k1 + 1);
			}
		}
	}
}

//************************************************************************************************
// UVSphereTessellator3D
//************************************************************************************************

DEFINE_CLASS (UVSphereTessellator3D, Object)
DEFINE_CLASS_UID (UVSphereTessellator3D, 0xebd3bd6c, 0xa986, 0x4372, 0xa2, 0x36, 0xb1, 0xd5, 0x39, 0x8c, 0x68, 0xc3)

//////////////////////////////////////////////////////////////////////////////////////////////////

UVSphereTessellator3D::UVSphereTessellator3D ()
: radius (0),
  numberOfParallels (0),
  numberOfMeridians (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UVSphereTessellator3D::generate (uint32 flags)
{
	if(radius <= 0.f || numberOfParallels == 0 || numberOfMeridians == 0)
		return kResultFailed;

	generatePositions ();
	generateIndices (get_flag<uint32> (flags, kWindingOrderCCW));

	if(get_flag<uint32> (flags, kGenerateNormals))
		generateNormals (false);
	else if(get_flag<uint32> (flags, kGenerateInverseNormals))
		generateNormals (true);
	
	if(get_flag<uint32> (flags, kGenerateTextureCoordinates))
		generateTextureCoords ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API UVSphereTessellator3D::getPositions () const
{
	return positions.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API UVSphereTessellator3D::getNormals () const
{
	return normals.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF* CCL_API UVSphereTessellator3D::getTextureCoords () const
{
	return textureCoords.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API UVSphereTessellator3D::getVertexCount () const
{
	return positions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uint32* CCL_API UVSphereTessellator3D::getIndices () const
{
	return indices.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API UVSphereTessellator3D::getIndexCount () const
{
	return indices.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UVSphereTessellator3D::setRadius (float radius)
{
	if(radius == 0)
		return kResultInvalidArgument;

	this->radius = radius;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UVSphereTessellator3D::setNumberOfParallels (uint32 count)
{
	if(count == 0)
		return kResultInvalidArgument;

	numberOfParallels = count;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UVSphereTessellator3D::setNumberOfMeridians (uint32 count)
{
	if(count == 0)
		return kResultInvalidArgument;

	numberOfMeridians = count;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UVSphereTessellator3D::generatePositions ()
{
	positions.resize ((numberOfParallels + 1) * (numberOfMeridians + 1));

	float sectorStep = 2.f * M_PI / numberOfMeridians;
	float stackStep = M_PI / numberOfParallels;

	for(uint32 i = 0; i <= numberOfParallels; i++)
	{
		float stackAngle = M_PI / 2.f - i * stackStep;
		float xy = radius * cosf (stackAngle);
		float z = radius * sinf (stackAngle);

		for(uint32 j = 0; j <= numberOfMeridians; j++)
		{
			float sectorAngle = j * sectorStep;

			float x = xy * cosf (sectorAngle);
			float y = xy * sinf (sectorAngle);

			positions.add ({ x, y, z });
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UVSphereTessellator3D::generateNormals (bool inverse)
{
	uint32 count = positions.count ();
	normals.setCount (count);

	float factor = (inverse ? -1.f : 1.f) / radius;
	for(uint32 i = 0; i < count; i++)
		normals[i] = positions[i] * factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UVSphereTessellator3D::generateTextureCoords ()
{
	uint32 count = positions.count ();
	textureCoords.setCount (count);

	// FIXME this mapping is not correct. Consider changing the sphere tesselator to generate an icosphere instead of a uv-sphere and update this method.
	
	for(uint32 i = 0; i <= numberOfParallels; i++)
	{
		for(uint32 j = 0; j <= numberOfMeridians; j++)
		{
			textureCoords[i * numberOfMeridians + j] = 
			{
				static_cast<float> (j) / numberOfMeridians,
				static_cast<float> (i) / numberOfParallels
			};
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UVSphereTessellator3D::generateIndices (bool windingOrderCW)
{
	indices.resize (numberOfParallels * (numberOfMeridians - 1) * 6);

	for(uint32 i = 0; i < numberOfParallels; i++)
	{
		uint32 k1 = i * (numberOfMeridians + 1);
		uint32 k2 = k1 + numberOfMeridians + 1;

		for(uint32 j = 0; j < numberOfMeridians; j++, k1++, k2++)
		{
			if(i != 0)
			{
				if(windingOrderCW)
				{
					indices.add (k1);
					indices.add (k2);
					indices.add (k1 + 1);
				}
				else
				{
					indices.add (k1);
					indices.add (k1 + 1);
					indices.add (k2);
				}
			}

			if(i != numberOfParallels - 1)
			{
				if(windingOrderCW)
				{
					indices.add (k1 + 1);
					indices.add (k2);
					indices.add (k2 + 1);
				}
				else
				{
					indices.add (k1 + 1);
					indices.add (k2 + 1);
					indices.add (k2);
				}
			}
		}
	}
}
