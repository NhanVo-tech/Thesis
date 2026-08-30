# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(ProjectDefinition ProjectName.c ../isr_hs.c)

target_include_directories(
  ProjectDefinition PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:include/ProjectDefinition>
)

install(TARGETS ProjectDefinition EXPORT ProjectDefinition-config)
install(EXPORT ProjectDefinition-config
        DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/ProjectDefinition/cmake
)

include(uwb_stack_llhw.cmake)
