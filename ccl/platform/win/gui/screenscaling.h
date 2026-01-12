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
// Filename    : ccl/platform/win/gui/screenscaling.h
// Description : Screen Scaling
//
//************************************************************************************************

#ifndef _ccl_screenscaling_h
#define _ccl_screenscaling_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/iwin32specifics.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// IDpiScale
//************************************************************************************************

struct IDpiScale
{
	virtual void toPixelPoint (Point& p) const = 0;
	virtual void toPixelRect (Rect& r) const = 0;
	virtual void toCoordPoint (Point& p) const = 0;
	virtual void toCoordRect (Rect& r) const = 0;
};

//************************************************************************************************
// SimpleDpiScale
//************************************************************************************************

struct SimpleDpiScale: IDpiScale
{
	float dpiFactor;

	SimpleDpiScale (float dpiFactor = 1)
	: dpiFactor (dpiFactor)
	{}

	// IDpiScale
	void toPixelPoint (Point& p) const override	{ DpiScale::toPixelPoint (p, dpiFactor); }
	void toPixelRect (Rect& r) const override	{ DpiScale::toPixelRect (r, dpiFactor); }
	void toCoordPoint (Point& p) const override	{ DpiScale::toCoordPoint (p, dpiFactor); }
	void toCoordRect (Rect& r) const override	{ DpiScale::toCoordRect (r, dpiFactor); }
};

//************************************************************************************************
// ScreenInformation
//************************************************************************************************

struct ScreenInformation: IDpiScale
{
	void* handle;		// HMONITOR
	Rect pixelRect;
	Rect pixelWorkArea;
	Rect coordRect;
	Rect coordWorkArea;
	float scaleFactor;

	ScreenInformation (void* handle = nullptr)
	: handle (handle),
	  scaleFactor (1)
	{}

	bool operator == (const ScreenInformation& other) const
	{
		return handle == other.handle;
	}

	// IDpiScale
	void toPixelPoint (Point& p) const override;
	void toPixelRect (Rect& r) const override;
	void toCoordPoint (Point& p) const override;
	void toCoordRect (Rect& r) const override;
};

//************************************************************************************************
// ScreenManager
/** Handles screen coordinate conversion for multiple monitors with different scaling factors. */
//************************************************************************************************

class ScreenManager: public Object,
					 public IScreenInfo,
					 private IDpiScale
{
public:
	DECLARE_CLASS (ScreenManager, Object)

	ScreenManager ();
	~ScreenManager ();

	void update ();
	void displayChanged (); ///< deferred update

	#if DEBUG
	void dump () const;
	#endif

	int getCount () const;
	const ScreenInformation& getAt (int index) const;

	int getPrimaryIndex () const;
	const ScreenInformation& getPrimaryScreen () const;
	bool isPrimaryScreen (const ScreenInformation& screen) const;

	const IDpiScale& getScale () const;
	const IDpiScale* operator -> () const;

	const ScreenInformation& screenForWindowHandle (void* hwnd) const;
	const ScreenInformation& screenForCoordRect (RectRef coordRect) const;
	const ScreenInformation& screenForPixelRect (RectRef pixelRect) const;
	const ScreenInformation& screenForCoord (PointRef p) const;
	const ScreenInformation& screenForPixel (PointRef p) const;
	int getIndexAtCoord (PointRef p, bool defaultToPrimary) const;
	int getIndexAtPixel (PointRef p, bool defaultToPrimary) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IScreenInfo, Object)

protected:
	static Object* __createSingleton (); // used by meta class

	struct Segment;

	static const int kMaxScreenCount = 16;
	typedef FixedSizeVector<ScreenInformation, kMaxScreenCount> ScreenList;

	ScreenList screens;
	int primaryScreenIndex;
	SimpleDpiScale systemScale;
	IDpiScale* activeScale;
	bool updatePending;
	int64 lastUpdateTime;

	void recalcSystemDpi ();
	void recalcPerMonitorDpi ();

	// IDpiScale
	void toPixelPoint (Point& p) const override;
	void toPixelRect (Rect& r) const override;
	void toCoordPoint (Point& p) const override;
	void toCoordRect (Rect& r) const override;

	// IScreenInfo
	void CCL_API screenPixelToGlobalCoord (Point& p) const override;
};

extern ScreenManager gScreens;

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int ScreenManager::getCount () const { return screens.count (); }
inline const ScreenInformation& ScreenManager::getAt (int index) const { return screens.at (index); }
inline int ScreenManager::getPrimaryIndex () const { return primaryScreenIndex; }
inline const ScreenInformation& ScreenManager::getPrimaryScreen () const { return screens[primaryScreenIndex]; }
inline bool ScreenManager::isPrimaryScreen (const ScreenInformation& screen) const { return &screen == &getPrimaryScreen (); }
inline const IDpiScale* ScreenManager::operator -> () const { return &getScale (); }

} // namespace Win32
} // namespace CCL

#endif // _ccl_screenscaling_h
