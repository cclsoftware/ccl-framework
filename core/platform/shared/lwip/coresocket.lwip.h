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
// Filename    : core/platform/shared/lwip/coresocket.lwip.h
// Description : Lightweight IP Socket Functions
//
//************************************************************************************************

#ifndef _coresocket_lwip_h
#define _coresocket_lwip_h

#include "core/platform/shared/posix/coresocket.posix.h"

#include "core/platform/shared/lwip/corenetwork.lwip.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// POSIX compatibility
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int accept (int s, struct sockaddr* addr, socklen_t* addrlen) { return lwip_accept (s, addr, addrlen); }
inline int bind (int sockfd, const struct sockaddr* addr, socklen_t addrlen) { return lwip_bind (sockfd, addr, addrlen); }
inline int shutdown (int s, int how) { return lwip_shutdown (s, how); }
inline int close (int s) { return lwip_close (s); }
inline int connect (int s, const struct sockaddr* name, socklen_t namelen) { return lwip_connect (s, name, namelen); }
inline int getsockname (int s, struct sockaddr* name, socklen_t* namelen) { return lwip_getsockname (s, name, namelen); }
inline int getpeername (int s, struct sockaddr* name, socklen_t* namelen) { return lwip_getpeername (s, name, namelen); }
inline int setsockopt (int s, int level, int optname, const void* optval, socklen_t optlen) { return lwip_setsockopt (s, level, optname, optval, optlen); }
inline int getsockopt (int s, int level, int optname, void* optval, socklen_t* optlen) { return lwip_getsockopt (s, level, optname, optval, optlen); }
inline int listen (int s, int backlog) { return lwip_listen (s, backlog); }
inline int recv (int s, void* mem, size_t len, int flags) { return lwip_recv (s, mem, len, flags); }
inline int recvfrom (int s, void* mem, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen) { return lwip_recvfrom (s, mem, len, flags, from, fromlen); }
inline int send (int s, const void* data, size_t size, int flags) { return lwip_send (s, data, size, flags); }
inline int sendto (int s, const void* data, size_t size, int flags, const struct sockaddr* to, socklen_t tolen) { return lwip_sendto (s, data, size, flags, to, tolen); }
inline int socket (int domain, int type, int protocol) { return lwip_socket (domain, type, protocol); }
inline int select (int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset, struct timeval* timeout) { return lwip_select (maxfdp1, readset, writeset, exceptset, timeout); }
inline int ioctl (int s, long cmd, void* argp) { return lwip_ioctl (s, cmd, argp); }
inline int read (int s, void* mem, size_t len) { return lwip_read (s, mem, len); }
inline int write (int s, const void* data, size_t size) { return lwip_write (s, data, size); }
inline int fcntl (int s, int cmd, int val) { return lwip_fcntl (s, cmd, val); }

namespace Core {
namespace Platform {

//************************************************************************************************
// CtlSocket
//************************************************************************************************

class LwIPSocket: public PosixSocket
{
public:
	LwIPSocket (SocketID socket);
	LwIPSocket (Sockets::AddressFamily addressFamily, Sockets::SocketType type, Sockets::ProtocolType protocol);

protected:
	// PosixSocket
	bool checkForError () const override;
};

typedef LwIPSocket Socket;

//************************************************************************************************
// SocketIDSet
//************************************************************************************************

typedef PosixSocketIDSet SocketIDSet;

} // namespace Platform
} // namespace Core

#endif // _coresocket_lwip_h
