# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(ktailctl PRIVATE notification_planner.hpp notification_planner.cpp notifier.hpp notifier.cpp)
target_include_directories(ktailctl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
