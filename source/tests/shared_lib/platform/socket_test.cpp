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
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);
