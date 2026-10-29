//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audiographbase.cpp
// Description : Audio Graph Base Classes
//
//************************************************************************************************

#include "audiographbase.h"

using namespace CCL;

//************************************************************************************************
// AudioNodeBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioNodeBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioNodeBase::AudioNodeBase ()
: graph (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioNodeBase::setID (StringID id)
{
	nodeId = id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AudioNodeBase::getID () const
{
	return nodeId;
}

//************************************************************************************************
// AudioGraphBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioGraphBase, AudioProcessorBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioGraphBase::AudioGraphBase ()
{
	nodes.objectCleanup ();

	processConfiguration.sampleRate = kDefaultSampleRate;
	fallbackContext.sampleRate = kDefaultSampleRate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioGraphBase::~AudioGraphBase ()
{
	ASSERT (cleanupDone == true) // must be done by derived class
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::cleanupAll ()
{
	ASSERT (!isProcessing () && !isActive ())

	for(auto* node : iterate_as<Node> (nodes))
	{
		if(isPrepared ())
			node->unprepare ();

		node->onNodeDetached (); // assuming this also disconnects the node

		cleanupConnections (node);
	}
	
	nodes.removeAll ();

	ASSERT (activeConnections.isEmpty ())

	cleanupDone = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioGraphBase::addNode (IAudioNode* _node)
{
	auto* node = unknown_cast<Node> (_node);
	ASSERT (node && node->getGraph () == nullptr)
	if(!node || node->getGraph () != nullptr)
		return kResultInvalidArgument;

	ASSERT (!nodes.contains (node))
	nodes.add (node);
	
	node->setGraph (this);
	node->onNodeAttached ();

	if(isPrepared ())
		node->prepare (processConfiguration);

	node->onDisplayLatencyChanged (storedDisplayLatency);

	if(isActive ())
		node->onActiveChanged (true);

	if(isProcessing ())
		node->onProcessingChanged (true);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioGraphBase::removeNode (IAudioNode* _node)
{
	auto* node = unknown_cast<Node> (_node);
	ASSERT (node && node->getGraph () == this)	
	if(!node || node->getGraph () != this)
		return kResultInvalidArgument;

	if(isProcessing ())
		node->onProcessingChanged (false);
	
	node->disconnectAll ();
	cleanupConnections (node); // cleanup destination-side connections

	if(isActive ())
		node->onActiveChanged (false);

	if(isPrepared ())
		node->unprepare ();

	node->onNodeDetached ();
	node->setGraph (nullptr);

	nodes.remove (node);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioNode* CCL_API AudioGraphBase::findNode (StringID nodeId) const
{
	for(auto* node : iterate_as<Node> (nodes))
		if(node->getID () == nodeId)
			return node;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioGraphBase::setDisplayLatency (int latencyInSamples)
{
	if(storedDisplayLatency != latencyInSamples)
	{
		storedDisplayLatency = latencyInSamples;
		
		for(auto* node : iterate_as<Node> (nodes))
			node->onDisplayLatencyChanged (latencyInSamples);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::onConfigurationChanged ()
{
	fallbackContext.sampleRate = processConfiguration.sampleRate;

	for(auto* node : iterate_as<Node> (nodes))
		node->prepare (processConfiguration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::onActiveChanged (bool state)
{
	for(auto* node : iterate_as<Node> (nodes))
		node->onActiveChanged (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::onProcessingChanged (bool state)
{
	for(auto* node : iterate_as<Node> (nodes))
		node->onProcessingChanged (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const AudioProcessContext& AudioGraphBase::getCurrentProcessContext () const
{
	return currentProcessContext ? *currentProcessContext : fallbackContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::storeConnection (const Connection& c)
{
	ASSERT (!activeConnections.contains (c))
	activeConnections.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::unstoreSourceConnection (Node* source, int outputIndex)
{
	int removed = activeConnections.removeIf ([&] (const Connection& c) 
		{
			return c.matchesSource (source, outputIndex);
		});

	ASSERT (removed == 1)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::unstoreSourceConnections (Node* source)
{
	activeConnections.removeIf ([&] (const Connection& c) 
	{
		return c.matchesSource (source);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioGraphBase::cleanupConnections (Node* node)
{
	activeConnections.removeIf ([&] (const Connection& c) 
	{
		return c.matchesAny (node);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioGraphBase methods forwarded to base class
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioGraphBase::initialize (IUnknown* context)
{ return SuperClass::initialize (context); }
tresult CCL_API AudioGraphBase::terminate ()
{ return SuperClass::terminate (); }
tbool CCL_API AudioGraphBase::canTerminate () const
{ return SuperClass::canTerminate (); }
IAudioComponent::CapabilityFlags CCL_API AudioGraphBase::getCapabilities () const
{ return SuperClass::getCapabilities (); }
tresult CCL_API AudioGraphBase::setStreamLayout (const IAudioStreamLayout& layout)
{ return SuperClass::setStreamLayout (layout); }
const IAudioStreamLayout& CCL_API AudioGraphBase::getStreamLayout () const
{ return SuperClass::getStreamLayout (); }
tresult CCL_API AudioGraphBase::setStreamActive (AudioStreamDirection direction, int index, tbool active)
{ return SuperClass::setStreamActive (direction, index, active); }
tresult CCL_API AudioGraphBase::getLatency (int& latencyInSamples, AudioStreamDirection direction) const
{ return SuperClass::getLatency (latencyInSamples, direction); }
tresult CCL_API AudioGraphBase::getProcessConfiguration (AudioProcessConfiguration& configuration) const
{ return SuperClass::getProcessConfiguration (configuration); }
tresult CCL_API AudioGraphBase::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{ return SuperClass::setProcessConfiguration (configuration); }
tbool CCL_API AudioGraphBase::isActive () const
{ return SuperClass::isActive (); }
tresult CCL_API AudioGraphBase::setActive (tbool state)
{ return SuperClass::setActive (state); }

//************************************************************************************************
// AudioGraphBase::ConnectionKeeper
//************************************************************************************************

AudioGraphBase::ConnectionKeeper::ConnectionKeeper (AudioGraphBase& graph, Node& node)
{
	for(const Connection& c : graph.activeConnections)
		if(c.matchesAny (&node))
			restoreList.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioGraphBase::ConnectionKeeper::~ConnectionKeeper ()
{
	for(const Connection& c : restoreList)
	{
		tresult result = c.source->connect (c.destination, c.outputIndex, c.inputIndex);
		SOFT_ASSERT (result == kResultOk, "Failed to restore previous audio graph connection\n")
	}
}

