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
// Filename    : ccl/extras/portable/gameenvironment.h
// Description : Game environment
//
//************************************************************************************************

#ifndef _ccl_gameenvironment_h
#define _ccl_gameenvironment_h

#include "ccl/app/component.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "core/public/gui/coregameinterface.h"

namespace CCL {

//************************************************************************************************
// GameEnvironment
//************************************************************************************************

class GameEnvironment: public Component,
					   public IdleClient
{
public:
	DECLARE_CLASS (GameEnvironment, Component)
	DECLARE_METHOD_NAMES (GameEnvironment)

	GameEnvironment (StringRef name = nullptr);
	~GameEnvironment ();

	void setScreenSize (PointRef screenSize, int screenFormat = IBitmap::kAny);
	int getScreenWidth () const;
	int getScreenHeight () const;
	int getScreenFormat () const;

	bool loadGame (UIDRef cid);
	void setGame (Core::IGameCore* game);
	void startGame (bool state);
	void runGame ();

	bool isJoypadButtonPressed (Core::JoypadButton button) const;
	void setJoypadButtonPressed (Core::JoypadButton button, bool state);
	void resetJoypadState ();

	PROPERTY_OBJECT (Point, pointerPosition, PointerPosition)
	PROPERTY_BOOL (pointerDown, PointerDown)
	int getPointerValue (Core::PointerValue which) const;

	Core::IGameBitmapRenderer* getRenderer () const;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (ITimerTask, Component)

protected:
	class Wrapper;

	Wrapper* wrapper;
	Point screenSize;
	int screenFormat;
	int joypadState;
	ICoreClass* gameClass;
	Core::IGameCore* game;
	Core::IGameBitmapRenderer* renderer;
	ViewPtr gameView;

	// IdleClient
	void onIdleTimer () override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// GameView
//************************************************************************************************

class GameView: public UserControl
{
public:
	DECLARE_CLASS_ABSTRACT (GameView, UserControl)

	GameView (GameEnvironment& environment, RectRef size);
	~GameView ();

	// UserControl
	void draw (const DrawEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;

protected:
	class PointerHandler;

	GameEnvironment& environment;
	AutoPtr<IImage> bitmap;
	AutoPtr<IBitmapFilter> backgroundFilter;

	bool mapJoypayKey (const KeyEvent& event);
};

} // namespac CCL

#endif // _ccl_gameenvironment_h
