// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import * as LitHtml from '../../third_party/lit-html/lit-html.js';
import * as Static from './static.js';
export {Directive, type TemplateResult} from '../../third_party/lit-html/lit-html.js';

const {render, svg, Directives, nothing, noChange} = LitHtml;
const {html, literal, flattenTemplate} = Static;

type LitTemplate = LitHtml.TemplateResult|typeof nothing;

export {
  render,
  Directives,
  nothing,
  noChange,
  svg,
  html,
  literal,
  flattenTemplate,  // Exposed for unit testing.
  type LitTemplate,
};
