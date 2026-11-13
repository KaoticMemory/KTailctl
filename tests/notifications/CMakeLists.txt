# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

add_ktailctl_test(test_notification_planner
    SOURCES
        test_notification_planner.cpp
        ${CMAKE_SOURCE_DIR}/src/notifications/notification_planner.cpp)
