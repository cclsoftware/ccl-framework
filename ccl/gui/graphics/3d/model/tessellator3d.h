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
// Filename    : ccl/gui/graphics/3d/model/tessellator3d.h
// Description : 3D tessellators for geometric primitives
//
//************************************************************************************************

#ifndef _ccl_tessellator3d_h
#define _ccl_tessellator3d_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"

#include "ccl/public/gui/graphics/3d/itessellator3d.h"

namespace CCL {

//************************************************************************************************
// CubeTessellator3D
//************************************************************************************************

class CubeTessellator3D: public Object,
                         public ICubeTessellator3D
{
public:
	DECLARE_CLASS (CubeTessellator3D, Object)

	CubeTessellator3D ();

	PROPERTY_FLAG (flags, 1 << 0, inverseNormals)
	PROPERTY_FLAG (flags, 1 << 1, windingOrderCW)
	PROPERTY_FLAG (flags, 1 << 2, useTextureCoordinates)
	
	// ICubeTessellator3D
	uint32 CCL_API getVertexCount () const override;
	uint32 CCL_API getIndexCount () const override;
	const PointF3D* CCL_API getPositions () const override;
	const PointF3D* CCL_API getNormals () const override;
	const PointF* CCL_API getTextureCoords () const override;
	const uint32* CCL_API getIndices () const override;
	tresult CCL_API generate (uint32 flags) override;

	CLASS_INTERFACE (ICubeTessellator3D, Object)

private:
	static constexpr uint32 kVertexCount = 24;
	static constexpr uint32 kIndexCount = 36;

	static const PointF3D kPositions[kVertexCount];
	static const PointF3D kNormals[kVertexCount];
	static const PointF3D kInverseNormals[kVertexCount];
	static const uint32 kIndicesCW[kIndexCount];
	static const uint32 kIndicesCCW[kIndexCount];
	static const PointF kTextureCoords[kVertexCount];

	int flags;
};

//************************************************************************************************
// GridTessellator3D
//************************************************************************************************

class GridTessellator3D: public Object,
                         public IGridTessellator3D
{
public:
	DECLARE_CLASS (GridTessellator3D, Object)

	GridTessellator3D ();

	// IGridTessellator3D
	uint32 CCL_API getVertexCount () const override;
	uint32 CCL_API getIndexCount () const override;
	const PointF3D* CCL_API getPositions () const override;
	const PointF3D* CCL_API getNormals () const override;
	const PointF* CCL_API getTextureCoords () const override;
	const uint32* CCL_API getIndices () const override;
	tresult CCL_API generate (uint32 flags) override;
	tresult CCL_API setGridSize (uint32 width, uint32 height) override;
	tresult CCL_API setCellSize (float width, float height) override;

	CLASS_INTERFACE (IGridTessellator3D, Object)

private:
	uint32 gridWidth;
	uint32 gridHeight;
	float cellWidth;
	float cellHeight;
	Vector<PointF3D> positions;
	Vector<PointF3D> normals;
	Vector<PointF> textureCoords;
	Vector<uint32> indices;

	void generatePositions ();
	void generateNormals (bool inverse);
	void generateTextureCoords ();
	void generateIndices (bool windingOrderCW);
};

//************************************************************************************************
// UVSphereTessellator3D
//************************************************************************************************

class UVSphereTessellator3D: public Object,
                             public IUVSphereTessellator3D
{
public:
	DECLARE_CLASS (UVSphereTessellator3D, Object)

	UVSphereTessellator3D ();

	// IUVSphereTessellator
	uint32 CCL_API getVertexCount () const override;
	uint32 CCL_API getIndexCount () const override;
	const PointF3D* CCL_API getPositions () const override;
	const PointF3D* CCL_API getNormals () const override;
	const PointF* CCL_API getTextureCoords () const override;
	const uint32* CCL_API getIndices () const override;
	tresult CCL_API generate (uint32 flags) override;
	tresult CCL_API setRadius (float radius) override;
	tresult CCL_API setNumberOfParallels (uint32 count) override;
	tresult CCL_API setNumberOfMeridians (uint32 count) override;

	CLASS_INTERFACE (IUVSphereTessellator3D, Object)

private:
	float radius;
	uint32 numberOfParallels; // i.e. horizontal lines
	uint32 numberOfMeridians; // i.e. vertical lines
	Vector<PointF3D> positions;
	Vector<PointF3D> normals;
	Vector<PointF> textureCoords;
	Vector<uint32> indices;

	void generatePositions ();
	void generateNormals (bool inverse);
	void generateTextureCoords ();
	void generateIndices (bool windingOrderCW);
};

} // namespace CCL

#endif // _ccl_tessellator3d_h
