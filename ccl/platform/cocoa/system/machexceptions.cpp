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
// Filename    : ccl/platform/cocoa/system/machexceptions.cpp
// Description : Mach Exception Handler (based on Mac OS X Internals by Amit Singh, Chapter 9.7)
//
//************************************************************************************************

#include "ccl/public/base/debug.h"
#include "machexceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/i386/thread_state.h>

using namespace MacOS;

#define REPORT_MACH_ERROR(msg, retval) \
    if (kr != KERN_SUCCESS) { mach_error(msg ":" , kr); goto error; }

pthread_t exceptionThread = 0;
mach_port_t exceptionPort = 0;
mach_port_t messagePort = 0;
void *ExceptionHandler (void* param);
extern "C" boolean_t exc_server (mach_msg_header_t *request, mach_msg_header_t *reply);
	
kern_return_t repair_instruction(mach_port_t victim);
void          graceful_dead(void);

//************************************************************************************************
// MachExceptionHandler
//************************************************************************************************

CCL::String MachExceptionHandler::messageContext;
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void MachExceptionHandler::setMessageContext (CCL::StringRef message)
{
	messageContext = message;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
void MachExceptionHandler::install ()
{
    mach_port_t taskSelf = mach_task_self();
    mach_port_t threadSelf = mach_thread_self();
	
    // create a receive right
    kern_return_t kr = mach_port_allocate (taskSelf, MACH_PORT_RIGHT_RECEIVE, &exceptionPort);
	REPORT_MACH_ERROR("mach_port_allocate", kr);
	
    // insert a send right: we will now have combined receive/send rights
    kr = mach_port_insert_right (taskSelf, exceptionPort, exceptionPort, MACH_MSG_TYPE_MAKE_SEND);
    REPORT_MACH_ERROR("mach_port_insert_right", kr);


	// Setup message port
    kr = mach_port_allocate (taskSelf, MACH_PORT_RIGHT_RECEIVE, &messagePort);
    REPORT_MACH_ERROR("mach_port_allocate", kr);

/*
	mach_port_request_notification (taskSelf, exceptionPort, 
		MACH_NOTIFY_DEAD_NAME,
		0,
		messagePort,
		MACH_MSG_TYPE_MAKE_SEND_ONCE,
		&old_port_to_receive_notification);

	//task_set_special_port (taskSelf, TASK_NOTIFY_PORT, messagePort);

*/

    kr = thread_set_exception_ports (threadSelf,                 // target thread
                                    EXC_MASK_BAD_INSTRUCTION | EXC_MASK_BAD_ACCESS, // exception types
                                    exceptionPort,           // the port
                                    EXCEPTION_DEFAULT,        // behavior
                                    THREAD_STATE_NONE);       // flavor
    REPORT_MACH_ERROR("thread_set_exception_ports", kr);

	int result = pthread_create (&exceptionThread, (pthread_attr_t *)0, ExceptionHandler, 0);
	if(result != 0)
	{
        CCL_WARN ("Failed to install exception handler", 0);
        goto error;
    }

    CCL_PRINTLN ("about to dispatch exception_handler pthread")
    pthread_detach (exceptionThread);

error:
    mach_port_deallocate(taskSelf, threadSelf);
    if(exceptionPort)
        mach_port_deallocate(taskSelf, exceptionPort);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void MachExceptionHandler::remove ()
{
    mach_port_t taskSelf = mach_task_self();
    mach_port_t threadSelf = mach_thread_self();
	
    mach_port_deallocate (taskSelf, threadSelf);
    if(exceptionPort)
        mach_port_deallocate (taskSelf, exceptionPort);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

// exception message we will receive from the kernel
struct exc_msg_t
{
    mach_msg_header_t          Head;
    mach_msg_body_t            msgh_body; // start of kernel-processed data
    mach_msg_port_descriptor_t thread;    // victim thread
    mach_msg_port_descriptor_t task;      // end of kernel-processed data
    NDR_record_t               NDR;       // see osfmk/mach/ndr.h
    exception_type_t           exception;
    mach_msg_type_number_t     codeCnt;   // number of elements in code[]
    exception_data_t           code;      // an array of integer_t
    char                       pad[512];  // for avoiding MACH_MSG_RCV_TOO_LARGE
};

// reply message we will send to the kernel
struct reply_msg_t
{
    mach_msg_header_t          Head;
    NDR_record_t               NDR;       // see osfmk/mach/ndr.h
    kern_return_t              RetCode;   // indicates to the kernel what to do
};
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void *ExceptionHandler (void* param)
{
    kern_return_t kr;
    exc_msg_t     msg_recv;
    reply_msg_t   msg_resp;

    CCL_PRINTLN ("beginning");

    msg_recv.Head.msgh_local_port = exceptionPort;
    msg_recv.Head.msgh_size = sizeof(msg_recv);

    kr = mach_msg (&(msg_recv.Head),            // message
                  MACH_RCV_MSG|MACH_RCV_LARGE, // options
                  0,                           // send size (irrelevant here)
                  sizeof(msg_recv),            // receive limit
                  exceptionPort,               // port for receiving
                  MACH_MSG_TIMEOUT_NONE,       // no timeout
                  MACH_PORT_NULL);             // notify port (irrelevant here)
    REPORT_MACH_ERROR ("mach_msg_receive", kr);

    CCL_PRINTLN("received message");
    CCL_PRINTF("victim thread is %#lx\n", (long)msg_recv.thread.name);
    CCL_PRINTF("victim thread's task is %#lx\n", (long)msg_recv.task.name);

    CCL_PRINTLN("calling exc_server");
    exc_server (&msg_recv.Head, &msg_resp.Head);
    // now msg_resp.RetCode contains return value of catch_exception_raise()

    CCL_PRINTLN("sending reply");
    kr = mach_msg (&(msg_resp.Head),        // message
                  MACH_SEND_MSG,           // options
                  msg_resp.Head.msgh_size, // send size
                  0,                       // receive limit (irrelevant here)
                  MACH_PORT_NULL,          // port for receiving (none)
                  MACH_MSG_TIMEOUT_NONE,   // no timeout
                  MACH_PORT_NULL);         // notify port (we don't want one)
    REPORT_MACH_ERROR ("mach_msg_send", kr);
	
error:
    pthread_exit((void *)0);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

kern_return_t catch_exception_raise(mach_port_t            port,
                      mach_port_t            victim,
                      mach_port_t            task,
                      exception_type_t       exception,
                      exception_data_t       code,
                      mach_msg_type_number_t code_count)
{
    CCL_PRINTLN ("beginning");

    if (exception != EXC_BAD_INSTRUCTION && exception != EXC_BAD_ACCESS) {
        // this should not happen, but we should forward an exception that we
        // were not expecting... here, we simply bail out
        exit(-1);
    }

    return repair_instruction(victim);
}

kern_return_t repair_instruction (mach_port_t victim)
{
    kern_return_t      kr;
    unsigned int       count;
#if defined (__i386__) 
	x86_thread_state_t state;
#else
    ppc_thread_state_t state;
#endif
	
    CCL_PRINTLN ("fixing instruction");

    count = MACHINE_THREAD_STATE_COUNT;
    kr = thread_get_state(victim,                 // target thread
                          MACHINE_THREAD_STATE,   // flavor of state to get
                          (thread_state_t)&state, // state information
                          &count);                // in/out size
    REPORT_MACH_ERROR ("thread_get_state", kr);

#if defined (__i386__)
	state.uts.ts32.eip = (vm_address_t)graceful_dead;
#else
    // SRR0 is used to save the address of the instruction at which execution
    // continues when rfid executes at the end of an exception handler routine
    state.srr0 = (vm_address_t)graceful_dead;
#endif
    kr = thread_set_state(victim,                      // target thread
                          MACHINE_THREAD_STATE,        // flavor of state to set
                          (thread_state_t)&state,      // state information
                          MACHINE_THREAD_STATE_COUNT); // in size
    REPORT_MACH_ERROR ("thread_set_state", kr);
error:
    return KERN_SUCCESS;
}

void graceful_dead ()
{
    CCL_PRINTLN ("dying graceful death")
	
	exit (-1);
}
