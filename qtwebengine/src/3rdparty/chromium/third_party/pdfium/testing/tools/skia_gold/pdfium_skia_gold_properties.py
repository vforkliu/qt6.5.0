#!/usr/bin/env python
# Copyright 2021 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""PDFium implementation of //build/skia_gold_common/skia_gold_properties.py."""

import subprocess
import sys

import pdfium_root
from skia_gold_common import skia_gold_properties


class PDFiumSkiaGoldProperties(skia_gold_properties.SkiaGoldProperties):

  @staticmethod
  def _GetGitOriginMainHeadSha1():
    root_finder = pdfium_root.RootDirectoryFinder()
    try:
      return subprocess.check_output(['git', 'rev-parse', 'origin/main'],
                                     shell=_IsWin(),
                                     cwd=root_finder.pdfium_root).strip()
    except subprocess.CalledProcessError:
      return None


def _IsWin():
  return sys.platform == 'win32'
