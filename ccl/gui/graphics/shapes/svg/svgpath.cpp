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
// Filename    : ccl/gui/graphics/shapes/svg/svgpath.cpp
// Description : SVG Path parser
//
//************************************************************************************************

#include "ccl/gui/graphics/shapes/svg/svgpath.h"

#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/base/memorystream.h"

namespace CCL {
namespace SVG {

//////////////////////////////////////////////////////////////////////////////////////////////////
// CommandHandlers are methods of PathParser
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool (PathParser::*CommandHandler) (SVG::Length* args);

struct PathCommand
{
	uchar letter;
	int numArgs;
	CommandHandler handler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static const PathCommand pathCommands[] =
{
	{'m', 2, &PathParser::moveToRelative },
	{'M', 2, &PathParser::moveToAbsolute },
	{'l', 2, &PathParser::lineToRelative },
	{'L', 2, &PathParser::lineToAbsolute },
	{'h', 1, &PathParser::lineToRelativeH },
	{'H', 1, &PathParser::lineToAbsoluteH },
	{'v', 1, &PathParser::lineToRelativeV },
	{'V', 1, &PathParser::lineToAbsoluteV },
	{'c', 6, &PathParser::curveToRelative },
	{'C', 6, &PathParser::curveToAbsolute },
	{'s', 4, &PathParser::shortCurveToRelative },
	{'S', 4, &PathParser::shortCurveToAbsolute },
	{'q', 4, &PathParser::quadCurveToRelative },
	{'Q', 4, &PathParser::quadCurveToAbsolute },
	{'t', 2, &PathParser::shortquadCurveToRelative },
	{'T', 2, &PathParser::shortquadCurveToAbsolute },
	{'a', 7, &PathParser::arcToRelative },
	{'A', 7, &PathParser::arcToAbsolute },
	{'z', 0, &PathParser::closePath },
	{'Z', 0, &PathParser::closePath },
};
enum
{
	kMaxPathArguments = 7,
	kNumPathCommands = sizeof(pathCommands)/sizeof(PathCommand)
};

//************************************************************************************************
// PathParser
//************************************************************************************************

PathParser::PathParser (IStream& stream, IGraphicsPath::FillMode fillMode)
: TextParser (stream),
  path (NEW GraphicsPath),
  command (nullptr),
  lastCommand (&pathCommands[kNumPathCommands-1]),
  x (0),
  y (0),
  startX (0),
  startY (0),
  controlX (0),
  controlY (0)
{
	addWhitespace (',');

	path->setFillMode (fillMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath* PathParser::parsePath (StringRef data, IGraphicsPath::FillMode fillMode)
{
	StringChars chars (data);
	MemoryStream memstream ((void*)(const uchar*)chars, (data.length () + 1) * sizeof(uchar));
	PathParser parser (memstream, fillMode);
	return parser.parse ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath* PathParser::parsePolyLine (StringRef points, IGraphicsPath::FillMode fillMode)
{
	StringChars chars (points);
	MemoryStream memstream ((void*)(const uchar*)chars, (points.length () + 1) * sizeof(uchar));
	TextParser parser (memstream);
	parser.addWhitespace (',');

	GraphicsPath* path = NEW GraphicsPath;
	path->setFillMode (fillMode);

	bool first = true;
	Length x, y;
	PointF point;

	while(true)
	{
		// try to parse a coordinate pair
		parser.skipWhite ();
		if(!parser.readFloat (x))
			break; // missing / failed coord

		parser.skipWhite ();
		if(!parser.readFloat (y))
			break; // missing / failed coord

		point = makePointF (x, y);
		if(first)
		{
			path->startFigure (point);
			first = false;
		}
		else
			path->lineTo (point);
	}
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath* PathParser::parsePolygon (StringRef points, IGraphicsPath::FillMode fillMode)
{
 	GraphicsPath* path = parsePolyLine (points, fillMode);
	path->closeFigure ();
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PathCommand* PathParser::getCommand (uchar letter)
{
	for(int i=0; i<kNumPathCommands; ++i)
		if(pathCommands[i].letter == letter)
			return &pathCommands[i];
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath* PathParser::parse ()
{
	Length args [kMaxPathArguments];

	skipWhite ();

	uchar c;
	while((c = peek ()) != 0)
	{
		// check for a command letter
		if(Unicode::isAlpha (c))
		{
			// get command from table
			command = getCommand (c);
			advance ();
		}
		// (if not a letter, the last command stays valid)

		if(!command)
			return nullptr;// command letter expected

		// try to parse the expected number of arguments
		int numArgs = command->numArgs;
		for(int i=0; i<numArgs; ++i)
		{
			skipWhite ();
			if(!readFloat (args[i]))
				return nullptr; // missing / failed argument
		}

		if(!(this->*command->handler) (args))
			return nullptr;

		lastCommand = command;
		skipWhite ();
	}
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::moveToRelative (SVG::Length* args)
{
	x += args[0];
	y += args[1];
	startX = x;
	startY = y;
	path->startFigure (makePointF (startX, startY));

	// moveto followed by multiple pairs of coordinates: treat as implicit lineto commands
	command = getCommand ('l');
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::moveToAbsolute (SVG::Length* args)
{
	x = args[0];
	y = args[1];
	startX = x;
	startY = y;
	path->startFigure (makePointF (startX, startY));

	command = getCommand ('L');
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToRelative (SVG::Length* args)
{
	args[0] += x;
	args[1] += y;
	return lineToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToAbsolute (SVG::Length* args)
{
	x = args[0];
	y = args[1];
	path->lineTo (makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToRelativeH (SVG::Length* args)
{
	args[0] += x;
	return lineToAbsoluteH (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToAbsoluteH (SVG::Length* args)
{
	Length oldX = x;
	x = args[0];
	path->lineTo (makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToRelativeV (SVG::Length* args)
{
	args[0] += y;
	return lineToAbsoluteV (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::lineToAbsoluteV (SVG::Length* args)
{
	Length oldY = y;
	y = args[0];
	path->lineTo (makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::curveToRelative (SVG::Length* args)
{
	args[0] += x;
	args[1] += y;
	args[2] += x;
	args[3] += y;
	args[4] += x;
	args[5] += y;
	return curveToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::curveToAbsolute (SVG::Length* args)
{
	Length oldX = x;
	Length oldY = y;
	Length x1 = args[0];
	Length y1 = args[1];
	controlX  = args[2];	// second control point (saved)
	controlY  = args[3];
	x = args[4];
	y = args[5];
	path->addBezier (makePointF (oldX, oldY), makePointF (x1, y1), makePointF (controlX, controlY), makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::shortCurveToRelative (SVG::Length* args)
{
	args[0] += x;
	args[1] += y;
	args[2] += x;
	args[3] += y;
	return shortCurveToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::shortCurveToAbsolute (SVG::Length* args)
{
	Length oldX = x;
	Length oldY = y;

	Length x1, y1;
	uchar lastLetter = Unicode::toLowercase (lastCommand->letter);
	if(lastLetter == 'c' || lastLetter == 's')
	{
		// mirrored second control point from last curve, relative to current point
		x1 = 2 * x - controlX;
		y1 = 2 * y - controlY;
	}
	else
	{
		x1 = x;
		y1 = y;
	}

	// second control point (saved)
	controlX = args[0];
	controlY = args[1];
	x = args[2];
	y = args[3];
	path->addBezier (makePointF (oldX, oldY), makePointF (x1, y1), makePointF (controlX, controlY), makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::quadCurveToRelative (SVG::Length* args)
{
	args[0] += x;
	args[1] += y;
	args[2] += x;
	args[3] += y;
	return quadCurveToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::quadCurveToAbsolute (SVG::Length* args)
{
	Length oldX = x;
	Length oldY = y;
	controlX = args[0];
	controlY = args[1];
	x = args[2];
	y = args[3];
	PointF c (makePointF (controlX, controlY));
	path->addBezier (makePointF (oldX, oldY), c, c, makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::shortquadCurveToRelative (SVG::Length* args)
{
	args[0] += x;
	args[1] += y;
	return shortquadCurveToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

 bool PathParser::shortquadCurveToAbsolute (SVG::Length* args)
{
	Length oldX = x;
	Length oldY = y;

	uchar lastLetter = Unicode::toLowercase (lastCommand->letter);
	if(lastLetter == 'q' || lastLetter == 't')
	{
		// mirrored control point from last curve, relative to current point
		controlX = 2 * x - controlX;
		controlY = 2 * y - controlY;
	}
	else
	{
		controlX = x;
		controlY = y;
	}

	x = args[0];
	y = args[1];
	PointF c (makePointF (controlX, controlY));
	path->addBezier (makePointF (oldX, oldY), c, c, makePointF (x, y));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::arcToRelative (SVG::Length* args)
{
	args[5] += x;
	args[6] += y;
	return arcToAbsolute (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::arcToAbsolute (SVG::Length* args)
{
	using MathFuncs = Math::Functions<Length>;

	auto calcDotProduct = [] (Length ux, Length uy, Length vx, Length vy)
	{
		return ux * vx + uy * vy;
	};

	auto calcMagnitude = [] (Length x, Length y)
	{
		return MathFuncs::sqrt (x * x + y * y);
	};

	auto calcVectorAngle = [&] (Length ux, Length uy, Length vx, Length vy)
	{
		Length r = calcDotProduct (ux, uy, vx, vy) / (calcMagnitude (ux, uy) * calcMagnitude (vx, vy));
		r = ccl_bound (r, -1.f, 1.f);
		return ((ux * vy < uy * vx) ? -1.0f : 1.0f) * acosf (r); // todo: acos/asin in Math::Functions?
	};

	Length x1	  = x;
	Length y1	  = y;
	Length rx     = args[0];
	Length ry     = args[1];
	Length angle  = args[2];
	bool largeArc = args[3] == 1;
	bool sweep    = args[4] == 1;
	x             = args[5];
	y             = args[6];

	CCL_PRINTF ("Arc: (%f, %f), (%f, %f), rx %f, ry %f, angle %f, largeArc %d, sweep %d\n", x1, y1, x, y, rx, ry, angle, largeArc, sweep);

	// convert from "endpoint" to "center" parameterization:
	// https://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes

	Length angleRad = Math::degreesToRad (angle);

	// 1) compute (x1', y1')
	Length dxh = (x1 - x) / 2.f;
	Length dyh = (y1 - y) / 2.f;
	Length sinA = MathFuncs::sin (angleRad);
	Length cosA = MathFuncs::cos (angleRad);

	Length x1p =  cosA * dxh + sinA * dyh;
	Length y1p = -sinA * dxh + cosA * dyh;

	// ensure radii are large enough (F.6.6)
	Length d = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
	if(d > 1)
	{
		d = MathFuncs::sqrt (d);
		rx *= d;
		ry *= d;
	}

	// 2) compute (cx', cy')
	Length s = 0.0f;
	Length sa = (rx * rx) * (ry * ry) - (rx * rx) * (y1p * y1p) - (ry * ry) * (x1p * x1p);
	Length sb = (rx * rx) * (y1p * y1p) + (ry * ry) * (x1p * x1p);
	if(sa < 0.0f)
		sa = 0.0f;
	if(sb > 0.0f)
		s = MathFuncs::sqrt (sa / sb);
	if(largeArc == sweep)
		s = -s;
	Length cxp = s *  rx * y1p / ry;
	Length cyp = s * -ry * x1p / rx;

	//CCL_PRINTF ("    c' (%f, %f) \n", cxp, cyp);

	// 3) compute (cx, cy) from (cx', cy')
	Length cx = (x1 + x) / 2.0f + cosA * cxp - sinA * cyp;
	Length cy = (y1 + y) / 2.0f + sinA * cxp + cosA * cyp;

	// 4) compute initial angle and delta angle
	Length ux = (x1p - cxp) / rx;
	Length uy = (y1p - cyp) / ry;
	Length vx = (-x1p - cxp) / rx;
	Length vy = (-y1p - cyp) / ry;
	Length startAngle = calcVectorAngle (1.0f, 0.0f, ux, uy);
	Length deltaAngle = calcVectorAngle (ux, uy, vx, vy);

	if(!sweep && deltaAngle > 0)
		deltaAngle -= 2 * Math::Constants<Length>::kPi;
	else if(sweep && deltaAngle < 0)
		deltaAngle += 2 * Math::Constants<Length>::kPi;

	startAngle = Math::radToDegrees (startAngle);
	deltaAngle = Math::radToDegrees (deltaAngle);

	CCL_PRINTF ("    center (%f, %f) angle: %f, delta angle: %f\n", cx, cy, startAngle, deltaAngle);

	RectF rect (cx - rx, cy - ry, cx + rx, cy + ry);
	path->addArc (rect, startAngle, deltaAngle);

	// todo: rotation of the ellipse's x-axis ("angle" argument)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathParser::closePath (SVG::Length* args)
{
	// restore start position of figure
	x = startX;
	y = startY;
	startX = startY = 0; // or do we need a stack?

	path->closeFigure ();
	return true;
}

} // namespace SVG
} // namespace CCL
