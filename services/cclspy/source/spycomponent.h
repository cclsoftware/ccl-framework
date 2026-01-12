//************************************************************************************************
//
// CCL Spy
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
// Filename    : spycomponent.h
// Description : Spy Component
//
//************************************************************************************************

#ifndef _spycomponent_h
#define _spycomponent_h

#include "objectinfo.h"
#include "viewsprite.h"

#include "ccl/base/objectnode.h"

#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {
interface IUserInterface;
interface ISceneNode3D; 
interface IMaterial3D; }

namespace Spy {

class ViewTreeBrowser;

//************************************************************************************************
// SpyComponent
//************************************************************************************************

class SpyComponent: public CCL::ObjectNode,
					public CCL::AbstractController,
					public CCL::ITimerTask,
					public CCL::ICommandHandler,
					public CCL::IParamObserver
{
public:
	DECLARE_CLASS (SpyComponent, ObjectNode)

	SpyComponent ();
	~SpyComponent ();

	PropertiesItemModel* getPropertyItems () { return propertyItems; }

	void load (const Attributes& attribs);
	void save (Attributes& attribs) const;

	// IController
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index) const override;
	IParameter* CCL_API findParameter (StringID name) const override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	// TODO: getParameterByTag?

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {};

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// ObjectNode
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IController, ITimerTask, ObjectNode)

	static bool reloadingSkin;

private:
	enum { kHiliteTime = 300 };

	IUserInterface& GUI;
	CCL::Point mousePos;
	bool wasKeyPressed;
	CCL::ParamList paramList;
	CCL::AutoPtr<ObjectInfo> mouseViewObject;
	CCL::AutoPtr<ObjectInfo> currentObject;
	CCL::AutoPtr<PropertiesItemModel> propertyItems;
	ViewTreeBrowser* viewTreeBrowser;
	ViewSprite highliteSprite;	
	class CommandHandler;
	CommandHandler* commandHandler;

	IView* findMouseView (IView* skipView = nullptr);
	IView* resolveEmbeddedView (IView* view);
	void setMouseView (IView* mouseView);
	bool setCurrentObject (IUnknown* object);
	void setCurrentView (IView* view, bool hightlite = true);
	IView* getCurrentView ();
	void takeScreenshot (IView* view);
	void highliteView (IView* view);
	void hiliteCurrentView ();
	void showParent ();
	bool moveView (PointRef dir);
	
	void inspectUnknown (ObjectInfo& info, IUnknown* unknown);
	void inspectView (ObjectInfo& info, IView* view);
	void inspectVisualStyle (ObjectInfo& info, IVisualStyle* visualStyle);
	void inspectSceneNode (ObjectInfo& info, ISceneNode3D* sceneNode);
	void inspectMaterial (ObjectInfo& info, IMaterial3D* material, ISceneNode3D* contextNode);

	void editProperty (PropertyList::Property* prop, VariantRef newValue);

	// IObject
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
};

} // namespace Spy

#endif // _spycomponent_h
