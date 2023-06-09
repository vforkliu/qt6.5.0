// Copyright (C) 2016 the V8 project authors. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
es12id: 25.1.5.1
esid: sec-get-arraybuffer.prototype.bytelength
description: Returns 0 if the buffer is detached
info: |
  25.1.5.1 get ArrayBuffer.prototype.byteLength

  1. Let O be the this value.
  ...
  4. If IsDetachedBuffer(O) is true, return 0.
  ...
includes: [detachArrayBuffer.js]
---*/

var ab = new ArrayBuffer(1);

$DETACHBUFFER(ab);

assert.sameValue(ab.byteLength, 0);
