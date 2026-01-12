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
// Filename    : ccl/platform/win/gui/screenscaling.cpp
// Description : Screen Scaling
//
//************************************************************************************************

#define DEBUG_LOG 1
#define FORCE_SCREEN_INFORMATION_UPDATE 1

#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// ScreenManager
//************************************************************************************************

ScreenManager CCL::Win32::gScreens;
DEFINE_SINGLETON_CLASS (ScreenManager, Object)
DEFINE_CLASS_UID (ScreenManager, 0x2fb1afb6, 0x56ce, 0x43ae, 0xba, 0x91, 0x23, 0xc7, 0xd, 0xc, 0x87, 0xd7)

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ScreenManager::__createSingleton ()
{
	return return_shared (&gScreens);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScreenManager::ScreenManager ()
: primaryScreenIndex (0),
  activeScale (&systemScale),
  updatePending (false),
  lastUpdateTime (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScreenManager::~ScreenManager ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::update ()
{
	primaryScreenIndex = 0;
	screens.removeAll ();

#if (0 && DEBUG)
	// TEST screens:
	const RECT testScreens[] =
	{
		{0, 0, 1440, 900},
		{1440, -300, 3360, 900}
	};

	for(int i = 0; i < ARRAY_COUNT (testScreens); i++)
	{
		ScreenInformation screen;
		Win32::fromSystemRect (screen.pixelRect, testScreens[i]);
		screen.pixelWorkArea = screen.pixelRect;
		screen.scaleFactor = i == 0 ? 1.f : 2.f;
		screens.add (screen);
	}
#else
	typedef FixedSizeVector<HMONITOR, kMaxScreenCount> MonitorList;
	struct CB
	{
		static BOOL CALLBACK GetHandleCallback (HMONITOR handle, HDC hdc, LPRECT intersect, LPARAM dwData)
		{
			return reinterpret_cast<MonitorList*> (dwData)->add (handle);
		}
	};

	MonitorList monitors;
	::EnumDisplayMonitors (NULL, nullptr, CB::GetHandleCallback, reinterpret_cast<LPARAM> (&monitors));

	MONITORINFO info = {0};
	info.cbSize = sizeof(MONITORINFO);
	for(int i = 0; i < monitors.count (); i++)
	{
		::GetMonitorInfo (monitors[i], &info);

		ScreenInformation screen (monitors[i]);
		GdiInterop::fromSystemRect (screen.pixelRect, info.rcMonitor);
		GdiInterop::fromSystemRect (screen.pixelWorkArea, info.rcWork);
		screen.scaleFactor = gDpiInfo.getDpiFactorForMonitor (monitors[i]);
		if(info.dwFlags & MONITORINFOF_PRIMARY)
			primaryScreenIndex = i;

		screens.add (screen);
	}
#endif

	systemScale.dpiFactor = gDpiInfo.getSystemDpiFactor ();
	if(gDpiInfo.isPerMonitorDpi ()) // always prefer per-monitor to adapt to "on the fly" DPI changes!
	{
		recalcPerMonitorDpi ();
		activeScale = static_cast<IDpiScale*> (this);
	}
	else
	{
		recalcSystemDpi ();
		activeScale = &systemScale;
	}

	#if DEBUG_LOG
	dump ();
	#endif

	updatePending = false;
	lastUpdateTime = System::GetSystemTicks ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::displayChanged ()
{
	if(!updatePending)
	{
		#if FORCE_SCREEN_INFORMATION_UPDATE
		int64 delta = System::GetSystemTicks () - lastUpdateTime;
		CCL_PRINTF ("Time since last screen information update = %" FORMAT_INT64 "d\n", delta)
		if(delta <= 1000) // assume it's the same update when changed again within 1 second
			return;
		#endif

		updatePending = true;
		(NEW Message ("displayChanged"))->post (this, -1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScreenManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "displayChanged")
	{
		if(updatePending)
			update ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IDpiScale& ScreenManager::getScale () const
{
	// make sure scale information is up-to-date
	#if FORCE_SCREEN_INFORMATION_UPDATE
	if(updatePending)
		const_cast<ScreenManager*> (this)->update ();
	#endif

	return *activeScale;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::recalcSystemDpi ()
{
	for(int i = 0; i < screens.count (); i++)
	{
		ScreenInformation& s = screens[i];
		s.coordRect = s.pixelRect;
		DpiScale::toCoordRect (s.coordRect, systemScale.dpiFactor);
		s.coordWorkArea = s.pixelWorkArea;
		DpiScale::toCoordRect (s.coordWorkArea, systemScale.dpiFactor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct ScreenManager::Segment // per direction (X/Y)
{
	int screenIndex;
	bool isPrimary;
	int pixelStart;
	int pixelEnd;
	float scaleFactor;
	int coordStart;
	int coordEnd;

	Segment (int screenIndex = 0, bool isPrimary = false, int pixelStart = 0, int pixelEnd = 0, float scaleFactor = 1.f)
	: screenIndex (screenIndex),
	  isPrimary (isPrimary),
	  pixelStart (pixelStart),
	  pixelEnd (pixelEnd),
	  scaleFactor (scaleFactor),
	  coordStart (-1),
	  coordEnd (-1)
	{}

	bool operator > (const Segment& other) const
	{
		return pixelStart > other.pixelStart;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::recalcPerMonitorDpi ()
{
	typedef FixedSizeVector<Segment, kMaxScreenCount> SegmentList;

	// build list of sorted segments per axis
	SegmentList segmentsX, segmentsY;
	for(int i = 0; i < screens.count (); i++)
	{
		const ScreenInformation& s = screens[i];
		bool isPrimary = i == primaryScreenIndex;
		Segment xs (i, isPrimary, s.pixelRect.left, s.pixelRect.right, s.scaleFactor);
		segmentsX.addSorted (xs);
		Segment ys (i, isPrimary, s.pixelRect.top, s.pixelRect.bottom, s.scaleFactor);
		segmentsY.addSorted (ys);
	}

	auto getPrimarySegmentIndex = [] (const SegmentList& segments)
	{
		for(int i = 0; i < segments.count (); i++)
			if(segments[i].isPrimary)
				return i;
		ASSERT (0)
		return 0;
	};

	auto getCoordLength = [] (const Segment& s)
	{
		return DpiScale::pixelToCoord (s.pixelEnd - s.pixelStart, s.scaleFactor);
	};

	// calculate coordinates per axis
	auto recalcAxis = [&] (SegmentList& segments)
	{
		int primarySegmentIndex = getPrimarySegmentIndex (segments);
		Segment& primary = segments[primarySegmentIndex];
		primary.coordStart = 0;
		primary.coordEnd = getCoordLength (primary);

		const Segment* prev = &primary;
		for(int i = primarySegmentIndex-1; i >= 0; i--)
		{
			Segment& s = segments[i];

			if(s.pixelStart < prev->pixelStart)
			{
				int deltaInPixel = prev->pixelStart - s.pixelStart;
				int delta = DpiScale::pixelToCoord (deltaInPixel, s.scaleFactor);
				s.coordStart = prev->coordStart - delta;
				prev = &s;
			}
			else
				s.coordStart = prev->coordStart;

			int length = getCoordLength (s);
			s.coordEnd = s.coordStart + length;
		}

		prev = &primary;
		for(int i = primarySegmentIndex+1; i < segments.count (); i++)
		{
			Segment& s = segments[i];
			if(s.pixelStart > prev->pixelStart)
			{
				int deltaInPixel = s.pixelStart - prev->pixelEnd;
				int delta = DpiScale::pixelToCoord (deltaInPixel, s.scaleFactor);
				s.coordStart = prev->coordEnd + delta;
				prev = &s;
			}
			else
				s.coordStart = prev->coordStart;

			int length = getCoordLength (s);
			s.coordEnd = s.coordStart + length;
		}
	};

	recalcAxis (segmentsX);
	recalcAxis (segmentsY);

	for(int i = 0; i < segmentsX.count (); i++)
	{
		const Segment& xs = segmentsX[i];
		ScreenInformation& s = screens[xs.screenIndex];
		s.coordRect.left = xs.coordStart;
		s.coordRect.right = xs.coordEnd;
	}

	for(int i = 0; i < segmentsY.count (); i++)
	{
		const Segment& ys = segmentsY[i];
		ScreenInformation& s = screens[ys.screenIndex];
		s.coordRect.top = ys.coordStart;
		s.coordRect.bottom = ys.coordEnd;
	}

	// calculate work area in coordinates
	for(int i = 0; i < screens.count (); i++)
	{
		ScreenInformation& s = screens[i];

		Point offsetInPixel;
		offsetInPixel.x = s.pixelWorkArea.left - s.pixelRect.left;
		offsetInPixel.y = s.pixelWorkArea.top - s.pixelRect.top;
		s.coordWorkArea.left = s.coordRect.left + DpiScale::pixelToCoord (offsetInPixel.x, s.scaleFactor);
		s.coordWorkArea.top = s.coordRect.top + DpiScale::pixelToCoord (offsetInPixel.y, s.scaleFactor);

		int w = DpiScale::pixelToCoord (s.pixelWorkArea.getWidth (), s.scaleFactor);
		int h = DpiScale::pixelToCoord (s.pixelWorkArea.getHeight (), s.scaleFactor);
		s.coordWorkArea.right = s.coordWorkArea.left + w;
		s.coordWorkArea.bottom = s.coordWorkArea.top + h;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void ScreenManager::dump () const
{
	for(int i = 0; i < screens.count (); i++)
	{
		const ScreenInformation& s = screens[i];
		Debugger::printf ("Screen %d: scale factor = %.2f\n", i, s.scaleFactor);
		dumpRect (s.pixelRect, " pixel rect");
		dumpRect (s.coordRect, " coord rect");
		dumpRect (s.pixelWorkArea, " pixel work area");
		dumpRect (s.coordWorkArea, " coord work area");
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

const ScreenInformation& ScreenManager::screenForWindowHandle (void* hwnd) const
{
	if(hwnd != nullptr)
	{
		HMONITOR hMonitor = ::MonitorFromWindow ((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
		ASSERT (hMonitor)
		int index = screens.index (ScreenInformation (hMonitor));
		ASSERT (index >= 0)
		if(index >= 0)
			return screens[index];
	}
	return screens[primaryScreenIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ScreenInformation& ScreenManager::screenForCoordRect (RectRef coordRect) const
{
	int screenIndex = getIndexAtCoord (coordRect.getCenter (), true);
	return screens[screenIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ScreenInformation& ScreenManager::screenForPixelRect (RectRef pixelRect) const
{
	int screenIndex = getIndexAtPixel (pixelRect.getCenter (), true);
	return screens[screenIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ScreenInformation& ScreenManager::screenForCoord (PointRef p) const
{
	int screenIndex = getIndexAtCoord (p, true);
	return screens[screenIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ScreenInformation& ScreenManager::screenForPixel (PointRef p) const
{
	int screenIndex = getIndexAtPixel (p, true);
	return screens[screenIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScreenManager::getIndexAtCoord (PointRef p, bool defaultToPrimary) const
{
	for(int i = 0; i < screens.count (); i++)
	{
		const ScreenInformation& s = screens[i];
		if(s.coordRect.pointInside (p))
			return i;
	}
	return defaultToPrimary ? primaryScreenIndex : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScreenManager::getIndexAtPixel (PointRef p, bool defaultToPrimary) const
{
	for(int i = 0; i < screens.count (); i++)
	{
		const ScreenInformation& s = screens[i];
		if(s.pixelRect.pointInside (p))
			return i;
	}
	return defaultToPrimary ? primaryScreenIndex : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::toPixelPoint (Point& p) const
{
	const ScreenInformation& s = screenForCoord (p);
	s.toPixelPoint (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::toPixelRect (Rect& r) const
{
	const ScreenInformation& s = screenForCoordRect (r);
	s.toPixelRect (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::toCoordPoint (Point& p) const
{
	const ScreenInformation& s = screenForPixel (p);
	s.toCoordPoint (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenManager::toCoordRect (Rect& r) const
{
	const ScreenInformation& s = screenForPixelRect (r);
	s.toCoordRect (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScreenManager::screenPixelToGlobalCoord (Point& p) const
{
	return toCoordPoint (p);
}

//************************************************************************************************
// ScreenInformation
//************************************************************************************************

void ScreenInformation::toPixelPoint (Point& p) const
{
	Point delta;
	delta.x = p.x - coordRect.left;
	delta.y = p.y - coordRect.top;

	Point deltaInPixel;
	deltaInPixel.x = DpiScale::coordToPixel (delta.x, scaleFactor);
	deltaInPixel.y = DpiScale::coordToPixel (delta.y, scaleFactor);

	p.x = pixelRect.left + deltaInPixel.x;
	p.y = pixelRect.top + deltaInPixel.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenInformation::toPixelRect (Rect& r) const
{
	Point p (r.getLeftTop ());
	toPixelPoint (p);

	int w = DpiScale::coordToPixel (r.getWidth (), scaleFactor);
	int h = DpiScale::coordToPixel (r.getHeight (), scaleFactor);

	r (p.x, p.y, p.x + w, p.y + h);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenInformation::toCoordPoint (Point& p) const
{
	Point deltaInPixel;
	deltaInPixel.x = p.x - pixelRect.left;
	deltaInPixel.y = p.y - pixelRect.top;

	Point delta;
	delta.x = DpiScale::pixelToCoord (deltaInPixel.x, scaleFactor);
	delta.y = DpiScale::pixelToCoord (deltaInPixel.y, scaleFactor);

	p.x = coordRect.left + delta.x;
	p.y = coordRect.top + delta.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenInformation::toCoordRect (Rect& r) const
{
	Point p (r.getLeftTop ());
	toCoordPoint (p);

	int w = DpiScale::pixelToCoord (r.getWidth (), scaleFactor);
	int h = DpiScale::pixelToCoord (r.getHeight (), scaleFactor);

	r (p.x, p.y, p.x + w, p.y + h);
}
