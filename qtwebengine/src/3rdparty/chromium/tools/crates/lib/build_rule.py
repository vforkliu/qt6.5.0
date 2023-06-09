# python3
# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from lib import consts
from lib import common
from lib import cargo
from lib.compiler import BuildConditionSet

import argparse
from typing import Optional, Tuple


class BuildRuleUsage:
    """Container for data that differs depending how the crate is consumed.

    There are 3 ways a crate can be consumed, and data for generating the
    resulting build rule will differ:
    * For normal usage. The crate is used from another crate, in their library
      or executable outputs.
    * For build scripts. The crate is used from build.rs in another crate.
    * For tests. The crate is used from tests in another crate.
    """

    def __init__(self):
        # Whether the crate should be allowed for direct use from first-party
        # code or not.
        self.for_first_party: bool = False
        # Set of architectures where another crate consumes this crate. If
        # empty, no GN target needs to be written.
        self.used_on_archs: BuildConditionSet = BuildConditionSet.EMPTY()
        # Dictionary of features, each defining a feature for building each of
        # the crate's targets (lib, binaries, build.rs, and tests). This list
        # includes "default" if default features should be used, or omits it
        # otherwise, which differs slightly from how rustc and cargo represent
        # including or excluding default features (they use a separate bool
        # and command line flag.
        #   key: The name of the feature as string
        #   value: `BuildConditionSet` for where the feature is enabled.
        self.features: list[tuple[str, BuildConditionSet]] = []
        # List of dictionaries, each defining a dependency for building the
        # crate's library and binaries.
        #   deppath: GN target path of the dependency as string.
        #   compile_modes: `BuildConditionSet` for where the dependency is used.
        self.deps: list[dict] = []
        # List of dictionaries, each defining a dependency for building the
        # crate's build.rs (or equivalent) file, defined in `build_root` if it
        # exists.
        #   deppath: GN target path of the dependency as string.
        #   compile_modes: `BuildConditionSet` for where the dependency is used.
        self.build_deps: list[dict] = []
        # List of dictionaries, each defining a dependency for building the
        # crate's tests.
        #   deppath: GN target path of the dependency as string.
        #   compile_modes: `BuildConditionSet` for where the dependency is used.
        self.dev_deps: list[dict] = []

    def sort_internals(self):
        """Sorts any sortable containers to help make reproducible output."""
        self.features.sort(key=lambda t: t[0])
        self.build_deps.sort(key=lambda d: d["deppath"])
        self.deps.sort(key=lambda d: d["deppath"])
        self.dev_deps.sort(key=lambda d: d["deppath"])


class BuildRule:
    """A structured representation of the data used to generate a BUILD file.

    Contains data from the crate's Cargo.toml as well as data gathered from
    cargo with this tool."""

    def __init__(self, crate_name: str, epoch: str):
        # Name of the crate, not normalized. This is how rust code would refer
        # to the crate.
        self.crate_name: str = crate_name
        # The version epoch of the crate, which is used to generate metadata for
        # the crate's output.
        self.epoch: str = epoch

        # None if there is no lib build target, or the relative GN file path
        # string to the lib's root .rs file.
        self.lib_root: Optional[str] = None
        # Empty if there is no lib build target, or the relative GN file path
        # string to all Rust files that are part of the lib build, including
        # files generated by the build script if any.
        self.lib_sources: list[str] = []
        # If lib_root is not None, then this is "rlib" or "proc-macro",
        # specifying the type of lib.
        self.lib_type: Optional[str] = None
        # Empty if there are no bin targets. List of dictionaries with keys:
        #   name: executable name
        #   root: The relative GN path string to the bin's root .rs file.
        #   sources: A list of relative GN path strings to all Rust files that
        #            are part of the bin build, including files generated by
        #            the build script if any.
        self.bins: list[dict] = []

        # Stuff shared between the lib (if present) and bins (if present).

        # If not none, a GN file path string to the build.rs file (or
        # equivalent) which is the root module of the build script.
        self.build_root: Optional[str] = None
        # The full set of source files including the root .rs file and those
        # used by it.
        self.build_sources: list[str] = []
        # The set of output files the build.rs would create, if there are any. A
        # human has to go and figure this out, so it must come from
        # third_party.toml.
        self.build_script_outputs: list[str] = []
        # The rust edition to build the crate with.
        self.edition: Optional[str] = None

        self.cargo_pkg_name: Optional[str] = None
        self.cargo_pkg_description: Optional[str] = None
        self.cargo_pkg_version: Optional[str] = None
        self.cargo_pkg_authors: Optional[str] = None

        self.normal_usage = BuildRuleUsage()
        self.buildrs_usage = BuildRuleUsage()
        self.test_usage = BuildRuleUsage()

    def get_usage(self, usage: cargo.CrateUsage) -> BuildRuleUsage:
        """Returns a `BuildRuleUsage` for a `cargo.CrateUsage`.

        These are also accessible directly as fields on the class, but this
        method helps to choose the correct one based on `cargo.CrateUsage`."""
        if usage == cargo.CrateUsage.FOR_NORMAL:
            return self.normal_usage
        if usage == cargo.CrateUsage.FOR_BUILDRS:
            return self.buildrs_usage
        if usage == cargo.CrateUsage.FOR_TESTS:
            return self.test_usage
        assert False  # Unhandled CrateUsage?

    def sort_internals(self):
        """Sorts any sortable containers to help make reproducible output."""
        self.bins.sort(key=lambda d: d["name"])
        self.build_sources.sort()
        self.build_script_outputs.sort()
        self.lib_sources.sort()
        self.normal_usage.sort_internals()
        self.buildrs_usage.sort_internals()
        self.test_usage.sort_internals()

    def _write(self, indent: int, s: str):
        """Append a string onto `self.out`.

        The string is indented with a number of spaces based on the `indent`
        argument."""
        self.out.append("{}{}\n".format(" " * indent, s))

    def _write_compile_modes_conditions(self, indent: int,
                                        compile_modes: BuildConditionSet):
        """Write a GN if statement that is true for the `BuildConditionSet`.

        This appends the generated text to `out`."""
        conds = compile_modes.get_gn_conditions()
        if len(conds) == 1:
            self._write(indent, "if ({}) {{".format(conds[0]))
        elif len(conds) > 1:
            self._write(indent, "if (({}) ||".format(conds[0]))
            for cond in conds[1:-1]:
                self._write(indent + 4, "({}) ||".format(cond))
            self._write(indent + 4, "({})) {{".format(conds[-1]))

    def _write_for_compile_modes(self, indent: int,
                                 compile_modes: BuildConditionSet,
                                 to_write: list[tuple[int, str]]):
        """Write a GN if statement and body for a `BuildConditionSet`

        This appends the generated text to `out` and returns the result.

        Args:
            indent: How much to indent each line generated by this function.
            compile_modes: Defines when the if statement should resolve to true.
            to_write: A list of strings to place in the body of the if
                statement. Each string comes with an indent value, which will
                be added to the top level `indent`.
        """
        self._write_compile_modes_conditions(indent, compile_modes)

        conds = compile_modes.get_gn_conditions()
        if conds:
            indent += 2

        for (write_indent, write_str) in to_write:
            self._write(indent + write_indent, write_str)

        if conds:
            indent -= 2
            self._write(indent, "}")

    def _write_common(self, indent: int, build_rule: BuildRule, sources: list,
                      usage: cargo.CrateUsage):
        """Write the GN content that's common for libraries and executables.

        This appends the generated text to `out` and returns the result.

        Args:
            build_rule: The BuildRule being written from.
            sources: The set of Rust source files. This is passed separately
                since it is stored differently for libraries vs executables in
                the BuildRule.
            usage: How this GN target is going to be used by other crates (or
                cargo.CrateUsage.FOR_NORMAL if it's generating an executable).
        """
        assert sources  # There's always a root source at least.
        self._write(indent, "sources = [")
        for s in sources:
            self._write(indent + 2, "\"{}\",".format(s))
        self._write(indent, "]")

        self._write(indent, "edition = \"{}\"".format(build_rule.edition))
        if build_rule.cargo_pkg_version:
            self._write(
                indent, "cargo_pkg_version = \"{}\"".format(
                    build_rule.cargo_pkg_version))
        if build_rule.cargo_pkg_authors:
            self._write(
                indent, "cargo_pkg_authors = \"{}\"".format(", ".join(
                    build_rule.cargo_pkg_authors)))
        if build_rule.cargo_pkg_name:
            self._write(
                indent, "cargo_pkg_name = \"{}\"".format(
                    build_rule.cargo_pkg_name.replace('\n', '')))
        if build_rule.cargo_pkg_description:
            self._write(
                indent, "cargo_pkg_description = \"{}\"".format(
                    build_rule.cargo_pkg_description.replace('\n', '').replace(
                        '"', '\'')))

        # Add these if, in the future, we want to explicitly mark each
        # third-party crate instead of doing so from the GN template.
        #
        # self._write(indent,
        #  "configs -= [ \"//build/config/compiler:chromium_code\" ]")
        # self._write(indent,
        #  "configs += [ \"//build/config/compiler:no_chromium_code\" ]")

        build_rule_usage = build_rule.get_usage(usage)

        for (deps, gn_name) in [(build_rule_usage.deps, "deps"),
                                (build_rule_usage.build_deps, "build_deps"),
                                (build_rule_usage.dev_deps, "test_deps")]:
            if not deps:
                continue
            global_deps = []
            specific_deps = []

            for d in deps:
                compile_modes = d["compile_modes"]
                if compile_modes.is_always_true(
                ) or compile_modes == build_rule_usage.used_on_archs:
                    global_deps += [d]
                else:
                    specific_deps += [d]

            if global_deps or specific_deps:
                self._write(indent, "{} = [".format(gn_name))
            for d in global_deps:
                self._write(indent + 2, "\"{}\",".format(d["deppath"]))
            if global_deps or specific_deps:
                self._write(indent, "]")
            for d in specific_deps:
                compile_modes = d["compile_modes"]
                self._write_for_compile_modes(
                    indent, compile_modes,
                    [(0, "deps += [ \"{}\" ]".format(d["deppath"]))])

        global_features = []
        specific_features = []

        for (feature, compile_modes) in build_rule_usage.features:
            # The default feature is a cargo thing, and just translates to
            # other features specified by the Cargo.toml.
            if feature == "default":
                continue
            if compile_modes.is_always_true(
            ) or compile_modes == build_rule_usage.used_on_archs:
                global_features += [feature]
            else:
                specific_features += [(feature, compile_modes)]

        if global_features or specific_features:
            self._write(indent, "features = [")
        for f in global_features:
            self._write(indent + 2, "\"{}\",".format(f))
        if global_features or specific_features:
            self._write(indent, "]")
        for (f, compile_modes) in specific_features:
            self._write_for_compile_modes(
                2, compile_modes, [(0, "features += [ \"{}\" ]".format(f))])

        if build_rule.build_root:
            self._write(indent,
                        "build_root = \"{}\"".format(build_rule.build_root))
            self._write(indent, "build_sources = [")
            for s in build_rule.build_sources:
                self._write(indent + 2, "\"{}\",".format(s))
            self._write(indent, "]")
            if build_rule.build_script_outputs:
                self._write(indent, "build_script_outputs = [")
                for o in build_rule.build_script_outputs:
                    self._write(indent + 2, "\"{}\",".format(o))
                self._write(indent, "]")

    def generate_gn(self, args: argparse.Namespace, copyright_year: str) -> str:
        """Generate a BUILD.gn file contents.

        The BuildRule has all data needed to construct a BUILD file. This
        generates a BUILD.gn file.

        Args:
            args: The command-line arguments.
            copyright_year: The year as a string.
        """
        self.out: list[str] = []
        self._write(0, consts.GN_HEADER.format(year=copyright_year))

        for bin in self.bins:
            self._write(0, "cargo_crate(\"{}\") {{".format(bin["name"]))
            self._write(2, "crate_type = \"bin\"")
            self._write(2, "crate_root = \"{}\"".format(bin["root"]))
            self._write_common(2, self, bin["sources"],
                               cargo.CrateUsage.FOR_NORMAL)
            self._write(0, "}")

        if self.lib_root:
            for usage in cargo.CrateUsage:
                # TODO(danakj): If the BuildRuleUsage is the same as another we
                # should only generate one, and point the duplicate over by
                # using a GN group() target. This would avoid building a crate
                # multiple times if the same features will be used each time.

                used_on_archs = self.get_usage(usage).used_on_archs
                if not used_on_archs:
                    continue
                indent = 0
                if not used_on_archs.is_always_true():
                    self._write_compile_modes_conditions(indent, used_on_archs)
                    indent += 2

                self._write(
                    indent,
                    "cargo_crate(\"{}\") {{".format(usage.gn_target_name()))
                self._write(indent + 2,
                            "crate_name = \"{}\"".format(
                                common.crate_name_normalized(
                                    self.crate_name)))  # yapf: disable
                self._write(indent + 2, "epoch = \"{}\"".format(self.epoch))
                self._write(indent + 2,
                            "crate_type = \"{}\"".format(self.lib_type))
                if not self.get_usage(usage).for_first_party:
                    for c in consts.GN_VISIBILITY_COMMENT.split("\n"):
                        self._write(indent + 2, c)
                    self._write(indent + 2,
                                "visibility = [ \"{}\" ]".format(
                                    common.gn_third_party_path(
                                        rel_path=["*"])))  # yapf: disable
                if usage == cargo.CrateUsage.FOR_TESTS:
                    self._write(indent + 2, "testonly = \"true\"")
                self._write(indent + 2,
                            "crate_root = \"{}\"".format(self.lib_root))
                if usage == cargo.CrateUsage.FOR_NORMAL:
                    if args.with_tests:
                        self._write(indent + 2,
                                    "build_native_rust_unit_tests = true")
                    else:
                        for c in consts.GN_TESTS_COMMENT.split("\n"):
                            self._write(indent + 2, c)
                        self._write(indent + 2,
                                    "build_native_rust_unit_tests = false")
                else:
                    self._write(indent + 2,
                                "build_native_rust_unit_tests = false")
                self._write_common(indent + 2, self, self.lib_sources, usage)
                self._write(indent, "}")

                if not used_on_archs.is_always_true():
                    indent -= 2
                    self._write(indent, "}")

        return ''.join(self.out)
