# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Definitions of builders in the chromium.swangle builder group."""

load("//lib/builders.star", "reclient", "sheriff_rotations")
load("//lib/ci.star", "ci")
load("//lib/consoles.star", "consoles")

ci.defaults.set(
    builder_group = "chromium.swangle",
    executable = "recipe:angle_chromium",
    execution_timeout = ci.DEFAULT_EXECUTION_TIMEOUT,
    reclient_instance = reclient.instance.DEFAULT_TRUSTED,
    reclient_jobs = reclient.jobs.DEFAULT,
    pool = ci.gpu.POOL,
    service_account = ci.gpu.SERVICE_ACCOUNT,
    sheriff_rotations = sheriff_rotations.CHROMIUM_GPU,

    # TODO(crbug.com/1362440): remove this.
    omit_python2 = False,
)

consoles.console_view(
    name = "chromium.swangle",
    ordering = {
        None: ["DEPS", "ToT SwiftShader", "Chromium"],
        "*os*": ["Windows", "Mac"],
        "*cpu*": consoles.ordering(short_names = ["x86", "x64"]),
        "DEPS": "*os*",
        "DEPS|Windows": "*cpu*",
        "DEPS|Linux": "*cpu*",
        "ToT SwiftShader": "*os*",
        "ToT SwiftShader|Windows": "*cpu*",
        "ToT SwiftShader|Linux": "*cpu*",
        "Chromium": "*os*",
    },
)

ci.gpu.linux_builder(
    name = "linux-swangle-chromium-x64",
    console_view_entry = consoles.console_view_entry(
        category = "Chromium|Linux",
        short_name = "x64",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
    reclient_jobs = reclient.jobs.HIGH_JOBS_FOR_CI,
)

ci.gpu.linux_builder(
    name = "linux-swangle-tot-swiftshader-x64",
    console_view_entry = consoles.console_view_entry(
        category = "ToT SwiftShader|Linux",
        short_name = "x64",
    ),
    reclient_jobs = reclient.jobs.HIGH_JOBS_FOR_CI,
)

ci.gpu.linux_builder(
    name = "linux-swangle-x64",
    console_view_entry = consoles.console_view_entry(
        category = "DEPS|Linux",
        short_name = "x64",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
    reclient_jobs = reclient.jobs.HIGH_JOBS_FOR_CI,
)

ci.gpu.mac_builder(
    name = "mac-swangle-chromium-x64",
    console_view_entry = consoles.console_view_entry(
        category = "Chromium|Mac",
        short_name = "x64",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
)

ci.gpu.windows_builder(
    name = "win-swangle-chromium-x86",
    console_view_entry = consoles.console_view_entry(
        category = "Chromium|Windows",
        short_name = "x86",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
    reclient_jobs = reclient.jobs.LOW_JOBS_FOR_CI,
)

ci.gpu.windows_builder(
    name = "win-swangle-tot-swiftshader-x64",
    console_view_entry = consoles.console_view_entry(
        category = "ToT SwiftShader|Windows",
        short_name = "x64",
    ),
    reclient_jobs = reclient.jobs.LOW_JOBS_FOR_CI,
)

ci.gpu.windows_builder(
    name = "win-swangle-tot-swiftshader-x86",
    console_view_entry = consoles.console_view_entry(
        category = "ToT SwiftShader|Windows",
        short_name = "x86",
    ),
    reclient_jobs = reclient.jobs.LOW_JOBS_FOR_CI,
)

ci.gpu.windows_builder(
    name = "win-swangle-x64",
    console_view_entry = consoles.console_view_entry(
        category = "DEPS|Windows",
        short_name = "x64",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
    reclient_jobs = reclient.jobs.LOW_JOBS_FOR_CI,
)

ci.gpu.windows_builder(
    name = "win-swangle-x86",
    console_view_entry = consoles.console_view_entry(
        category = "DEPS|Windows",
        short_name = "x86",
    ),
    executable = ci.DEFAULT_EXECUTABLE,
    reclient_jobs = reclient.jobs.LOW_JOBS_FOR_CI,
)
