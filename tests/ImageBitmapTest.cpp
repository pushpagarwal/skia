/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkBitmap.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "tests/Test.h"

// skbug.com/40036261
// Test that when we make an image from a subset of a bitmap, that it
// has a diff (ID, dimensions) from an image made from the entire
// bitmap or a different subset of the image.
DEF_TEST(ImageBitmapIdentity, r) {
    SkBitmap bm, a, b;
    bm.allocN32Pixels(32, 64);
    bm.eraseColor(SK_ColorBLACK);
    bm.setImmutable();
    (void)bm.extractSubset(&a, SkIRect::MakeXYWH(0, 0, 32, 32));
    (void)bm.extractSubset(&b, SkIRect::MakeXYWH(0, 32, 32, 32));
    REPORTER_ASSERT(r, a.getGenerationID() == b.getGenerationID());
    auto img = bm.asImage();
    auto imgA = a.asImage();
    auto imgB = b.asImage();
    REPORTER_ASSERT(r, img->uniqueID() == bm.getGenerationID());
    REPORTER_ASSERT(r, img->uniqueID() != imgA->uniqueID());
    REPORTER_ASSERT(r, img->uniqueID() != imgB->uniqueID());
    REPORTER_ASSERT(r, imgA->uniqueID() != imgB->uniqueID());
    REPORTER_ASSERT(r, imgA->uniqueID() != a.getGenerationID());
    REPORTER_ASSERT(r, imgB->uniqueID() != b.getGenerationID());
}
