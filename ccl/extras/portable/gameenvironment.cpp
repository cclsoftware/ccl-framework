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
// Filename    : ccl/extras/portable/gameenvironment.cpp
// Description : Game environment
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/extras/portable/gameenvironment.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"

#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// GameEnvironment::Wrapper
//************************************************************************************************

class GameEnvironment::Wrapper: public Core::IGameEnvironment
{
public:
	Wrapper (GameEnvironment& owner)
	: owner (owner) 
	{}

	// IGameEnvironment
	void setProperty (const Core::Property& value) override {}
	void getProperty (Core::Property& value) override {}
	void release () override {}
	int getScreenWidth () const override { return owner.getScreenWidth (); }
	int getScreenHeight () const override { return owner.getScreenHeight (); }
	int getScreenFormat () const override { return owner.getScreenFormat (); }
	bool isJoypadButtonPressed (Core::JoypadButton button) const override { return owner.isJoypadButtonPressed (button); }
	int getPointerValue (Core::PointerValue which) const override { return owner.getPointerValue (which); }

protected:
	GameEnvironment& owner;
};

} // namespace CCL

using namespace CCL;
using namespace Core;

//************************************************************************************************
// GameEnvironment
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GameEnvironment, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

GameEnvironment::GameEnvironment (StringRef name)
: Component (name),
  wrapper (nullptr),
  screenSize (320, 200),
  screenFormat (IBitmap::kRGBAlpha),
  joypadState (0),
  pointerDown (false),
  gameClass (nullptr),
  game (nullptr),
  renderer (nullptr)
{
	wrapper = NEW Wrapper (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GameEnvironment::~GameEnvironment ()
{
	setGame (nullptr);

	delete wrapper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::setScreenSize (PointRef newSize, int newFormat)
{
	if(newSize.x > 0 && newSize.y > 0)
		screenSize = newSize;
	if(newFormat != IBitmap::kAny)
		screenFormat = newFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameEnvironment::loadGame (UIDRef cid)
{
	if(auto newClass = ccl_new<ICoreClass> (cid))
	{
		if(auto newGame = newClass->getClassInfo ().createInstance<IGameCore> ())
		{
			setGame (newGame);
			gameClass = newClass;
			return true;
		}

		ccl_release (newClass);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::setGame (IGameCore* _game)
{
	if(game != _game)
	{
		if(game)
			game->release ();
				
		game = _game;
		renderer = GetInterface<IGameBitmapRenderer> (game);

		if(gameClass)
			ccl_release (gameClass),
			gameClass = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::startGame (bool state)
{
	if(game)
	{
		if(state)
		{
			game->startup (wrapper);
			startTimer ();
		}
		else
		{
			game->shutdown ();
			stopTimer ();
		}

		if(gameView)
			ViewBox (gameView).invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::runGame ()
{
	if(game)
	{
		int result = game->run ();
		if(result & IGameCore::kFrameDirty)
		{
			if(gameView)
				ViewBox (gameView).invalidate ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API GameEnvironment::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "GameView")
	{
		auto newView = NEW GameView (*this, bounds);
		ASSERT (!gameView)
		gameView = *newView;
		return *newView;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::onIdleTimer ()
{
	runGame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GameEnvironment::getScreenWidth () const
{
	return screenSize.x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GameEnvironment::getScreenHeight () const
{
	return screenSize.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GameEnvironment::getScreenFormat () const
{
	return screenFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameEnvironment::isJoypadButtonPressed (JoypadButton button) const
{
	return get_bit (joypadState, button);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::setJoypadButtonPressed (JoypadButton button, bool state)
{
	set_bit (joypadState, button, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameEnvironment::resetJoypadState ()
{
	joypadState = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GameEnvironment::getPointerValue (PointerValue which) const
{
	switch(which)
	{
	case kPointerDown : return pointerDown ? 1 : 0;
	case kPointerPositionX : return pointerPosition.x;
	case kPointerPositionY : return pointerPosition.y;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGameBitmapRenderer* GameEnvironment::getRenderer () const
{
	return renderer; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (GameEnvironment)
	DEFINE_METHOD_ARGR ("startGame", "state: bool = true", "")
END_METHOD_NAMES (GameEnvironment)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GameEnvironment::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "startGame")
	{
		bool state = msg.getArgCount () > 0 ? msg[0].asBool () : true;
		startGame (state);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// GameView::PointerHandler
//************************************************************************************************

class GameView::PointerHandler: public UserControl::MouseHandler
{
public:
	PointerHandler (GameView& view)
	: MouseHandler (&view)
	{}

	GameView* getGameView () const
	{
		return static_cast<GameView*> (control);
	}

	// MouseHandler
	void onBegin () override
	{
		getGameView ()->environment.setPointerPosition (current.where);
		getGameView ()->environment.setPointerDown (true);
		CCL_PRINTLN ("GameView pointer down")
	}

	bool onMove (int moveFlags) override
	{
		getGameView ()->environment.setPointerPosition (current.where);
		return true;
	}

	void onRelease (bool canceled) override
	{
		getGameView ()->environment.setPointerDown (false);
		CCL_PRINTLN ("GameView pointer up")
	}
};

//************************************************************************************************
// GameView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GameView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

GameView::GameView (GameEnvironment& environment, RectRef size)
: UserControl (size),
  environment (environment)
{
	environment.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GameView::~GameView ()
{
	environment.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GameView::draw (const DrawEvent& event)
{	
	auto renderer = environment.getRenderer ();
	ASSERT (renderer)
	if(renderer)
	{
		auto pixelFormat = environment.getScreenFormat ();

		if(!bitmap) // allocate on first use
			bitmap = GraphicsFactory::createBitmap (environment.getScreenWidth (),
													environment.getScreenHeight (),
													pixelFormat);

		if(!backgroundFilter)
		{
			#if 1
			backgroundFilter = GraphicsFactory::createBitmapFilter (BitmapFilters::kFill);
			Color backColor = getVisualStyle ().getBackColor ();
			UnknownPtr<IObject> (backgroundFilter)->setProperty (IBitmapFilter::kColorID, (int)(uint32)backColor);
			#else
			backgroundFilter = GraphicsFactory::createBitmapFilter (BitmapFilters::kClear);
			#endif
		}

		BitmapDataLocker locker (UnknownPtr<IBitmap> (bitmap), pixelFormat, IBitmap::kLockWrite);
		ASSERT (locker.result == kResultOk)
		if(locker.result == kResultOk)
		{
			backgroundFilter->processData (locker.data, locker.data);
			renderer->renderFrame (locker.data, 0, 0);
		}
	}

	if(bitmap)
		event.graphics.drawImage (bitmap, Point ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::onMouseEnter (const MouseEvent& event)
{
	environment.setPointerPosition (event.where);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::onMouseMove (const MouseEvent& event)
{
	environment.setPointerPosition (event.where);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::onMouseDown (const MouseEvent& event)
{
	takeFocus ();
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* CCL_API GameView::createMouseHandler (const MouseEvent& event)
{
	return NEW PointerHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::onKeyDown (const KeyEvent& event)
{
	return mapJoypayKey (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::onKeyUp (const KeyEvent& event)
{
	return mapJoypayKey (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GameView::mapJoypayKey (const KeyEvent& event)
{
	int button = -1;
	switch(event.vKey)
	{
	case VKey::kLeft : 
		button = Core::kJoypadLeft;
		break;

	case VKey::kRight :
		button = Core::kJoypadRight;
		break;
	
	case VKey::kUp :
		button = Core::kJoypadUp;
		break;

	case VKey::kDown :
		button = Core::kJoypadDown;
		break;

	case VKey::kHome :
		button = Core::kJoypadStart;
		break;

	case VKey::kEnd :
		button = Core::kJoypadSelect;
		break;

	case VKey::kPageUp :
	case VKey::kShift :
		button = Core::kJoypadA;
		break;

	case VKey::kPageDown :
	case VKey::kCommand :
		button = Core::kJoypadB;
		break;

	default : // key not mapped
		return false;
	}

	bool pressed = event.eventType == KeyEvent::kKeyDown;
	environment.setJoypadButtonPressed (button, pressed);
	CCL_PRINTF ("GameView joypad button %d %s\n", button, pressed ? "down" : "up")
	return true;
}
