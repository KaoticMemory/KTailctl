# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

ecm_qt_declare_logging_category(
  ktailctl
  HEADER
  "logging_tailscale_ping.hpp"
  IDENTIFIER
  "Logging::Tailscale::Ping"
  CATEGORY_NAME
  "org.fkoehler.KTailctl.Tailscale.Ping"
  DEFAULT_SEVERITY
  Info)

target_sources(
  ktailctl
  PRIVATE ping_result.hpp
          ping_result.cpp
          peer_ping.hpp
          peer_ping.cpp
          ${CMAKE_CURRENT_BINARY_DIR}/logging_tailscale_ping.cpp)
target_include_directories(ktailctl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
