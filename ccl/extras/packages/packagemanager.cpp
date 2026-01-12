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
// Filename    : ccl/extras/packages/packagemanager.cpp
// Description : Package Manager
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_PACKAGES (0 && DEBUG)

#include "ccl/extras/packages/packagemanager.h"
#include "ccl/extras/packages/packagehandlerregistry.h"
#include "ccl/extras/packages/unifiedpackageaction.h"
#include "ccl/extras/packages/packagefilter.h"

#include "ccl/app/application.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/controls/draghandler.h"
#include "ccl/app/components/inplaceprogresscomponent.h"
#include "ccl/app/components/notificationcomponent.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/file.h"
 
#include "ccl/public/app/signals.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/text/translation.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// PackageDragHandler
//************************************************************************************************

class PackageDragHandler: public DragHandler
{
public:
	PackageDragHandler (PackageManager* manager, IView* view);

	// DragHandler
	tbool CCL_API drop (const DragEvent& event) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;

protected:
	virtual void install (UnifiedPackage* package);
	SharedPtr<PackageManager> manager;
};

//************************************************************************************************
// ObjectItemModel
//************************************************************************************************

class ObjectItemModel: public Component,
					   public ItemViewObserver<AbstractItemModel>
{
public:
	DECLARE_CLASS (ObjectItemModel, Component)

	ObjectItemModel (StringRef name = "ObjectItemModel", StringRef managerName = "PackageManager", StringRef title = nullptr);
	~ObjectItemModel ();

	void addItem (Object* item);
	void removeItem (Object* item);
	void removeAllItems ();
	const Container& getItems () const;
	int count () const;
	bool isEmpty () const;
	bool isVisible () const;

	// AbstractItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	void CCL_API viewAttached (IItemView* itemView) override;	
	void CCL_API viewDetached (IItemView* itemView) override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IItemModel, Component)

protected:
	ObjectArray items;
	String managerName;
	int viewCount;
};

//************************************************************************************************
// PackageComponentModel
//************************************************************************************************

class PackageComponentModel: public ObjectItemModel
{
public:
	DECLARE_CLASS (PackageComponentModel, ObjectItemModel)

	PackageComponentModel (StringRef name = "PackageList", StringRef managerName = "PackageManager", StringRef title = nullptr);
	~PackageComponentModel ();

	PackageComponent* findItem (StringRef id, bool filter = true) const;
	PackageComponent* findItem (const UnifiedPackage& package, bool filter = true) const;

	void addFilter (IObjectFilter* filter);
	bool matchesFilters (UnifiedPackage& package) const;
	void resetFilters ();

	void setSectionPropertyId (StringID sectionPropertyId, bool ascending = true);
	StringID getSectionPropertyId () const;
	void sortComponents (PackageSorter* sorter);
	void applyConfiguration (int configuration);
	void selectAll (bool state);
	void deselectFiltered ();
	bool makeVisible (StringRef packageId);

#if DEBUG_PACKAGES
	void dump ();
#endif

	// ObjectItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	ObjectItemModel* filterComponentModel;
	ObjectArray sectionHeaders;
	CString sectionPropertyId;
	bool sectionPropertyAscending;
};

//************************************************************************************************
// CombinedPackage
//************************************************************************************************

class CombinedPackage: public UnifiedPackage
{
public:
	CombinedPackage (StringRef id = nullptr);
	CombinedPackage (const UnifiedPackage& package);

	void merge (const UnifiedPackage& other);
};

//************************************************************************************************
// PackageComponent
//************************************************************************************************

class PackageComponent: public Component,
						public AbstractItemModel,
						public IUnifiedPackageHandlerObserver
{
public:
	DECLARE_CLASS (PackageComponent, Component)

	PackageComponent (PackageManager* manager = nullptr, UnifiedPackage* package = nullptr, PackageComponent* parent = nullptr);
	~PackageComponent ();

	void setParentComponent (PackageComponent* parent);
	PackageComponent* getParentComponent () const;
	PackageComponent* getFirstSubItem () const;

	UnifiedPackage* getPackage () const;
	void merge (UnifiedPackage* package);
	void removeAllActions ();
	const ObjectArray& getActions () const;
	void removeAllDetails ();

	void updateSubItemCount ();
	void updateParameters ();
	bool updateActions ();
	bool updateDetailComponents ();
	void updateParentSelectionState (bool defer = false);

	bool ownsPackage (UnifiedPackage* package);
	bool isEmpty ();
	bool canMergeWithChild () const;

	void performAction (UnifiedPackageAction& action, bool confirmed = false);
	void pauseAction (UnifiedPackageAction& action, bool state);
	bool performActionWithID (StringID actionID, bool recursive = false);
	double getProgress () const;
	bool isSelected () const;

	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// AbstractItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;

	// IUnifiedPackageHandlerObserver
	void onProgress (const UnifiedPackageAction& action, double progress) override;
	void onCompletion (const UnifiedPackageAction& action, bool succeeded) override;
	void onPackageChanged (UnifiedPackage* package) override;
	void onPause (const UnifiedPackageAction& action, bool state) override;
	void requestRestart (const UnifiedPackageAction& action, StringRef message) override;
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	CLASS_INTERFACE2 (IUnifiedPackageHandlerObserver, IItemModel, Component)

protected:
	struct Macro
	{
		CString actionId;
		String title;
		int state = UnifiedPackageAction::kDisabled;
		bool cancelEnabled = false;
		bool pauseEnabled = false;
		bool resumeEnabled = false;
	};

	DECLARE_STRINGID_MEMBER (kUpdateParentSelectionState);

	PackageManager* manager;
	SharedPtr<PackageComponent> parentComponent;
	InplaceProgressComponent* inplaceProgress;
	int childProgressFinishCounter;

	ObjectArray sourcePackages;
	AutoPtr<CombinedPackage> package;
	ObjectArray actions;
	Vector<Macro> macros;
	ObjectItemModel* details;

	ObjectArray matchingChildren;

	IParameter* selected;
	IImageProvider* icon;
	IParameter* title;
	IParameter* description;
	IParameter* vendor;
	IParameter* website;
	IParameter* size;
	IParameter* version;
	IParameter* licenseData;
	IParameter* id;
	IParameter* showChildren;
	IParameter* type;
	IParameter* state;

	int numSubItems;
	bool isUpdatingActions;

	void addDetail (Component* detail);
	String getChildrenStateDescription () const;
	bool getSubItems (IUnknownList& items) const;
	void refresh ();
	void reset ();
	void updateMacros ();
	void cancelAction (UnifiedPackageAction& action);
	void performMacro (const Macro& macro, bool confirmed = false);
	void cancelMacro (const Macro& macro);
	void pauseMacro (const Macro& macro, bool state = true);
	void sortActions ();
	enum ProgressState { kProgressStart, kProgressUpdate, kProgressEnd };
	void onChildProgress (PackageComponent* child, double progress, ProgressState state);
	void handleSelection (int state);
	IImage* createIconForPackage (const UnifiedPackage& package, bool canMergeWithChild) const;
};

//************************************************************************************************
// PackageComponentSorter
//************************************************************************************************

class PackageComponentSorter
{
public:
	static void setSorter (PackageSorter* sorter) { packageSorter = sorter; }
	static void setSectionPropertyId (IObject::MemberID sectionPropertyId, bool _ascending) { propertyId = sectionPropertyId; ascending = _ascending; }

	static DEFINE_ARRAY_COMPARE (compare, PackageComponent, lhsComponent, rhsComponent)
		Variant lhsProperty;
		Variant rhsProperty;
		if(propertyId.isEmpty () == false && lhsComponent->getProperty (lhsProperty, propertyId) && rhsComponent->getProperty (rhsProperty, propertyId))
		{
			if(lhsProperty < rhsProperty)
				return ascending ? -1 : 1;
			else if(lhsProperty > rhsProperty)
				return ascending ? 1 : -1;
		}
		if(packageSorter)
			return packageSorter->compare (lhsComponent->getPackage () , rhsComponent->getPackage ());
		return 0;
	}

private:
	static PackageSorter* packageSorter;
	static CString propertyId;
	static bool ascending;
};

PackageSorter* PackageComponentSorter::packageSorter = nullptr;
CString PackageComponentSorter::propertyId;
bool PackageComponentSorter::ascending = true;

//************************************************************************************************
// PackageNotificationFilter
//************************************************************************************************

class PackageNotificationFilter: public Object,
								 public IObjectFilter
{
public:
	DECLARE_CLASS (PackageNotificationFilter, Object)
	
	static const String kSubCategory;

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, Object)
};

} // namespace Packages
} // namespace CCL

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PackageManagerTags
	{
		kSortBy = 100,
		kConfiguration,
		kSelectAll,
		kCancelAll,
		kPauseAll,
		kResumeAll
	};

	enum PackageDisplayTags
	{
		kSelected = 100,
		kIcon,
		kId,
		kTitle,
		kDescription,
		kVendor,
		kWebsite,
		kSize,
		kVersion,
		kLicenseData,
		kType,
		kState,
		kShowChildren
	};

	enum PackageState
	{
		kNotInstalled,
		kActionRequired,
		kUpdateAvailable,
		kFullyUsable,

		kNumPackageStates
	};

	enum SelectionState
	{
		kUnchecked,
		kMixed,
		kChecked
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageManager")
	XSTRING (ItemCount, "Contains %(1) items")
	XSTRING (ActionState, "%(2)/%(3) %(1)")
	XSTRING (InstalledPackages, "Installed Packages")
	XSTRING (AvailablePackages, "Available Packages")

	XSTRING (MinimalInstall, "Minimal Installation")
	XSTRING (RecommendedInstall, "Recommended Installation")
	XSTRING (FullInstall, "Full Installation")
	XSTRING (CustomInstall, "Custom Installation")

	XSTRING (NotInstalled, "Available")
	XSTRING (ActionRequired, "Action Required")
	XSTRING (UpdateAvailable, "Updates Available")
	XSTRING (FullyUsable, "Active")
	
	XSTRING (Installation, "$APPNAME Installation")
	XSTRING (RestartNow, "Restart Now")
END_XSTRINGS

//************************************************************************************************
// PackageManager
//************************************************************************************************

DEFINE_CLASS (PackageManager, Component)
DEFINE_CLASS_NAMESPACE (PackageManager, NAMESPACE_CCL)

DEFINE_STRINGID_MEMBER_ (PackageManager, kUpdate, "update");
DEFINE_STRINGID_MEMBER_ (PackageManager, kSort, "sort");
DEFINE_STRINGID_MEMBER_ (PackageManager, kUpdateProgress, "updateActionsActive");
DEFINE_STRINGID_MEMBER_ (PackageManager, kUpdateSelectedActions, "updateSelectedActions");
DEFINE_STRINGID_MEMBER_ (PackageManager, kMakeVisible, "makeVisible");
DEFINE_STRINGID_MEMBER_ (PackageManager, kSelect, "select");
DEFINE_STRINGID_MEMBER_ (PackageManager, kSetInstallConfiguration, "setInstallConfiguration");
DEFINE_STRINGID_MEMBER_ (PackageManager, kRestartAction, "packageRestartAction")

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageManager::PackageManager (StringRef name, StringRef title)
: Component (name, title),
  packageComponentModel (NEW PackageComponentModel ("PackageList", name)),
  inplaceProgress (nullptr),
  sortBy (nullptr),
  isUpdating (false)
{
	addComponent (inplaceProgress = NEW InplaceProgressComponent ());
	addComponent (notificationComponent = NEW NotificationListComponent ("PackageNotifications", NEW PackageNotificationFilter));
	notificationComponent->setItemFormName ("PackageManager/PackageNotificationsItem");

	addObject ("PackageList", ccl_as_unknown (packageComponentModel));
	addComponent (packageComponentModel);

	sortBy = paramList.addInteger (0, 0, "sortBy", Tag::kSortBy);
	sorters.objectCleanup ();

	paramList.addList ("configurationList", Tag::kConfiguration);
	paramList.addInteger (Tag::kUnchecked, Tag::kChecked, "selectAll", Tag::kSelectAll);
	paramList.addParam ("cancelAll", Tag::kCancelAll);
	paramList.addParam ("pauseAll", Tag::kPauseAll);
	paramList.addParam ("resumeAll", Tag::kResumeAll);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageManager::~PackageManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageManager::initialize (IUnknown* context)
{
	SuperClass::initialize (context);

	packageComponentModel->addObserver (this);

	UnknownPtr<IListParameter> configurationList = paramList.byTag (Tag::kConfiguration);
	configurationList->appendString (XSTR (MinimalInstall));		// kMinimalInstall
	configurationList->appendString (XSTR (RecommendedInstall));	// kRecommendedInstall
	configurationList->appendString (XSTR (FullInstall));			// kFullInstall
	configurationList->appendString (XSTR (CustomInstall));			// kCustomInstall
	setInstallConfiguration (kCustomInstall);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageManager::terminate ()
{
	packageComponentModel->removeObserver (this);
	packageComponentModel->removeAllItems ();

	sorters.removeAll ();

	for(IUnifiedPackageSource* source : sources)
	{
		source->removeSink (this);
		source->release ();
	}
	sources.removeAll ();

	cancelSignals ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::openWindow ()
{
	System::GetWindowManager ().openWindow (MutableCString (getName ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::refresh ()
{
	removeAllComponents ();
	deferChanged ();
	retrievePackages (UnifiedPackageUrl (), true);
	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::addSource (IUnifiedPackageSource* source)
{
	sources.add (source);
	source->addSink (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::addFilter (IObjectFilter* filter)
{
	packageComponentModel->addFilter (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::addSorter (PackageSorter* sorter)
{
	sorters.add (sorter);
	sortBy = getParameterByTag (Tag::kSortBy);
	sortBy->setMax (sorters.count () - 1);
	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::setSectionPropertyId (StringID propertyId, bool ascending)
{
	if(packageComponentModel->getSectionPropertyId () != propertyId)
		sortComponents (true);
	packageComponentModel->setSectionPropertyId (propertyId, ascending);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PackageManager::getSectionPropertyId () const
{
	return packageComponentModel->getSectionPropertyId ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::addOrigin (int originId, StringRef title)
{
	origins.add ({ originId, title });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::setInstallConfiguration (int configuration, bool defer)
{
	if(defer)
		(NEW Message (kSetInstallConfiguration, configuration))->post (this, -1);
	else
		paramList.byTag (Tag::kConfiguration)->setValue (configuration, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::retrievePackages (UrlRef url, bool refresh)
{
	for(IUnifiedPackageSource* source : sources)
		source->retrievePackages (url, refresh);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::updateAll (bool defer)
{
	if(defer || isUpdating)
		(NEW Message (kUpdate))->post (this, -1);
	else
	{
		ScopedVar<bool> scope (isUpdating, true);
		deferChanged ();
		retrievePackages ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::update (PackageComponent* component)
{
	deferChanged ();
	if(component)
		retrievePackages (buildPackageUrl (*component));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url PackageManager::buildPackageUrl (StringRef id) const
{
	PackageComponent* packageComponent = nullptr;
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		if(component->getPackage ()->getId () == id)
			packageComponent = component;

	if(packageComponent == nullptr)
		return Url ();

	return buildPackageUrl (*packageComponent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url PackageManager::buildPackageUrl (PackageComponent& packageComponent) const
{
	PackageComponent* component = &packageComponent;
	Vector<String> ids;
	while(component)
	{
		ids.add (component->getPackage ()->getId ());
		component = component->getParentComponent ();
	}
	ids.reverse ();

	UnifiedPackageUrl url;
	for(StringRef id : ids)
		url.descend (id, IUrl::kFolder);

	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::onShowChildren (UnifiedPackage& package)
{
	if(package.retrieveChildren ())
	{
		for(UnifiedPackage* child : package.getChildren ())
			retrievePackages (buildPackageUrl (child->getId ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::clearMessages ()
{
	UnknownList notifications;
	notificationComponent->getSubItems (notifications, ItemIndex ());
	ForEachUnknown (notifications, unk)
		NotificationComponent* component = unknown_cast<NotificationComponent> (unk);
		if(component)
			System::GetNotificationCenter ().removeNotification (component->getNotification ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotification* PackageManager::findNotification (StringRef message) const
{
	UnknownList notifications;
	notificationComponent->getSubItems (notifications, ItemIndex ());
	ForEachUnknown (notifications, unk)
		NotificationComponent* component = unknown_cast<NotificationComponent> (unk);
		if(component && component->getNotification ()->getBody () == message)
			return component->getNotification ();
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::sendNotification (const Alert::Event& event, const Vector<NotificationActionProperties>& actionProperties)
{
	if(INotification* notification = findNotification (event.message))
		System::GetNotificationCenter ().removeNotification (notification);

	Attributes notificationAttributes;
	IImage* icon = nullptr;
	switch(event.type)
	{
	case Alert::kInformation : icon = getTheme ()->getImage ("PMInfoIcon"); break;
	case Alert::kWarning : icon = getTheme ()->getImage ("PMWarningIcon"); break;
	case Alert::kError : icon = getTheme ()->getImage ("PMErrorIcon"); break;
	}
	if(icon)
		notificationAttributes.setAttribute (INotification::kIcon, icon, Attributes::kShare);
	notificationAttributes.setAttribute (INotification::kSubCategory, PackageNotificationFilter::kSubCategory);

	INotification* notification = System::GetNotificationCenter ().sendInAppNotification (XSTR (Installation), event.message, &notificationAttributes, actionProperties, actionProperties.count ());
	if(notification && isVisible ())
		System::GetNotificationCenter ().setState (notification, INotification::kSeen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::requestRestart (StringRef message)
{
	Vector<NotificationActionProperties> actionProperties;
	actionProperties.add ({kRestartAction, XSTR (RestartNow)});
	sendNotification ({message.isEmpty () ? ApplicationStrings::RestartRequired () : message, Alert::kInformation}, actionProperties);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageManager::reportEvent (const Alert::Event& event)
{
	sendNotification (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageManager::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::hasPackages () const
{
	return !packageComponentModel->getItems ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::hasActiveActions (bool paused) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
		{
			if(!paused && action->getState () == UnifiedPackageAction::kActive)
				return true;
			else if(paused && action->getState () == UnifiedPackageAction::kPaused)
				return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::getPackages (Container& packages) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		packages.add (return_shared (component->getPackage ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::getInstallableProducts (Container& packages) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
	{
		UnifiedPackage* package = component->getPackage ();

		if(!package->isProduct ())
			continue;

		while(PackageComponent* parent = component->getParentComponent ())
		{
			component = parent;
			package = component->getPackage ();
		}
		
		bool installable = false;

		Vector<PackageComponent*> components;
		components.add (component);
		for(UnifiedPackage* child : package->getChildren ())
			components.addOnce (findPackageComponent (*child));
		for(PackageComponent* component : components)
		{
			if(component == nullptr)
				continue;
			if(component->getPackage ()->isLocalPackage ())
			{
				installable = true;
				break;
			}
			for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
			{
				if(action->getId () == UnifiedPackageHandler::kInstall)
				{
					installable = true;
					break;	
				}
			}
			if(installable)
				break;
		}

		if(installable && !packages.contains (package))
			packages.add (return_shared (package));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* PackageManager::findPackage (StringRef packageId) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
	{
		if(component->getPackage ()->getId () == packageId)
			return component->getPackage ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::getActions (Container& actions, StringRef packageId) const
{
	if(PackageComponent* component = packageComponentModel->findItem (packageId, false))
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
			actions.add (return_shared (action));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::getActions (Container& actions, const UnifiedPackage& package) const
{
	if(PackageComponent* component = packageComponentModel->findItem (package, false))
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
			actions.add (return_shared (action));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* PackageManager::createDragHandler (const DragEvent& event, IView* view)
{
	AutoPtr<PackageDragHandler> handler (NEW PackageDragHandler (this, view));
	if(handler->prepare (event.session.getItems (), &event.session))
	{
		event.session.setResult (IDragSession::kDropCopyReal);
		return handler.detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* PackageManager::createPackageFromFile (UrlRef url) const
{
	for(IUnifiedPackageSource* source : sources)
	{
		UnifiedPackage* package = source->createFromFile (url);
		if(package != nullptr)
			return package;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageManager::findPackageComponent (const UnifiedPackage& package) const
{
	return packageComponentModel->findItem (package, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageManager::findPackageComponent (StringRef packageID) const
{
	return packageComponentModel->findItem (packageID, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::matchesFilters (UnifiedPackage& package) const
{
	return packageComponentModel->matchesFilters (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::sortComponents (bool defer)
{
	if(defer)
		(NEW Message (kSort))->post (this, -1);
	else
	{
		if(sortBy == nullptr)
			return;

		int index = sortBy->getValue ().asInt ();
		PackageSorter* sorter = ccl_cast<PackageSorter> (sorters.at (index));
		if(sorter == nullptr)
			return;

		packageComponentModel->sortComponents (sorter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::removeAllComponents ()
{
	packageComponentModel->removeAllItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::mergeExistingChildren (UnifiedPackage* package)
{
	for(int i = 0; i < package->getChildren ().count (); ++i)
	{
		UnifiedPackage* child = package->getChildren ()[i];
		if(PackageComponent* component = packageComponentModel->findItem (child->getId (), false))
			component->merge (child);
		mergeExistingChildren (child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::ownsPackage (UnifiedPackage* package) const
{
	if(PackageComponent* component = packageComponentModel->findItem (*package, false))
		if(component->getPackage () == package)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* PackageManager::getPackageModel ()
{
	return packageComponentModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::addPackage (UnifiedPackage* package)
{
	PackageComponent* component = packageComponentModel->findItem (package->getId (), false);
	if(component == nullptr)
	{
		component = NEW PackageComponent (this, package);
		packageComponentModel->addItem (component);
	}
	else
		component->merge (package);

	for(UnifiedPackage* childPackage : package->getChildren ())
	{
		PackageComponent* childComponent = packageComponentModel->findItem (childPackage->getId (), false);
		if(childComponent == nullptr)
		{
			childComponent = NEW PackageComponent (this, childPackage);
			packageComponentModel->addItem (childComponent);
		}
		if(childComponent)
			childComponent->setParentComponent (component);
	}

	mergeExistingChildren (package);

	sortComponents (true);

	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::requestUpdate (IUnifiedPackageSource& source, int updateFlags)
{
	if((updateFlags & (kPackageRemoved|kPackageChanged)) != 0)
		updateAll (true);
	else if((updateFlags & kPackageAdded) != 0)
		source.retrievePackages (UnifiedPackageUrl (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::installPackage (StringRef packageID)
{
	PackageComponent* component = findPackageComponent (packageID);
	if(component == nullptr)
		return false;

	bool success = component->performActionWithID (UnifiedPackageHandler::kInstall, true);
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double PackageManager::getInstallationProgress (StringRef packageID) const
{
	PackageComponent* component = findPackageComponent (packageID);
	if(component == nullptr)
		return -1.0;
	return component->getProgress ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::cancelActions (StringRef packageID)
{
	PackageComponent* component = findPackageComponent (packageID);
	if(component == nullptr)
		return false;

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
	{
		if(action->getState () == UnifiedPackageAction::kActive || action->getState () == UnifiedPackageAction::kPaused)
			action->cancel ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::installPackage (UnifiedPackage* package)
{
	AutoPtr<UnifiedPackageAction> installAction = getInstallAction (package);
	if(installAction.isValid () == false)
		return false;

	clearMessages ();
	installAction->setObserver (findPackageComponent (package->getId ()));
	bool succeeded = installAction->perform ();
	if(succeeded == false)
		installAction->setObserver (nullptr);
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasPackages")
	{
		var = hasPackages ();
		return true;
	}
	if(propertyId == "numSortModes")
	{
		var = sorters.count ();
		return true;
	}
	if(propertyId == "numMessages")
	{
		var = messages.count ();
		return true;
	}
	if(propertyId == "numActions")
	{
		var = selectedActions.count ();
		return true;
	}
	if(propertyId == "pauseAllEnabled")
	{
		var = hasActiveActions ();
		return true;
	}
	if(propertyId == "resumeAllEnabled")
	{
		var = hasActiveActions (true);
		return true;
	}
	if(propertyId.contains ("-"))
	{
		int index = -1;
		MutableCString postfix = propertyId.subString (propertyId.index ("-") + 1);
		index = String (postfix).scanInt ();

		if(propertyId.startsWith ("sortBy-"))
		{
			var = ccl_cast<PackageSorter> (sorters.at (index))->getTitle ();
			return true;
		}
		else if(propertyId.startsWith ("message-"))
		{
			if(index >= messages.count () || index < 0)
				return false;
			var.fromString (messages.at (index).message);
			return true;
		}
		else if(propertyId.startsWith ("messageType-"))
		{
			if(index >= messages.count () || index < 0)
				return false;
			var = messages.at (index).type;
			return true;
		}
		else if(propertyId.startsWith ("action-"))
		{
			if(index >= selectedActions.count ())
				return false;
			var.fromString (String ("action-").append (String (Text::kUTF8, selectedActions.at (index).id)));
			return true;
		}
		if(propertyId.startsWith ("actionTitle-"))
		{
			if(index >= selectedActions.count ())
				return false;
			var.fromString (selectedActions.at (index).composedTitle);
			return true;
		}
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PackageManager::getObject (StringID name, UIDRef classID)
{
	if(name == "dropTarget")
		return asUnknown ();

	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageManager::paramChanged (IParameter* param)
{
	CStringRef name = param->getName ();
	int index = -1;
	CStringRef postfix = name.subString (name.index ("-") + 1);
	index = String (postfix).scanInt ();

	if(param->getTag () == Tag::kSortBy)
	{
		sortComponents (true);
		return true;
	}
	else if(param->getTag () == Tag::kConfiguration)
	{
		int value = param->getValue ().asInt ();
		applyConfiguration (value);
		param->setValue (value);
		return true;
	}
	else if(param->getTag () == Tag::kSelectAll)
	{
		selectAll (param->getValue () == param->getMax ());
		return true;
	}
	else if(param->getTag () == Tag::kCancelAll)
	{
		cancelAllActions ();
		return true;
	}
	else if(param->getTag () == Tag::kPauseAll)
	{
		pauseAllActions (true);
		return true;
	}
	else if(param->getTag () == Tag::kResumeAll)
	{
		pauseAllActions (false);
		return true;
	}
	else if(param->getName ().startsWith ("discardMessage-"))
	{
		if(index >= 0 && index < messages.count ())
		{
			messages.removeAt (index);
			deferSignal (NEW Message (kPropertyChanged));
			return true;
		}
		return false;
	}
	else if(param->getName ().startsWith ("action-"))
	{
		for(int i = 0; i < selectedActions.count (); i++)
		{
			if(selectedActions.at (i).id == postfix)
			{
				performSelectedAction (i);
				return true;
			}
		}
		return false;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kUpdate)
		updateAll (false);
	else if(msg == kSort)
		sortComponents (false);
	else if(msg == kUpdateProgress)
		updateProgress (false);
	else if(msg == kUpdateSelectedActions)
		updateSelectedActions (false);
	else if(msg == kMakeVisible && msg.getArgCount () > 0)
		makeVisible (msg[0].asString ());
	else if(msg == kSelect && msg.getArgCount () > 1)
		select (msg[0].asString (), msg[1].asBool ());
	else if(msg == kSetInstallConfiguration && msg.getArgCount () > 0)
		setInstallConfiguration (msg[0].asInt (), false);
	else if(msg == kChanged && subject == packageComponentModel)
	{
		updateSelectedActions (true);
#if DEBUG_PACKAGES
		packageComponentModel->dump ();
#endif
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageManager::getOriginTitle (int originId)
{
	for(const TitleMapping& type : origins)
		if(originId & type.id)
			return type.title;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::updateProgress (bool defer)
{
	if(defer)
	{	
		(NEW Message (kUpdateProgress))->post (this, -1);
	}
	else
	{
		bool anyActionActive = false;
		bool allActionsPaused = true;
		double overallProgress = 0.;
		int count = 0;
		for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		{
			if(allActionsPaused)
				for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
					if(action->getState () == UnifiedPackageAction::kActive)
						allActionsPaused = false;
			
			double progress = component->getProgress ();
			if(progress >= 0)
			{
				anyActionActive = true;
				overallProgress += progress;
				count++;
			}
		}
		overallProgress /= count;

		bool started = inplaceProgress->isInProgress ();
		if(started == false && anyActionActive == true)
			inplaceProgress->beginProgress ();
		else if(started == true && anyActionActive == false)
			inplaceProgress->endProgress ();

		if(!allActionsPaused)
			inplaceProgress->updateProgress (overallProgress);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::updateSelectedActions (bool defer)
{
	if(defer)
	{
		(NEW Message (kUpdateSelectedActions))->post (this, -1);
	}
	else
	{
		bool anySelected = false;
		bool anyNotSelected = false;

		selectedActions.removeAll ();
		for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		{
			if(matchesFilters (*component->getPackage ()) == false)
				continue;

			PackageComponent* topLevelComponent = component;
			while(topLevelComponent && !topLevelComponent->getPackage ()->isTopLevel ())
				topLevelComponent = topLevelComponent->getParentComponent ();
			if(topLevelComponent && !matchesFilters (*topLevelComponent->getPackage ()))
				continue;

			if(component->isSelected () == false)
				anyNotSelected = true;
			else
			{
				anySelected = true;
				for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
				{
					if(action->getState () != UnifiedPackageAction::kEnabled)
						continue;

					if(action->isHidden ())
						continue;

					if(component->canMergeWithChild ())
						if(PackageComponent* child = component->getFirstSubItem ())
							if(child->getActions ().contains (*action))
								continue;

					int index = selectedActions.index (action->getId ());
					if(index < 0)
					{
						selectedActions.add (action->getId ());
						index = selectedActions.count () - 1;
					}
					else
						selectedActions.at (index).count++;

					selectedActions.at (index).size += component->getPackage ()->getSize ();

					if(selectedActions.at (index).title.isEmpty ())
						selectedActions.at (index).title = action->getTitle ();

					if(action->needsConfirmation ())
						selectedActions.at (index).needsConfirmation = true;

					if(selectedActions.at (index).titleCount < 3)
					{
						selectedActions.at (index).packageTitles.append (String::getLineEnd ());
						selectedActions.at (index).packageTitles.append (component->getPackage ()->getTitle ());
						selectedActions.at (index).titleCount++;
					}
					else if(selectedActions.at (index).titleCount == 3)
					{
						selectedActions.at (index).packageTitles.append (String::getLineEnd ());
						selectedActions.at (index).packageTitles.append ("...");
						selectedActions.at (index).titleCount++;
					}

					if(action->getId () == UnifiedPackageHandler::kInstall)
						action->composeTitle (selectedActions.at (index).composedTitle, selectedActions.at (index).count, Format::ByteSize::print (selectedActions.at (index).size));
					else
						action->composeTitle (selectedActions.at (index).composedTitle, selectedActions.at (index).count);
				}
			}
		}

		for(Action& action : selectedActions)
		{
			MutableCString paramId = MutableCString ("action-").append (action.id);
			IParameter* actionParam = paramList.findParameter (paramId);
			if(actionParam == nullptr)
				actionParam = paramList.addParam (paramId);
		}

		if(anySelected == true && anyNotSelected == false)
			paramList.byTag (Tag::kSelectAll)->setValue (Tag::kChecked);
		else if(anySelected == true && anyNotSelected == true)
			paramList.byTag (Tag::kSelectAll)->setValue (Tag::kMixed);
		else if(anySelected == false && anyNotSelected == true)
			paramList.byTag (Tag::kSelectAll)->setValue (Tag::kUnchecked);

		deferSignal (NEW Message (kPropertyChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::updateOverallActionState (bool defer)
{
	if(defer)
		deferSignal (NEW Message (kPropertyChanged));
	else
		signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::performSelectedAction (int index, bool confirmed)
{
	MutableCString actionId = selectedActions.at (index).id;
	bool needsConfirmation = selectedActions.at (index).needsConfirmation;
	int count = selectedActions.at (index).count;

	if(confirmed == false && needsConfirmation)
	{
		SharedPtr<PackageManager> This (this);
		String message (FileStrings::DoYouWantToDeleteTheseFiles (count));
		message.append (String::getLineEnd ());
		message.append (selectedActions.at (index).packageTitles);
		Promise warn (Alert::askAsync (message));
		warn.then ([This, index] (IAsyncOperation& operation)
		{
			if(operation.getResult ().asInt () == Alert::kYes)
			{
				This->performSelectedAction (index, true);
			}
		});
	}
	else
	{
		for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
		{
			if(component->isSelected () && matchesFilters (*component->getPackage ()))
			{
				PackageComponent* topLevelComponent = component;
				while(topLevelComponent && !topLevelComponent->getPackage ()->isTopLevel ())
					topLevelComponent = topLevelComponent->getParentComponent ();
				if(topLevelComponent && !matchesFilters (*topLevelComponent->getPackage ()))
					continue;

				for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
				{
					if(action->getId () != actionId)
						continue;

					if(action->getState () != UnifiedPackageAction::kEnabled)
						break;

					if(component->canMergeWithChild ())
						if(PackageComponent* child = component->getFirstSubItem ())
							if(child->getActions ().contains (*action))
								break;

					SharedPtr<UnifiedPackageAction> guard (action);
					component->performAction (*action, true);
					break;
				}
			}
		}
		updateSelectedActions (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::cancelAllActions ()
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
		{
			if(action->getState () == UnifiedPackageAction::kActive || action->getState () == UnifiedPackageAction::kPaused)
				action->cancel ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::pauseAllActions (bool state)
{
	for(PackageComponent* component : iterate_as<PackageComponent> (packageComponentModel->getItems ()))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->getActions ()))
		{
			if(state && action->getState () == UnifiedPackageAction::kActive)
				component->pauseAction (*action, true);
			else if(!state && action->getState () == UnifiedPackageAction::kPaused)
				component->pauseAction (*action, false);
		}
	}
	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::applyConfiguration (int value)
{
	packageComponentModel->applyConfiguration (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::selectAll (bool state)
{
	packageComponentModel->selectAll (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::deselectFiltered ()
{
	packageComponentModel->deselectFiltered ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::select (StringRef packageId, bool state, bool defer)
{
	if(defer)
	{
		(NEW Message (kSelect, packageId, state))->post (this);
		return true;
	}

	PackageComponent* component = findPackageComponent (packageId);
	if(component == nullptr)
		return false;
	component->getParameterByTag (Tag::kSelected)->setValue (state ? Tag::kChecked : Tag::kUnchecked, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::makeVisible (StringRef packageId, bool defer)
{
	if(defer)
	{
		(NEW Message (kMakeVisible, packageId))->post (this, 500);
		return true;
	}
	else
		return packageComponentModel->makeVisible (packageId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageManager::resetFilters ()
{
	packageComponentModel->resetFilters ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageAction* PackageManager::getInstallAction (UnifiedPackage* package, bool checkEnabled) const
{
	ObjectArray actions;
	actions.objectCleanup ();
	for(IUnifiedPackageHandler* handler : PackageHandlerRegistry::instance ().getHandlers ())
		if(handler->canHandle (package))
			handler->getActions (actions, package);

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == UnifiedPackageHandler::kInstall && (action->getState () == UnifiedPackageAction::kEnabled || checkEnabled == false))
			return return_shared (action);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::canInstall (StringRef packageId) const
{
	UnifiedPackage* package = findPackage (packageId);
	if(package == nullptr)
		return false;
	AutoPtr<UnifiedPackageAction> installAction = getInstallAction (package, false);
	if(installAction == nullptr)
		return false;
	if(installAction->getState () >= UnifiedPackageAction::kEnabled)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageManager::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	AutoPtr<DragHandler> dragHandler (NEW PackageDragHandler (this, targetView));
	if(dragHandler && dragHandler->prepare (data, session))
	{
		if(session)
		{
			session->setDragHandler (dragHandler);
			session->setResult (IDragSession::kDropCopyReal);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageManager::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	CCL_NOT_IMPL ("PackageManager::insertData")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageManager::isVisible () const
{
	return packageComponentModel->isVisible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageManager::canExecute (StringID actionId, const INotification& n) const
{
	if(actionId == kRestartAction)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageManager::execute (StringID actionId, INotification& n)
{
	if(actionId == kRestartAction)
	{
		SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, String::kEmpty, true));
		return kResultOk;
	}
	return kResultFailed;
}

//************************************************************************************************
// CombinedPackage
//************************************************************************************************

CombinedPackage::CombinedPackage (StringRef id)
: UnifiedPackage (id)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CombinedPackage::CombinedPackage (const UnifiedPackage& package)
: UnifiedPackage (package)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CombinedPackage::merge (const UnifiedPackage& other)
{
	if(getTitle ().isEmpty () || (!other.getTitle ().isEmpty () && other.isLocalPackage ()))
		setTitle (other.getTitle ());

	if(getIcon () == nullptr)
		setIcon (other.getIcon ());

	if(getDescription ().isEmpty ())
		setDescription (other.getDescription ());

	if(getVendor ().isEmpty ())
		setVendor (other.getVendor ());

	if(getWebsite ().isEmpty ())
		setWebsite (other.getWebsite ());

	if(getType ().isEmpty () || (other.getType ().isEmpty () == false && other.isLocalPackage ()))
		setType (other.getType ());

	if(!getFileType ().isValid ())
		setFileType (other.getFileType ());

	if((!isLocalPackage () || isLocalPackage () && other.isLocalPackage ()) && getSize () < other.getSize ())
		setSize (other.getSize ());

	setOrigin (other.getOrigin () | getOrigin ());

	if(other.getInstalledVersion () > getInstalledVersion ())
		setInstalledVersion (other.getInstalledVersion ());

	if(other.getCurrentVersion () > getCurrentVersion ())
		setCurrentVersion (other.getCurrentVersion ());

	if(getLicenseData ().isEmpty ())
		setLicenseData (other.getLicenseData ());

	if(getAuthorizerId ().isEmpty ())
		setAuthorizerId (other.getAuthorizerId ());

	if(!isMinimum ())
		isMinimum (other.isMinimum ());

	if(!isRecommended ())
		isRecommended (other.isRecommended ());

	if(!isLocalPackage ())
		isLocalPackage (other.isLocalPackage ());

	if(isTopLevel ())
		isTopLevel (other.isTopLevel ());

	if(!isCritical ())
		isCritical (other.isCritical ());

	if(!isProduct ())
		isProduct (other.isProduct ());

	for(StringRef dependency : other.getDependencies ())
		addDependency (dependency);

	for(StringRef tag : other.getTags ())
		addTag (tag);

	mergeData (other);
}

//************************************************************************************************
// PackageComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageComponent, Component)

DEFINE_STRINGID_MEMBER_ (PackageComponent, kUpdateParentSelectionState, "updateParentSelection");

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent::PackageComponent (PackageManager* _manager, UnifiedPackage* sourcePackage, PackageComponent* parent)
: manager (_manager),
  icon (nullptr),
  title (nullptr),
  id (nullptr),
  showChildren (nullptr),
  type (nullptr),
  version (nullptr),
  size (nullptr),
  description (nullptr),
  state (nullptr),
  selected (nullptr),
  details (NEW ObjectItemModel ("DetailsModel", _manager->getObjectID ())),
  numSubItems (0),
  inplaceProgress (nullptr),
  childProgressFinishCounter (0),
  isUpdatingActions (false)
{
	actions.objectCleanup ();
	sourcePackages.objectCleanup ();

	ASSERT (sourcePackage != nullptr)
	package = NEW CombinedPackage (*sourcePackage);
	package->addObserver (this);

	sourcePackages.add (return_shared (sourcePackage));
	sourcePackage->addObserver (this);

	setParentComponent (parent);

	addObject ("ChildrenList", this);
	addObject ("DetailsList", details);

	addComponent (inplaceProgress = NEW InplaceProgressComponent ());

	id = paramList.addString (CSTR ("id"), Tag::kId);
	title = paramList.addString (CSTR ("title"), Tag::kTitle);
	description = paramList.addString (CSTR ("description"), Tag::kDescription);
	vendor = paramList.addString (CSTR ("vendor"), Tag::kVendor);
	website = paramList.addString (CSTR ("website"), Tag::kWebsite);
	type = paramList.addString (CSTR ("type"), Tag::kType);
	size = paramList.addString (CSTR ("size"), Tag::kSize);
	version = paramList.addString (CSTR ("version"), Tag::kVersion);
	licenseData = paramList.addString (CSTR ("licenseData"), Tag::kLicenseData);
	icon = paramList.addImage (CSTR ("icon"), Tag::kIcon);
	state = paramList.addInteger (0, Tag::kNumPackageStates - 1, CSTR ("state"), Tag::kState);

	selected = paramList.addInteger (Tag::kUnchecked, Tag::kChecked, CSTR ("selected"), Tag::kSelected);
	showChildren = paramList.addParam (CSTR ("showChildren"), Tag::kShowChildren);

	refresh ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent::~PackageComponent ()
{
	for(UnifiedPackage* sourcePackage : iterate_as<UnifiedPackage> (sourcePackages))
		sourcePackage->removeObserver (this);
	package->removeObserver (this);
	if(parentComponent)
		removeObserver (parentComponent);
	details->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::setParentComponent (PackageComponent* parent)
{
	if(parentComponent == parent)
		return;

	if(parentComponent)
		removeObserver (parentComponent);

	// prevent recursion, might happen with conflicting information from different sources
	if(parent && parent->getParentComponent () == this)
		parentComponent = nullptr;
	else
		parentComponent = parent;

	if(parentComponent)
		addObserver (parentComponent);

	if(parent)
	{
		parent->getPackage ()->addChild (package);
		parent->refresh ();
	}

	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageComponent::getParentComponent () const
{
	return parentComponent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* PackageComponent::getPackage () const
{
	return package;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::removeAllActions ()
{
	actions.removeAll ();
	macros.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& PackageComponent::getActions () const
{
	return actions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::removeAllDetails ()
{
	details->removeAllItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::addDetail (Component* detail)
{
	details->addItem (detail);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::merge (UnifiedPackage* other)
{
	if(other->getFileType ().isValid ())
	{
		if(package->isProduct () && package->isLocalPackage ())
		{
			// The new package is a file package while existing source packages are product packages.
			// This could be caused by a clash between product and content IDs.
			// Remove the product packages and keep the file package.
			String message = "ID clash between file package ";
			other->toString (message);
			message << " and product package ";
			package->toString (message);
			message <<  ". Discarding product packages.";
			UnifiedPackage::reportPackageWarning (message);
			
			for(UnifiedPackage* sourcePackage : iterate_as<UnifiedPackage> (sourcePackages))
				sourcePackage->removeObserver (this);
			sourcePackages.removeAll ();
		}
		else if(other->isProduct () && other->isLocalPackage () && package->getFileType ().isValid ())
		{
			// The new package is a product package while existing source packages are file packages.
			// This could be caused by a clash between product and content IDs.
			// Ignore the new product package.
			String message = "ID clash between file package ";
			package->toString (message);
			message << " and product package ";
			other->toString (message);
			message <<  ". Ignoring product package.";
			UnifiedPackage::reportPackageWarning (message);
			
			return;
		}
	}
	if(!sourcePackages.contains (other))
	{
		sourcePackages.add (return_shared (other));
		other->addObserver (this);
	}
	package->merge (*other);
	(NEW Message (kChanged))->post (this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::updateParameters ()
{
	auto getActionParameter = [&] (StringID prefix, StringID id) -> IParameter*
	{
		MutableCString name (prefix);
		name.append (id);
		IParameter* actionParam = paramList.findParameter (name);
		if(actionParam == nullptr)
			actionParam = paramList.addParam (name);
		return actionParam;
	};
	
	bool canInstall = false;
	bool anyRequired = false;
	bool anyUpdates = false;
	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		MutableCString id ("action-");
		id.append (action->getId ());

		IParameter* param = paramList.findParameter (id);
		if(param == nullptr)
		{
			param = paramList.addParam (id);

			MutableCString iconId ("actionIcon-");
			iconId.append (action->getId ());
			IImageProvider* iconParam = paramList.addImage (iconId, Tag::kIcon);
			iconParam->setImage (action->getIcon ());
		}
		param->enable (action->getState () == UnifiedPackageAction::kEnabled);
		
		if(auto cancelParam = getActionParameter ("cancelAction-", action->getId ()))
			cancelParam->enable (action->getState () == UnifiedPackageAction::kActive || action->getState () == UnifiedPackageAction::kPaused);
		if(auto pauseParam = getActionParameter ("pauseAction-", action->getId ()))
			pauseParam->enable (action->getState () == UnifiedPackageAction::kActive);
		if(auto resumeParam = getActionParameter ("resumeAction-", action->getId ()))
			resumeParam->enable (action->getState () == UnifiedPackageAction::kPaused);
		
		if(!package->isLocalPackage () && action->getId () == UnifiedPackageHandler::kInstall)
			canInstall = true;
		if(action->isRequired ())
			anyRequired = true;
		if(action->getId () == UnifiedPackageHandler::kUpdate && action->getState () > UnifiedPackageAction::kDisabled)
			anyUpdates = true;
	}

	updateMacros ();
	for(const Macro& macro : macros)
	{
		MutableCString id ("macro-");
		id.append (macro.actionId);
		IParameter* param = paramList.findParameter (id);
		if(param == nullptr)
			param = paramList.addParam (id);
		
		param->enable (macro.state == UnifiedPackageAction::kEnabled);

		if(auto cancelParam = getActionParameter ("cancelMacro-", macro.actionId))
			cancelParam->enable (macro.cancelEnabled);
		if(auto pauseParam = getActionParameter ("pauseMacro-", macro.actionId))
			pauseParam->enable (macro.pauseEnabled);
		if(auto resumeParam = getActionParameter ("resumeMacro-", macro.actionId))
			resumeParam->enable (macro.resumeEnabled);
		
		if(macro.actionId == UnifiedPackageHandler::kUpdate && macro.state > UnifiedPackageAction::kDisabled)
			anyUpdates = true;
	}

	if(canInstall == false && anyRequired == false && anyUpdates == false)
	{
		for(UnifiedPackage* child : package->getChildren ())
		{
			PackageComponent* childComponent = manager->findPackageComponent (*child);
			if(childComponent == nullptr)
				continue;
			
			if(childComponent->state->getValue ().asInt () == Tag::kActionRequired)
			{
				anyRequired = true;
				break;
			}
			else if(childComponent->state->getValue ().asInt () == Tag::kUpdateAvailable)
				anyUpdates = true;
		}
	}
	state->setValue (canInstall ? Tag::kNotInstalled : (anyRequired ? Tag::kActionRequired : (anyUpdates ? Tag::kUpdateAvailable : Tag::kFullyUsable)), true);

	id->setValue (package->getId (), true);
	if(package->getId ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			id->setValue (child->getId ());
	title->setValue (package->getTitle (), true);
	if(package->getTitle ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			title->setValue (child->getTitle ());
	description->setValue (package->getDescription (), true);
	if(package->getDescription ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			description->setValue (child->getDescription ());
	vendor->setValue (package->getVendor (), true);
	if(package->getVendor ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			vendor->setValue (child->getVendor ());
	website->setValue (package->getWebsite (), true);
	if(package->getWebsite ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			website->setValue (child->getWebsite ());

	int64 byteSize = package->getSize ();
	for(UnifiedPackage* child : package->getChildren ())
	{
		if(manager->matchesFilters (*child))
			byteSize += child->getSize ();
	}
	size->setValue (byteSize > 0 ? Format::ByteSize::print (byteSize) : "");

	VersionNumber versionNumber = package->isLocalPackage () ? package->getInstalledVersion () : package->getCurrentVersion ();
	for(UnifiedPackage* child : package->getChildren ())
	{
		if(versionNumber > 0)
			break;
		versionNumber = child->isLocalPackage () ? child->getInstalledVersion () : child->getCurrentVersion ();
	}
	version->setValue (versionNumber > 0 ? versionNumber.print () : "");

	String license = package->getLicenseData ();
	for(UnifiedPackage* child : package->getChildren ())
	{
		if(license.isEmpty () == false)
			break;
		license = child->getLicenseData ();
	}
	licenseData->setValue (license);

	type->setValue (package->getType ());
	if(package->getType ().isEmpty () && canMergeWithChild ())
		if(UnifiedPackage* child = package->getChildren ().at (0))
			type->setValue (child->getType ());

	AutoPtr<IImage> iconImage = createIconForPackage (*package, canMergeWithChild ());
	icon->setImage (iconImage);

	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PackageComponent::createIconForPackage (const UnifiedPackage& package, bool canMergeWithChild) const
{
	IImage* iconImage = package.getIcon ();
	if(iconImage == nullptr && canMergeWithChild)
		if(UnifiedPackage* child = package.getChildren ().at (0))
			iconImage = child->getIcon ();
	if(iconImage)
		return return_shared (iconImage);

	FileType fileType = package.getFileType ();
	if(fileType.isValid () == false && canMergeWithChild)
		if(UnifiedPackage* child = package.getChildren ().at (0))
			fileType = child->getFileType ();
	if(iconImage == nullptr && fileType.isValid ())
	{
		IImage* fileIcon = FileIcons::instance ().createIcon (fileType, FileIcons::kNoDefaultFolderIcon);
		if(fileIcon != nullptr)
			return fileIcon;
	}

	if(iconImage == nullptr && package.getChildren ().isEmpty () == false)
	{
		UnknownList childIcons;
		for(UnifiedPackage* child : package.getChildren ())
		{
			if(!manager->matchesFilters (*child))
				continue;
			IImage* childIcon = createIconForPackage (*child, false);
			if(childIcon != nullptr)
				childIcons.add (childIcon);
		}
		IImage* packageFolderIcon = getTheme ()->getImage ("PackageFolderIcon"); 
		if(packageFolderIcon == nullptr)
			packageFolderIcon = FileIcons::instance ().getDefaultFolderIcon ();
			
		IImage* folderPreview = FileIcons::instance ().createFolderPreview (packageFolderIcon, childIcons, 64);
		if(folderPreview != nullptr)
			return folderPreview;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::ownsPackage (UnifiedPackage* package)
{
	return this->package == package || sourcePackages.contains (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::isEmpty ()
{
	if(actions.isEmpty () == false)
		return false;
	if(details->isEmpty () == false)
		return false;
	for(UnifiedPackage* child : package->getChildren ())
		if(PackageComponent* childComponent = manager->findPackageComponent (*child))
			if(childComponent->isEmpty () == false)
				return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::isSelected () const
{
	return selected->getValue ().asInt () == Tag::kChecked;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::performActionWithID (StringID actionID, bool recursive)
{
	bool succeeded = false;
	updateActions ();
	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == actionID)
		{
			if(action->getState () == UnifiedPackageAction::kEnabled)
			{
				performAction (*action);
				succeeded = true;			
			}
			break;
		}
	}
	if(recursive)
	{
		for(UnifiedPackage* child : package->getChildren ())
			if(PackageComponent* childComponent = manager->findPackageComponent (*child))
				succeeded |= childComponent->performActionWithID (actionID, true);
	}
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double PackageComponent::getProgress () const
{
	if(inplaceProgress->isInProgress ())
		return inplaceProgress->getProgressValue ();
	return -1.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "package")
	{
		var = ccl_as_unknown (package);
		return true;
	}
	if(propertyId == "numChildren")
	{
		if(canMergeWithChild ())
			var = 0;
		else
			var = numSubItems;
		return true;
	}
	if(propertyId == "numActions")
	{
		if(canMergeWithChild ())
			var = actions.count ();
		else
			var = actions.count () + macros.count ();
		return true;
	}
	if(propertyId == "selectable")
	{
		var = false;
		if(package->isTopLevel () && selected->getValue ().asInt () > Tag::kUnchecked)
			var = true;
		else
		{
			for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
			{
				if(action->getState () == UnifiedPackageAction::kEnabled)
				{
					var = true;
					break;
				}
			}
			if(var.asBool () == false && canMergeWithChild () == false)
			{
				for(const Macro& macro : macros)
				{
					if(macro.state == UnifiedPackageAction::kEnabled)
					{
						var = true;
						break;
					}
				}
			}
		}
		return true;
	}
	if(propertyId.contains ("-"))
	{

		int index = -1;
		MutableCString postfix = propertyId.subString (propertyId.index ("-") + 1);
		index = String (postfix).scanInt ();

		if(propertyId.startsWith ("action-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var.fromString (String ("action-").append (String (ccl_cast<UnifiedPackageAction> (actions.at (index))->getId ())));
			else
				var.fromString (String ("macro-").append (String (macros.at (index - actions.count ()).actionId)));
			return true;
		}
		if(propertyId.startsWith ("cancelAction-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var.fromString (String ("cancelAction-").append (String (ccl_cast<UnifiedPackageAction> (actions.at (index))->getId ())));
			else
				var.fromString (String ("cancelMacro-").append (String (macros.at (index - actions.count ()).actionId)));
			return true;
		}
		if(propertyId.startsWith ("pauseAction-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var.fromString (String ("pauseAction-").append (String (ccl_cast<UnifiedPackageAction> (actions.at (index))->getId ())));
			else
				var.fromString (String ("pauseMacro-").append (String (macros.at (index - actions.count ()).actionId)));
			return true;
		}
		if(propertyId.startsWith ("resumeAction-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var.fromString (String ("resumeAction-").append (String (ccl_cast<UnifiedPackageAction> (actions.at (index))->getId ())));
			else
				var.fromString (String ("resumeMacro-").append (String (macros.at (index - actions.count ()).actionId)));
			return true;
		}
		if(propertyId.startsWith ("actionTitle-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var.fromString (ccl_cast<UnifiedPackageAction> (actions.at (index))->getTitle ());
			else
				var.fromString (macros.at (index - actions.count ()).title);
			return true;
		}
		if(propertyId.startsWith ("actionStateLabel-"))
		{
			if(index >= actions.count ())
				return false;
			var.fromString (ccl_cast<UnifiedPackageAction> (actions.at (index))->getStateLabel ());
			return true;
		}
		if(propertyId.startsWith ("actionHidden-"))
		{
			if(index >= actions.count ())
				return false;
			var = ccl_cast<UnifiedPackageAction> (actions.at (index))->isHidden ();
			return true;
		}
		if(propertyId.startsWith ("actionState-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var = ccl_cast<UnifiedPackageAction> (actions.at (index))->getState ();
			else
				var = macros.at (index - actions.count ()).state;
			return true;
		}
		if(propertyId.startsWith ("actionCancelEnabled-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
				var = ccl_cast<UnifiedPackageAction> (actions.at (index))->isCancelEnabled ();
			else
				var = macros.at (index - actions.count ()).cancelEnabled;
			return true;
		}
		if(propertyId.startsWith ("actionPauseEnabled-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
			{
				auto action = ccl_cast<UnifiedPackageAction> (actions.at (index));
				var = action->isResumable () && action->getState () == UnifiedPackageAction::kActive;
			}
			else
				var = macros.at (index - actions.count ()).pauseEnabled;
			return true;
		}
		if(propertyId.startsWith ("actionResumeEnabled-"))
		{
			if(index >= actions.count () + macros.count ())
				return false;
			if(index < actions.count ())
			{
				auto action = ccl_cast<UnifiedPackageAction> (actions.at (index));
				var = action->isResumable () && action->getState () == UnifiedPackageAction::kPaused;
			}
			else
				var = macros.at (index - actions.count ()).resumeEnabled;
			return true;
		}
	}
	if(propertyId == "hasIcon")
	{
		var = icon->getImage () != nullptr;
		return true;
	}
	if(propertyId == "hasType")
	{
		var = type->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasVendor")
	{
		var = vendor->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasWebsite")
	{
		var = website->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasDescription")
	{
		var = description->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasSize")
	{
		var = size->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasVersion")
	{
		var = version->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "hasLicenseData")
	{
		var = licenseData->getValue ().asString ().isEmpty () == false;
		return true;
	}
	if(propertyId == "childrenStates")
	{
		var.fromString (getChildrenStateDescription ());
		return true;
	}
	if(propertyId == "numDetails")
	{
		var = details->count ();
		return true;
	}
	if(propertyId == "state-title")
	{
		int packageState = state->getValue ().asInt ();
		switch(packageState)
		{
		case Tag::kNotInstalled :
			var.fromString (XSTR (NotInstalled));
			return true;
		case Tag::kActionRequired :
			var.fromString (XSTR (ActionRequired));
			return true;
		case Tag::kUpdateAvailable :
			var.fromString (XSTR (UpdateAvailable));
			return true;
		case Tag::kFullyUsable :
			var.fromString (XSTR (FullyUsable));
			return true;
		default :
			return false;
		}
	}
	if(SuperClass::getProperty (var, propertyId))
		return true;
	if(IParameter* param = findParameter (propertyId))
	{
		var = param->getValue ();
		return true;
	}
	else
		return manager->getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageComponent::getChildrenStateDescription () const
{
	UnknownList subItems;
	getSubItems (subItems);

	struct State
	{
		CString groupId;
		String title;
		int done;
		int count;
	};
	Vector<State> states;
	int total = 0;

	ForEachUnknown (subItems, unk)
		++total;
		PackageComponent* component = unknown_cast<PackageComponent> (unk);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
		{
			if(action->getState () < UnifiedPackageAction::kDisabled)
				continue;

			CStringRef groupId = action->getGroupId ();
			StringRef title = action->getTitle ();
			StringRef stateLabel = action->getGroupStateLabel ();

			bool haveState = false;
			for(State& state : states)
				if(groupId == state.groupId)
				{
					++state.count;
					if(action->getId () != groupId)
						++state.done;
					haveState = true;
					break;
				}
			if(haveState == false && groupId.isEmpty () == false && stateLabel.isEmpty () == false)
				states.add ({ groupId, stateLabel, action->getId () != groupId ? 1 : 0, 1 });
		}
	EndFor

	String description;
	description.appendFormat (XSTR (ItemCount), total);

	String actionStates;
	for(State& state : states)
	{
		if(actionStates.isEmpty () == false)
			actionStates.append (", ");
		actionStates.appendFormat (XSTR (ActionState), state.title, state.done, state.count);
	}

	if(actionStates.isEmpty () == false)
		description.appendFormat (" (%(1))", actionStates);

	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::updateMacros ()
{
	auto findMacro = [this] (StringID actionId) -> Macro* {
		for(Macro& macro : macros)
			if(macro.actionId == actionId)
				return &macro;
		return nullptr;
	};

	UnknownList subItems;
	getSubItems (subItems);

	struct CompareMacroID
	{
		static DEFINE_VECTOR_COMPARE_OBJECT (sorter, Macro, m1, m2)
			return m1->actionId.compare (m2->actionId);
		}
	};

	macros.removeAll ();
	ForEachUnknown (subItems, unk)
		PackageComponent* component = unknown_cast<PackageComponent> (unk);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
		{
			StringRef macroTitle = action->getMacroTitle ();
			if(macroTitle.isEmpty () == false)
			{
				Macro* macro = findMacro (action->getId ());
				if(macro == nullptr)
				{
					macros.addSorted ({ action->getId (), macroTitle, UnifiedPackageAction::kDisabled, false, false, false}, CompareMacroID::sorter);
					macro = findMacro (action->getId ());
				}
				if(macro)
				{
					if(action->getState () == UnifiedPackageAction::kEnabled && macro->state == UnifiedPackageAction::kDisabled)
						macro->state = UnifiedPackageAction::kEnabled;
					if(action->getState () == UnifiedPackageAction::kActive || action->getState () == UnifiedPackageAction::kPaused)
						macro->state = UnifiedPackageAction::kActive;
					
					if(action->isCancelEnabled ())
						macro->cancelEnabled = true;
					
					if(action->isResumable ())
					{
						if(action->getState () == UnifiedPackageAction::kPaused)
							macro->resumeEnabled = true;
						else if(action->getState () == UnifiedPackageAction::kActive)
							macro->pauseEnabled = true;
					}
				}
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageComponent::paramChanged (IParameter* param)
{
	CStringRef name = param->getName ();
	int index = -1;
	CStringRef postfix = name.subString (name.index ("-") + 1);
	index = String (postfix).scanInt ();

	if(param->getName () == manager->getSectionPropertyId ())
		manager->sortComponents (true);

	switch(param->getTag ())
	{
	case Tag::kShowChildren :
		if(param->getValue ().asBool () == true)
			manager->onShowChildren (*package);
		return true;
	case Tag::kSelected :
		handleSelection (param->getValue ().asInt ());
		return true;
	}
	if(param->getName ().startsWith ("action-"))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
			if(action->getId () == postfix && action->getState () == UnifiedPackageAction::kEnabled)
			{
				performAction (*action);
				return true;
			}
	}
	else if(param->getName ().startsWith ("cancelAction-"))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
			if(action->getId () == postfix && (action->getState () == UnifiedPackageAction::kActive || action->getState () == UnifiedPackageAction::kPaused))
			{
				cancelAction (*action);
				return true;
			}
	}
	else if(param->getName ().startsWith ("pauseAction-"))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
			if(action->getId () == postfix && action->getState () == UnifiedPackageAction::kActive)
			{
				pauseAction (*action, true);
				signal (Message (kPropertyChanged));
				return true;
			}
	}
	else if(param->getName ().startsWith ("resumeAction-"))
	{
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
			if(action->getId () == postfix && action->getState () == UnifiedPackageAction::kPaused)
			{
				pauseAction (*action, false);
				signal (Message (kPropertyChanged));
				return true;
			}
	}
	else if(param->getName ().startsWith ("macro-"))
	{
		for(auto& macro : macros)
			if(macro.actionId == postfix && macro.state == UnifiedPackageAction::kEnabled)
			{
				performMacro (macro);
				return true;
			}
	}
	else if(param->getName ().startsWith ("cancelMacro-"))
	{
		for(auto& macro : macros)
			if(macro.actionId == postfix && (macro.state == UnifiedPackageAction::kActive || macro.state == UnifiedPackageAction::kPaused))
			{
				cancelMacro (macro);
				return true;
			}
	}
	else if(param->getName ().startsWith ("pauseMacro-"))
	{
		for(auto& macro : macros)
			if(macro.actionId == postfix && macro.state == UnifiedPackageAction::kActive)
			{
				pauseMacro (macro);
				return true;
			}
	}
	else if(param->getName ().startsWith ("resumeMacro-"))
	{
		for(auto& macro : macros)
			if(macro.actionId == postfix && macro.state == UnifiedPackageAction::kActive) // kPaused is not used on macros
			{
				pauseMacro (macro, false);
				return true;
			}
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::handleSelection (int state)
{
	if(state == Tag::kChecked)
	{
		for(StringRef dependency : package->getDependencies ())
		{
			if(PackageComponent* component = manager->findPackageComponent (dependency))
				component->selected->setValue (state, true);
		}
	}
	for(UnifiedPackage* child : package->getChildren ())
	{
		if(manager->matchesFilters (*child) == false)
			continue;
		if(PackageComponent* component = manager->findPackageComponent (*child))
			component->selected->setValue (state, true);
	}
	updateParentSelectionState (true);
	manager->setInstallConfiguration (PackageManager::kCustomInstall);
	manager->updateSelectedActions (true);

	signalPropertyChanged ("selectable", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::updateParentSelectionState (bool defer)
{
	if(defer)
		(NEW Message (kUpdateParentSelectionState))->post (this, -1);
	else
	{
		PackageComponent* parentComponent = getParentComponent ();
		while(parentComponent != nullptr)
		{
			bool anyNotSelected = false;
			bool anySelected = false;
			for(UnifiedPackage* child : parentComponent->getPackage ()->getChildren ())
			{
				if(manager->matchesFilters (*child) == false)
					continue;
				if(PackageComponent* component = manager->findPackageComponent (*child))
				{
					bool selected = component->isSelected ();
					if(selected)
						anySelected = true;
					else
						anyNotSelected = true;
				}
				if(anySelected && anyNotSelected)
					break;
			}
			if(anySelected == true && anyNotSelected == false)
				parentComponent->selected->setValue (Tag::kChecked);
			else if(anySelected == true && anyNotSelected == true)
				parentComponent->selected->setValue (Tag::kMixed);
			else if(anySelected == false && anyNotSelected == true)
				parentComponent->selected->setValue (Tag::kUnchecked);

			parentComponent = parentComponent->getParentComponent ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::performAction (UnifiedPackageAction& action, bool confirmed)
{	
	ASSERT (action.getPackage ())
	if(action.getPackage () == nullptr)
		return;

	if(ownsPackage (action.getPackage ()) == false && canMergeWithChild ())
	{
		PackageComponent* childComponent = getFirstSubItem ();
		if(childComponent && childComponent->ownsPackage (action.getPackage ()))
		{
			childComponent->performAction (action, confirmed);
			return;
		}
	}

	if(confirmed == false && action.needsConfirmation ())
	{
		SharedPtr<PackageComponent> This (this);
		
		Promise warn (Alert::askAsync (String () <<	FileStrings::DoYouWantToDeleteTheseFiles (1) << ENDLINE << ENDLINE << package->getTitle ()));
		warn.then ([This, &action] (IAsyncOperation& operation) 
		{
			if(operation.getResult ().asInt () == Alert::kYes)
			{
				This->performAction (action, true);
			}
		});
	}
	else
	{
		manager->clearMessages ();
		selected->setValue (Tag::kUnchecked);
		inplaceProgress->setProgressText (action.getTitle ());

		SharedPtr<PackageComponent> guard (this);
		
		action.setObserver (this);
		if(action.perform () == false)
			action.setObserver (nullptr);

		updateParameters ();
		deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::cancelAction (UnifiedPackageAction& action)
{
	action.cancel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::pauseAction (UnifiedPackageAction& action, bool state)
{
	action.pause (state);
	updateParameters ();
	deferChanged ();
	manager->updateOverallActionState (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::performMacro (const Macro& macro, bool confirmed)
{
	UnknownList subItems;
	getSubItems (subItems);

	if(confirmed == false)
	{
		String packageTitles;
		int titleCount = 0;

		bool needsConfirmation = false;
		ForEachUnknown (subItems, unk)
			PackageComponent* component = unknown_cast<PackageComponent> (unk);
			for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
			{
				if(action->getId () == macro.actionId && action->needsConfirmation ())
				{
					if(titleCount < 3)
					{
						packageTitles.append (String::getLineEnd ());
						packageTitles.append (component->getPackage ()->getTitle ());
					}
					titleCount++;
					needsConfirmation = true;
					break;
				}
			}
			if(needsConfirmation)
				break;
		EndFor

		if(needsConfirmation)
		{
			SharedPtr<PackageComponent> This (this);

			String message (FileStrings::DoYouWantToDeleteTheseFiles (package->getChildren ().count ()));
			message.append (String::getLineEnd ());
			message.append (packageTitles);
			if(titleCount > 3)
			{
				message.append (String::getLineEnd ());
				message.append ("...");
			}
			
			Promise warn (Alert::askAsync (message));
			warn.then ([This, macro] (IAsyncOperation& operation)
			{
				if(operation.getResult ().asInt () == Alert::kYes)
				{
					This->performMacro (macro, true);
				}
			});

			return;
		}
	}
	
	ForEachUnknown (subItems, unk)
		PackageComponent* component = unknown_cast<PackageComponent> (unk);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
		{
			if(action->getId () == macro.actionId)
			{
				component->performAction (*action, true);
				break;
			}
		}
	EndFor
	updateParameters ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::cancelMacro (const Macro& macro)
{
	UnknownList subItems;
	getSubItems (subItems);
	
	ForEachUnknown (subItems, unk)
		PackageComponent* component = unknown_cast<PackageComponent> (unk);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
			if(action->getId () == macro.actionId)
				component->cancelAction (*action);
	EndFor
	updateParameters ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::pauseMacro (const Macro& macro, bool state)
{
	UnknownList subItems;
	getSubItems (subItems);
	
	ForEachUnknown (subItems, unk)
		PackageComponent* component = unknown_cast<PackageComponent> (unk);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (component->actions))
			if(action->getId () == macro.actionId)
			{
				if(state && action->getState () == UnifiedPackageAction::kActive)
					component->pauseAction (*action, true);
				else if(!state && action->getState () == UnifiedPackageAction::kPaused)
					component->pauseAction (*action, false);
			}
	EndFor
	updateParameters ();
	manager->updateOverallActionState (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::sortActions ()
{
	auto sortByGroupId = LAMBDA_VECTOR_COMPARE (UnifiedPackageAction, lhs, rhs)
		int comp = lhs->getGroupId ().compare (rhs->getGroupId ());
		if(comp == 0)
			comp = lhs->getId ().compare (rhs->getId ());
		return comp;
	};
	actions.sort (sortByGroupId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		UnifiedPackage* sourcePackage = unknown_cast<UnifiedPackage> (subject);
		if(sourcePackage != nullptr && sourcePackages.contains (sourcePackage))
		{
			package->merge (*sourcePackage);
			updateParameters ();
		}
		else if(unknown_cast<PackageComponent> (subject) || subject == nullptr)
			refresh ();
	}
	else if(msg == kUpdateParentSelectionState)
		updateParentSelectionState (false);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::refresh ()
{
	reset ();
	for(UnifiedPackage* sourcePackage : iterate_as<UnifiedPackage> (sourcePackages))
	{
		package->merge (*sourcePackage);
		for(UnifiedPackage* child : sourcePackage->getChildren ())
		{
			if(PackageComponent* component = manager->findPackageComponent (child->getId ()))
				package->addChild (component->getPackage ());
		}
	}

	updateSubItemCount ();
	updateActions ();
	updateDetailComponents ();
	updateParameters ();
	manager->updateSelectedActions (true);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::updateSubItemCount ()
{
	matchingChildren.removeAll ();
	numSubItems = 0;
	for(UnifiedPackage* child : package->getChildren ())
	{
		if(manager->matchesFilters (*child) && !child->getTitle ().isEmpty ())
		{
			matchingChildren.add (child);
			++numSubItems;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::reset ()
{
	*package = CombinedPackage (package->getId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PackageComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	MutableCString nameSpace = manager->getObjectID ();
	nameSpace.append ("/");
	if(name == "ChildrenListItem")
		return getTheme ()->createView (nameSpace.append ("ChildrenListItem"), data.asUnknown ());
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageComponent::getSubItems (IUnknownList& outItems, ItemIndexRef index)
{
	return getSubItems (outItems);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::getSubItems (IUnknownList& outItems) const
{
	for(UnifiedPackage* child : iterate_as<UnifiedPackage> (matchingChildren))
	{
		if(PackageComponent* component = manager->findPackageComponent (*child))
			outItems.add (component->asUnknown (), true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageComponent::getFirstSubItem () const
{
	if(matchingChildren.isEmpty () == false)
	{
		return manager->findPackageComponent (*static_cast<UnifiedPackage*> (matchingChildren.at (0)));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::onChildProgress (PackageComponent* child, double progress, ProgressState state)
{
	if(state == kProgressStart)
	{
		inplaceProgress->beginProgress ();
		childProgressFinishCounter = 0; // reset if new child starts, creates jump anyway
		manager->updateProgress (true);
	}
	else if(state == kProgressEnd)
	{
		inplaceProgress->endProgress ();
		if(ccl_equals (progress, 1.0, 0.001))
			childProgressFinishCounter++;
		manager->updateProgress (true);

		selected->setValue (Tag::kUnchecked);
	}

	if(inplaceProgress->isInProgress ())
	{
		UnknownList subItems;
		getSubItems (subItems);
		int count = childProgressFinishCounter;
		double total = childProgressFinishCounter;		

		ForEachUnknown (subItems, unk)
			if(PackageComponent* subItem = unknown_cast<PackageComponent> (unk))
			{
				if(subItem->inplaceProgress->isInProgress ())
				{
					count++;
					total += subItem->inplaceProgress->getProgressValue ();
				}
			}
		EndFor
		if(count > 0)
			inplaceProgress->updateProgress (total / count);
	}
	else
	{
		childProgressFinishCounter = 0;	
		manager->updateProgress (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::onProgress (const UnifiedPackageAction& action, double progress)
{
	UnifiedPackage* package = action.getPackage ();
	if(package == nullptr)
		return;

	if(ownsPackage (package))
	{
		bool started = false;
		if(inplaceProgress->isInProgress () == false)
		{
			inplaceProgress->beginProgress ();
			manager->updateProgress (true);

			deferSignal (NEW Message (kPropertyChanged));

			started = true;
		}

		inplaceProgress->updateProgress (progress);

		if(parentComponent)
			parentComponent->onChildProgress (this, progress, started ? kProgressStart : kProgressUpdate);

		manager->updateProgress (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::onCompletion (const UnifiedPackageAction& action, bool succeeded)
{
	SOFT_ASSERT (ownsPackage (action.getPackage ()), "Action for unknown package completed")

	if(inplaceProgress->isInProgress ())
	{
		inplaceProgress->endProgress ();
		if(parentComponent)
			parentComponent->onChildProgress (this, succeeded ? 1.0 : 0.0, kProgressEnd);
		manager->updateProgress (true);
	}

	selected->setValue (Tag::kUnchecked);
	updateParameters ();
	deferChanged ();

	manager->onCompletion (action, succeeded);
	manager->update (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::onPackageChanged (UnifiedPackage* package)
{
	ASSERT (ownsPackage (package))
		
	for(int i = 0; i < package->getChildren ().count (); ++i)
	{
		UnifiedPackage* child = package->getChildren ().at (i);
		if(child)
		{
			if(PackageComponent* component = manager->findPackageComponent (*child))
				component->onPackageChanged (child);
		}
	}

	reset ();

	for(UnifiedPackage* sourcePackage : iterate_as<UnifiedPackage> (sourcePackages))
		sourcePackage->removeObserver (this);
	sourcePackages.removeAll ();

	manager->update (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::onPause (const UnifiedPackageAction& action, bool state)
{
	updateParameters ();
	deferChanged ();
	manager->updateOverallActionState (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::requestRestart (const UnifiedPackageAction& action, StringRef message)
{
	manager->requestRestart (message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponent::reportEvent (const Alert::Event& event)
{
	manager->reportEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageComponent::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::updateActions ()
{
	ASSERT (isUpdatingActions == false)
	if(isUpdatingActions)
		return false;

	ScopedVar<bool> scope (isUpdatingActions, true);

	ObjectArray packageActions;
	packageActions.objectCleanup ();

	for(IUnifiedPackageHandler* handler : PackageHandlerRegistry::instance ().getHandlers ())
	{
		if(handler->canHandle (package))
			handler->getActions (packageActions, package);
	}

	removeAllActions ();

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (packageActions))
	{
		if(action->getState () >= UnifiedPackageAction::kDisabled)
			actions.add (return_shared (action));
	}

	if(canMergeWithChild ())
	{
		PackageComponent* subItem = getFirstSubItem ();
		if(subItem)
		{
			subItem->updateActions ();
			for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (subItem->actions))
			{
				if(!actions.contains (*action))
					actions.add (return_shared (action));
			}
		}
	}

	sortActions ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::updateDetailComponents ()
{
	removeAllDetails ();

	for(IUnifiedPackageHandler* handler : PackageHandlerRegistry::instance ().getHandlers ())
	{
		if(handler->canHandle (package))
			if(Component* detailComponent = handler->createComponent (package))
				addDetail (detailComponent);
	}

	if(canMergeWithChild ())
	{
		PackageComponent* subItem = getFirstSubItem ();
		if(subItem)
			for(IUnifiedPackageHandler* handler : PackageHandlerRegistry::instance ().getHandlers ())
				if(handler->canHandle (subItem->getPackage ()))
					if(Component* detailComponent = handler->createComponent (subItem->getPackage ()))
						addDetail (detailComponent);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponent::canMergeWithChild () const
{
	return package->getChildren ().count () == 1 && package->isCritical () == false;
}

//************************************************************************************************
// PackageComponentModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageComponentModel, ObjectItemModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponentModel::PackageComponentModel (StringRef name, StringRef managerName, StringRef title)
: ObjectItemModel (name, managerName, title),
  filterComponentModel (NEW ObjectItemModel),
  sectionPropertyAscending (true)
{
	addObject ("FilterList", filterComponentModel);
	filterComponentModel->addObserver (this);

	sectionHeaders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponentModel::~PackageComponentModel ()
{
	filterComponentModel->removeObserver (this);
	filterComponentModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageComponentModel::findItem (StringRef id, bool filter) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
	{
		if(component->getPackage ()->getId () == id)
		{
			if(filter == false || matchesFilters (*component->getPackage ()))
				return component;
			else
				return nullptr;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageComponent* PackageComponentModel::findItem (const UnifiedPackage& package, bool filter) const
{
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
	{
		if(component->getPackage () == &package)
		{
			if(filter == false || matchesFilters (*component->getPackage ()))
				return component;
			else
				return nullptr;
		}
	}
	return findItem (package.getId (), filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::addFilter (IObjectFilter* filter)
{
	if(Object* obj = unknown_cast<Object> (filter))
		filterComponentModel->addItem (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponentModel::matchesFilters (UnifiedPackage& package) const
{
	for(IObject* obj : filterComponentModel->getItems ())
		if(UnknownPtr<IObjectFilter> filter = obj)
			if(filter->matches (package.asUnknown ()) == false)
				return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::resetFilters ()
{
	for(IObject* obj : filterComponentModel->getItems ())
		if(PackageFilterComponent* filter = unknown_cast<PackageFilterComponent> (obj))
			if(filter->isHidden () == false)
				filter->reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::setSectionPropertyId (StringID propertyId, bool ascending)
{
	sectionPropertyId = propertyId;
	sectionPropertyAscending = ascending;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PackageComponentModel::getSectionPropertyId () const
{
	return sectionPropertyId;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::sortComponents (PackageSorter* sorter)
{
	filterComponentModel->deferChanged ();
	PackageComponentSorter::setSectionPropertyId (sectionPropertyId, sectionPropertyAscending);
	PackageComponentSorter::setSorter (sorter);
	items.sort (PackageComponentSorter::compare);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::applyConfiguration (int type)
{
	if(type != PackageManager::kCustomInstall)
	{
		for(IObject* obj : filterComponentModel->getItems ())
		{
			if(PackageFilterComponent* filter = unknown_cast<PackageFilterComponent> (obj))
			{
				if(!filter->isHidden ())
					filter->reset ();
			}
		}

		auto matches = [] (const PackageComponent& component, int type)
		{
			switch(type)
			{
			case PackageManager::kMinimalInstall :
				return component.getPackage ()->isMinimum ();
			case PackageManager::kRecommendedInstall :
				return component.getPackage ()->isRecommended ();
			case PackageManager::kFullInstall :
				return true;
			default : // Tag::kCustomInstall
				return component.isSelected ();
			}
		};

		for(PackageComponent* component : iterate_as<PackageComponent> (items))
		{
			if(matches (*component, type) == false)
				component->getParameterByTag (Tag::kSelected)->setValue (Tag::kUnchecked, true);
		}
		for(PackageComponent* component : iterate_as<PackageComponent> (items))
		{
			if(matches (*component, type))
				component->getParameterByTag (Tag::kSelected)->setValue (Tag::kChecked, true);
		}
	}

	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::selectAll (bool state)
{
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
	{
		if(matchesFilters (*component->getPackage ()) == false)
			continue;
		component->getParameterByTag (Tag::kSelected)->setValue (state ? Tag::kChecked : Tag::kUnchecked);
	}
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageComponentModel::deselectFiltered ()
{
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
	{
		if(matchesFilters (*component->getPackage ()) == false)
			component->getParameterByTag (Tag::kSelected)->setValue (Tag::kUnchecked);
	}
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageComponentModel::makeVisible (StringRef packageId)
{
	IItemView* view = getItemView ();
	if(view == nullptr)
		return false;

	ItemIndex index;

	UnknownList items;
	if(getSubItems (items, ItemIndex ()) == false)
		return false;

	int i = 0;
	ForEachUnknown (items, unk)
		PackageComponent* packageComponent = unknown_cast<PackageComponent> (unk);
		if(packageComponent != nullptr)
		{
			UnifiedPackage* package = packageComponent->getPackage ();
			if(package->getId () == packageId)
			{
				index = i;
				break;
			}
			bool foundChild = false;
			for(UnifiedPackage* child : package->getChildren ())
			{
				if(child->getId () == packageId)
				{
					index = i;
					foundChild = true;
					break;
				}
			}
			if(foundChild)
				break;
		}
		i++;
	EndFor

	if(index.isValid ())
		view->makeItemVisible (index);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageComponentModel::getSubItems (IUnknownList& outItems, ItemIndexRef index)
{
	sectionHeaders.removeAll ();
	Variant sectionPropertyValue;
	Variant lastSectionPropertyValue;
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
		if(matchesFilters (*component->getPackage ()) && component->getPackage ()->isTopLevel ())
		{
			if(sectionPropertyId.isEmpty () == false && component->getProperty (sectionPropertyValue, sectionPropertyId))
			{
				if(lastSectionPropertyValue != sectionPropertyValue)
				{
					String title = sectionPropertyValue.asString ();
					Variant propertyTitle;
					if(component->getProperty (propertyTitle, MutableCString (sectionPropertyId).append ("-title")))
						title = propertyTitle.asString ();

					Component* header = NEW Component ("SectionHeader", title);
					sectionHeaders.add (header);
					outItems.add (header->asUnknown (), true);
					lastSectionPropertyValue = sectionPropertyValue;
				}
			}
			outItems.add (component->asUnknown (), true);
		}

	if(outItems.isEmpty ())
		outItems.add (ccl_as_unknown (NEW Component ("Placeholder")), false);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageComponentModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject == filterComponentModel)
	{
		deferChanged ();
		for(PackageComponent* item : iterate_as<PackageComponent> (items))
		{
			item->updateSubItemCount ();
			item->updateActions ();
			item->updateParameters ();
			item->updateParentSelectionState (true);
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_PACKAGES
void PackageComponentModel::dump ()
{
	String indent;
	const auto dumpPackage = [this, &indent] (UnifiedPackage& package) -> void {
		auto dumpPackage_ = [this, &indent] (UnifiedPackage& package, auto& dumpFn) -> void {
			CCL_PRINTF ("%s%s\n", MutableCString (indent).str (), MutableCString (package.getId ()).str ())
			indent.appendASCII ("\t");
			for(UnifiedPackage* child : package.getChildren ())
				if(matchesFilters (*child))
					dumpFn (*child, dumpFn);
			indent.remove (indent.length () - 1, 1);
		};
		dumpPackage_ (package, dumpPackage_);
	};

	CCL_PRINTLN ("*********************************************")
	CCL_PRINTF ("%s\n", MutableCString (getObjectID ()).str ())
	CCL_PRINTLN ("*********************************************")
	for(PackageComponent* component : iterate_as<PackageComponent> (items))
		if(component->getPackage ()->isTopLevel () && matchesFilters (*component->getPackage ()))
			dumpPackage (*component->getPackage ());
	CCL_PRINTLN ("")
}
#endif

//************************************************************************************************
// ObjectItemModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ObjectItemModel, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectItemModel::ObjectItemModel (StringRef name, StringRef managerName, StringRef title)
: Component (name, title),
  managerName (managerName),
  viewCount (0)
{
	items.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectItemModel::~ObjectItemModel ()
{
	removeAllItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectItemModel::addItem (Object* item)
{
	items.add (item);
	item->addObserver (this);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectItemModel::removeItem (Object* item)
{
	if(items.remove (item))
	{
		deferChanged ();
		item->removeObserver (this);
		item->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectItemModel::removeAllItems ()
{
	for(Object* item : items)
		item->removeObserver (this);
	items.removeAll ();
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ObjectItemModel::getItems () const
{
	return items;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectItemModel::count () const
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectItemModel::isEmpty () const
{
	return count () == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectItemModel::isVisible () const
{
	return viewCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectItemModel::getSubItems (IUnknownList& outItems, ItemIndexRef index)
{
	for(IObject* item : items)
		outItems.add (item, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ObjectItemModel::createView (StringID name, VariantRef data, const Rect& bounds)
{
	MutableCString nameSpace = managerName;
	nameSpace.append ("/");
	if(Component* comp = unknown_cast<Component> (data.asUnknown ()))
		if(IView* itemView = comp->createView (name, data, bounds))
			return itemView;
	return getTheme ()->createView (nameSpace.append (name), data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectItemModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && items.contains (unknown_cast<Object> (subject)))
		deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectItemModel::viewAttached (IItemView* itemView)
{
	viewCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectItemModel::viewDetached (IItemView* itemView)
{
	viewCount--;
}

//************************************************************************************************
// PackageDragHandler
//************************************************************************************************

PackageDragHandler::PackageDragHandler (PackageManager* manager, IView* view)
: DragHandler (view),
  manager (manager)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageDragHandler::drop (const DragEvent& event)
{
	UnifiedPackage* package = unknown_cast<UnifiedPackage> (getData ().getFirst ());
	ASSERT (package)
	if(package)
	{
		manager->addPackage (package);
		install (package);
	}

	return DragHandler::drop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PackageDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IUrl> path (&item);
	if(path)
	{
		AutoPtr<File> file = NEW File (*path);
		if(file->isFile () == false || file->exists () == false)
			return nullptr;

		String name;
		path->getName (name);
		AutoPtr<UnifiedPackage> package = manager->createPackageFromFile (*path);
		if(package.isValid () == false)
			package = NEW UnifiedPackage (name);
		package->setData<File> (file);
		package->isLocalInstallationAllowed (true);

		AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*path));
		spriteBuilder.addItem (icon, name);

		package->retain ();
		return package->asUnknown ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageDragHandler::finishPrepare ()
{
	if(getData ().isEmpty ())
		return;
	
	UnifiedPackage* package = unknown_cast<UnifiedPackage> (getData ().getFirst ());
	AutoPtr<UnifiedPackageAction> installAction = manager->getInstallAction (package, false);
	if(installAction.isValid ())
	{
		String title;
		installAction->composeTitle (title, 1);
		spriteBuilder.addHeader (title, -1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageDragHandler::install (UnifiedPackage* package)
{
	manager->addPackage (package);
	IUnifiedPackageHandlerObserver* observer = manager->findPackageComponent (*package);
	if(observer == nullptr)
		return;

	AutoPtr<UnifiedPackageAction> installAction = manager->getInstallAction (package);
	if(installAction.isValid ())
	{
		manager->clearMessages ();
		installAction->setObserver (observer);
		if(installAction->perform () == false)
			installAction->setObserver (nullptr);
	}
	manager->makeVisible (package->getId ());
}

//************************************************************************************************
// PackageNotificationFilter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageNotificationFilter, Object)
const String PackageNotificationFilter::kSubCategory = "Packages";

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageNotificationFilter::matches (IUnknown* object) const
{
	UnknownPtr<INotification> notification (object);
	if(!notification.isValid ())
		return false;

	Variant subCategory;
	if(!notification->getAttributes ().getAttribute (subCategory, INotification::kSubCategory))
		return false;

	if(subCategory.asString () != kSubCategory)
		return false;

	return true;
}
