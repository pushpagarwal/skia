/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkScalar.h"
#include "include/core/SkShader.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkTypes.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "tests/CtsEnforcement.h"
#include "tests/Test.h"
#include "tools/Resources.h"

#include <cstring>

class GrRecordingContext;
struct GrContextOptions;

static void test_bitmap_equality(skiatest::Reporter* reporter, SkBitmap& bm1, SkBitmap& bm2) {
    REPORTER_ASSERT(reporter, bm1.computeByteSize() == bm2.computeByteSize());
    REPORTER_ASSERT(reporter, 0 == memcmp(bm1.getPixels(), bm2.getPixels(), bm1.computeByteSize()));
}

static void paint_source(SkSurface* sourceSurface) {
    SkCanvas* sourceCanvas = sourceSurface->getCanvas();
    sourceCanvas->clear(0xFFDEDEDE);

    SkPaint paintColor;
    paintColor.setColor(0xFFFF0000);
    paintColor.setStyle(SkPaint::kFill_Style);

    SkRect rect = SkRect::MakeXYWH(
            SkIntToScalar(1),
            SkIntToScalar(0),
            SkIntToScalar(1),
            SkIntToScalar(sourceSurface->height()));

    sourceCanvas->drawRect(rect, paintColor);
}

static void run_shader_test(skiatest::Reporter* reporter, SkSurface* sourceSurface,
                            SkSurface* destinationSurface, SkImageInfo& info) {
    paint_source(sourceSurface);

    sk_sp<SkImage> sourceImage(sourceSurface->makeImageSnapshot());
    sk_sp<SkShader> sourceShader = sourceImage->makeShader(
            SkTileMode::kRepeat, SkTileMode::kRepeat, SkSamplingOptions());

    SkPaint paint;
    paint.setShader(sourceShader);

    SkCanvas* destinationCanvas = destinationSurface->getCanvas();
    destinationCanvas->clear(SK_ColorTRANSPARENT);
    destinationCanvas->drawPaint(paint);

    SkBitmap bmOrig;
    bmOrig.allocN32Pixels(info.width(), info.height());
    sourceSurface->readPixels(bmOrig, 0, 0);


    SkBitmap bm;
    bm.allocN32Pixels(info.width(), info.height());
    destinationSurface->readPixels(bm, 0, 0);

    test_bitmap_equality(reporter, bmOrig, bm);

    // Test with a translated shader
    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(-1), SkIntToScalar(0));

    sk_sp<SkShader> sourceShaderTranslated = sourceImage->makeShader(
            SkTileMode::kRepeat,
            SkTileMode::kRepeat,
            SkSamplingOptions(), &matrix);

    destinationCanvas->clear(SK_ColorTRANSPARENT);

    SkPaint paintTranslated;
    paintTranslated.setShader(sourceShaderTranslated);

    destinationCanvas->drawPaint(paintTranslated);

    SkBitmap bmt;
    bmt.allocN32Pixels(info.width(), info.height());
    destinationSurface->readPixels(bmt, 0, 0);

    //  Test correctness
    {
        for (int y = 0; y < info.height(); y++) {
            REPORTER_ASSERT(reporter, 0xFFFF0000 == bmt.getColor(0, y));

            for (int x = 1; x < info.width(); x++) {
                REPORTER_ASSERT(reporter, 0xFFDEDEDE == bmt.getColor(x, y));
            }
        }
    }
}

DEF_TEST(ImageNewShader, reporter) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(5, 5);

    auto sourceSurface(SkSurfaces::Raster(info));
    auto destinationSurface(SkSurfaces::Raster(info));

    run_shader_test(reporter, sourceSurface.get(), destinationSurface.get(), info);
}

static void gpu_to_gpu(skiatest::Reporter* reporter, GrRecordingContext* rContext) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(5, 5);

    auto sourceSurface(SkSurfaces::RenderTarget(rContext, skgpu::Budgeted::kNo, info));
    auto destinationSurface(SkSurfaces::RenderTarget(rContext, skgpu::Budgeted::kNo, info));

    run_shader_test(reporter, sourceSurface.get(), destinationSurface.get(), info);
}

static void raster_to_gpu(skiatest::Reporter* reporter, GrRecordingContext* rContext) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(5, 5);

    auto sourceSurface(SkSurfaces::Raster(info));
    auto destinationSurface(SkSurfaces::RenderTarget(rContext, skgpu::Budgeted::kNo, info));

    run_shader_test(reporter, sourceSurface.get(), destinationSurface.get(), info);
}

DEF_GANESH_TEST_FOR_RENDERING_CONTEXTS(ImageNewShader_GPU,
                                       reporter,
                                       ctxInfo,
                                       CtsEnforcement::kApiLevel_T) {
    auto dContext = ctxInfo.directContext();

    //  GPU -> GPU
    gpu_to_gpu(reporter, dContext);

    //  GPU -> RASTER not currently supported

    //  RASTER -> GPU
    raster_to_gpu(reporter, dContext);
}

DEF_TEST(ImageRawShader, reporter) {
    auto image = GetResourceAsImage("images/mandrill_32.png");
    REPORTER_ASSERT(reporter, image);

    // We should be able to turn this into a "raw" image shader:
    REPORTER_ASSERT(reporter, image->makeRawShader(SkFilterMode::kNearest));

    // ... but not if we request cubic filtering
    REPORTER_ASSERT(reporter, !image->makeRawShader(SkCubicResampler::Mitchell()));
}
