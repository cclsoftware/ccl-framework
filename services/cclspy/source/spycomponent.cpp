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
// Filename    : spycomponent.cpp
// Description : Spy Component
//
//************************************************************************************************

#define DEBUG_LOG 0
#define INSPECT_SELF (0 && DEBUG)
#define OBJECTBROWSER_ENABLED	!CCL_STATIC_LINKAGE
#define THREADMONITOR_ENABLED	!CCL_STATIC_LINKAGE
#define DOCBROWSER_ENABLED		!CCL_STATIC_LINKAGE

#include "spycomponent.h"
#include "objectinfo.h"
#include "viewproperty.h"
#include "styleproperties.h"
#include "scene3dproperties.h"
#include "viewtree.h"
#include "viewclass.h"
#include "viewsprite.h"
#include "shadowview.h"

#if OBJECTBROWSER_ENABLED
#include "objecttablebrowser.h"
#endif

#if THREADMONITOR_ENABLED
#include "threadmonitor.h"
#endif

#if DOCBROWSER_ENABLED
#include "docbrowser.h"
#include "ccl/extras/modeling/modelbrowser.h"
#endif

#include "ccl/base/message.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iviewanimation.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/iusercontrol.h"
#include "ccl/public/gui/framework/iembeddedviewhost.h"
#include "ccl/public/gui/framework/viewfinder.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Spy;

enum Tags
{
	kMousePosTag = 100,
	kMousePosRelativeTag,
	kMouseViewInfoTag,
	kShowParentTag,
	kHiliteViewTag,
	kShowViewTreeTag
};

//************************************************************************************************
// SpyComponent::CommandHandler
//************************************************************************************************

class SpyComponent::CommandHandler: public Unknown,
        					        public CCL::ICommandHandler
{
public:
	CommandHandler (SpyComponent& component): component (component) {}
	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override {return component.checkCommandCategory (category);}
	tbool CCL_API interpretCommand (const CommandMsg& msg) override {return component.interpretCommand (msg); }
	
	CLASS_INTERFACE (ICommandHandler, Unknown)
protected:
	SpyComponent& component;
};

//************************************************************************************************
// SpyComponent
//************************************************************************************************

bool SpyComponent::reloadingSkin = false;
DEFINE_CLASS_HIDDEN (SpyComponent, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

SpyComponent::SpyComponent ()
: GUI (System::GetGUI ()),
  wasKeyPressed (false),
  currentObject (nullptr),
  propertyItems (NEW PropertiesItemModel),
  viewTreeBrowser (nullptr),
  commandHandler (nullptr)
{
	paramList.setController (this);
	paramList.addString ("mousePos", kMousePosTag);
	paramList.addString ("mousePosRelative", kMousePosRelativeTag);
	paramList.addString ("mouseViewInfo", kMouseViewInfoTag);
	paramList.addParam	("showParent", kShowParentTag);
	paramList.addParam	("hilite", kHiliteViewTag);
	paramList.addParam ("showViewTree", kShowViewTreeTag);

	GUI.addIdleTask (this);

	// command table adds a refcount, which would prevent us from being released by our window if we would not use the CommandHandler delegate
	commandHandler = NEW CommandHandler (*this);
	System::GetCommandTable ().addHandler (commandHandler);

	viewTreeBrowser = NEW ViewTreeBrowser ();
	viewTreeBrowser->addObserver (this);
	addChild (viewTreeBrowser);

	#if OBJECTBROWSER_ENABLED
	ObjectTableBrowser* objectTableBroser = NEW ObjectTableBrowser ();
	objectTableBroser->addObserver (this);
	addChild (objectTableBroser);
	#endif

	#if THREADMONITOR_ENABLED
	addChild (NEW ThreadMonitor);
	#endif

	#if DOCBROWSER_ENABLED
	addChild (NEW DocumentationBrowser);
	#endif

	if(propertyItems)
		propertyItems->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpyComponent::~SpyComponent ()
{
	GUI.removeIdleTask (this);
	System::GetCommandTable ().removeHandler (commandHandler);
	safe_release (commandHandler);

	#if OBJECTBROWSER_ENABLED
	if(ObjectTableBrowser* objectTableBroser = unknown_cast<ObjectTableBrowser> (findChild (CCLSTR ("ObjectTableBrowser"))))
		objectTableBroser->removeObserver (this);
	#endif

	if(viewTreeBrowser)
		viewTreeBrowser->removeObserver (this);
	if(propertyItems)
		propertyItems->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasObjectTableBrowser")
	{
		#if OBJECTBROWSER_ENABLED
		var = 1;
		#endif
		return true;
	}
	if(propertyId == "hasThreadMonitor")
	{
		#if THREADMONITOR_ENABLED
		var = 1;
		#endif
		return true;
	}
	if(propertyId == "hasDocumentationBrowser")
	{
		#if DOCBROWSER_ENABLED
		var = 1;
		#endif
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::load (const Attributes& attribs)
{
	paramList.byTag (kShowViewTreeTag)->setValue (attribs.getBool ("showViewTree"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::save (Attributes& attribs) const
{
	attribs.setAttribute ("showViewTree", paramList.byTag (kShowViewTreeTag)->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SpyComponent::countParameters () const
{
	return paramList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API SpyComponent::getParameterAt (int index) const
{
	return paramList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API SpyComponent::findParameter (StringID name) const
{
	return paramList.lookup (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API SpyComponent::getObject (StringID name, UIDRef classID)
{
	if(classID == ccl_iid<IItemModel> ())
	{
		if(name == "Properties")
		{
			if(propertyItems)
				return propertyItems->asUnknown ();
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpyComponent::setCurrentObject (IUnknown* unknown)
{
	if(unknown == nullptr)
	{
		currentObject.release ();
		if(propertyItems)
		{
			propertyItems->setProperties (nullptr);
			propertyItems->signal (Message (kChanged));
		}
		return true;
	}

	if(currentObject && isEqualUnknown (unknown, currentObject->getObject ()))
		return false;

	AutoPtr<ObjectInfo> info = NEW ObjectInfo (unknown);
	inspectUnknown (*info, unknown);
	PropertyList* g = info->getGroupAt (0);
	if(g && propertyItems)
	{
		propertyItems->setProperties (g);
		propertyItems->signal (Message (kChanged));
	}
	currentObject.share (info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::setCurrentView (IView* view, bool hightlite)
{
	bool isNewView = setCurrentObject (view);

	if(viewTreeBrowser)
		viewTreeBrowser->browseView (view);

	if(hightlite)
	{
		if(isNewView || !highliteSprite.isVisible ())
			highliteView (view);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* SpyComponent::getCurrentView ()
{
	UnknownPtr<IView> view (currentObject ? currentObject->getObject () : nullptr);
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::highliteView (IView* view)
{
	highliteSprite.show (view, kHiliteTime);
	highliteSprite.setShowUntilMouseUp (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::takeScreenshot (IView* view)
{
	AutoPtr<IViewScreenCapture> capture = ccl_new<IViewScreenCapture> (ClassID::ViewScreenCapture);
	AutoPtr<IImage> image = capture ? capture->takeScreenshot (view) : nullptr;
	if(!image)
		return;

	Url path ("local://$desktop/Screenshot.png");
	path.makeUnique ();
	GraphicsFactory::saveImageFile (path, image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* SpyComponent::resolveEmbeddedView (IView* view)
{
	if(ShadowView* existingShadowView = ShadowView::cast_IView<ShadowView> (view))
	{
		// try to detect if the shadow view tree must be updated
		// we want to keep the tree if possible to allow working in a "stable" spy tree view

		// find IEmbeddedViewHost upwards
		IView* parent = view;
		while(parent = parent->getParentView ())
		{
			UnknownPtr<IUserControlHost> userControlHost (parent);
			UnknownPtr<IEmbeddedViewHost> embeddedViewHost (userControlHost ? userControlHost->getUserControl () : nullptr);
			if(embeddedViewHost)
			{
				IView* hostView = parent;

				// create a new shadow tree
				AutoPtr<IView> newShadowView (ShadowView::buildViewTree (*embeddedViewHost));
				if(newShadowView)
				{
					Point p2 (mousePos);
					hostView->screenToClient (p2);

					// if we find same view in the new shadow tree as before, we keep the tree
					IView* shadowChild = newShadowView->getChildren ().findChildView (p2, true);
					if(shadowChild)
					{
						// todo: compare more properties...
						if(shadowChild->getSize () == existingShadowView->getSize ())
							return view;
					}
					else if(!ShadowView::cast_IView<ShadowView> (view->getParentView ()))
						return view; // no child found at mousePos in new tree, old view is root as well

					// take new shadow tree
					newShadowView->retain ();
					hostView->getChildren ().removeAll ();
					hostView->getChildren ().add (newShadowView);

					return shadowChild ? shadowChild : (IView*)newShadowView;
				}
				break;
			}
		}
	}

	// check for UserControl with IEmbeddedViewHost interface
	UnknownPtr<IUserControlHost> userControlHost (view); 
	UnknownPtr<IEmbeddedViewHost> embeddedView (userControlHost ? userControlHost->getUserControl () : nullptr);
	if(embeddedView)
	{
		if(IView* shadowView = ShadowView::buildViewTree (*embeddedView))
		{
			view->getChildren ().removeAll ();
			view->getChildren ().add (shadowView);

			// find view in embedded "shadow" view tree
			Point p2 (mousePos);
			view->screenToClient (p2);

			IView* shadowChild = shadowView->getChildren ().findChildView (p2, true);
			return shadowChild ? shadowChild : shadowView;
		}
	}
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SpyComponent::onTimer (ITimer* timer)
{
	if(reloadingSkin) // suppress during skin reload to avoid crashes with dead form elements
		return;

	IView* newMouseView = nullptr;

	KeyState keyState;
	GUI.getKeyState (keyState);
	bool shiftPressed = (keyState.getModifiers () & KeyState::kShift) != 0;
	bool keyPressed = (keyState.getModifiers () & KeyState::kCommand) != 0;

	Point p;
	GUI.getMousePosition (p);

	// update mouse view
	if(p != mousePos || (keyPressed != wasKeyPressed))
	{
		mousePos = p;
		newMouseView = findMouseView ();

		newMouseView = resolveEmbeddedView (newMouseView);

		Variant args[] = { mousePos.x, mousePos.y };
		String mouseString;
		mouseString.appendFormat ("Mouse: %(1), %(2)",  args, ARRAY_COUNT(args));
		paramList.byTag (kMousePosTag)->fromString (mouseString, true);
	}
	wasKeyPressed = keyPressed;

	// take mouseview as current object
	if(newMouseView && keyPressed)
	{
		IView* newCurrentView = newMouseView;
		UnknownPtr<IView> oldMouseView (mouseViewObject ? mouseViewObject->getObject () : nullptr);

		// if mouseView is still the same and there is no highlited view (key was released), try another view (covered beyond)
		if(newMouseView == oldMouseView && highliteSprite.getView () == nullptr)
			if(IView* nextView = findMouseView (getCurrentView ()))
				newCurrentView = nextView;

		CCL_PRINTF ("setCurrentView (%s)\n\n", MutableCString (ViewBox (newCurrentView).getName ()).str ())
		if(0 && shiftPressed)
			takeScreenshot (newCurrentView);
		setCurrentView (newCurrentView);

		// if the spy window is disabled (by a modal dialog), try to breakout
		if(IWindow* spyWindow = System::GetDesktop ().getWindowByOwner (this->asUnknown ()))
		{
			ViewBox windowBox (spyWindow);
			if(!windowBox.isEnabled ())
			{
				// reopen spy window
				FormBox spyForm (windowBox.getChildren ().getFirstView ());
				windowBox.getChildren ().remove (spyForm);
				spyWindow->close ();
				spyForm.openWindow ();
			}
		}
	}

	if(newMouseView)
	{
		if(!mouseViewObject || !isEqualUnknown (newMouseView, mouseViewObject->getObject ()))
			setMouseView (newMouseView);

		newMouseView->screenToClient (p);
		Variant args[] = { p.x, p.y };
		String mouseString;
		mouseString.appendFormat ("(%(1), %(2))",  args, ARRAY_COUNT(args));
		paramList.byTag (kMousePosRelativeTag)->fromString (mouseString, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void appendProperty (String& description, ObjectInfo* info, StringID id, StringRef before, StringRef after)
{
	String str (info->getPropertyString (id));
	if(!str.isEmpty ())
	{
		description.append (before);
		description.append (str);
		description.append (after);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::setMouseView (IView* mouseView)
{
	AutoPtr<ObjectInfo> info = NEW ObjectInfo (mouseView);
	inspectUnknown (*info, mouseView);

	String description = info->getPropertyString ("Class");
	appendProperty (description, info, "Title", " \"", "\"");
	description.append ("\n");
	appendProperty (description, info, "Size", "Size: ", "");
	description.append ("\n");
	appendProperty (description, info, "SizeLimits", " Limits: ", "");

	paramList.byTag (kMouseViewInfoTag)->fromString (description, true);

	mouseViewObject.share (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* SpyComponent::findMouseView (IView* skipView)
{
	IWindow* window = System::GetDesktop ().findWindowUnderCursor ();
	if(window)
	{
		#if !INSPECT_SELF
		if(window->getController () == this->asUnknown ())
			return nullptr;

		// also check for popup from ElementInspector in Documentation tab (kindly sets spy als source controller of PopupSelectorClient)
		UnknownPtr<IPopupSelectorClient> client (window);
		UnknownPtr<IObjectNode> node (client);
		if(node && isEqualUnknown (node->findChild ("source"), this->asUnknown ()))
			return nullptr;
		#endif

		UnknownPtr<IView> view (window);
		if(view)
		{
			Point p (mousePos);
			view->screenToClient (p);

			Rect vc;
			view->getVisibleClient (vc);
			if(vc.pointInside (p))
			{
				if(skipView)
					if(IView* next = ViewFinder (skipView).findNextView (view, p))
						return next;

				if(IView* child = view->getChildren ().findChildView (p, true))
					return child;

				return view;
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::inspectUnknown (ObjectInfo& info, IUnknown* unknown)
{
	#if OBJECTBROWSER_ENABLED
	ObjectItem* item = unknown_cast<ObjectItem> (unknown);
	if(item)
		item->getProperties (*info.getGroup (nullptr, true));
	else
	#endif
	{
		UnknownPtr<IObject> object (unknown);
		if(object)
		{
			info.addProperty ("Class", String (object->getTypeInfo ().getClassName ()));

			if(UnknownPtr<IView> view = unknown)
				inspectView (info, view);
			else if(UnknownPtr<IVisualStyle> visualStyle = unknown)
				inspectVisualStyle (info, visualStyle);
			else if(UnknownPtr<ISceneNode3D> sceneNode = unknown)
				inspectSceneNode (info, sceneNode);
			else if(UnknownPtr<IMaterial3D> material = unknown)
			{
				// keep relation to scene
				UnknownPtr<ISceneNode3D> contextNode;
				if(currentObject)
					contextNode = currentObject->getObject ();

				inspectMaterial (info, material, contextNode);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::inspectView (ObjectInfo& info, IView* view)
{
	ViewClass& viewClass = ViewClassRegistry::instance ().getClass (view);
	viewClass.getProperties (*info.getGroupAt (0), view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::inspectVisualStyle (ObjectInfo& info, IVisualStyle* visualStyle)
{
	info.getGroupAt (0)->setProperty ("name", String (visualStyle->getName ()));

	UnknownPtr<IObject> vsObject (visualStyle);
	if(vsObject)
	{
		AutoPtr<PropertyHandler> styleHandler (NEW VisualStyleProperty);
		Variant inherited = visualStyle->getInherited ();
		if(inherited.asUnknown ())
			info.addProperty ("inherited", inherited, styleHandler);

		CString itemTypes [] = {IVisualStyle::kImages, IVisualStyle::kColors, IVisualStyle::kFonts, IVisualStyle::kMetrics, IVisualStyle::kOptions };
		static const AutoPtr<PropertyHandler> propertyHandlers[] = { NEW ImagePropertyHandler, NEW ColorPropertyHandler, NEW FontPropertyHandler, nullptr, nullptr };
		// TODO: IVisualStyle::kStrings, IVisualStyle::kGradients...

		for(int t = 0; t < ARRAY_COUNT (itemTypes); t++)
		{
			Variant arrayVar;
			if(vsObject->getProperty (arrayVar, itemTypes[t]))
			{
				UnknownPtr<IArrayObject> itemArray (arrayVar);
				if(itemArray)
				{
					UnknownPtr<IVisualStyleItem> item;
					Variant itemVar;
					Variant value;

					int numItems = itemArray->getArrayLength ();
					for(int i = 0; i < numItems; i++)
						if(itemArray->getArrayElement (itemVar, i))
							if(item = itemVar)
							{
								item->getItemValue (value);
								PropertyHandler* propHandler = propertyHandlers[t];
								info.getGroupAt (0)->setProperty (item->getItemName (), value, propHandler);
							}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::inspectSceneNode (ObjectInfo& info, ISceneNode3D* sceneNode)
{
	// reusable property handler
	static AutoPtr<PropertyHandler> nodeHandler = NEW SceneNode3DProperty;
	static AutoPtr<PropertyHandler> colorHandler = NEW MutableColorPropertyHandler;
	static AutoPtr<PropertyHandler> materialHandler = NEW Material3DPropertyHandler;
	//static AutoPtr<PropertyHandler> position3dHandler = NEW Position3DPropertyHandler;

	info.addObjectProperty (ISceneNode3D::kName);
	if(ISceneNode3D* parentNode = sceneNode->getParentNode ())
		info.addObjectProperty (ISceneNode3D::kParent, nodeHandler);

	if(get_flag<int> (sceneNode->getNodeFlags (), ISceneNode3D::kHasPosition))
	{
		//info.addObjectProperty (sceneNode, ISceneNode3D::kPosition, 0, position3dHandler);
		info.addObjectProperty (ISceneNode3D::kPositionX, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kPositionY, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kPositionZ, PropertyHandler::getNumericHandler ());
	}

	if(get_flag<int> (sceneNode->getNodeFlags (), ISceneNode3D::kHasOrientation))
	{
		info.addObjectProperty (ISceneNode3D::kYawAngle, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kPitchAngle, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kRollAngle, PropertyHandler::getNumericHandler ());
	}

	if(get_flag<int> (sceneNode->getNodeFlags (), ISceneNode3D::kHasScale))
	{
		info.addObjectProperty (ISceneNode3D::kScaleX, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kScaleY, PropertyHandler::getNumericHandler ());
		info.addObjectProperty (ISceneNode3D::kScaleZ, PropertyHandler::getNumericHandler ());
	}

	if(sceneNode->getNodeType () == ISceneNode3D::kLight)
	{
		if(UnknownPtr<ILightSource3D> lightSource = sceneNode)
		{
			info.addObjectProperty (ILightSource3D::kLightColor, colorHandler);
		}

		if(UnknownPtr<IPointLight3D> pointLight = sceneNode)
		{
			info.addObjectProperty (IPointLight3D::kAttenuationRadius, PropertyHandler::getNumericHandler ());
			info.addObjectProperty (IPointLight3D::kAttenuationMinimum, PropertyHandler::getNumericHandler ());
			info.addObjectProperty (IPointLight3D::kAttenuationLinearFactor, PropertyHandler::getNumericHandler ());
			info.addObjectProperty (IPointLight3D::kAttenuationConstantTerm, PropertyHandler::getNumericHandler ());
		}
	}

	if(sceneNode->getNodeType () == ISceneNode3D::kCamera)
	{
		if(UnknownPtr<ICamera3D> camera = sceneNode)
		{
			info.addObjectProperty (ICamera3D::kFieldOfViewAngle, PropertyHandler::getNumericHandler ());
		}
	}

	if(sceneNode->getNodeType () == ISceneNode3D::kModel)
	{
		UnknownPtr<IModel3D> model;
		if(UnknownPtr<IModelNode3D> modelNode = sceneNode)
			model = modelNode->getModelData ();

		if(model)
		{
			for(int i = 0; i < model->getGeometryCount (); i++)
			{
				if(IMaterial3D* material = model->getMaterialAt (i))
				{
					info.addProperty (MutableCString ().appendFormat ("Material[%d]", i++), 
									  Variant (material, true), 
									  materialHandler);
				}
			}
		}
	}

	if(ISceneChildren3D* children = sceneNode->getChildren ())
	{
		int childIndex = 0;
		ForEachUnknown (*children, unk)
			if(UnknownPtr<ISceneNode3D> childNode = unk)
			{
				info.addProperty (MutableCString ().appendFormat ("Children[%d]", childIndex++), 
								  Variant (childNode, true), 
								  nodeHandler);
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::inspectMaterial (ObjectInfo& info, IMaterial3D* material, ISceneNode3D* contextNode)
{
	// reusable property handler
	static AutoPtr<PropertyHandler> nodeHandler = NEW SceneNode3DProperty;
	static AutoPtr<PropertyHandler> colorHandler = NEW MutableColorPropertyHandler;

	if(contextNode)
		info.addProperty (ISceneNode3D::kParent, contextNode, nodeHandler);

	if(UnknownPtr<ISolidColorMaterial3D> colorMaterial = material)
	{
		info.addObjectProperty (ISolidColorMaterial3D::kMaterialColor, colorHandler);
		info.addObjectProperty (ISolidColorMaterial3D::kShininess, PropertyHandler::getNumericHandler ());
	}

	if(UnknownPtr<ITextureMaterial3D> textureMaterial = material)
	{
		info.addObjectProperty (ITextureMaterial3D::kOpacity, PropertyHandler::getNumericHandler ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::editProperty (PropertyList::Property* prop, VariantRef newValue)
{
	ASSERT (currentObject)
	if(currentObject)
	{
		if(UnknownPtr<IObject> object = currentObject->getObject ())
		{
			// scene needs edit notifications
			IScene3D* scene = nullptr;
			if(UnknownPtr<ISceneNode3D> sceneNode = object)
				scene = sceneNode->getRootNode ();
			SceneEdit3D scope (scene);

			if(object->setProperty (prop->getID (), newValue))
			{
				prop->set (newValue);
				propertyItems->signal (Message (kChanged));
			}			
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kShowParentTag:
		showParent ();
		break;

	case kHiliteViewTag:
		hiliteCurrentView ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SpyComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "ViewItemFocused")
	{
		UnknownPtr<IView> view (msg[0]);
		if(view)
		{
			ScopedVar<ViewTreeBrowser*> guard (viewTreeBrowser, 0); // suspend rebuilding view tree
			setCurrentView (view);
		}
	}
	else if(msg == "ObjectFocused")
	{
		setCurrentObject (msg[0]);
	}
	else if(msg == "inspectObject")
	{
		if(IUnknown* obj = msg[0])
			setCurrentObject (obj);
	}
	else if(msg == "editProperty")
	{
		if(auto prop = unknown_cast<PropertyList::Property> (msg[0]))
			editProperty (prop, msg[1]);
	}
	#if DOCBROWSER_ENABLED
	else if(msg == "Reveal View Documentation")
	{
		DocumentationBrowser* docBrowser = unknown_cast<DocumentationBrowser> (findChild ("DocumentationBrowser"));
		ClassModelBrowser* classBrowser = docBrowser ? docBrowser->getClassBrowser () : nullptr;
		if(classBrowser)
		{
			// request showing documentation browser (via skin trigger for spy view)
			IWindow* spyWindow = System::GetDesktop ().getWindowByOwner (this->asUnknown ());
			UnknownPtr<ISubject> spyView (ViewBox (spyWindow).getChildren ().getFirstView ());
			if(spyView)
				spyView->signal (Message ("showDocumentationBrowser"));

			// reveal class
			classBrowser->notify (this, Message ("RevealClass", msg[0], CCLSTR ("Skin Elements")));
		}
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyComponent::checkCommandCategory (CStringRef category) const
{
	return category == CCL_SPY_COMMAND_CATEGORY;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SpyComponent::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == CCL_SPY_COMMAND_CATEGORY)
	{
		if(msg.name == "Move View Left")
		{
			return msg.checkOnly () ? true : moveView (Point (-1, 0));
		}
		else if (msg.name == "Move View Right")
		{
			return msg.checkOnly () ? true : moveView (Point (+1, 0));
		}
		if(msg.name == "Move View Up")
		{
			return msg.checkOnly () ? true : moveView (Point (0, -1));
		}
		else if (msg.name == "Move View Down")
		{
			return msg.checkOnly () ? true : moveView (Point (0, +1));
		}
		else if(msg.name == "Show Parent")
		{
			if(!msg.checkOnly ())
				showParent ();
			return true;
		}
		else if(msg.name == "Hilite View")
		{
			if(!msg.checkOnly ())
				hiliteCurrentView ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpyComponent::moveView (PointRef offset)
{
	UnknownPtr<IView> view (currentObject ? currentObject->getObject () : nullptr);
	if(view)
	{
		Rect r (view->getSize ());
		r.offset (offset);
		view->setSize (r);
		CCL_PRINTF ("Move View %d, %d\n", offset.x, offset.y)
		currentObject.release ();
		setCurrentView (view, false);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::showParent ()
{
	if(currentObject)
	{
		if(UnknownPtr<IView> view = currentObject->getObject ())
		{
			IView* parent = view->getParentView ();
			if(parent)
				setCurrentView (parent);
		}
		else if(UnknownPtr<ISceneNode3D> sceneNode = currentObject->getObject ())
		{
			if(ISceneNode3D* parentNode = sceneNode->getParentNode ())
				setCurrentObject (parentNode);
		}
		else if(UnknownPtr<IMaterial3D> material = currentObject->getObject ())
		{
			// figure out relation to scene
			UnknownPtr<ISceneNode3D> contextNode;
			if(auto propertyList = propertyItems->getProperties ())
				if(auto prop = propertyList->getProperty (ISceneNode3D::kParent))
					contextNode = prop->getValue ().asUnknown ();

			if(contextNode)
				setCurrentObject (contextNode);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpyComponent::hiliteCurrentView ()
{
	IView* view = getCurrentView ();
	if(view)
		highliteView (view);
}
