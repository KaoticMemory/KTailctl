# SPDX-FileCopyrightText: 2026 Nick
# SPDX-License-Identifier: GPL-3.0-or-later

ecm_qt_declare_logging_category(
  ktailctl
  HEADER
  "logging_tailscale_taildrop.hpp"
  IDENTIFIER
  "Logging::Tailscale::Taildrop"
  CATEGORY_NAME
  "org.fkoehler.KTailctl.Tailscale.Taildrop"
  DEFAULT_SEVERITY
  Info)

target_sources(
  ktailctl
  PRIVATE paths.hpp
          paths.cpp
          waiting_file.hpp
          waiting_file.cpp
          taildrop.hpp
          taildrop.cpp
          ${CMAKE_CURRENT_BINARY_DIR}/logging_tailscale_taildrop.cpp)
target_include_directories(ktailctl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
