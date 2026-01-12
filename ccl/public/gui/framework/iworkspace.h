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
// Filename    : ccl/public/gui/framework/iworkspace.h
// Description : Workspace Interfaces
//
//************************************************************************************************

#ifndef _ccl_iworkspace_h
#define _ccl_iworkspace_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

interface IView;
interface IViewAnimator;
interface IWindowClass;
interface IAttributeList;
interface IPerspectiveActivator;
interface IImage;
interface IWorkspace;
interface IController;
interface IObjectFilter;

//************************************************************************************************
// IPerspective
//************************************************************************************************

interface IPerspective: IUnknown
{
	/** Get perspective id. */
	virtual StringID CCL_API getID () const = 0;

	/** Make accessible in perspective switcher, menus, etc. */
	virtual void CCL_API setActivator (IPerspectiveActivator* activator) = 0;

	/** Initialize a view state attribute for a specific window class in a specific frame. */
	virtual tbool CCL_API initViewState (StringRef frameID, StringID windowID, StringID attribID, VariantRef value) = 0;

	/** Initialize frame content with a specific window class. */
	virtual tbool CCL_API initFrame (StringRef frameID, StringID windowID) = 0;

	/** Get workspace. */
	virtual IWorkspace* CCL_API getIWorkspace () const = 0;

	/** Get custom parameters defined in skin for this perspective. */
	virtual IController& CCL_API getICustomParams () = 0;
	
	/** Get divider groups defined in skin for this perspective. */
	virtual IController& CCL_API getIDividerGroups () = 0;

	/** The perspective has been selected / deselected; (OUT) args[0] (tbool): state. */
	DECLARE_STRINGID_MEMBER (kPerspectiveSelected)

	DECLARE_IID (IPerspective)
};

DEFINE_IID (IPerspective, 0xe5080780, 0xad01, 0x4925, 0xbc, 0x8, 0xe4, 0x97, 0x1f, 0x40, 0x60, 0x4d)
DEFINE_STRINGID_MEMBER (IPerspective, kPerspectiveSelected, "PerspectiveSelected")

//************************************************************************************************
// IWorkspace
//************************************************************************************************

interface IWorkspace: IUnknown
{
	/** Select perspective by name. */
	virtual tbool CCL_API selectPerspective (StringID perspectiveID) = 0;

	/** Select perspective by instance. */
	virtual tbool CCL_API selectPerspective (IPerspective* perspective) = 0;

	/** Get identifier of selected perspective. */
	virtual StringID CCL_API getSelectedPerspectiveID () const = 0;

	/** Open view with given id. */
	virtual tbool CCL_API openView (StringID viewID) = 0;

	/** Close view with given id. */
	virtual tbool CCL_API closeView (StringID viewID) = 0;

	/** Check if view with given id is open. */
	virtual tbool CCL_API isViewOpen (StringID viewID) = 0;

	/** Check if view with given id is detached. */
	virtual tbool CCL_API isViewDetached (StringID viewID) = 0;

	/** Clone perspective. */
	virtual IPerspective* CCL_API clonePerspective (StringID perspectiveID) = 0;

	/** Activate a (cloned) workspace instance . */
	virtual void CCL_API activateInstance (IWorkspace* instance) = 0;

	/** Get the active instance of a workspace prototype. */
	virtual IWorkspace* CCL_API getActiveInstance () const = 0;
	
	/** Get window style. */
	virtual StyleRef CCL_API getWindowStyle () const = 0;
	
	/** Apply new window style. */
	virtual void CCL_API applyWindowStyle (StyleRef windowStyle) = 0;

	/** Get the most recently activated perspective that matches the filter. */
	virtual	IPerspective* getRecentIPerspective (IObjectFilter& filter) const = 0;

	/** A perspective has been selected / deselected; (OUT) args[0] (tbool): state; args[1] (String): perspectiveId; args[2]: last activation time of perspective (system ticks). */
	DECLARE_STRINGID_MEMBER (kPerspectiveSelected)

	/** The contents of a "shared" detached frame will change; sent to controller of window class that will be replaced. (OUT) args[0]: previous window class ID, args[1]: new window class ID. */
	DECLARE_STRINGID_MEMBER (kReplacingView)

	DECLARE_IID (IWorkspace)
};

DEFINE_IID (IWorkspace, 0x6cc0e2e0, 0xfa7d, 0x4662, 0xba, 0x3, 0x84, 0xa1, 0xe4, 0xbd, 0xdf, 0x76)
DEFINE_STRINGID_MEMBER (IWorkspace, kReplacingView, "ReplacingView")
DEFINE_STRINGID_MEMBER (IWorkspace, kPerspectiveSelected, "PerspectiveSelected")

//************************************************************************************************
// IWorkspaceManager
//************************************************************************************************

interface IWorkspaceManager: IUnknown
{
	/** Get Workspace with given id. */
	virtual IWorkspace* CCL_API getWorkspace (StringID workspaceID) const = 0;

	/** Clone a workspace prototype. */
	virtual IWorkspace* CCL_API cloneWorkspace (StringID workspaceID, IUnknown* context) = 0; // context is not shared, and must live at least until the corresponding removeWorkspace

	/** Remove a cloned workspace instance. */
	virtual void CCL_API removeWorkspaceInstance (IWorkspace* workspace) = 0;

	/** Get perspective a given view lives in. */
	virtual IPerspective* CCL_API getPerspectiveFromView (IView* view) = 0;

	DECLARE_IID (IWorkspaceManager)
};

DEFINE_IID (IWorkspaceManager, 0x19b91de1, 0xd25a, 0x4ff7, 0xb2, 0xc5, 0x3a, 0xeb, 0x7b, 0x67, 0x0, 0xd6)

//************************************************************************************************
// IPerspectiveActivator
//************************************************************************************************

interface IPerspectiveActivator: IUnknown
{
	/** Get title for the perspective. */
	virtual String CCL_API getPerspectiveTitle () = 0;

	/** Get title for the perspective. */
	virtual String CCL_API getPerspectiveDescription () = 0;

	/** Get icon for the perspective. */
	virtual IImage* CCL_API getPerspectiveIcon () = 0;

	/** Active the perspective. */
	virtual void CCL_API activatePerspective () = 0;

	/** Notify before perspective selection is executed. */
	virtual void CCL_API notifyPerspectiveSelected () = 0;

	DECLARE_IID (IPerspectiveActivator)
};

DEFINE_IID (IPerspectiveActivator, 0xB8730476, 0x9FE7, 0x44B2, 0xAE, 0x85, 0xD9, 0x39, 0xFA, 0xE3, 0x97, 0xEE)

//************************************************************************************************
// WorkspaceEvent
//************************************************************************************************

struct WorkspaceEvent
{
	enum Types
	{
		kOpenView = 1,
		kCloseView,
		kPinned,
		kUnpinned
	};

	int type;
	IView* view;
	const IWindowClass* windowClass;
	IAttributeList* arguments;
	IViewAnimator* animator;

	WorkspaceEvent (int type, IView* view = nullptr)
	: type (type),
	  view (view),
	  windowClass (nullptr),
	  arguments (nullptr),
	  animator (nullptr)
	{}
};

//************************************************************************************************
// IWorkspaceEventHandler
//************************************************************************************************

interface IWorkspaceEventHandler: IUnknown
{
	virtual void CCL_API onWorkspaceEvent (const WorkspaceEvent& e) = 0;

	DECLARE_IID (IWorkspaceEventHandler)
};

DEFINE_IID (IWorkspaceEventHandler, 0xe28812dd, 0xa5ee, 0x401b, 0xb3, 0x9a, 0xcc, 0xc5, 0xbd, 0x71, 0x52, 0x79)

//////////////////////////////////////////////////////////////////////////////////////////////////

// optional parameters provided by IController of popup frame
namespace PopupFramesParams
{
	const CStringPtr kFrameTitle = "frameTitle";
	const CStringPtr kWindowStyle = "windowStyle";
	const CStringPtr kHelpID = "helpid";
}

} // namespace CCL

#endif // _ccl_iworkspace_h
