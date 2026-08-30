#!/usr/bin/env python3

import os
from CreateTargetCommon__dev import CreateTargetCommon

if __name__ == "__main__":
    # Build environment variables
    project_base = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
    target_os = "FreeRTOS"
    board = "Q-TAG"
    sample = "QANI"

    create_target = CreateTargetCommon(project_base, target_os, board, sample)

    # Run the target creation process
    create_target.run()
