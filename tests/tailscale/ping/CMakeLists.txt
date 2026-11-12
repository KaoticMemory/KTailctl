# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

add_ktailctl_test(test_ping_result
    SOURCES
        test_ping_result.cpp
        ${CMAKE_SOURCE_DIR}/src/tailscale/ping/ping_result.cpp
    LIBS
        KF6::I18n)
