#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

import os
from CreateTargetCommon import CreateTargetCommon

if __name__ == "__main__":
    # Build environment variables
    project_base = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
    target_os = "FreeRTOS"
    board = "DWM3001CDK"
    sample = "CLI"

    create_target = CreateTargetCommon(project_base, target_os, board, sample)

    # Run the target creation process
    create_target.run()
