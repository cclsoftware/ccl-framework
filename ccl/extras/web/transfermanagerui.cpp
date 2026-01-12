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
// Filename    : ccl/extras/web/transfermanagerui.cpp
// Description : Transfer Manager UI
//
//************************************************************************************************

#include "ccl/extras/web/transfermanagerui.h"

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/shellcommand.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/cclerror.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/netservices.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// TransferManagerUI::TransferList
//************************************************************************************************

class TransferManagerUI::TransferList: public ListViewModel,
									   public IdleClient
{
public:
	TransferList (TransferManagerUI& component);
	~TransferList ();

	void pauseAll (bool state = true);
	void cancelAll ();

	enum Columns
	{
		kIcon,
		kFile
	};

	class TItem: public ListViewItem
	{
	public:
		DECLARE_CLASS (TItem, ListViewItem)

		TItem ();

		PROPERTY_SHARED_AUTO (ITransfer, transfer, Transfer)
		PROPERTY_VARIABLE (ITransfer::State, oldState, OldState)
		PROPERTY_VARIABLE (double, oldProgress, OldProgress)
		PROPERTY_VARIABLE (double, oldSpeed, OldSpeed)

		// ListViewItem
		bool equals (const Object& obj) const override;
	};

	bool onCancel (CmdArgs args, VariantRef data);
	bool onRestart (CmdArgs args, VariantRef data);
	bool onPause (CmdArgs args, VariantRef data);
	bool onResume (CmdArgs args, VariantRef data);
	bool onRemoveFromHistory (CmdArgs args, VariantRef data);
	bool showFileInSystem (CmdArgs args, VariantRef data);

	// ListViewModel
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API viewDetached (IItemView* itemView) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	bool removeItems (ItemIndexRef index, const IItemSelection& selection) override;

	CLASS_INTERFACE (ITimerTask, ListViewModel)

protected:
	TransferManagerUI& component;
	bool itemsNeeded;

	void viewVisible (bool state);
	TItem* createItem (ITransfer* transfer) const;

	// IdleClient
	void onIdleTimer () override;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum TransferManagerUITags
	{
		kActivity = 100,
		kState,
		kAutoShow,
		kPauseAll,
		kResumeAll,
		kCancelAll
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("TransferManager")
	XSTRING (Download, "Download")
	XSTRING (Uploading, "Uploading...")
	XSTRING (Downloading, "Downloading...")
	XSTRING (UndeterminedFileName, "New File")
	XSTRING (Copying, "Copying...")
	XSTRING (WorkingXofY, "%(1) of %(2)")
	XSTRING (Waiting, "Queued")
	XSTRING (Failed, "Failed!")
	XSTRING (Canceled, "Canceled!")
	XSTRING (Cancel, "Cancel")
	XSTRING (Restart, "Restart")
	XSTRING (Pause, "Pause")
	XSTRING (Paused, "Paused")
	XSTRING (Resume, "Resume")
	XSTRING (RemoveFromHistory, "Remove From History")
	XSTRING (FileTransfersActive, "Files are being transferred in background.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (TransferManagerUI, kSetupLevel)
{
	TransferManagerUI::instance ();
	System::GetTransferManager ().setFormatter (AutoPtr<TransferFormatter> (NEW TransferFormatter));
	return true;
}

CCL_KERNEL_TERM_LEVEL (TransferManagerUI, kFirstRun)
{
	System::GetTransferManager ().setFormatter (nullptr);
}

//************************************************************************************************
// TransferFormatter
//************************************************************************************************

void CCL_API TransferFormatter::printState (String& string, ITransfer& transfer, ITransfer::State state, double progress, double speed)
{
	if(state == ITransfer::kTransferring)
	{
		if(transfer.getDirection () == ITransfer::kDownload)
		{
			if(transfer.getSrcLocation ().isNativePath ())
				string = XSTR (Copying);
			else
				string = XSTR (Downloading);
		}
		else
		{
			if(transfer.getDstLocation ().isNativePath ())
				string = XSTR (Copying);
			else
				string = XSTR (Uploading);
		}

		if(transfer.getFileSize () != -1) // -1: unknown size
		{
			string << " ";

			if(transfer.isChunked ())
			{
				double bytesDone = (double)transfer.getFileSize ();
				string << Format::ByteSize::print (bytesDone);
			}
			else
			{					
				double v = progress;
				double fileSize = (double)transfer.getFileSize ();
				double bytesDone = v * fileSize;

				string.appendFormat (XSTR (WorkingXofY), Format::ByteSize::print (bytesDone), Format::ByteSize::print (fileSize));
			}

			if(speed > 0.)
			{
				string << " (";
				string << Format::BytesPerSecond::print (speed);
				string << ")";
			}
		}
	}
	else if(state == ITransfer::kCompleted)
	{
		if(transfer.getFileSize () != -1) // -1: unknown size
		{
			double fileSize = (double)transfer.getFileSize ();
			string = Format::ByteSize::print (fileSize);
		}

		String displayString;
		if(transfer.getDirection () == ITransfer::kDownload)
			displayString = transfer.getSrcDisplayString ();
		else
			displayString = transfer.getDstDisplayString ();

		if(!string.isEmpty ())
			string << " - ";
		string << displayString;

		string << " - ";
		string << Format::TimeAgo::print (transfer.getTimestamp ());
	}
	else switch(state)
	{
	case ITransfer::kFailed : string = XSTR (Failed); break;
	case ITransfer::kCanceled : string = XSTR (Canceled); break;
	case ITransfer::kPaused : string = XSTR (Paused); break;
	default : string = XSTR (Waiting); break;
	}
}

//************************************************************************************************
// TransferManagerUI
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TransferManagerUI, Component)
DEFINE_COMPONENT_SINGLETON (TransferManagerUI)

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManagerUI::TransferManagerUI ()
: Component (String ("TransferManager")),
  signalSink (*NEW SignalSink (Signals::kTransfers)),
  transferList (nullptr)
{
	signalSink.setObserver (this);
	signalSink.enable (true);

	transferList = NEW TransferList (*this);

	paramList.addParam ("activity", Tag::kActivity);
	paramList.addParam ("pauseAll", Tag::kPauseAll);
	paramList.addParam ("resumeAll", Tag::kResumeAll);
	paramList.addParam ("cancelAll", Tag::kCancelAll);
	paramList.addInteger (kEmpty, kLastState, "state", Tag::kState);

	bool autoShow = true;
	Settings::instance ().getAttributes (getName ()).getBool (autoShow, "autoShow");
	paramList.addParam ("autoShow", Tag::kAutoShow)->setValue (autoShow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManagerUI::~TransferManagerUI ()
{
	transferList->release ();

	cancelSignals ();

	signalSink.enable (false);
	delete &signalSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API TransferManagerUI::getObject (StringID name, UIDRef classID)
{
	if(name == "transferList")
		return transferList->asUnknown ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManagerUI::initialize (IUnknown* context)
{
	// restore finished transfers
	System::GetTransferManager ().restore ();

	transferList->startTimer (500);
	
	ISubject::addObserver (&System::GetGUI (), this);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManagerUI::terminate ()
{
	ISubject::removeObserver (&System::GetGUI (), this);
	
	transferList->stopTimer ();

	// store finished transfers
	System::GetTransferManager ().store ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::canShutdown () const
{
	ITransferManager::ActivityInfo transfers;
	System::GetTransferManager ().getActivity (transfers);
	if(transfers.numActive > 0)
	{
		ccl_raise (XSTR (FileTransfersActive));
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::updateActivity ()
{
	Web::ITransferManager::ActivityInfo transfers;
	System::GetTransferManager ().getActivity (transfers);

	int state = transfers.numTotal == 0 ? kEmpty : transfers.numActive == 0 ? kCompleted : kActive;
	paramList.byTag (Tag::kState)->setValue (state);

	bool anyActive = transfers.numActive > 0;
	paramList.byTag (Tag::kActivity)->setValue (anyActive);
	
	paramList.byTag (Tag::kCancelAll)->enable (anyActive || transfers.numPaused > 0);
	paramList.byTag (Tag::kPauseAll)->enable (transfers.numResumable > 0);
	paramList.byTag (Tag::kResumeAll)->enable (transfers.numPaused > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManagerUI::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kRevealTransfer)
	{
		bool force = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		if(force == false)
		{
			bool autoShow = paramList.byTag (Tag::kAutoShow)->getValue ();
			if(!autoShow)
				return;
		}

		if(System::GetWindowManager ().canOpenWindow ("TransferManager"))
			if(System::GetWindowManager ().openWindow ("TransferManager"))
				(NEW Message (msg))->post (transferList);
	}
	if(msg == IApplication::kAppSuspended)
		onApplicationSuspend ();
	else if(msg == IApplication::kAppResumed)
		onApplicationResume ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kAutoShow)
		Settings::instance ().getAttributes (getName ()).set ("autoShow", param->getValue ().asBool ());
	else if(param->getTag () == Tag::kPauseAll)
		transferList->pauseAll (true);
	else if(param->getTag () == Tag::kResumeAll)
		transferList->pauseAll (false);
	else if(param->getTag () == Tag::kCancelAll)
		transferList->cancelAll ();
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::onApplicationSuspend ()
{
	ForEachUnknown (System::GetTransferManager (), unk)
		UnknownPtr<ITransfer> transfer (unk);
		ASSERT (transfer)
		if(transfer->getState() == ITransfer::kTransferring && !transfer->canTransferInBackground ())
			if(System::GetTransferManager ().pause (transfer) == kResultOk)
				suspendedTransfers.add (transfer);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::onApplicationResume ()
{
	VectorForEach (suspendedTransfers, ITransfer*, transfer)
		if(System::GetTransferManager ().find (transfer) && transfer->getState () == ITransfer::kPaused)
			System::GetTransferManager ().resume (transfer);
	EndFor
	suspendedTransfers.removeAll ();
}

//************************************************************************************************
// TransferManagerUI::TransferList::TItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TransferManagerUI::TransferList::TItem, ListViewItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManagerUI::TransferList::TItem::TItem ()
: oldState (ITransfer::kNone),
  oldProgress (0),
  oldSpeed (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::TItem::equals (const Object& obj) const
{
	const TItem* other = ccl_cast<TItem> (&obj);
	return other ? isEqualUnknown (transfer, other->transfer) : SuperClass::equals (obj);
}

//************************************************************************************************
// TransferManagerUI::TransferList
//************************************************************************************************

TransferManagerUI::TransferList::TransferList (TransferManagerUI& component)
: component (component),
  itemsNeeded (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManagerUI::TransferList::~TransferList ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::TransferList::pauseAll (bool state)
{
	ArrayForEach (items, TItem, item)
		ITransfer* t = item->getTransfer ();
		if(state && t->getState () == ITransfer::kTransferring)
			System::GetTransferManager ().pause (t);
		else if(!state && t->getState () == ITransfer::kPaused)
			System::GetTransferManager ().resume (t);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::TransferList::cancelAll ()
{
	ArrayForEach (items, TItem, item)
		ITransfer* t = item->getTransfer ();
		if(t->getState () == ITransfer::kTransferring || t->getState () == ITransfer::kPaused)
			System::GetTransferManager ().cancel (t);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManagerUI::TransferList::TItem* TransferManagerUI::TransferList::createItem (ITransfer* transfer) const
{
	ASSERT (transfer)

	TItem* item = NEW TItem;
	item->setTitle (transfer->getFileName ());
	item->setTransfer (transfer);
	item->setOldState (transfer->getState ());
	item->setOldProgress (transfer->getProgressValue ());
	item->setOldSpeed (transfer->getBytesPerSecond ());
	
	if(transfer->getSrcLocation ().isFolder ())
		item->setIcon (FileIcons::instance ().getDefaultFolderIcon ());
	else
	{
		AutoPtr<IImage> icon = FileIcons::instance ().createIcon (transfer->getFileName ());
		item->setIcon (icon);
	}
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManagerUI::TransferList::viewAttached (IItemView* itemView)
{
	SuperClass::viewAttached (itemView);
	viewVisible (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManagerUI::TransferList::viewDetached (IItemView* itemView)
{
	viewVisible (false);
	SuperClass::viewDetached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::TransferList::viewVisible (bool state)
{
	if(state)
	{
		ForEachUnknown (System::GetTransferManager (), unk)
			UnknownPtr<ITransfer> transfer (unk);
			ASSERT (transfer)
			TItem* item = createItem (transfer);
			items.add (item);
		EndFor

		ISubject::addObserver (&System::GetTransferManager (), this);

		itemsNeeded = true;
	}
	else
	{
		itemsNeeded = false;

		items.removeAll ();

		ISubject::removeObserver (&System::GetTransferManager (), this);
	}

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManagerUI::TransferList::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kRevealTransfer)
	{
		UnknownPtr<ITransfer> transfer (msg[0]);
		ASSERT (transfer)

		TItem item;
		item.setTransfer (transfer);
		int index = items.index (item);
		if(index != -1)
		{
			if(IItemView* itemView = getItemView ())
				itemView->setFocusItem (ItemIndex (index));
		}
	}
	else if(msg == ITransferManager::kTransferAdded)
	{
		UnknownPtr<ITransfer> transfer (msg[0]);
		ASSERT (transfer)

		TItem* item = createItem (transfer);
		int index = items.count ();
		items.add (item);

		signal (Message (kChanged));

		if(IItemView* itemView = getItemView ())
			itemView->setFocusItem (ItemIndex (index));
	}
	else if(msg == ITransferManager::kTransferRemoved)
	{
		UnknownPtr<ITransfer> transfer (msg[0]);
		ASSERT (transfer)
		
		TItem item;
		item.setTransfer (transfer);
		TItem* existing = (TItem*)items.findEqual (item);
		if(existing)
		{
			items.remove (existing);
			existing->release ();

			signal (Message (kChanged));
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManagerUI::TransferList::onIdleTimer ()
{
	// always update activity
	component.updateActivity ();

	if(itemsNeeded == false) // view not visible
		return;

	IItemView* itemView = getItemView ();
	ASSERT (itemView)

	int index = 0;
	ArrayForEach (items, TItem, item)
		ITransfer* t = item->getTransfer ();
		bool fileNameChanged = t->getFileName () != item->getTitle ();
		if(fileNameChanged ||
		   t->getState () != item->getOldState () || 
		   t->getProgressValue () != item->getOldProgress () ||
		   t->getBytesPerSecond () != item->getOldSpeed ())
		{
			if(fileNameChanged)
			{			
				item->setTitle (t->getFileName ());

				// update icon
				AutoPtr<IImage> icon = FileIcons::instance ().createIcon (t->getFileName ());
				item->setIcon (icon);
			}

			item->setOldState (t->getState ());
			item->setOldProgress (t->getProgressValue ());
			item->setOldSpeed (t->getBytesPerSecond ());

			if(itemView)
				itemView->invalidateItem (ItemIndex (index));
		}
		index++;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (48);  // kIcon
	list.addColumn (300); // kFile
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	TItem* item = (TItem*)resolve (index);
	if(!item)
		return false;

	switch(column)
	{
	case kIcon :
		if(item->getIcon ())
		{
			bool enabled = item->getTransfer ()->getDirection () == ITransfer::kUpload || item->getOldState () == ITransfer::kCompleted;
			
			const int kIconSize = 32; // limit icons to 32x32
			Rect iconRect (0, 0, kIconSize, kIconSize);
			iconRect.center (info.rect);
			const DrawInfo info2 = {info.view, info.graphics, iconRect, info.style, info.state};
			
			drawIcon (info2, item->getIcon (), enabled);
		}
		break;

	case kFile :
		{
			String subTitle;
			TransferFormatter ().printState (subTitle, *item->getTransfer (), item->getOldState (), item->getOldProgress (), item->getOldSpeed ());
			String title (item->getTitle ());
			if(item->getTransfer ()->isUndeterminedFileName ())
				title = XSTR (UndeterminedFileName);
			drawTitleWithSubtitle (info, title, subTitle);
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API TransferManagerUI::TransferList::createDragSessionData (ItemIndexRef index)
{
	TItem* item = (TItem*)resolve (index);
	ITransfer* t = item ? item->getTransfer () : nullptr;
	if(t)
	{
		if(t->getDirection () == ITransfer::kDownload)
		{
			// allow drag of URL of finished download
			if(t->getState () == ITransfer::kCompleted)
			{
				UrlRef dst = t->getDstLocation ();
				return (NEW Url (dst))->asUnknown ();
			}
		}
		else
			return (NEW Url (t->getSrcLocation ()))->asUnknown ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	TItem* item = (TItem*)resolve (index);
	ITransfer* t = item ? item->getTransfer () : nullptr;
	if(t && t->getDirection () == ITransfer::kDownload && t->getState () == ITransfer::kCompleted)
	{
		// try to open URL of finished download (our window might close here)
		System::GetSystemShell ().openUrl (t->getDstLocation (), /*System::kDoNotOpenExternally|*/System::kDeferOpenURL);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::appendItemMenu (IContextMenu& menu, ItemIndexRef focusIndex, const IItemSelection& selection)
{
	ObjectArray items;
	ForEachItem (selection, index)
		if(TItem* item = (TItem*)resolve (index))
			items.add (item);
	EndFor

	TItem* focusItem = (TItem*)resolve (focusIndex);
	if(focusItem && !items.contains (focusItem))
		items.add (focusItem);

	if(!items.isEmpty ())
	{
		// collect items which can be canceled/restarted/removed
		AutoPtr<Container> cancelItems = NEW ObjectArray;
		cancelItems->objectCleanup (true);

		AutoPtr<Container> restartItems = NEW ObjectArray;
		restartItems->objectCleanup (true);

		AutoPtr<Container> pauseItems = NEW ObjectArray;
		pauseItems->objectCleanup (true);
		
		AutoPtr<Container> resumeItems = NEW ObjectArray;
		resumeItems->objectCleanup (true);

		AutoPtr<Container> removeItems = NEW ObjectArray;
		removeItems->objectCleanup (true);

		ForEach (items, TItem, item)
			ITransfer::State state = item->getTransfer ()->getState ();
			if(state <= ITransfer::kTransferring)
			{
				cancelItems->add (return_shared (item));
				if(item->getTransfer ()->isResumable ())
					pauseItems->add (return_shared (item));
			}
			else if(state == ITransfer::kCanceled || state == ITransfer::kFailed)
			{
				if(item->getTransfer ()->isRestartAllowed ())
					restartItems->add (return_shared (item));
			}
			else if(state == ITransfer::kPaused)
			{
				cancelItems->add (return_shared (item));
				if(item->getTransfer ()->isResumable ())
					resumeItems->add (return_shared (item));
			}

			if(state == ITransfer::kCompleted || state == ITransfer::kFailed || state == ITransfer::kCanceled || state == ITransfer::kPaused)
				removeItems->add (return_shared (item));
		EndFor

		if(!cancelItems->isEmpty ())
		{
			menu.addCommandItem (XSTR (Cancel), "Transfer", "Cancel",
								 CommandDelegate<TransferList>::make (this, &TransferList::onCancel, cancelItems->asUnknown ()));
		}
				
		if(!restartItems->isEmpty ())
		{
			menu.addCommandItem (XSTR (Restart), "Transfer", "Restart",
								 CommandDelegate<TransferList>::make (this, &TransferList::onRestart, restartItems->asUnknown ()));
		}

		if(!pauseItems->isEmpty ())
		{
			menu.addCommandItem (XSTR (Pause), "Transfer", "Pause",
								 CommandDelegate<TransferList>::make (this, &TransferList::onPause, pauseItems->asUnknown()));
		}

		if(!resumeItems->isEmpty ())
		{
			menu.addCommandItem (XSTR (Resume), "Transfer", "Resume",
								 CommandDelegate<TransferList>::make (this, &TransferList::onResume, resumeItems->asUnknown()));
		}
		
		if(focusItem)
		{
			menu.addSeparatorItem ();
			menu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), "Transfer", "Show in Explorer/Finder",
								 CommandDelegate<TransferList>::make (this, &TransferList::showFileInSystem, focusItem->asUnknown ()));
		}

		menu.addSeparatorItem ();

		if(!removeItems->isEmpty ())
		{
			menu.addCommandItem (XSTR (RemoveFromHistory), "Transfer", "Remove From History",
								 CommandDelegate<TransferList>::make (this, &TransferList::onRemoveFromHistory, removeItems->asUnknown ()));
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::canRemoveItem (ItemIndexRef index)
{
	TItem* item = (TItem*)resolve (index);
	ITransfer::State state = item ? item->getTransfer ()->getState () : ITransfer::kTransferring;
	return state == ITransfer::kCompleted || state == ITransfer::kFailed || state == ITransfer::kCanceled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::removeItems (ItemIndexRef index, const IItemSelection& selection)
{
	// collect selected and removable items
	ObjectList removableItems;
	ForEachItem (selection, i)
		if(canRemoveItem (i))
			if(TItem* item = (TItem*)resolve (i))
				removableItems.append (item);
	EndFor
	
	if(!removableItems.isEmpty ())
	{
		// trigger removal of transfer; the list items will be removed when we receive kTransferRemoved
		for(auto item: iterate_as<TItem> (removableItems))
			System::GetTransferManager ().remove (item->getTransfer ());

		// overwrite previously saved transfers
		System::GetTransferManager ().store ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransferManagerUI::TransferList::removeItem (ItemIndexRef index)
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::onCancel (CmdArgs args, VariantRef data)
{
	Container* items = unknown_cast<Container> (data);
	ASSERT (items != nullptr)
	if(!args.checkOnly ())
	{
		ForEach (*items, TItem, item)
			System::GetTransferManager ().cancel (item->getTransfer ());
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::onRestart (CmdArgs args, VariantRef data)
{
	Container* items = unknown_cast<Container> (data);
	ASSERT (items != nullptr)
	if(!args.checkOnly ())
	{
		ForEach (*items, TItem, item)
			System::GetTransferManager ().restart (item->getTransfer ());
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::onPause (CmdArgs args, VariantRef data)
{
	Container* items = unknown_cast<Container> (data);
	ASSERT (items != nullptr)
	if(!args.checkOnly ())
	{
		ForEach (*items, TItem, item)
			System::GetTransferManager ().pause (item->getTransfer ());
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::onResume (CmdArgs args, VariantRef data)
{
	Container* items = unknown_cast<Container> (data);
	ASSERT (items != nullptr)
	if(!args.checkOnly ())
	{
		ForEach (*items, TItem, item)
			System::GetTransferManager ().resume (item->getTransfer ());
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::onRemoveFromHistory (CmdArgs args, VariantRef data)
{
	Container* items = unknown_cast<Container> (data);
	ASSERT (items != nullptr)
	if(!args.checkOnly ())
	{
		ForEach (*items, TItem, item)
			System::GetTransferManager ().remove (item->getTransfer ());
		EndFor

		// overwrite previously saved transfers
		System::GetTransferManager ().store ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManagerUI::TransferList::showFileInSystem (CmdArgs args, VariantRef data)
{
	TItem* item = unknown_cast<TItem> (data.asUnknown ());
	ITransfer* t = item ? item->getTransfer () : nullptr;
	if(!t)
		return false;

	const IUrl* path = nullptr;
	if(t->getDirection () == ITransfer::kDownload)
		path = &t->getDstLocation ();
	else
		path = &t->getSrcLocation ();
	
	return ShellCommand::showFileInSystem (*path, args.checkOnly ());
}
