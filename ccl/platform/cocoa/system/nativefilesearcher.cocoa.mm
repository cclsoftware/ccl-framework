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
// Filename    : ccl/platform/cocoa/system/nativefilesearcher.cocoa.mm
// Description : Cocoa native file searcher
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/system/nativefilesystem.cocoa.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/collections/unknownlist.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {
namespace MacOS {
class SpotlightFileSearcher;
}}

//************************************************************************************************
// SearchController
//************************************************************************************************

@interface CCL_ISOLATED (SearchController): NSObject
{
	CCL::MacOS::SpotlightFileSearcher* fileSearcher;
}

- (id)initWithFileSearcher:(CCL::MacOS::SpotlightFileSearcher*)searcher;
- (void)queryDidFinish:(NSNotification*)notification;
- (void)dataAvailable:(NSNotification*)notification;
@end

namespace CCL {
namespace MacOS {

//************************************************************************************************
// MacOS::SpotlightFileSearcher
//************************************************************************************************

class SpotlightFileSearcher: public Unknown, 
							 public AbstractSearcher
{
public:
	SpotlightFileSearcher (ISearchDescription& description);
	~SpotlightFileSearcher ();

	void stop ();
	void dataRead ();
	
	static ISearcher* createInstance (ISearchDescription& description);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;
	
	CLASS_INTERFACE (ISearcher, Unknown)

protected:
	ISearchResultSink* sink;
	NSMetadataQuery* query;
	CCL_ISOLATED (SearchController)* delegate;
	IProgressNotify* progress;
	NSUInteger itemsDelivered;
};

} // namespace MacOS
} // namespace CCL

//************************************************************************************************
// SearchController
//************************************************************************************************

@implementation CCL_ISOLATED (SearchController)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFileSearcher:(CCL::MacOS::SpotlightFileSearcher*)searcher
{
	self = [super init];
	fileSearcher = searcher;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

-(void)queryDidFinish:(NSNotification*)notification
{
	fileSearcher->stop ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

-(void)dataAvailable:(NSNotification*)notification
{
	fileSearcher->dataRead ();
}
@end

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// CocoaNativeFileSystem
//************************************************************************************************

ISearcher* CCL_API CocoaNativeFileSystem::createSearcher (ISearchDescription& description)
{
	return MacOS::SpotlightFileSearcher::createInstance (description);
}

//************************************************************************************************
// MacOS::SpotlightFileSearcher
//************************************************************************************************

ISearcher* SpotlightFileSearcher::createInstance (ISearchDescription& description)
{
	return NEW SpotlightFileSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpotlightFileSearcher::SpotlightFileSearcher (ISearchDescription& description)
: AbstractSearcher (description),
  sink (nil),
  query (nil),
  delegate (nil),
  progress (nullptr),
  itemsDelivered (0)
{
	query = [[NSMetadataQuery alloc] init];
	
	// prepare scope
	UrlDisplayString path (description.getStartPoint ());
	[query setSearchScopes: @[[(NSString*)path.createNativeString<CFStringRef> () autorelease]]];
	
	int tokenCount = searchDescription.getSearchTokenCount ();
	if(tokenCount > 1)
	{
		NSMutableArray* nsSearchTokens = [NSMutableArray array];
		
		for(int i = 0; i < tokenCount; i++)
		{
			StringRef token = searchDescription.getSearchToken (i);
			NSString* nsSearchToken = [(NSString*)token.createNativeString<CFStringRef> () autorelease];
			[nsSearchTokens addObject:nsSearchToken];
		}
		
		NSMutableArray* nsPredicates = [NSMutableArray arrayWithCapacity:[nsSearchTokens count]];
		for(NSString* nsSearchToken in nsSearchTokens)
		{
			NSPredicate* nsPredicate = [NSPredicate predicateWithFormat:@"%K contains[cd] %@", NSMetadataItemFSNameKey, nsSearchToken];
			[nsPredicates addObject:nsPredicate];
		}
		NSPredicate* nsCompoundPredicate = [NSCompoundPredicate andPredicateWithSubpredicates:nsPredicates];
		[query setPredicate:nsCompoundPredicate];
	}
	else
	{
		String searchString;
		if(tokenCount == 1)
			searchString = searchDescription.getSearchToken (0);
		if(searchString.isEmpty ())
			searchString = description.getSearchTerms ();
			
		NSString* nsSearchString = [(NSString*)searchString.createNativeString<CFStringRef> () autorelease];
		[query setPredicate: [NSPredicate predicateWithFormat:@"%K contains[cd] %@", NSMetadataItemFSNameKey, nsSearchString]];
	}
	
	[query setNotificationBatchingInterval:0.1];
	
	delegate = [[CCL_ISOLATED (SearchController) alloc] initWithFileSearcher: this];

	[[NSNotificationCenter defaultCenter] addObserver:delegate selector:@selector(dataAvailable:) name:NSMetadataQueryGatheringProgressNotification object:query];
    [[NSNotificationCenter defaultCenter] addObserver:delegate selector:@selector(queryDidFinish:) name:NSMetadataQueryDidFinishGatheringNotification object:query];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpotlightFileSearcher::~SpotlightFileSearcher ()
{
	[[NSNotificationCenter defaultCenter] removeObserver:delegate];
	[query release];
	[delegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SpotlightFileSearcher::find (ISearchResultSink& resultSink, CCL::IProgressNotify* _progress)
{
	SharedPtr<SpotlightFileSearcher> saver (this);
	ScopedVar<CCL::IProgressNotify*> progressScope (progress, _progress);
	sink = &resultSink;
	itemsDelivered = 0;
	@autoreleasepool
	{
		[query startQuery];
		while(![query isStopped])
		{
			@try
			{
				[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
			}
			@catch(NSException* exception)
			{
				stop ();
				return kResultFailed;
			}
			
			progress->updateProgress (0);
			
			if(progress->isCanceled ())
			{
				stop ();
				return kResultFailed;
			}
		}
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpotlightFileSearcher::dataRead ()
{
	if(!sink)
		return;

	[query disableUpdates];
	
	UnknownList results;
	int listCount = 0;
	
	NSUInteger currentItems = [query resultCount];
	for(NSUInteger i = itemsDelivered; i < currentItems; i++)
	{
		NSString* nsPath = [[query resultAtIndex:i] valueForAttribute:NSMetadataItemPathKey];
		if(nsPath)
		{
			String pathString;
			pathString.appendNativeString (nsPath);
			AutoPtr<Url> path = NEW Url;
			path->fromDisplayString (pathString, Url::kFile);
			// only accept if fileName without extension matches searchTerms
			String nameWithout;
			path->getName (nameWithout, false);
			if(searchDescription.matchesName (nameWithout))
			{
				results.add (static_cast<IUrl*> (path.detach ()));
				listCount++;
			}
		}
		if(listCount >= 20)
		{
			sink->addResults (results);
			results.removeAll ();
			listCount = 0;
		}
		
		if(progress)
		{
			progress->updateProgress (0);
			if(progress->isCanceled ())
			{
				stop ();
				return;
			}
		}
	}
	
	if(!results.isEmpty ())
		sink->addResults (results);
		
	itemsDelivered = currentItems;
	
	[query enableUpdates];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpotlightFileSearcher::stop ()
{
	[query stopQuery];
}
