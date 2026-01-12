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
// Filename    : ccl/extras/packages/packagemanager.h
// Description : Package Manager
//
//************************************************************************************************

#ifndef _ccl_packagemanager_h
#define _ccl_packagemanager_h

#include "ccl/app/component.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/extras/packages/unifiedpackage.h"
#include "ccl/extras/packages/packagesorter.h"
#include "ccl/extras/packages/unifiedpackagesource.h"

#include "ccl/public/gui/framework/inotificationcenter.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/system/alerttypes.h"

namespace CCL {
interface IItemModel;
class InplaceProgressComponent;
class NotificationListComponent;

namespace Packages {
class PackageComponent;
class PackageComponentModel;
class PackageFilterComponent;
class UnifiedPackageAction;

//************************************************************************************************
// PackageManager
/** Component used to display and manage packages. @sa UnifiedPackage */
//************************************************************************************************

class PackageManager: public Component,
					  public IUnifiedPackageSink,
					  public Alert::IReporter,
					  public IDataTarget,
					  public INotificationActionHandler
{
public:
	DECLARE_CLASS (PackageManager, Component)

	PackageManager (StringRef name = "PackageManager", StringRef title = nullptr);
	~PackageManager ();

	enum InstallConfiguration
	{
		kMinimalInstall,
		kRecommendedInstall,
		kFullInstall,
		kCustomInstall
	};

	void addSource (IUnifiedPackageSource* source);
	void addFilter (IObjectFilter* filter);
	void addSorter (PackageSorter* sorter);
	void setSectionPropertyId (StringID propertyId, bool ascending = true);
	StringID getSectionPropertyId () const;
	void setInstallConfiguration (int configuration, bool defer = false);

	void addOrigin (int originId, StringRef title);
	String getOriginTitle (int originId);

	void openWindow ();
	void clearMessages ();
	void refresh ();
	void updateAll (bool defer = false);
	void update (PackageComponent* component);
	void sortComponents (bool defer = false);
	virtual bool makeVisible (StringRef packageId, bool defer = false);

	bool hasPackages () const;
	bool hasActiveActions (bool paused = false) const;
	void getPackages (Container& packages) const;
	UnifiedPackage* findPackage (StringRef packageId) const;
	void getActions (Container& actions, StringRef packageId) const;
	void getActions (Container& actions, const UnifiedPackage& package) const;
	UnifiedPackageAction* getInstallAction (UnifiedPackage* package, bool checkEnabled = true) const;
	bool canInstall (StringRef packageId) const;
	bool installPackage (StringRef packageID);
	double getInstallationProgress (StringRef packageID) const;
	bool cancelActions (StringRef packageID);
	void updateProgress (bool defer);
	void updateSelectedActions (bool defer);
	void updateOverallActionState (bool defer);
	void requestRestart (StringRef message = nullptr);
	virtual void onCompletion (const UnifiedPackageAction& action, bool succeeded) {}

	IDragHandler* createDragHandler (const DragEvent& event, IView* view);
	UnifiedPackage* createPackageFromFile (UrlRef url) const;

	PackageComponent* findPackageComponent (const UnifiedPackage& package) const;
	PackageComponent* findPackageComponent (StringRef packageID) const;
	bool matchesFilters (UnifiedPackage& package) const;
	void onShowChildren (UnifiedPackage& package);

	// IUnifiedPackageSink
	void addPackage (UnifiedPackage* package) override;
	void requestUpdate (IUnifiedPackageSource& source, int updateFlags) override;

	// IReporter
	void CCL_API reportEvent (const Alert::Event& event) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
		
	// INotificationActionHandler
	tbool CCL_API canExecute (StringID actionId, const INotification& n) const override;
	tresult CCL_API execute (StringID actionId, INotification& n) override;

	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;

	CLASS_INTERFACE3 (IUnifiedPackageSink, IReporter, IDataTarget, Component)

protected:
	struct TitleMapping
	{
		int id;
		String title;
	};

	struct Action
	{
		CString id;
		String title;
		String composedTitle;
		int count;
		int64 size;
		bool needsConfirmation;
		String packageTitles;
		int titleCount;

		Action (StringID id = nullptr, StringRef title = nullptr, int count = 1)
		: id (id),
		  title (title),
		  composedTitle (title),
		  count (count),
		  size (0),
		  needsConfirmation (false),
		  titleCount (0)
		{}

		bool operator == (const Action& other) const
		{
			return id == other.id;
		}

		bool operator > (const Action& other) const
		{
			return id > other.id;
		}
	};

	PackageComponentModel* packageComponentModel;
	Vector<IUnifiedPackageSource*> sources;
	ObjectArray sorters;
	Vector<TitleMapping> origins;
	Vector<Alert::Event> messages;
	Vector<Action> selectedActions;
	IParameter* sortBy;
	InplaceProgressComponent* inplaceProgress;
	NotificationListComponent* notificationComponent;
	bool isUpdating;

	DECLARE_STRINGID_MEMBER (kUpdate)
	DECLARE_STRINGID_MEMBER (kSort)
	DECLARE_STRINGID_MEMBER (kUpdateProgress)
	DECLARE_STRINGID_MEMBER (kUpdateSelectedActions)
	DECLARE_STRINGID_MEMBER (kMakeVisible)
	DECLARE_STRINGID_MEMBER (kSelect)
	DECLARE_STRINGID_MEMBER (kSetInstallConfiguration)
	DECLARE_STRINGID_MEMBER (kRestartAction)

	void removeAllComponents ();
	CCL::IItemModel* getPackageModel ();
	
	Url buildPackageUrl (StringRef id) const;
	Url buildPackageUrl (PackageComponent& component) const;
	virtual void retrievePackages (UrlRef url = UnifiedPackageUrl (), bool refresh = false);
	void mergeExistingChildren (UnifiedPackage* package);
	bool ownsPackage (UnifiedPackage* package) const;
	bool installPackage (UnifiedPackage* package);
	virtual void performSelectedAction (int index, bool confirmed = false);
	void cancelAllActions ();
	void pauseAllActions (bool state);
	virtual void applyConfiguration (int value);
	void selectAll (bool state);
	void deselectFiltered ();
	bool select (StringRef packageId, bool state, bool defer = false);
	void resetFilters ();
	void getInstallableProducts (Container& packages) const;
	bool isVisible () const;
	INotification* findNotification (StringRef message) const;
	void sendNotification (const Alert::Event& event, const Vector<NotificationActionProperties>& actionProperties = {});
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_packagemanager_h
