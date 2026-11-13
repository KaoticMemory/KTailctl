# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

add_ktailctl_test(test_taildrop_paths
    SOURCES
        test_taildrop_paths.cpp
        ${CMAKE_SOURCE_DIR}/src/tailscale/taildrop/paths.cpp)
