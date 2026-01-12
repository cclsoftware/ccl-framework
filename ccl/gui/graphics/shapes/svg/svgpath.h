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
// Filename    : ccl/gui/graphics/shapes/svg/svgpath.h
// Description : SVG Path parser
//
//************************************************************************************************

#ifndef _ccl_svgpath_h
#define _ccl_svgpath_h

#include "ccl/base/storage/textparser.h"

#include "ccl/gui/graphics/shapes/svg/svgtypes.h"

#include "ccl/public/gui/graphics/igraphicspath.h"

namespace CCL {

class GraphicsPath;

namespace SVG {

struct PathCommand;

//************************************************************************************************
// PathParser
//************************************************************************************************

class PathParser : private TextParser
{
public:
	static GraphicsPath* parsePath (StringRef data, IGraphicsPath::FillMode fillMode);
	static GraphicsPath* parsePolyLine (StringRef points, IGraphicsPath::FillMode fillMode);
	static GraphicsPath* parsePolygon  (StringRef points, IGraphicsPath::FillMode fillMode);

public:
	// command handler methods
	#define PATH_COMMAND(method)bool method (SVG::Length* args);
	PATH_COMMAND (moveToRelative)
	PATH_COMMAND (moveToAbsolute)
	PATH_COMMAND (lineToRelative)
	PATH_COMMAND (lineToAbsolute)
	PATH_COMMAND (lineToRelativeH)
	PATH_COMMAND (lineToAbsoluteH)
	PATH_COMMAND (lineToRelativeV)
	PATH_COMMAND (lineToAbsoluteV)
	PATH_COMMAND (curveToRelative)
	PATH_COMMAND (curveToAbsolute)
	PATH_COMMAND (shortCurveToRelative)
	PATH_COMMAND (shortCurveToAbsolute)
	PATH_COMMAND (quadCurveToRelative)
	PATH_COMMAND (quadCurveToAbsolute)
	PATH_COMMAND (shortquadCurveToRelative)
	PATH_COMMAND (shortquadCurveToAbsolute)
	PATH_COMMAND (arcToRelative)
	PATH_COMMAND (arcToAbsolute)
	PATH_COMMAND (closePath)
	#undef PATH_COMMAND

private:
	PathParser (IStream& stream, IGraphicsPath::FillMode fillMode);
	GraphicsPath* parse ();
	const PathCommand* getCommand (uchar letter);

	GraphicsPath* path;
	const PathCommand* command;     // current command
	const PathCommand* lastCommand; // last command
	Length x, y;                    // current postion
	Length startX, startY;          // start of current figure
	Length controlX, controlY;      // last control point of a curve
};

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace SVG
} // namespace CCL

#endif // _ccl_svgpath_h
