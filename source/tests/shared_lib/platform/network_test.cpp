// ==============================================================
//	This file is part of MegaGlest Unit Tests (www.megaglest.org)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include <cppunit/extensions/HelperMacros.h>
#include "socket.h"
#include <unistd.h>

using namespace Shared::Platform;

// Base port for these tests; chosen to avoid well-known services.
static const int TEST_PORT_BASE = 34527;

// Poll server.accept() for up to ~100 ms (50 × 2 ms sleeps).
static Socket *pollAccept(ServerSocket &server, int tries = 50) {
    for (int i = 0; i < tries; ++i) {
        Socket *s = server.accept(false);
        if (s != nullptr) return s;
        usleep(2000);
    }
    return nullptr;
}

// Returns true if this platform supports IPv6 dual-stack: an AF_INET6 socket
// can be created and IPV6_V6ONLY can be cleared.  ServerSocket::bind() already
// does this probe and falls back to IPv4 silently, so we just check which
// family was actually chosen.
static bool platformHasIPv6DualStack() {
    ServerSocket probe;
    probe.bind(TEST_PORT_BASE + 9);
    return probe.getSocketFamily() == AF_INET6;
}

class NetworkTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(NetworkTest);
    CPPUNIT_TEST(test_server_bind);
    CPPUNIT_TEST(test_ipv4_client_connect);
    CPPUNIT_TEST(test_ipv4_client_address_on_accept);
    CPPUNIT_TEST(test_ipv6_client_connect);
    CPPUNIT_TEST(test_ipv6_client_address_on_accept);
    CPPUNIT_TEST_SUITE_END();

  public:
    // Server binds on a port and reports the port as bound
    void test_server_bind() {
        ServerSocket server;
        server.bind(TEST_PORT_BASE);
        server.listen(1);
        CPPUNIT_ASSERT(server.isPortBound());
        CPPUNIT_ASSERT(server.isSocketValid());
    }

    // An IPv4 client can connect to the listening server
    void test_ipv4_client_connect() {
        ServerSocket server;
        server.bind(TEST_PORT_BASE + 1);
        server.listen(1);

        ClientSocket client;
        client.connect(Ip("127.0.0.1"), TEST_PORT_BASE + 1);

        Socket *accepted = pollAccept(server);
        CPPUNIT_ASSERT_MESSAGE("IPv4 connection was not accepted", accepted != nullptr);
        CPPUNIT_ASSERT(accepted->isSocketValid());
        delete accepted;
    }

    // When an IPv4 client connects to a dual-stack server the accepted socket's
    // IP is a plain IPv4 string, not the mapped form "::ffff:127.0.0.1"
    void test_ipv4_client_address_on_accept() {
        ServerSocket server;
        server.bind(TEST_PORT_BASE + 2);
        server.listen(1);

        ClientSocket client;
        client.connect(Ip("127.0.0.1"), TEST_PORT_BASE + 2);

        Socket *accepted = pollAccept(server);
        CPPUNIT_ASSERT_MESSAGE("IPv4 connection was not accepted", accepted != nullptr);
        CPPUNIT_ASSERT_EQUAL(std::string("127.0.0.1"), accepted->getIpAddress());
        delete accepted;
    }

    // An IPv6 client can connect to the dual-stack server.
    // Skipped on platforms without IPv6 support.
    void test_ipv6_client_connect() {
        if (!platformHasIPv6DualStack()) {
            std::cout << " [skipped: no IPv6 dual-stack] ";
            return;
        }

        ServerSocket server;
        server.bind(TEST_PORT_BASE + 3);
        server.listen(1);

        ClientSocket client;
        client.connect(Ip("::1"), TEST_PORT_BASE + 3);

        Socket *accepted = pollAccept(server);
        CPPUNIT_ASSERT_MESSAGE("IPv6 connection was not accepted", accepted != nullptr);
        CPPUNIT_ASSERT(accepted->isSocketValid());
        delete accepted;
    }

    // When an IPv6 client connects, the accepted socket reports the IPv6
    // address of the client
    void test_ipv6_client_address_on_accept() {
        if (!platformHasIPv6DualStack()) {
            std::cout << " [skipped: no IPv6 dual-stack] ";
            return;
        }

        ServerSocket server;
        server.bind(TEST_PORT_BASE + 4);
        server.listen(1);

        ClientSocket client;
        client.connect(Ip("::1"), TEST_PORT_BASE + 4);

        Socket *accepted = pollAccept(server);
        CPPUNIT_ASSERT_MESSAGE("IPv6 connection was not accepted", accepted != nullptr);
        CPPUNIT_ASSERT_EQUAL(std::string("::1"), accepted->getIpAddress());
        delete accepted;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(NetworkTest);
