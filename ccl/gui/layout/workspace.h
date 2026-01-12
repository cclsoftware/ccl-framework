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
// Filename    : ccl/gui/layout/workspace.h
// Description : Workspace
//
//************************************************************************************************

#ifndef _ccl_workspace_h
#define _ccl_workspace_h

#include "ccl/gui/layout/dockpanel.h"
#include "ccl/gui/windows/windowmanager.h"

#include "ccl/base/storage/settings.h"
#include "ccl/public/base/iactivatable.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iuserinterface.h"

namespace CCL {

class View;
class Window;
class WindowClass;
class WorkspaceInstance;
class Menu;
class FrameItem;
class RootFrameItem;
class Workspace;
class Setting;
class DividerGroups;
typedef const WindowClass& WindowClassRef;
interface IObjectFilter;

//************************************************************************************************
// Perspective
//************************************************************************************************

class Perspective: public Object,
				   public IPerspective,
				   public IStorable
{
public:
	DECLARE_CLASS (Perspective, Object)
	DECLARE_STYLEDEF (customStyles)
	DECLARE_STYLEDEF (orientations)

	Perspective (StringID name = nullptr, RootFrameItem* rootFrame = nullptr); ///< rootFrame is owned by perspective
	Perspective (const Perspective&);
	~Perspective ();

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_VARIABLE (int64, lastActivated, LastActivated)
	PROPERTY_VARIABLE (OrientationType, orientation, Orientation)
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)
	PROPERTY_MUTABLE_CSTRING (backCommandCategory, BackCommandCategory)
	PROPERTY_MUTABLE_CSTRING (backCommandName, BackCommandName)
	PROPERTY_POINTER (Workspace, workspace, Workspace)
	PROPERTY_OBJECT (StyleFlags, style, Style)
	PROPERTY_SHARED_AUTO (VisualStyle, visualStyle, VisualStyle)
	PROPERTY_OBJECT (StyleFlags, backgroundOptions, BackgroundOptions)
	PROPERTY_BOOL (fullScreenEntered, FullScreenEntered)

	enum PerspectiveOptions
	{
		kExplicit	= 1<<0,			///< perspective can only be opened by explicit intent, e\.g\. not via rotation
		kFullScreen	= 1<<1,			///< perspective should be appear in fullscreen
		kWindowTransition = 1<<2	///< a specified transition should be applied to the whole window, not only to the perspective content (useful when system frames outside change their content, too)
	};

	String getOriginalID () const;

	RootFrameItem* getRootFrame () const { return rootFrame; }
	FrameItem* findFrameItem (IRecognizer& recognizer);
	FrameItem* findFrameByID (StringRef id);
	void collectFrames (Container& container, IObjectFilter& filter);
	String newFrameID ();

	IAttributeList* getLayoutState (StringRef path, bool create = false);
	const Settings& getLayoutStates () const;
	Settings& getLayoutStates ();

	IPerspectiveActivator* getActivator ();
	void CCL_API setActivator (IPerspectiveActivator* activator) override;
	void prepareSelect ();

	bool supportsOrientation (OrientationType orientation) const;
	DividerGroups* getDividerGroups ();

	class CustomParams;
	CustomParams& getCustomParams ();
	void addCustomParam (IParameter* param);

	// IPerspective
	StringID CCL_API getID () const override { return getName (); }
	tbool CCL_API initFrame (StringRef frameID, StringID windowID) override;
	tbool CCL_API initViewState (StringRef frameID, StringID windowID, StringID attribID, VariantRef value) override;
	IWorkspace* CCL_API getIWorkspace () const override;
	IController& CCL_API getICustomParams () override;
	IController& CCL_API getIDividerGroups () override;

	// IStorable
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
	tbool CCL_API getFormat (FileType& format) const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE2 (IPerspective, IStorable, Object)

private:
	void setRootFrame (RootFrameItem* rootFrame);
	void checkFrameIDs (FrameItem& parent);

	AutoPtr<RootFrameItem> rootFrame;
	SharedPtr<IPerspectiveActivator> activator;
	Settings layoutStates;
	CustomParams* customParams;
	DividerGroups* dividerGroups;
	int frameIDCounter;
	mutable int cloneCounter;
};

//************************************************************************************************
// PerspectiveState
//************************************************************************************************

class PerspectiveState: public Object
{
public:
	DECLARE_CLASS (PerspectiveState, Object)

	PerspectiveState ();
	
	PROPERTY_MUTABLE_CSTRING (name, Name)
	
	void store (const Perspective& perspective);
	void restore (Perspective& perspective, bool checkClassIDs = false) const;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	class FrameState: public Object
	{
	public:
		DECLARE_CLASS (FrameState, Object)

		FrameState ();

		void store (const FrameItem& item);
		void restore (FrameItem& item) const;

		PROPERTY_STRING (name, Name)
		PROPERTY_BOOL (visible, Visible)
		PROPERTY_BOOL (detached, Detached)
		PROPERTY_MUTABLE_CSTRING (windowID, WindowID)
		PROPERTY_VARIABLE (int, zIndex, ZIndex)
		ObjectList viewStates;

		// Object
		bool load (const Storage& storage) override;
		bool save (const Storage& storage) const override;
	};

	ObjectList states;
	Settings layoutStates;
	Attributes paramValues;

	void storeFrames (FrameItem* parent);
	FrameItem* findFrame (FrameItem* parent, StringRef name) const;
};

//************************************************************************************************
// Workspace
//************************************************************************************************

class Workspace: public Object,
				 public AbstractNode,
				 public IWorkspace,
				 public IViewFactory
{
public:
	DECLARE_CLASS (Workspace, Object)

	Workspace ();
	Workspace (const Workspace& w);
	~Workspace ();

	PROPERTY_BOOL (storable, Storable)
	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_POINTER (Theme, theme, Theme)

	void setWindowStyle (StyleRef windowStyle);

	void addPerspective (Perspective* perspective); ///< perspective is owned by workspace
	Perspective* getPerspective (StringID name);
	Perspective* getCurrentPerspective ();
	Perspective* getRecentPerspective (IObjectFilter& filter) const;
	Iterator* newIterator () const;

	FrameItem* findFrameItem (IRecognizer& recognizer);
	void collectFrames (Container& container, IObjectFilter& filter);

	bool openView (WindowClassRef windowClass);
	bool closeView (WindowClassRef windowClass);
	bool centerView (WindowClassRef windowClass);
	bool replaceView (WindowClassRef oldClass, WindowClassRef newClass);
	bool canOpenView (WindowClassRef windowClass);
	bool isViewOpen (WindowClassRef windowClass);
	bool canReuseView (WindowClassRef windowClass);

	Window* getWorkspaceWindow ();
	View* getWorkspaceView ();
	View* createWorkspaceView (RectRef bounds);

	void store (Settings& settings);
	void restore (Settings& settings);

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	String getSettingsID () const;

	bool isRestoringViews () const;
	void onOrientationChanged (OrientationType newOrientation);
	bool onSize (PointRef size);
	void applyPerspectiveStyle ();

	virtual IWorkspaceEventHandler* getEventHandler ();

	class ThemeScope;
	
	// instances
	void addInstance (WorkspaceInstance* instance);
	void removeInstance (Workspace* instance);
	void CCL_API activateInstance (IWorkspace* instance) override;
	IWorkspace* CCL_API getActiveInstance () const override;
	Iterator* getInstances () const;
	int countInstances () const;

	// IWorkspace
	tbool CCL_API selectPerspective (StringID perspectiveID) override;
	tbool CCL_API selectPerspective (IPerspective* perspective) override;
	StringID CCL_API getSelectedPerspectiveID () const override;
	tbool CCL_API openView (StringID viewID) override;
	tbool CCL_API closeView (StringID viewID) override;
	tbool CCL_API isViewOpen (StringID viewID) override;
	tbool CCL_API isViewDetached (StringID viewID) override;
	IPerspective* CCL_API clonePerspective (StringID perspectiveID) override;
	StyleRef CCL_API getWindowStyle () const override;
	void CCL_API applyWindowStyle (StyleRef windowStyle) override;
	IPerspective* getRecentIPerspective (IObjectFilter& filter) const override;

	CLASS_INTERFACE3 (IWorkspace, IViewFactory, IObjectNode, Object)

private:
	ObjectList* instances; // Workspace instances cloned from this prototype
	ObjectArray perspectives;
	Perspective* currentPerspective;
	ViewPtr dockPanelView;
	Menu* workspaceMenu;
	bool restoringViews;
	mutable int cloneCounter;
	StyleFlags windowStyle;
	MutableCString pendingPerspectiveID; // on orientation change

	class PerspectiveContainer;

	bool selectPerspective (Perspective* p);
	RootFrameItem* getRootFrame ();
	DockPanelView* getDockPanelView ();
	void connectDockPanelView ();
	View* createPerspectiveContainer (RectRef bounds);
	View* openViewInFrame (WindowClassRef windowClass, FrameItem& frameItem);
	tbool replaceViewInFrame (WindowClassRef newClass, FrameItem& frameItem);
	bool makeVisible (View& view);
	void addMenuItem (Perspective* perspective);
	void buildMenu ();
	void signalPerspectiveChanged ();

	View* findExistingView (WindowClassRef windowClass);
	FrameItem* findFrameItem (StringRef groupID);
	FrameItem* findVisibleFrameItem (WindowClassRef wc);
	#if DEBUG
	void log (DockPanelItem* item = nullptr, MutableCString* indent = nullptr);
	#endif
};

//************************************************************************************************
// WorkspaceInstance
/** Cloned instance of a workspace prototype. */
//************************************************************************************************

class WorkspaceInstance: public Workspace,
						 public IActivatable
{
public:
	DECLARE_CLASS_ABSTRACT (WorkspaceInstance, Workspace)

	WorkspaceInstance (Workspace& prototype);

	PROPERTY_SHARED_AUTO (IUnknown, context, Context)

	Workspace& getPrototype () { return prototype; }

	// IActivatable
	tbool CCL_API isActive () const override;
	void CCL_API activate () override;
	void CCL_API deactivate () override;

	// Workspace
	IWorkspaceEventHandler* getEventHandler () override;

	CLASS_INTERFACE (IActivatable, Workspace)

private:
	Workspace& prototype;
};

//************************************************************************************************
// WorkspaceSystem
//************************************************************************************************

class WorkspaceSystem: public WindowSystem,
					   public IWorkspaceManager,
					   public ICommandHandler,
					   public AbstractNode,
					   public Singleton<WorkspaceSystem>
{
public:
	DECLARE_CLASS (WorkspaceSystem, WindowSystem)

	void addWorkspace (Workspace* workspace);
	void removeWorkspace (Workspace* workspace);
	Iterator* newIterator () const;

	// IWorkspaceManager
	IWorkspace* CCL_API getWorkspace (StringID workspaceID) const override;
	IWorkspace* CCL_API cloneWorkspace (StringID workspaceID, IUnknown* context) override;
	void CCL_API removeWorkspaceInstance (IWorkspace* workspace) override;
	IPerspective* CCL_API getPerspectiveFromView (IView* view) override;

	// WindowSystem
	bool openWindow (WindowClassRef windowClass) override;
	bool closeWindow (WindowClassRef windowClass) override;
	bool replaceWindow (WindowClassRef oldClass, WindowClassRef newClass) override;
	bool centerWindow (WindowClassRef windowClass) override;
	bool canReuseWindow (WindowClassRef oldClass) override;
	bool canOpenWindow (WindowClassRef windowClass) override;
	bool isWindowOpen (WindowClassRef windowClass) override;
	void storeWindowStates (Settings& settings) override;
	void restoreWindowStates (Settings& settings) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// IObjectNode
	IObjectNode* CCL_API lookupChild (StringRef path) const override;

	// commands
	bool onToggleFloating (const CommandMsg& args);
	bool onToggleOptional (const CommandMsg& args);
	bool onPinFrame (const CommandMsg& args);
	bool onFocusFrame (const CommandMsg& args);
	bool onNavigationBack (const CommandMsg& args);

	static bool makeFrameUrl (String& string, const FrameItem& frameItem);

	CLASS_INTERFACE3 (IWorkspaceManager, ICommandHandler, IObjectNode, WindowSystem)

protected:
	friend class Singleton<WorkspaceSystem>;
	WorkspaceSystem ();
	~WorkspaceSystem ();

private:
	Workspace* getWorkspace (WindowClassRef windowClass);
	Workspace* findTopLevelWorkspace (StringID workspaceId, Theme* theme) const;

	DesktopWindowSystem desktopSystem;
	ObjectArray workspaces;

	class FrameFamily;
	friend class FrameFamily;

	FrameFamily* floatingFamily;
	FrameFamily* optionalFamily;
};

//************************************************************************************************
// PerspectiveActivator
//************************************************************************************************

class PerspectiveActivator: public Object,
							public IPerspectiveActivator
{
public:
	PerspectiveActivator (Perspective* perspective, StringRef title);

	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)

	// IPerspectiveActivator
	String CCL_API getPerspectiveTitle () override			{ return title; }
	String CCL_API getPerspectiveDescription () override	{ return description; }
	IImage* CCL_API getPerspectiveIcon () override			{ return icon; }
	void CCL_API activatePerspective () override;
	void CCL_API notifyPerspectiveSelected () override		{}

	CLASS_INTERFACE (IPerspectiveActivator, Object)

private:
	Perspective* perspective;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const Settings& Perspective::getLayoutStates () const { return layoutStates; }
inline       Settings& Perspective::getLayoutStates ()       { return layoutStates; }
inline IPerspectiveActivator* Perspective::getActivator ()   { return activator; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Workspace::isRestoringViews () const { return restoringViews; }
inline int Workspace::countInstances () const { return instances ? instances->count () : 0; }
inline Iterator* Workspace::newIterator () const { return perspectives.newIterator (); }
inline View* Workspace::getWorkspaceView () { return getDockPanelView (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Iterator* WorkspaceSystem::newIterator () const { return workspaces.newIterator (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PerspectiveActivator::PerspectiveActivator (Perspective* perspective, StringRef title)
: perspective (perspective), title (title) {}

inline void CCL_API PerspectiveActivator::activatePerspective ()
{ perspective->getWorkspace ()->selectPerspective ((IPerspective*)perspective); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_workspace_h
