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
// Filename    : ccl/gui/layout/workspace.cpp
// Description : Workspace
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_TREE (1 && DEBUG_LOG)
#define DEFER_ORIENTATION_CHANGE 1

#include "ccl/gui/layout/workspace.h"
#include "ccl/gui/layout/workspaceframes.h"
#include "ccl/gui/layout/perspectiveswitcher.h"
#include "ccl/gui/layout/dividergroup.h"
#include "ccl/gui/windows/appwindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/viewanimation.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/views/focusnavigator.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/gui.h"

#include "ccl/app/params.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/base/iactivatable.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_TREE
#define LOG_ITEMS log ();
#else
#define LOG_ITEMS 
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWorkspaceManager& CCL_API System::CCL_ISOLATED (GetWorkspaceManager) ()
{
	return WorkspaceSystem::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

static StringID kPerspectiveCategory	("Perspective");
static StringID kCurrentPerspectiveID	("~");
static StringID recentPerspectiveID		("recentPerspective");
static const String strWorkspace (CCLSTR ("Workspace"));

//************************************************************************************************
// WorkspaceView
//************************************************************************************************

class WorkspaceView: public DockPanelView
{
public:
	DECLARE_CLASS_ABSTRACT (WorkspaceView, DockPanelView)

	WorkspaceView (const Rect& size)
	: DockPanelView (size)
	{}

	PROPERTY_SHARED_AUTO (VisualStyle, originalContainerStyle, OriginalContainerStyle)
	PROPERTY_OBJECT (StyleFlags, originalContainerOptions, OriginalContainerOptions)

	Workspace* getWorkspace () const
	{
		auto root = ccl_cast<RootFrameItem> (items);
		return root ? root->getWorkspace () : nullptr;
	}

	// View 
	void onSize (const Point& delta) override
	{
		if(Workspace* workspace = getWorkspace ())
			if(workspace->onSize (getSize ().getSize ()))
				return;

		SuperClass::onSize (delta);
	}

	void attached (View* parent) override
	{
		SuperClass::attached (parent);

		if(Workspace* workspace = getWorkspace ())
			workspace->applyPerspectiveStyle ();
	}
};

DEFINE_CLASS_ABSTRACT_HIDDEN (WorkspaceView, DockPanelView)

//************************************************************************************************
// WorkspaceSystem::FrameFamily
//************************************************************************************************

class WorkspaceSystem::FrameFamily
{
public:
	FrameFamily (IObjectFilter* filter);

	bool show ();
	bool hide ();
	void toggle ();

private:
	AutoPtr<IObjectFilter> filter;
	ObjectList hiddenClasses;
	MutableCString workspaceID;
	MutableCString perspectiveID;

	void reset (Workspace* workspace = nullptr);
	Workspace* findSource ();
};

//************************************************************************************************
// Workspace::ThemeScope
//************************************************************************************************

class Workspace::ThemeScope
{
public:
	ThemeScope (Workspace& workspace)
	: ThemeScope (workspace.getTheme ())
	{}

	ThemeScope (Workspace* workspace)
	: ThemeScope (workspace ? workspace->getTheme () : nullptr)
	{}

private:
	ThemeSelector themeSelector;

	ThemeScope (Theme* theme)
	: themeSelector (theme ? theme : ThemeSelector::currentTheme) // keep old currentTheme when workspace has no theme
	{}
};

//************************************************************************************************
// Frame Filters
//************************************************************************************************

struct FloatingFramesFilter: public ObjectFilter
{
	tbool CCL_API matches (IUnknown* object) const override
	{
		PopupFrameItem* frame = unknown_cast<PopupFrameItem> (object);
		return frame && frame->isVisible () && frame->getCurrentWindowClass ()
			&& !frame->isRequired ();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct OptionalFramesFilter: public ObjectFilter
{
	tbool CCL_API matches (IUnknown* object) const override
	{
		FrameItem* frame = unknown_cast<FrameItem> (object);
		return frame && frame->isReallyVisible () && frame->getCurrentWindowClass ()
			&& !frame->isRequired ();
	}
};

//************************************************************************************************
// Frame Recognizers
//************************************************************************************************

struct GroupIdRecognizer: public Recognizer
{
	StringRef groupID;
	GroupIdRecognizer (StringRef groupID) : groupID (groupID) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		FrameItem* frame = unknown_cast<FrameItem> (object);
		return frame && frame->hasGroupID (groupID) && frame->checkCondition (groupID);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct GroupIdStyleRecognizer: public GroupIdRecognizer
{
	int customFrameStyle;
	GroupIdStyleRecognizer (StringRef groupID, int customFrameStyle) 
	: GroupIdRecognizer (groupID), customFrameStyle (customFrameStyle)
	{}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		FrameItem* frame = unknown_cast<FrameItem> (object);
		bool hasCustomStyleFlag = get_flag (frame->getStyle (), customFrameStyle);
		return frame && hasCustomStyleFlag && frame->hasGroupID (groupID);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////

struct FrameIdRecognizer: public Recognizer
{
	StringRef frameID;
	FrameIdRecognizer (StringRef frameID) : frameID (frameID) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		DockPanelItem* frame = unknown_cast<DockPanelItem> (object);
		return frame && frame->getName () == frameID;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct VisibleWindowClassRecognizer: public Recognizer
{
	WindowClassRef wc;
	VisibleWindowClassRecognizer (WindowClassRef wc) : wc (wc) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		FrameItem* frame = unknown_cast<FrameItem> (object);
		const WindowClass* wc2 = frame ? frame->getCurrentWindowClass () : nullptr;
		return wc2 && wc2 == &wc && frame->isReallyVisible ();
	}
};

} // namespace CCL

//************************************************************************************************
// Perspective::CustomParams
//************************************************************************************************

class Perspective::CustomParams: public ParamContainer,
								 public AbstractNode,
								 public IParamObserver
{
public:
	CustomParams (Perspective* perspective)
	: perspective (perspective)
	{
		setController (this);
	}

	CustomParams (Perspective* perspective, const CustomParams& params)
	: perspective (perspective)
	{
		setController (this);
		addParametersFrom (params);
	}

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
	{
		// allow property urls with properties of parameters
		if(IParameter* param = findParameter (propertyId))
		{
			var = Variant (param, true);
			return true;
		}
		return Object::getProperty (var, propertyId);
	}

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override
	{
		return true;
	}

	void CCL_API paramEdit (IParameter* param, tbool begin) override
	{
		// triggering document dirty state for storable params
		if(!begin && param->isStorable ())
			perspective->signal (Message (kChanged));
	}

	CLASS_INTERFACE2 (IObjectNode, IParamObserver, ParamContainer)

private:
	Perspective* perspective;
};

//************************************************************************************************
// Perspective
//************************************************************************************************

BEGIN_STYLEDEF (Perspective::customStyles)
	{"explicit", Perspective::kExplicit},
	{"fullscreen", Perspective::kFullScreen},
	{"windowtransition", Perspective::kWindowTransition},
END_STYLEDEF

BEGIN_STYLEDEF (Perspective::orientations)
	{"landscape", Styles::kLandscape},
	{"portrait",  Styles::kPortrait},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Perspective, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective::Perspective (StringID name, RootFrameItem* rootFrame)
: name (name),
  workspace (nullptr),
  frameIDCounter (0),
  cloneCounter (0),
  lastActivated (0),
  orientation (Styles::kAnyOrientation),
  transitionType (Styles::kTransitionNone),
  fullScreenEntered (false),
  dividerGroups (nullptr),
  customParams (nullptr)
{
	setRootFrame (rootFrame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective::Perspective (const Perspective& p)
: name (p.name),
  workspace (p.getWorkspace ()),
  frameIDCounter (0),
  cloneCounter (0),
  lastActivated (0),
  style (p.style),
  orientation (p.orientation),
  transitionType (p.transitionType),
  backCommandCategory (p.backCommandCategory),
  backCommandName (p.backCommandName),
  fullScreenEntered (false),
  dividerGroups (nullptr),
  customParams (nullptr)
{
	if(p.rootFrame)
		setRootFrame ((RootFrameItem*)p.rootFrame->clone ());

	if(p.customParams)
		customParams = NEW CustomParams (this, *p.customParams);

	name.appendFormat (":%d", p.cloneCounter++); // make a unique name for the clone
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective::~Perspective ()
{
	if(dividerGroups)
		dividerGroups->release ();
	if(customParams)
		customParams->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Perspective::getOriginalID () const
{
	// remove "clone counter" (see copy constructor)
	String id (getID ());
	int index = id.lastIndex (":");
	if(index >= 0)
		id.truncate (index);
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::setRootFrame (RootFrameItem* item)
{
	ASSERT (rootFrame == nullptr)
	rootFrame = item;
	if(rootFrame)
	{
		rootFrame->setPerspective (this);
		rootFrame->isHidable (false);

		checkFrameIDs (*rootFrame);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::checkFrameIDs (FrameItem& parent)
{
	ForEach (parent, DockPanelItem, item)
		FrameItem* frameItem = ccl_cast<FrameItem> (item);
		if(frameItem)
		{
			if(frameItem->getName ().isEmpty () && !frameItem->canCast (ccl_typeid <FrameGroupItem> ()))
				frameItem->setName (newFrameID ());

			checkFrameIDs (*frameItem); // recursion
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::setActivator (IPerspectiveActivator* a)
{
	activator = a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::prepareSelect ()
{
	if(activator)
		activator->notifyPerspectiveSelected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Perspective::supportsOrientation (OrientationType orientation) const
{
	return this->orientation == orientation || this->orientation == Styles::kAnyOrientation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Perspective::load (const Storage& storage)
{
	CCL_DEBUGGER ("Should not get here, see PerspectiveState!\n")
	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Perspective::save (const Storage& storage) const
{
	CCL_DEBUGGER ("Should not get here, see PerspectiveState!\n")
	return SuperClass::save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Perspective::save (IStream& stream) const
{
	XmlArchive archive (stream);
	PerspectiveState state;
	state.store (*this);
	return archive.saveObject ("PerspectiveState", state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Perspective::load (IStream& stream)
{
	XmlArchive archive (stream);
	PerspectiveState state;
	if(!archive.loadObject ("PerspectiveState", state))
		return false;

	state.restore (*this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Perspective::initFrame (StringRef frameID, StringID windowID)
{
	if(FrameItem* frame = findFrameByID (frameID))
	{
		frame->setWindowID (windowID);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Perspective::initViewState (StringRef frameID, StringID windowID, StringID attribID, VariantRef value)
{
	if(FrameItem* frame = findFrameByID (frameID))
	{
		frame->initViewState (windowID, attribID, value);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Perspective::getFormat (FileType& format) const
{
	format = XmlArchive::getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Perspective::newFrameID ()
{
	String id ("Frame");
	id.appendIntValue (frameIDCounter++);
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* Perspective::findFrameItem (IRecognizer& recognizer)
{
	if(RootFrameItem* rootItem = getRootFrame ())
	{
		if(recognizer.recognize (rootItem->asUnknown ()))
			return rootItem;

		// try detached frames first
		if(FrameItem* detachedFrame = rootItem->findDetachedFrame (recognizer))
			return detachedFrame;

		return rootItem->findChildFrame (recognizer);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::collectFrames (Container& container, IObjectFilter& filter)
{
	if(RootFrameItem* rootItem = getRootFrame ())
	{
		if(filter.matches (rootItem->asUnknown ()))
			container.add (rootItem);

		rootItem->collectChildFrames (container, filter);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* Perspective::findFrameByID (StringRef id)
{
	FrameIdRecognizer r (id);
	return findFrameItem (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* Perspective::getLayoutState (StringRef path, bool create)
{
	if(Settings::Section* section = layoutStates.getSection (path, create))
		return &section->getAttributes ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerGroups* Perspective::getDividerGroups ()
{
	if(!dividerGroups)
	{
		dividerGroups = NEW DividerGroups;
		dividerGroups->setDirtySink (this);
	}
	return dividerGroups;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IController& CCL_API Perspective::getIDividerGroups ()
{
	return *getDividerGroups ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective::CustomParams& Perspective::getCustomParams ()
{
	if(!customParams)
		customParams = NEW CustomParams (this);
	return *customParams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IController& CCL_API Perspective::getICustomParams ()
{
	return getCustomParams ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Perspective::addCustomParam (IParameter* param)
{
	getCustomParams ().add (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspace* CCL_API Perspective::getIWorkspace () const
{
	return getWorkspace ();
}

//************************************************************************************************
// PerspectiveState
//************************************************************************************************

DEFINE_CLASS (PerspectiveState, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PerspectiveState::PerspectiveState ()
{
	states.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveState::store (const Perspective& perspective)
{
	states.removeAll ();
	perspective.getRootFrame ()->saveItemStates ();
	storeFrames (perspective.getRootFrame ());
	
	layoutStates.copyFrom (perspective.getLayoutStates ());

	// store customParams with storable flag
	const_cast<Perspective&> (perspective).getCustomParams ().storeValues (paramValues, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveState::restore (Perspective& perspective, bool checkClassIDs) const
{
	ForEach (states, FrameState, state)
		ASSERT (state->getName ().isEmpty () == false)
		if(state->getName ().isEmpty ())
			continue;

		FrameItem* frame = findFrame (perspective.getRootFrame (), state->getName ());
		if(frame)
		{
			state->restore (*frame);

			// back to default class when a stored window class does not exist
			if(checkClassIDs)
			{
				Workspace::ThemeScope scope (perspective.getWorkspace ());
				if(!WindowManager::instance ().getClass (frame->getWindowID ()))
					frame->setWindowID (frame->getDefaultWindowID ());
			}
		}
	EndFor

	perspective.getLayoutStates ().copyFrom (layoutStates);

	// restore customParams with storable flag
	perspective.getCustomParams ().restoreValues (paramValues, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveState::storeFrames (FrameItem* parent)
{
	IterForEach (parent->newIterator (), DockPanelItem, item)
		if(FrameItem* frame = ccl_cast<FrameItem> (item))
		{
			if(!frame->getName ().isEmpty ())
			{
				FrameState* state = NEW FrameState;
				state->store (*frame);
				states.add (state);
			}
			storeFrames (frame);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveState::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	FrameState* state;
	while((state = a.unqueueObject<FrameState> (nullptr)) != nullptr)
		states.add (state);
	
	a.get (name, "name");
	a.get (layoutStates, "layout");
	a.get (paramValues, "params");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveState::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	ForEach (states, FrameState, state)
		a.queue (nullptr, state);
	EndFor

	if(!name.isEmpty ())
		a.set ("name", name);
	a.set ("layout", layoutStates);
	a.set ("params", paramValues);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* PerspectiveState::findFrame (FrameItem* parent, StringRef name) const
{
	IterForEach (parent->newIterator (), DockPanelItem, item)
		if(FrameItem* frame = ccl_cast<FrameItem> (item))
		{
			if(frame->getName () == name)
				return frame;

			FrameItem* result = findFrame (frame, name);
			if(result)
				return result;
		}
	EndFor
	return nullptr;
}

//************************************************************************************************
// PerspectiveState::FrameState
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (PerspectiveState::FrameState, Object, "PerspectiveFrameState")

//////////////////////////////////////////////////////////////////////////////////////////////////

PerspectiveState::FrameState::FrameState ()
: visible (false),
  detached (false),
  zIndex (-1)
{
	viewStates.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveState::FrameState::store (const FrameItem& item)
{
	DockPanelItem* parentItem = item.getParentItem ();
	bool insideMultiFrame = parentItem && parentItem->canCast (ccl_typeid<MultiFrameItem>());

	if(insideMultiFrame)
		name = parentItem->getName ();
	else
		name = item.getName ();

	visible = item.isVisible ();
	windowID = item.getWindowID ();

	if(!insideMultiFrame) // MultiFrameItem now stores the shared viewsStates in a separare FrameState, childs don't store redundant viewStates
	{
		FrameItem& mutableItem (const_cast<FrameItem&> (item));
		mutableItem.saveViewState ();
		viewStates.removeAll ();
		viewStates.add (mutableItem.getViewStates (), Container::kClone);
	}

	zIndex = -1;
	detached = false;
	if(const PopupFrameItem* popupItem = ccl_cast<PopupFrameItem> (&item))
	{
		zIndex = popupItem->getZIndex ();
		if(const DetachedFrameItem* detachedItem = ccl_cast<DetachedFrameItem> (&item))
			detached = detachedItem->isDetached ();
	}
	CCL_PRINTF ("FrameState:store %s, %s\n", MutableCString (name).str (), MutableCString (windowID). str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveState::FrameState::restore (FrameItem& item) const
{
	MultiFrameItem* multiFrame = ccl_cast<MultiFrameItem> (&item);
	if(multiFrame && !windowID.isEmpty ()) // don't create new child for the multiFrame state, it only contains shared viewStates
	{
		FrameItem& childItem = multiFrame->newChildItem ();
		restore (childItem);

		if(childItem.isDetachedFrame () && item.isPinnable ())
		{
			if(childItem.wasPinned (windowID))
				childItem.setPinned (true);

			// might have to add another child as detach-target (if all other child instances are pinned)
			multiFrame->updateDetachedChilds ();
		}
	}
	else
	{
		CCL_PRINTF ("FrameState:restore %s, %s\n", MutableCString (name).str (), MutableCString (windowID). str ());

		item.isVisible (visible);
		item.setWindowID (windowID);

		if(!viewStates.isEmpty ())
		{
			item.getViewStates ().removeAll ();
			item.getViewStates ().add (viewStates, Container::kClone);
		}

		if(zIndex != -1)
			if(PopupFrameItem* popupItem = ccl_cast<PopupFrameItem> (&item))
				popupItem->setZIndex (zIndex);

		if(detached)
		{
			if(DetachedFrameItem* detachedItem = ccl_cast<DetachedFrameItem> (&item))
				detachedItem->setDetached (true);
			else if(multiFrame)
				multiFrame->restoreDetachedChildState ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveState::FrameState::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	name = a.getString ("name");
	visible = a.getBool ("visible");
	detached = a.getBool ("detached");
	windowID = a.getString ("windowID");
	a.get (viewStates, "viewStates");
	zIndex = a.getInt ("Z");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveState::FrameState::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("name", name);
	a.set ("visible", visible);
	a.set ("detached", detached);
	a.set ("windowID", windowID);
	a.set ("viewStates", viewStates);
	if(zIndex != -1)
		a.set ("Z", zIndex);
	return true;
}

//************************************************************************************************
// WorkspaceSystem::FrameFamily
//************************************************************************************************

WorkspaceSystem::FrameFamily::FrameFamily (IObjectFilter* filter)
: filter (filter)
{
	hiddenClasses.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::FrameFamily::reset (Workspace* workspace)
{
	hiddenClasses.removeAll ();

	if(workspace)
	{
		Perspective* perspective = workspace->getCurrentPerspective ();
		workspaceID = workspace->getID ();
		perspectiveID = perspective ? perspective->getID () : CString::kEmpty;
	}
	else
	{
		workspaceID.empty ();
		perspectiveID.empty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace* WorkspaceSystem::FrameFamily::findSource ()
{
	// find source workspace & perspective
	if(Workspace* workspace = unknown_cast<Workspace> (WorkspaceSystem::instance ().getWorkspace (workspaceID)))
		if(Perspective* perspective = workspace->getCurrentPerspective ())
			if(perspective->getID () == perspectiveID)
				return workspace;
	return nullptr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::FrameFamily::show ()
{
	if(!hiddenClasses.isEmpty ())
	{
		// try to show the hidden classes in the source workspace
		if(Workspace* workspace = findSource ())
		{
			MultiFrameItem::suspendReuse (true);
			bool didOpen = false;

			ForEach (hiddenClasses, WindowClass, wc)
				// check if class is still registered
				if(WindowManager::instance ().isClassRegistered (wc))
				{
					bool wasOpen = false;
					if(!didOpen) // only check if necessary
						wasOpen = workspace->isViewOpen (*wc);

					if(workspace->openView (*wc) && !wasOpen)
						didOpen = true;
				}
			EndFor
	
			MultiFrameItem::suspendReuse (false);

			if(didOpen)
			{
				reset ();
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::FrameFamily::hide ()
{
	ObjectList frames;

	// try all workspaces
	ForEach (WorkspaceSystem::instance (), Workspace, workspace)
		workspace->collectFrames (frames, *filter);
		if(!frames.isEmpty ())
		{
			reset (workspace);

			ForEach (frames, FrameItem, frame)
				WindowClass* wc = const_cast<WindowClass*> (frame->getCurrentWindowClass ());
				ASSERT (wc) // filter must not match frames without a current window class
				if(wc)
				{
					wc->retain ();
					hiddenClasses.add (wc);
					workspace->closeView (*wc);
				}
			EndFor
			return !hiddenClasses.isEmpty ();
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::FrameFamily::toggle ()
{
	if(!hide ())
		show ();
}

//************************************************************************************************
// WorkspaceSystem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WorkspaceSystem, WindowSystem)
DEFINE_SINGLETON (WorkspaceSystem)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (WorkspaceSystem, kFrameworkLevelFirst)
{
	WorkspaceSystem* ws = WorkspaceSystem::peekInstance ();
	if(ws)
		System::GetObjectTable ().unregisterObject (ws->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkspaceSystem::WorkspaceSystem ()
: floatingFamily (nullptr),
  optionalFamily (nullptr)
{
	workspaces.objectCleanup (true);

	System::GetObjectTable ().registerObject (this->asUnknown (), kNullUID, "Workspace");

	SignalSource::addObserver (Signals::kGUI, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkspaceSystem::~WorkspaceSystem ()
{
	SignalSource::removeObserver (Signals::kGUI, this);

	delete floatingFamily;
	delete optionalFamily;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::addWorkspace (Workspace* workspace)
{
	CCL_PRINTF ("addWorkspace: %s (%s)\n", workspace->getID ().str (), workspace->getTheme () ? workspace->getTheme ()->getThemeID ().str () : "")
	if(workspaces.isEmpty ())
		WindowManager::instance ().setWindowSystem (this);

	workspace->retain ();
	workspaces.add (workspace);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::removeWorkspace (Workspace* workspace)
{
	CCL_PRINTF ("removeWorkspace: %s (%s)\n", workspace->getID ().str (), workspace->getTheme () ? workspace->getTheme ()->getThemeID ().str () : "")
	if(workspaces.remove (workspace))
		workspace->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace* WorkspaceSystem::findTopLevelWorkspace (StringID workspaceId, Theme* referenceTheme) const
{
	Workspace* foundWorkspace = nullptr;

	for(auto* workspace : iterate_as<Workspace> (workspaces))
		if(workspace->getID () == workspaceId)
		{
			// prefer workspace with matching theme, but use first other workspace as fallback
			if(!referenceTheme || referenceTheme == workspace->getTheme ())
				return workspace;
			else if(!foundWorkspace)
				foundWorkspace = workspace;
		}

	return foundWorkspace;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspace* CCL_API WorkspaceSystem::cloneWorkspace (StringID workspaceID, IUnknown* context)
{
	// get theme for module for disambiguation
	UnknownPtr<IObject> contextObj (context);
	ModuleRef contextModule = contextObj ? contextObj->getTypeInfo ().getModuleReference () : nullptr;
	Theme* contextTheme = contextModule ? unknown_cast<Theme> (ThemeManager::instance ().getModuleTheme (contextModule)) : nullptr;

	// only clone toplevel workspaces (don't use getWorkspace)
	Workspace* workspace = findTopLevelWorkspace (workspaceID, contextTheme);
	if(workspace)
	{
		WorkspaceInstance* newWorkspace = NEW WorkspaceInstance (*workspace);
		newWorkspace->setContext (context);
		workspace->addInstance (newWorkspace);
		return newWorkspace;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WorkspaceSystem::removeWorkspaceInstance (IWorkspace* workspace)
{
	WorkspaceInstance* instance = unknown_cast<WorkspaceInstance> (workspace);
	ASSERT (instance)
	if(instance)
		instance->getPrototype ().removeInstance (instance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspace* CCL_API WorkspaceSystem::getWorkspace (StringID _workspaceID) const
{
	MutableCString workspaceID (_workspaceID);
	
	if(workspaceID == IObjectTable::kHostApp)
		if(auto app = GUI.getApplication ())
			workspaceID = app->getApplicationID ();

	Workspace* workspace = findTopLevelWorkspace (workspaceID, ThemeSelector::currentTheme);
	if(workspace)
	{
		if(workspace->countInstances () > 0)
			return workspace->getActiveInstance ();
		else
			return workspace;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspective* CCL_API WorkspaceSystem::getPerspectiveFromView (IView* _view)
{
	View* view = unknown_cast<View> (_view);
	FrameView* frameView = ccl_cast<FrameView> (view);
	if(frameView == nullptr && view)
		frameView = view->getParent<FrameView> ();

	return frameView ? frameView->getFrameItem ()->getPerspective () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace* WorkspaceSystem::getWorkspace (WindowClassRef windowClass)
{
	return unknown_cast<Workspace> (getWorkspace (windowClass.getWorkspaceID ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::openWindow (WindowClassRef windowClass)
{
	if(Workspace* workspace = getWorkspace (windowClass))
	{
		if(workspace->isRestoringViews ())
			return false;

		GUI.hideTooltip ();
		if(workspace->openView (windowClass))
			return true;

		return false; // there is no matching frame for this window in the current perspective
	}
	else
		return desktopSystem.openWindow (windowClass); // no workspace: open on Desktop
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::replaceWindow (WindowClassRef oldClass, WindowClassRef newClass)
{
	Workspace* workspace = getWorkspace (newClass);
	return workspace ? workspace->replaceView (oldClass, newClass) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::centerWindow (WindowClassRef windowClass)
{
	if(Workspace* workspace = getWorkspace (windowClass))
		return workspace->centerView (windowClass);

	return desktopSystem.centerWindow (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::canReuseWindow (WindowClassRef oldClass)
{
	Workspace* workspace = getWorkspace (oldClass);
	return workspace && workspace->canReuseView (oldClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::closeWindow (WindowClassRef windowClass)
{
	if(Workspace* workspace = getWorkspace (windowClass))
		return workspace->closeView (windowClass);

	return desktopSystem.closeWindow (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::canOpenWindow (WindowClassRef windowClass)
{
	if(Workspace* workspace = getWorkspace (windowClass))
		return workspace->canOpenView (windowClass);

	return desktopSystem.canOpenWindow (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::isWindowOpen (WindowClassRef windowClass)
{
	if(Workspace* workspace = getWorkspace (windowClass))
		return workspace->isViewOpen (windowClass);

	return desktopSystem.isWindowOpen (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::storeWindowStates (Settings& settings)
{
	// store states of (storable) workspaces
	ForEach (workspaces, Workspace, workspace)
		workspace->store (settings);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceSystem::restoreWindowStates (Settings& settings)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WorkspaceSystem::checkCommandCategory (CStringRef category) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_COMMAND_ARGS ("View", "Toggle Floating Windows", 0, "Show")
REGISTER_COMMAND_ARGS ("View", "Toggle Optional Views", 0, "Show")
REGISTER_COMMAND_ARGS ("View", "Pin Editor", 0, "State")
REGISTER_COMMAND ("View", "Next Perspective")
REGISTER_COMMAND ("View", "Previous Perspective")
REGISTER_COMMAND_ARGS ("View", "Focus Frame", CommandFlags::kHidden, "Frame, Workspace")

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WorkspaceSystem::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "View")
	{
		if(!msg.checkOnly ())
			if(Desktop.closePopupAndDeferCommand (this, msg))
				return true;

		if(msg.name == "Toggle Floating Windows")
			return onToggleFloating (msg);

		if(msg.name == "Toggle Optional Views")
			return onToggleOptional (msg);

		if(msg.name == "Next Perspective" || msg.name == "Previous Perspective")
		{
			if(msg.checkOnly ())
				return true;

			Workspace* workspace = (Workspace*)workspaces.at (0); // todo: route commands via WorkspaceView 
			AutoPtr<PerspectiveSwitcher> switcher (NEW PerspectiveSwitcher (workspace));
			switcher->run (msg.name == "Next Perspective");
			return true;
		}

		if(msg.name == "Focus Frame")
			return onFocusFrame (msg);

		if(msg.name == "Pin Editor")
			return onPinFrame (msg);

		// command "Toggle Detached xxx" with window class id
		static StringID strToggleDetach ("Toggle Detached ");
		if(msg.name.startsWith (strToggleDetach))
		{
			// find windowClass for msg.category
			MutableCString windowID (msg.name.subString (strToggleDetach.length ()));
			if(WindowManager::instance ().getClass (windowID))
			{
				const WindowClass* wc = nullptr;

				// find a DetachedFrame with the given groupID in the current perspective of any workspace, ignore pinned
				AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* obj)
				{
					DetachedFrameItem* frame = unknown_cast<DetachedFrameItem> (obj);
					return frame && frame->hasGroupID (wc->getGroupID ()) && !frame->isPinned ();
				}));

				ForEach (workspaces, Workspace, workspace)
					Workspace::ThemeScope scope (*workspace);
					wc = WindowManager::instance ().getClass (windowID);
					if(!wc)
						continue;

					if(DetachedFrameItem* detachedFrame = ccl_cast<DetachedFrameItem> (workspace->findFrameItem (*recognizer)))
					{
						if(!msg.checkOnly ())
						{
							bool state = !detachedFrame->isDetached ();
							CommandAutomator::Arguments (msg).getBool ("State", state);

							detachedFrame->setDetached (state);
						}
						return true;
					}
				EndFor
			}
		}

		// command "Perspective:workspaceID/perspectiveID/windowClassID" (optional window class id)
		static StringID strPerspective ("Perspective:");
		if(msg.name.startsWith (strPerspective))
		{
			String path (msg.name.subString (strPerspective.length ()));
			uchar delimiter = 0;
			MutableCString id;
			AutoPtr<IStringTokenizer> tokenizer (path.tokenize (CCLSTR ("/")));
			if(tokenizer && !tokenizer->done ())
			{
				Workspace* workspace = unknown_cast<Workspace> (getWorkspace (id = tokenizer->nextToken (delimiter)));
				if(workspace && !tokenizer->done ())
				{
					IPerspective* perspective = workspace->getPerspective (id = tokenizer->nextToken (delimiter));
					if(perspective)
					{
						if(!msg.checkOnly ())
						{
							workspace->selectPerspective (perspective);

							if(!tokenizer->done ())
								workspace->openView (id = tokenizer->nextToken (delimiter));
						}
						return true;
					}
				}
			}
		}
	}
	else if(msg.category == "Navigation")
	{
		if(msg.name == "Back")
			return onNavigationBack (msg);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::onToggleFloating (const CommandMsg& args)
{
	if(!args.checkOnly ())
	{
		if(!floatingFamily)
			floatingFamily = NEW FrameFamily (NEW FloatingFramesFilter);

		bool show = false;
		if(CommandAutomator::Arguments (args).getBool ("Show", show))
		{
			if(show)
				floatingFamily->show ();
			else
				floatingFamily->hide ();
		}
		else
			floatingFamily->toggle ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::onToggleOptional (const CommandMsg& args)
{
	if(!args.checkOnly ())
	{
		if(!optionalFamily)
			optionalFamily = NEW FrameFamily (NEW OptionalFramesFilter);

		bool show = false;
		if(CommandAutomator::Arguments (args).getBool ("Show", show))
		{
			if(show)
				optionalFamily->show ();
			else
				optionalFamily->hide ();
		}
		else
			optionalFamily->toggle ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::onPinFrame (const CommandMsg& args)
{
	auto tryWindow = [] (Window* window) -> PopupFrameItem*
	{
		PopupFrameItem* frame = PopupFrameItem::fromWindow (window);
		return frame && frame->isPinnable () ? frame : nullptr;
	};

	// find pinnable frame with open window: 1.) try active window, 2.) try topmost floating windows
	PopupFrameItem* frame = tryWindow (Desktop.getActiveWindow ());
	if(!frame)
	{
		for(auto layer : { kWindowLayerFloating, kWindowLayerIntermediate })
			if(frame = tryWindow (Desktop.getTopWindow (layer)))
				break;
	}

	if(frame)
	{
		if(!args.checkOnly ())
		{
			bool state = !frame->isPinned ();
			CommandAutomator::Arguments (args).getBool ("State", state);
			frame->setPinned (state);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::onFocusFrame (const CommandMsg& args)
{
	String frameID = CommandAutomator::Arguments (args).getString ("Frame");
	if(!frameID.isEmpty ())
	{
		if(!args.checkOnly ())
		{
			// lookup specified workspace or use first as default
			MutableCString workspaceID = CommandAutomator::Arguments (args).getString ("Workspace");
			Workspace* workspace = workspaceID.isEmpty () ? (Workspace*)workspaces.at (0) : unknown_cast<Workspace> (getWorkspace (workspaceID));
			if(!workspace)
				return false;

			// find frame in current perspective of that workspace
			Perspective* perspective = workspace->getCurrentPerspective ();
			FrameItem* frame = perspective ? perspective->findFrameByID (frameID) : nullptr;
			WindowBase* windowBase = frame ? ccl_cast<WindowBase> (frame->getView ()) : nullptr;
			if(windowBase && windowBase->canActivate ())
			{
				windowBase->activate ();
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::onNavigationBack (const CommandMsg& args)
{
	bool result = false;

	if(Window* window = Desktop.getActiveWindow ())
	{
		// try all workspaces in active window
		AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* obj)
		{
			if(auto workspaceView = unknown_cast<WorkspaceView> (obj))
			{
				// try "back" command provided by current perspective
				Workspace* workspace = workspaceView->getWorkspace ();
				Perspective* perspective = workspace ? workspace->getCurrentPerspective () : nullptr;
				if(perspective
					&& !perspective->getBackCommandCategory ().isEmpty ()
					&& !perspective->getBackCommandName ().isEmpty ())
				{
					CommandMsg msg (perspective->getBackCommandCategory (), perspective->getBackCommandName (), nullptr, args.flags);
					result = CommandTable::instance ().interpretCommand (msg);
					return result;
				}
			}
			return false;
		}));
		window->findView (*recognizer);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceSystem::makeFrameUrl (String& string, const FrameItem& frameItem)
{
	string.empty ();
	RootFrameItem* rootFrame = frameItem.getRootFrame ();
	if(rootFrame)
	{
		Perspective* perspective = rootFrame->getPerspective ();
		if(perspective)
		{
			Workspace* workspace = perspective->getWorkspace ();
			if(workspace)
			{
				if(WorkspaceInstance* instance = ccl_cast<WorkspaceInstance> (workspace))
					workspace = &instance->getPrototype (); // use workspace id of prototype (url refers to active instance)

				string << "object://Workspace/"
					<< workspace->getID () << "/"
					<< perspective->getID () << "/"
					<< frameItem.getName ();
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API WorkspaceSystem::lookupChild (StringRef path) const
{
	// find "workspaceID/perspectiveID/frameID"
	uchar delimiter = 0;
	MutableCString id;
	AutoPtr<IStringTokenizer> tokenizer (path.tokenize (CCLSTR ("/")));
	if(tokenizer && !tokenizer->done ())
	{
		Workspace* workspace = unknown_cast<Workspace> (getWorkspace (id = tokenizer->nextToken (delimiter)));
		if(workspace)
		{
			if(tokenizer->done ())
				return workspace;
			else
			{
				id = tokenizer->nextToken (delimiter);
				Perspective* perspective = workspace->getPerspective (id);
				if(perspective)
				{
					if(!tokenizer->done ())
					{
						StringRef frameID = tokenizer->nextToken (delimiter);
						if(frameID == "DividerGroups")
							return perspective->getDividerGroups ();
						else if(frameID == "CustomParams")
						{
							if(tokenizer->done ())
								return &perspective->getCustomParams ();
							else
								return perspective->getCustomParams ().lookupChild (tokenizer->nextToken (delimiter));
						}
						else
						{
							FrameIdRecognizer r (frameID);
							if(RootFrameItem* rootItem = perspective->getRootFrame ())
								return rootItem->findChildItem (r);
						}
					}
				}
				else if(id == "context")
				{
					if(WorkspaceInstance* instance = ccl_cast<WorkspaceInstance> (workspace))
					{
						UnknownPtr<IObjectNode> node (instance->getContext ());
						while(node && !tokenizer->done ())
							node = node->findChild (tokenizer->nextToken (delimiter));

						return node;
					}
				}
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WorkspaceSystem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kOrientationChanged)
	{
		OrientationType newOrientation = msg[0].asInt ();

		// pass to all workspaces / instances
		ForEach (workspaces, Workspace, workspace)
			if(workspace->countInstances () == 0)
				workspace->onOrientationChanged (newOrientation);
			else
				IterForEach (workspace->getInstances (), Workspace, instance)
					instance->onOrientationChanged (newOrientation);
				EndFor
		EndFor
	}
}

//************************************************************************************************
// Workspace::PerspectiveContainer
//************************************************************************************************

class Workspace::PerspectiveContainer: public WindowBase
{
public:
	typedef WindowBase SuperClass;

	PerspectiveContainer (const Rect& size = Rect (), StyleRef style = 0)
	: WindowBase (size, style)
	{}

	PROPERTY_SHARED_AUTO (Workspace, workspace, Workspace)

	// WindowBase
	IUnknown* CCL_API getController () const override
	{
		WorkspaceInstance* instance = ccl_cast<WorkspaceInstance> (workspace);
		return instance ? instance->getContext () : nullptr;
	}

	void removed (View* parent) override
	{
		// hide all frames (reset window states)
		if(workspace)
			if(Perspective* p = workspace->getCurrentPerspective ())
				if(FrameItem* rootFrame = p->getRootFrame ())
					rootFrame->hideAll ();

		SuperClass::removed (parent);
	}

	void onActivate (bool state) override
	{
		SuperClass::onActivate (state);

		UnknownPtr<IActivatable> activatable (ccl_as_unknown (workspace));
		if(activatable)
		{
			if(state)
				activatable->activate ();
			else
				activatable->deactivate ();
		}
	}
};

//************************************************************************************************
// Workspace
//************************************************************************************************

DEFINE_CLASS (Workspace, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace::Workspace ()
: currentPerspective (nullptr),
  workspaceMenu (nullptr),
  instances (nullptr),
  cloneCounter (0),
  theme (nullptr),
  windowStyle (Styles::panelWindowStyle),
  restoringViews (false),
  storable (false)
{
	perspectives.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace::Workspace (const Workspace& w)
: id (w.id),
  currentPerspective (nullptr),
  workspaceMenu (nullptr),
  instances (nullptr),
  cloneCounter (0),
  theme (w.theme),
  windowStyle (w.windowStyle),
  restoringViews (false),
  storable (false)
{
	ASSERT (w.dockPanelView == nullptr)
	perspectives.objectCleanup (true);

	ForEach (w.perspectives, Perspective, p)
		Perspective* newPerspective = (Perspective*)p->clone ();
		newPerspective->setName (p->getName ()); // keep orignal name without clone counter
		addPerspective (newPerspective);
	EndFor

	id.appendFormat (":%d", w.cloneCounter++); // make a unique name for the clone
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace::~Workspace ()
{
	if(instances)
		instances->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::addInstance (WorkspaceInstance* instance)
{
	if(!instances)
	{
		instances = NEW ObjectList;
		instances->objectCleanup ();
	}
	instances->add (instance);

	WindowManager::instance ().registerWorkspaceInstance (getID (), instance->getID (), instance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::removeInstance (Workspace* instance)
{
	ASSERT (instances && instances->contains (instance))
	WindowManager::instance ().unregisterWorkspaceInstance (getID (), instance->getID ());

	if(instances && instances->remove (instance))
		instance->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Workspace::activateInstance (IWorkspace* instance)
{
	if(instances)
	{
		Workspace* w = unknown_cast<Workspace> (instance);
		bool found = instances->remove (w);
		ASSERT (found)
		if(found)
			instances->prepend (w);

		WindowManager::instance ().onWorkspaceInstanceActivated (getID (), w->getID ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspace* CCL_API Workspace::getActiveInstance () const
{
	return instances ? (Workspace*)instances->getFirst () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Workspace::getInstances () const
{
	return instances ? instances->newIterator () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Workspace::getSettingsID () const
{
	return String ("Workspace/") << id; // section path inside WindowState settings
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::store (Settings& settings)
{
	if(isStorable ())
	{
		Attributes& attribs (settings.getAttributes (getSettingsID ()));
		attribs.remove ("perspectives");

		// store state of all perspectives
		ForEach (perspectives, Perspective, perspective)
			PerspectiveState* state = NEW PerspectiveState;
			state->setName (perspective->getName ());
			state->store (*perspective);
			attribs.queue ("perspectives", state, Attributes::kOwns);
		EndFor

		#if 0
		if(currentPerspective)
			attribs.set ("current", currentPerspective->getName ());
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::restore (Settings& settings)
{
	if(isStorable ())
	{
		Attributes& attribs (settings.getAttributes (getSettingsID ()));

		// restore state of all perspectives, but don't open windows (might need another option for the latter)
		while(AutoPtr<PerspectiveState> state = attribs.unqueueObject<PerspectiveState> ("perspectives"))
			if(Perspective* perspective = getPerspective (state->getName ()))
				state->restore (*perspective, true); // check classIDs

		#if 0
		String currentName;
		attribs.get ("current", currentName);
		if(!currentName.isEmpty ())
			selectPerspective (MutableCString (currentName));
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::addPerspective (Perspective* perspective)
{
	perspective->setWorkspace (this);
	perspectives.add (perspective);
	if(!currentPerspective && perspective->supportsOrientation (GUI.getInterfaceOrientation ()))
		selectPerspective (perspective);

	addMenuItem (perspective);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective* Workspace::getPerspective (StringID name)
{
	if(name == kCurrentPerspectiveID)
		return getCurrentPerspective ();

	if(name == recentPerspectiveID)
	{
		OrientationType orientation = GUI.getInterfaceOrientation ();
		return getRecentPerspective (*AutoPtr<ObjectFilter> (ObjectFilter::create ([&] (IUnknown* object)
			{
				Perspective* p = unknown_cast<Perspective> (object);
				return p != currentPerspective && p->supportsOrientation (orientation);
			})));
	}

	ForEach (perspectives, Perspective, p)
		if(p->getName () == name)
			return p;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective* Workspace::getRecentPerspective (IObjectFilter& filter) const
{
	Perspective* matchingPerspective = nullptr;

	ForEach (perspectives, Perspective, p)
		if(filter.matches (p->asUnknown ()))
			if(!matchingPerspective || p->getLastActivated () > matchingPerspective->getLastActivated ())
				matchingPerspective = p;
	EndFor
	return matchingPerspective;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspective* Workspace::getRecentIPerspective (IObjectFilter& filter) const
{
	return getRecentPerspective (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective* Workspace::getCurrentPerspective ()
{
	return currentPerspective;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootFrameItem* Workspace::getRootFrame ()
{
	Perspective* p = getCurrentPerspective ();
	return p ? p->getRootFrame () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::selectPerspective (StringID perspectiveID)
{
	return selectPerspective (getPerspective (perspectiveID));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::selectPerspective (IPerspective* perspective)
{
	return selectPerspective (unknown_cast<Perspective> (perspective));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Workspace::getSelectedPerspectiveID () const
{
	if(currentPerspective)
		return currentPerspective->getID ();
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::selectPerspective (Perspective* p)
{
	if(p)
	{
		if(p != currentPerspective)
		{
			bool mustLeaveFullScreen = false;
			if(currentPerspective)
			{
				mustLeaveFullScreen = currentPerspective->isFullScreenEntered ();
				currentPerspective->signal (Message (IPerspective::kPerspectiveSelected, false));

				signal (Message (kPerspectiveSelected, false, currentPerspective->getOriginalID (), currentPerspective->getLastActivated ()));
			}

			p->prepareSelect ();

			Window* window = getWorkspaceWindow ();
			Window::UpdateCollector uc (window);
			
			AutoPtr<ViewAnimator> animator;

			DockPanelItem* rootFrame = getRootFrame ();
			if(rootFrame)
			{
				auto shouldAnimate = [] () ->bool
				{
					if(auto app = GUI.getApplication ())
						return app->isQuitRequested () == 0;
					return false;
				};
									
				if(p->getTransitionType () != Styles::kTransitionNone && currentPerspective && dockPanelView && rootFrame->getView () && shouldAnimate ())
				{
					View* animationParent = getDockPanelView ();
					View* animationView = rootFrame->getView ();

					if(animationParent && p->getStyle ().isCustomStyle (Perspective::kWindowTransition))
						animationView = animationParent = animationParent->getWindow ();

					animator = ViewAnimator::create (animationParent, p->getTransitionType ());
					if(animator)
					{
						animator->snipFromView (animationView);
					
						// dockPanel center as default "fromRect"
						Rect dockPanelClient;
						getDockPanelView ()->getClientRect (dockPanelClient);
						Rect fromRect (0, 0, 10, 10);
						fromRect.center (dockPanelClient);
						animator->setFromRect (fromRect);
						animator->setDuration (0.4);
					
						// give event handler a chance to modify the appearance
						if(IWorkspaceEventHandler* eventHandler = getEventHandler ())
						{
							WorkspaceEvent e (WorkspaceEvent::kCloseView, rootFrame->getView ());
							e.windowClass = WindowManager::instance ().getCurrentWindowClass ();
							e.arguments = WindowManager::instance ().getCurrentArguments ();
							e.animator = animator;

							eventHandler->onWorkspaceEvent (e);
						}
					}
				}

				// "freeze" DockPanelView sizeLimits during changes (make current calculated limits explicit)
				if(DockPanelView* view = getDockPanelView ())
				{
					view->setSizeLimits (view->getSizeLimits ());

					if(Window* window = view->getWindow ())
						window->getTouchInputState ().discardTouchesForView (*view);
				}
				rootFrame->hideAll ();
			}
			bool isSkinReload = currentPerspective && currentPerspective->getID ().startsWith ("cclspy:SkinRefresh");

			currentPerspective = p;
			p->setLastActivated (System::GetSystemTicks ());

			rootFrame = getRootFrame ();
			if(rootFrame && rootFrame->countChildren () == 0)
				rootFrame->isHidable (true);

			connectDockPanelView ();
			applyPerspectiveStyle ();

			// give deferred layout tasks a chance to perform (SizeVariantLayout)
			System::GetSignalHandler ().flush ();

			// reset frozen limits
			if(DockPanelView* view = getDockPanelView ())
			{
				view->resetSizeLimits ();
				if(View* parent = view->getParent ())
					parent->onChildLimitsChanged (view);

				// enter / leave fullscreen
				if(window)
				{
					if(currentPerspective && currentPerspective->getStyle ().isCustomStyle (Perspective::kFullScreen))
					{
						bool wasFullScreen = window->setFullscreen (true);
						currentPerspective->setFullScreenEntered (!wasFullScreen);
					}
					else
					{
						// only leave fullscreen if the old perspective entered it
						if(mustLeaveFullScreen)
							window->setFullscreen (false);
						
						if(currentPerspective)
							currentPerspective->setFullScreenEntered (false);
					}
				}
			}

			if(animator && rootFrame && rootFrame->getView ())
			{
				View* animationView = rootFrame->getView ();
				if(animationView && p->getStyle ().isCustomStyle (Perspective::kWindowTransition))
					animationView = animationView->getWindow ();

				animator->snipToView (animationView);
				animator->makeTransition ();
			}
		
			// some assistance for reloading skin when there is no DockPanelView
			if(isSkinReload && getDockPanelView () == nullptr)
				if(RootFrameItem* rootFrame = getRootFrame ())
				{
					ScopedVar<bool> guard (restoringViews, true);
					rootFrame->restoreViews ();
				}

			// the above "flush" might have preponed a deferred master/slave divider sync too early: trigger it again
			struct SyncDividers: public IDockPanelItemVisitor
			{
				void visit (DockPanelItem& item) override
				{ 
					if(DividerItem* dividerItem = ccl_cast<DividerItem> (&item))
						dividerItem->checkSyncSlaves ();
				}
			} syncDividers;
			if(rootFrame)
				rootFrame->traverse (syncDividers);
		}
		
		if(currentPerspective)
			currentPerspective->signal (Message (IPerspective::kPerspectiveSelected, true));

		signal (Message (kPerspectiveSelected, true, currentPerspective->getOriginalID (), currentPerspective->getLastActivated ()));

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelView* Workspace::getDockPanelView ()
{
	return unknown_cast<DockPanelView> (dockPanelView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::applyPerspectiveStyle ()
{
	auto* workspaceView = ccl_cast<WorkspaceView> (getDockPanelView ());
	ImageView* styleTargetView = workspaceView;

	// WindowManager wraps our view (from createWorkspaceView) with another container ImageView that includes safe areas
	IApplication* application = GUI.getApplication ();
	if(application && application->getApplicationID () == getID ())
		if(auto* containerView = ccl_cast<ImageView> (WindowManager::instance ().getApplicationContainerView ()))
			styleTargetView = containerView;

	if(styleTargetView)
	{
		VisualStyle* perspectiveStyle = nullptr;
		StyleFlags backgroundOptions;

		if(workspaceView)
		{
			if(workspaceView->getOriginalContainerStyle () == nullptr)
			{
				// first time: remember original style / options of target view in our workspace view (note: this could be VisualStyle::emptyStyle, but never nullptr)
				workspaceView->setOriginalContainerStyle (unknown_cast<VisualStyle> (&styleTargetView->getVisualStyle ()));
				workspaceView->setOriginalContainerOptions (styleTargetView->getStyle ());
			}

			// use the original style as fallback
			perspectiveStyle = workspaceView->getOriginalContainerStyle ();
			backgroundOptions = workspaceView->getOriginalContainerOptions ();
		}

		if(currentPerspective)
		{
			if(currentPerspective->getVisualStyle ())
				perspectiveStyle = currentPerspective->getVisualStyle ();

			// combine flags iof original options with perspective options
			backgroundOptions.setCommonStyle (currentPerspective->getBackgroundOptions ().common);
			backgroundOptions.setCustomStyle (currentPerspective->getBackgroundOptions ().custom);
		}

		styleTargetView->setVisualStyle (perspectiveStyle);
		styleTargetView->setStyle (backgroundOptions);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::connectDockPanelView ()
{
	DockPanelView* view = getDockPanelView ();
	RootFrameItem* rootFrame = getRootFrame ();
	if(view && rootFrame)
	{
		view->setItems (rootFrame);

		if(rootFrame->getView ())
		{
			Rect r;
			view->getParent ()->getClientRect (r);
			view->setSize (r);
			rootFrame->getView ()->setSize (r);
		}

		ASSERT (!restoringViews)
		restoringViews = true;
		rootFrame->restoreViews ();
		restoringViews = false;

		LOG_ITEMS
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::addMenuItem (Perspective* perspective)
{
	IPerspectiveActivator* activator = perspective->getActivator ();
	if(activator && workspaceMenu)
	{
		String cmdName ("Show ");
		cmdName.append (perspective->getName ().str ());
		workspaceMenu->addItem (cmdName, activator->getPerspectiveTitle ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::buildMenu ()
{
	if(!workspaceMenu)
	{
		if(Window* window = getWorkspaceWindow ())
		{
			MenuBar* menuBar = window->getMenuBar ();
			if(menuBar)
			{
				workspaceMenu = PopupMenu::create (String (kPerspectiveCategory), strWorkspace); // hmm, shouldn't command and category name be a CString?
				menuBar->addMenu (workspaceMenu);

				ForEach (perspectives, Perspective, p)
					addMenuItem (p);
				EndFor
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::signalPerspectiveChanged ()
{
	if(Perspective* perspective = getCurrentPerspective ())
	{
		ASSERT (!isRestoringViews ()) // otherwise we must check that
		CCL_PRINTF ("signalPerspectiveChanged: %s\n", perspective->getID ().str ());
		perspective->signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Workspace::openViewInFrame (WindowClassRef windowClass, FrameItem& frameItem)
{
	ASSERT (frameItem.isPopup () || getWorkspaceWindow ())
	return frameItem.openView (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::makeVisible (View& view)
{
	Window* window = view.getWindow ();
	if(window)
	{
		if(window->isVisible ())
		{
			if(WindowManager::instance ().shouldActivateWindows () && !Desktop.isPopupActive ())
			{
				if(WindowBase* windowBase = view.getParent<WindowBase> ())
					windowBase->activate ();

				window->activate ();
				window->onActivate (true); // enforce activation event
			}
		}
		else
			window->show ();

		if(View* focusView = FocusNavigator::instance ().getFirstExplicit (&view))
			window->setFocusView (focusView);

		// todo: try to make view completely visible? (scroll)

		LOG_ITEMS
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::openView (WindowClassRef windowClass)
{
	View* view = nullptr;
/*	if(windowID.isAllowMultiple ()) // if multiple instances allowed...
	{
		// ...find frame with that group that doesn't contain this windowClass
	}
	else*/
	{
		view = findExistingView (windowClass);
		if(!view)
		{
			StringRef groupID = windowClass.getGroupID ();
			FrameItem* frameItem = findFrameItem (*AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
				{
					// find frame with matching groupID, ignore if pinned
					FrameItem* frame = unknown_cast<FrameItem> (obj);
					return frame && frame->hasGroupID (groupID) && frame->checkCondition (groupID) && !frame->isPinned ();
				})));
			if(frameItem)
			{
				if(frameItem->isEmbedded ())
				{
					// open parent window class of embedded frame first
					DockPanelItem* parent = frameItem;
					while(parent = (DockPanelItem*)parent->getParent ())
						if(EmbeddedFrameItem* embeddedFrame = ccl_cast<EmbeddedFrameItem> (parent))
						{
							openView (embeddedFrame->getParentClassID ());
							break;
						}
				}
				view = openViewInFrame (windowClass, *frameItem);

				if(view)
					signalPerspectiveChanged ();
			}
		}
	}

	if(view)
		return makeVisible (*view);

	return false; // no frame found for this group
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Workspace::replaceViewInFrame (WindowClassRef newClass, FrameItem& frameItem)
{
	StringID oldClassID = frameItem.getWindowID ();
	if(newClass.getID () != oldClassID && !newClass.isAllowMultiple ())
		closeView (newClass);

	View* view = openViewInFrame (newClass, frameItem);
	if(view)
	{
		signalPerspectiveChanged ();
		return makeVisible (*view);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::replaceView (WindowClassRef oldClass, WindowClassRef newClass)
{
	FrameItem* frameItem = findVisibleFrameItem (oldClass);
	if(frameItem && frameItem->isVisible ())
	{
		if(Perspective* perspective = getCurrentPerspective ())
			perspective->getDividerGroups ()->flush ();

		if(replaceViewInFrame (newClass, *frameItem))
		{
			// ignore possibly restored pinned state of new class when replacing view
			frameItem->setPinned (false);
			return true;
		}
	}

	return false; // old window class was not open
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::centerView (WindowClassRef windowClass)
{
	FrameItem* frameItem = findVisibleFrameItem (windowClass);
	if(frameItem)
	{
		PopupFrameItem* popupItem = ccl_cast<PopupFrameItem> (frameItem);
		if(popupItem)
		{
			Window* window = popupItem->getWindow ();
			if(window)
			{
				window->center ();
				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::canReuseView (WindowClassRef windowClass)
{
	FrameItem* frameItem = findVisibleFrameItem (windowClass);
	return frameItem && frameItem->isVisible () && !frameItem->isPinned ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::closeView (WindowClassRef windowClass)
{
	//if(dockPanelView) can be null when using popup frames only
	{
		FrameItem* frameItem = findVisibleFrameItem (windowClass);
		if(frameItem)
			if(!frameItem->viewIsLocked ())
			{
				// don't close required non-popup frames
				if(frameItem->isRequired () && !(frameItem->isPopup () || frameItem->isDetachedFrame ()))
				{
					// but replace the current content with the default window class instead
					if(!frameItem->getDefaultWindowID ().isEmpty () && frameItem->getDefaultWindowID () != windowClass.getID ())
					{
						ThemeScope scope (*this);
						if(const WindowClass* defaultClass = WindowManager::instance ().getClass (frameItem->getDefaultWindowID ()))
						{
							// don't activate the new default view if the replaced one was not active before
							auto frameView = ccl_cast<FrameView> (frameItem->getView ());
							tbool suspendActivation = !frameView || !frameView->isActive ();
							
							WindowManager::ActivationSuspender suspender (WindowManager::instance (), suspendActivation);
							return replaceViewInFrame (*defaultClass, *frameItem) != 0;
						}
					}
					return false;
				}
				
				frameItem->saveViewState ();
				frameItem->retain ();
				frameItem->hide ();
				frameItem->setWindowID (CString::kEmpty);
				frameItem->release ();
				
				signalPerspectiveChanged ();
				return true;
			}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::canOpenView (WindowClassRef windowClass)
{
	return findFrameItem (windowClass.getGroupID ()) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Workspace::isViewOpen (WindowClassRef windowClass)
{
	FrameItem* frameItem = findVisibleFrameItem (windowClass);
	if(frameItem)
		return frameItem->isViewOpen (windowClass);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Workspace::findExistingView (WindowClassRef windowClass)
{
	FrameItem* frameItem = findVisibleFrameItem (windowClass);
	if(frameItem)
		return frameItem->getViewForClass (windowClass);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* Workspace::findFrameItem (IRecognizer& recognizer)
{
	Perspective* p = getCurrentPerspective ();
	return p ? p->findFrameItem (recognizer) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* Workspace::findFrameItem (StringRef groupID)
{
	GroupIdRecognizer r (groupID);
	return findFrameItem (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
FrameItem* Workspace::findVisibleFrameItem (WindowClassRef wc)
{
	VisibleWindowClassRecognizer r (wc);
	return findFrameItem (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::collectFrames (Container& container, IObjectFilter& filter)
{
	if(Perspective* p = getCurrentPerspective ())
		p->collectFrames (container, filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* Workspace::getWorkspaceWindow ()
{
	View* view = getDockPanelView ();
	ASSERT (!view || view->getWindow ())
	return view ? view->getWindow () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Workspace::createWorkspaceView (RectRef bounds)
{
	ASSERT (!dockPanelView)

	// the application window can optionally be described by a form that must contain the perspective container
	IApplication* application = GUI.getApplication ();
	if(application && application->getApplicationID () == getID ())
	{
		ITheme& theme = *application->getApplicationTheme ();
		AutoPtr<View> view = unknown_cast<View> (theme.createView (IWindowManager::kApplicationFormName, this->asUnknown ()));
		if(view && dockPanelView)
			return view.detach ();
	}

	return createPerspectiveContainer (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Workspace::createPerspectiveContainer (RectRef bounds)
{
	ASSERT (!dockPanelView)

	DockPanelView* dpView = NEW WorkspaceView (bounds);
	dpView->setSizeMode (View::kAttachAll);
	dockPanelView = dpView;

	DockPanelItem* rootFrame = getRootFrame ();
	if(rootFrame && rootFrame->countChildren () == 0)
		rootFrame->isHidable (true);

	PerspectiveContainer* windowBase = NEW PerspectiveContainer (bounds);
	windowBase->setWorkspace (this);
	windowBase->setSizeMode (View::kAttachAll);
	windowBase->addView (dpView);

	connectDockPanelView ();
	return windowBase;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Workspace::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "PerspectiveContainer")
		return createPerspectiveContainer (bounds);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::openView (StringID viewID)
{
	RootFrameItem* rootFrame = getRootFrame ();
	if(rootFrame && rootFrame->isHidingAll ())
		return false;

	ThemeScope scope (*this);

	const WindowClass* wc = WindowManager::instance ().getClass (viewID);
	return wc ? openView (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::closeView (StringID viewID)
{
	ThemeScope scope (*this);

	const WindowClass* wc = WindowManager::instance ().getClass (viewID);
	return wc ? closeView (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::isViewOpen (StringID viewID)
{
	ThemeScope scope (*this);

	const WindowClass* wc = WindowManager::instance ().getClass (viewID);
	return wc ? isViewOpen (*wc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Workspace::isViewDetached (StringID viewID)
{
	ThemeScope scope (*this);

	const WindowClass* wc = WindowManager::instance ().getClass (viewID);
	FrameItem* frameItem = wc ? findVisibleFrameItem (*wc) : nullptr;
	return ccl_cast<DetachedFrameItem> (frameItem) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspective* CCL_API Workspace::clonePerspective (StringID perspectiveID)
{
	Perspective* p = getPerspective (perspectiveID);
	ASSERT (p != nullptr)
	if(p)
	{
		Perspective* newPerspective = (Perspective*)p->clone ();
		addPerspective (newPerspective);
		return newPerspective;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleRef CCL_API Workspace::getWindowStyle () const
{
	return windowStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::setWindowStyle (StyleRef windowStyle)
{
	this->windowStyle = windowStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Workspace::applyWindowStyle (StyleRef windowStyle)
{
	setWindowStyle (windowStyle);

	// apply to existing windows - copy to seprate list, because setting a style can reorder the window in Desktop
	LinkedList <SharedPtr <Window> > workspaceWindows; 

	for(int i = 0, numWindows = Desktop.countWindows (); i < numWindows; i++)
		if(Window* window = unknown_cast<Window> (Desktop.getWindow (i)))
			if(PopupFrameItem* item = PopupFrameItem::fromWindow (window))
				if(item->getWorkspace () == this)
					workspaceWindows.append (window);

	while(workspaceWindows.isEmpty () == false)
		workspaceWindows.removeFirst ()->setStyle (windowStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Workspace::onOrientationChanged (OrientationType newOrientation)
{
	// check if perspective needs to to be switched
	Perspective* perspective = getCurrentPerspective ();
	if(perspective && !perspective->supportsOrientation (newOrientation))
	{
		// find most recent matching perspective
		Perspective* matchingPerspective = getRecentPerspective (*AutoPtr<ObjectFilter> (ObjectFilter::create ([&] (IUnknown* object)
		{
			Perspective* p = unknown_cast<Perspective> (object);
			return !p->getStyle ().isCustomStyle (Perspective::kExplicit) && p->supportsOrientation (newOrientation);
		})));

		if(matchingPerspective)
		{
			#if DEFER_ORIENTATION_CHANGE 
			if(getDockPanelView ()) // not necessary when workspace not visible
				pendingPerspectiveID = matchingPerspective->getID ();
			else
			#endif
				selectPerspective (matchingPerspective);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
 
bool Workspace::onSize (PointRef size)
{
	if(!pendingPerspectiveID.isEmpty ())
	{
		Perspective* perspective = getPerspective (pendingPerspectiveID);
		pendingPerspectiveID.empty ();

		if(perspective)
		{
			selectPerspective (perspective);
			return true; // don't size created childs again
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspaceEventHandler* Workspace::getEventHandler ()
{
	return UnknownPtr<IWorkspaceEventHandler> (GUI.getApplication ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if DEBUG
void Workspace::log (DockPanelItem* item, MutableCString* indent)
{
	#if DEBUG_LOG
	if(!item)
	{
		item = getRootFrame ();
		CCL_PRINTLN ("")
	}
	if(item)
	{
		MutableCString childIndent;
		if(indent)
		{
			CCL_PRINT (indent->str ());
			childIndent = *indent;
		}
		CCL_PRINTF ("%s (%s)", item->myClass ().getPersistentName (), MutableCString (item->getName ()).str ())
		if(View* v = item->getView ())
		{
			MutableCString str (indent ? *indent : 0);
			if(ccl_cast<FrameView> (v))
			{
				View* child = v->getChild (0);
				if(child && !child->getTitle ().isEmpty ())
				{
					str.append ("\"");
					str.append (child->getTitle ());
					str.append ("\" ");
				}
			}
			v->log (str);
		}
		else
			CCL_PRINT (" (hidden)\n")

		childIndent.append ("   ");
		ForEach (*item, DockPanelItem, child)
			log (child, &childIndent);
		EndFor
	}
	#endif
}
#endif

//************************************************************************************************
// WorkspaceInstance
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WorkspaceInstance, Workspace)

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkspaceInstance::WorkspaceInstance (Workspace& prototype)
: Workspace (prototype),
  prototype (prototype)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WorkspaceInstance::isActive () const
{
	return prototype.getActiveInstance () == this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WorkspaceInstance::activate ()
{
	prototype.activateInstance (this);

	UnknownPtr<IActivatable> contextActivatable (context);
	if(contextActivatable)
		contextActivatable->activate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WorkspaceInstance::deactivate ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWorkspaceEventHandler* WorkspaceInstance::getEventHandler ()
{
	return UnknownPtr<IWorkspaceEventHandler> (context);
}
