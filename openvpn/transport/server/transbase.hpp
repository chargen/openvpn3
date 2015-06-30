//    OpenVPN -- An application to securely tunnel IP networks
//               over a single port, with support for SSL/TLS-based
//               session authentication and key exchange,
//               packet encryption, packet authentication, and
//               packet compression.
//
//    Copyright (C) 2012-2015 OpenVPN Technologies, Inc.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License Version 3
//    as published by the Free Software Foundation.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program in the COPYING file.
//    If not, see <http://www.gnu.org/licenses/>.

// Abstract base classes for server transport objects that implement UDP, TCP,
// HTTP Proxy, etc.

#ifndef OPENVPN_TRANSPORT_SERVER_TRANSBASE_H
#define OPENVPN_TRANSPORT_SERVER_TRANSBASE_H

#include <string>
#include <vector>

#include <asio.hpp>

#include <openvpn/common/exception.hpp>
#include <openvpn/common/rc.hpp>
#include <openvpn/buffer/buffer.hpp>
#include <openvpn/addr/route.hpp>
#include <openvpn/crypto/cryptodc.hpp>
#include <openvpn/tun/server/tunbase.hpp>
#include <openvpn/server/servhalt.hpp>
#include <openvpn/server/peerstats.hpp>
#include <openvpn/server/peeraddr.hpp>

namespace openvpn {

  // Base class for server transport object.
  struct TransportServer : public RC<thread_unsafe_refcount>
  {
    typedef RCPtr<TransportServer> Ptr;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::string local_endpoint_info() const = 0;
  };

  // Factory for server transport object.
  struct TransportServerFactory : public RC<thread_unsafe_refcount>
  {
    typedef RCPtr<TransportServerFactory> Ptr;

    virtual TransportServer::Ptr new_server_obj(asio::io_context& io_context) = 0;
  };

  // Base class for the per-client-instance state of the TransportServer.
  // Each client instance uses this class to send data to the transport layer.
  struct TransportClientInstanceSend : public virtual RC<thread_unsafe_refcount>
  {
    typedef RCPtr<TransportClientInstanceSend> Ptr;

    virtual bool defined() const = 0;
    virtual void stop() = 0;

    virtual bool transport_send_const(const Buffer& buf) = 0;
    virtual bool transport_send(BufferAllocated& buf) = 0;

    virtual const std::string& transport_info() const = 0;

    // bandwidth stats polling
    virtual bool stats_pending() const = 0;
    virtual PeerStats stats_poll() = 0;
  };

  // Base class for the client instance receiver.  Note that all
  // client instance receivers (transport, routing, management,
  // etc.) must inherit virtually from RC because the client instance
  // object will inherit from multiple receivers.
  struct TransportClientInstanceRecv : public virtual RC<thread_unsafe_refcount>
  {
    typedef RCPtr<TransportClientInstanceRecv> Ptr;

    virtual bool defined() const = 0;
    virtual void stop() = 0;

    virtual void start(const TransportClientInstanceSend::Ptr& parent,
		       const PeerAddr::Ptr& addr,
		       const int local_peer_id) = 0;

    // Called with OpenVPN-encapsulated packets from transport layer.
    // Returns true if packet successfully validated.
    virtual bool transport_recv(BufferAllocated& buf) = 0;

    // Disable keepalive for rest of session, but fetch
    // the keepalive parameters (in seconds).
    virtual void disable_keepalive(unsigned int &keepalive_ping,
				   unsigned int &keepalive_timeout) = 0;

    // override the data channel factory
    virtual void override_dc_factory(const CryptoDCFactory::Ptr& dc_factory) = 0;

    // override the tun provider
    virtual TunClientInstanceRecv* override_tun(TunClientInstanceSend* tun) = 0;

    // bandwidth stats notification
    virtual void stats_notify(const PeerStats& ps, const bool final) = 0;

    // client float notification
    virtual void float_notify(const PeerAddr::Ptr& addr) = 0;

    // push a halt or restart message to client
    virtual void push_halt_restart_msg(const HaltRestart::Type type,
				       const std::string& reason,
				       const bool tell_client) = 0;

  };

  // Base class for factory used to create TransportClientInstanceRecv objects.
  struct TransportClientInstanceFactory : public RC<thread_unsafe_refcount>
  {
    typedef RCPtr<TransportClientInstanceFactory> Ptr;

    virtual TransportClientInstanceRecv::Ptr new_client_instance() = 0;
    virtual bool validate_initial_packet(const Buffer& net_buf) = 0;
  };

} // namespace openvpn

#endif 
