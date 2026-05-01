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

using namespace Shared::Platform;

class SocketTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(SocketTest);

    CPPUNIT_TEST(test_ip_ipv4_string);
    CPPUNIT_TEST(test_ip_ipv4_cursor_stripped);
    CPPUNIT_TEST(test_ip_ipv4_bytes);
    CPPUNIT_TEST(test_ip_ipv6_string);
    CPPUNIT_TEST(test_ip_ipv6_cursor_stripped);
    CPPUNIT_TEST(test_ip_empty_string);

    CPPUNIT_TEST(test_parseHostPort_ipv4_with_port);
    CPPUNIT_TEST(test_parseHostPort_ipv4_no_port);
    CPPUNIT_TEST(test_parseHostPort_hostname_with_port);
    CPPUNIT_TEST(test_parseHostPort_ipv6_bracket_with_port);
    CPPUNIT_TEST(test_parseHostPort_ipv6_bracket_no_port);
    CPPUNIT_TEST(test_parseHostPort_ipv6_bare_no_port_extracted);
    CPPUNIT_TEST(test_parseHostPort_cursor_stripped);
    CPPUNIT_TEST(test_parseHostPort_ipv6_bracket_cursor_stripped);

    CPPUNIT_TEST(test_buildHostDisplay_ipv4_with_port);
    CPPUNIT_TEST(test_buildHostDisplay_ipv4_no_port);
    CPPUNIT_TEST(test_buildHostDisplay_ipv6_with_port);
    CPPUNIT_TEST(test_buildHostDisplay_ipv6_no_port);
    CPPUNIT_TEST(test_buildHostDisplay_hostname_with_port);
    CPPUNIT_TEST(test_buildHostDisplay_roundtrip_ipv6);
    CPPUNIT_TEST(test_buildHostDisplay_roundtrip_ipv4);

    CPPUNIT_TEST_SUITE_END();

  public:
    // Plain IPv4 address round-trips through the string constructor
    void test_ip_ipv4_string() {
        Ip ip("127.0.0.1");
        CPPUNIT_ASSERT_EQUAL(std::string("127.0.0.1"), ip.getString());
    }

    // The UI appends '_' as a cursor — it must be stripped
    void test_ip_ipv4_cursor_stripped() {
        Ip ip("192.168.1.100_");
        CPPUNIT_ASSERT_EQUAL(std::string("192.168.1.100"), ip.getString());
    }

    // Byte constructor still works (used by legacy code)
    void test_ip_ipv4_bytes() {
        Ip ip(10, 0, 0, 1);
        CPPUNIT_ASSERT_EQUAL(std::string("10.0.0.1"), ip.getString());
    }

    // IPv6 loopback address is stored and returned unchanged
    void test_ip_ipv6_string() {
        Ip ip("::1");
        CPPUNIT_ASSERT_EQUAL(std::string("::1"), ip.getString());
    }

    // UI cursor character is stripped from IPv6 addresses too
    void test_ip_ipv6_cursor_stripped() {
        Ip ip("::1_");
        CPPUNIT_ASSERT_EQUAL(std::string("::1"), ip.getString());
    }

    // Empty string gives the zero address
    void test_ip_empty_string() {
        Ip ip("");
        CPPUNIT_ASSERT_EQUAL(std::string("0.0.0.0"), ip.getString());
    }

    // --- parseHostPort ---

    // IPv4 address with port: extracts both
    void test_parseHostPort_ipv4_with_port() {
        std::string host = "192.168.1.1:6234";
        int port = 0;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("192.168.1.1"), host);
        CPPUNIT_ASSERT_EQUAL(6234, port);
    }

    // IPv4 address without port: host unchanged, port unchanged
    void test_parseHostPort_ipv4_no_port() {
        std::string host = "192.168.1.1";
        int port = 9999;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("192.168.1.1"), host);
        CPPUNIT_ASSERT_EQUAL(9999, port);
    }

    // Hostname with port
    void test_parseHostPort_hostname_with_port() {
        std::string host = "example.com:61367";
        int port = 0;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("example.com"), host);
        CPPUNIT_ASSERT_EQUAL(61367, port);
    }

    // IPv6 in bracket notation with port: the crash case
    void test_parseHostPort_ipv6_bracket_with_port() {
        std::string host = "[2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b]:61367";
        int port = 0;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b"), host);
        CPPUNIT_ASSERT_EQUAL(61367, port);
    }

    // IPv6 in bracket notation without port
    void test_parseHostPort_ipv6_bracket_no_port() {
        std::string host = "[::1]";
        int port = 9999;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("::1"), host);
        CPPUNIT_ASSERT_EQUAL(9999, port);
    }

    // Bare IPv6 (no brackets): host is left intact, port is not changed.
    // This is the exact input that caused the strToInt crash on "1c43".
    void test_parseHostPort_ipv6_bare_no_port_extracted() {
        std::string host = "2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b";
        int port = 9999;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b"), host);
        CPPUNIT_ASSERT_EQUAL(9999, port);
    }

    // UI cursor '_' is stripped before parsing
    void test_parseHostPort_cursor_stripped() {
        std::string host = "192.168.0.5:6234_";
        int port = 0;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("192.168.0.5"), host);
        CPPUNIT_ASSERT_EQUAL(6234, port);
    }

    // UI cursor stripped from bracketed IPv6 label text
    void test_parseHostPort_ipv6_bracket_cursor_stripped() {
        std::string host = "[::1]:61367_";
        int port = 0;
        Ip::parseHostPort(host, port);
        CPPUNIT_ASSERT_EQUAL(std::string("::1"), host);
        CPPUNIT_ASSERT_EQUAL(61367, port);
    }

    // --- buildHostDisplay ---

    // IPv4 with positive port: plain "host:port"
    void test_buildHostDisplay_ipv4_with_port() { CPPUNIT_ASSERT_EQUAL(std::string("192.168.1.1:6234"), Ip::buildHostDisplay("192.168.1.1", 6234)); }

    // IPv4 with port <= 0: host returned as-is
    void test_buildHostDisplay_ipv4_no_port() { CPPUNIT_ASSERT_EQUAL(std::string("192.168.1.1"), Ip::buildHostDisplay("192.168.1.1", 0)); }

    // IPv6 with port: bracket notation "[addr]:port"
    void test_buildHostDisplay_ipv6_with_port() {
        CPPUNIT_ASSERT_EQUAL(std::string("[2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b]:61367"), Ip::buildHostDisplay("2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b", 61367));
    }

    // IPv6 with port <= 0: host returned as-is (no brackets added)
    void test_buildHostDisplay_ipv6_no_port() { CPPUNIT_ASSERT_EQUAL(std::string("::1"), Ip::buildHostDisplay("::1", 0)); }

    // Hostname with port: plain "host:port"
    void test_buildHostDisplay_hostname_with_port() { CPPUNIT_ASSERT_EQUAL(std::string("example.com:6234"), Ip::buildHostDisplay("example.com", 6234)); }

    // buildHostDisplay output fed back into parseHostPort recovers the same
    // host and port — this is the round-trip that connectToServer() relies on
    void test_buildHostDisplay_roundtrip_ipv6() {
        const std::string addr = "2a04:1c43:31da:0:7bb0:7ebc:5759:9a6b";
        const int origPort = 61367;
        std::string display = Ip::buildHostDisplay(addr, origPort) + '_';

        std::string host = display;
        int port = 9999;
        Ip::parseHostPort(host, port);

        CPPUNIT_ASSERT_EQUAL(addr, host);
        CPPUNIT_ASSERT_EQUAL(origPort, port);
    }

    void test_buildHostDisplay_roundtrip_ipv4() {
        const std::string addr = "192.168.1.1";
        const int origPort = 6234;
        std::string display = Ip::buildHostDisplay(addr, origPort) + '_';

        std::string host = display;
        int port = 9999;
        Ip::parseHostPort(host, port);

        CPPUNIT_ASSERT_EQUAL(addr, host);
        CPPUNIT_ASSERT_EQUAL(origPort, port);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);
