/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkPathUtils.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkScalar.h"
#include "include/core/SkSize.h"
#include "include/core/SkString.h"
#include "include/core/SkTypes.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/utils/SkParsePath.h"
#include "src/base/SkFloatBits.h"
#include "src/base/SkRandom.h"
#include "tools/ToolUtils.h"

#include <string.h>

#if defined(SK_GRAPHITE)
#include "include/gpu/graphite/ContextOptions.h"
#endif

#define W   400
#define H   400
#define N   50

constexpr SkScalar SW = SkIntToScalar(W);
constexpr SkScalar SH = SkIntToScalar(H);

static void rnd_rect(SkRect* r, SkPaint* paint, SkRandom& rand) {
    SkScalar x = rand.nextUScalar1() * W;
    SkScalar y = rand.nextUScalar1() * H;
    SkScalar w = rand.nextUScalar1() * (W >> 2);
    SkScalar h = rand.nextUScalar1() * (H >> 2);
    SkScalar hoffset = rand.nextSScalar1();
    SkScalar woffset = rand.nextSScalar1();

    r->setXYWH(x, y, w, h);
    r->offset(-w/2 + woffset, -h/2 + hoffset);

    paint->setColor(rand.nextU());
    paint->setAlphaf(1.0f);
}


class StrokesGM : public skiagm::GM {
public:
    StrokesGM() {}

protected:
    SkString getName() const override { return SkString("strokes_round"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

#if defined(SK_GRAPHITE)
    void modifyGraphiteContextOptions(skgpu::graphite::ContextOptions* options) const override {
        options->fMaxPathAtlasTextureSize = 0;
        options->fAllowMultipleAtlasTextures = false;
    }
#endif

    void onDraw(SkCanvas* canvas) override {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(9)/2);

        for (int y = 0; y < 2; y++) {
            paint.setAntiAlias(!!y);
            SkAutoCanvasRestore acr(canvas, true);
            canvas->translate(0, SH * y);
            canvas->clipRect(SkRect::MakeLTRB(
                                              SkIntToScalar(2), SkIntToScalar(2)
                                              , SW - SkIntToScalar(2), SH - SkIntToScalar(2)
                                              ));

            SkRandom rand;
            for (int i = 0; i < N; i++) {
                SkRect r;
                rnd_rect(&r, &paint, rand);
                canvas->drawOval(r, paint);
                rnd_rect(&r, &paint, rand);
                canvas->drawRoundRect(r, r.width()/4, r.height()/4, paint);
                rnd_rect(&r, &paint, rand);
            }
        }
    }

private:
    using INHERITED = skiagm::GM;
};

/* See
   https://code.google.com/p/chromium/issues/detail?id=422974          and
   http://jsfiddle.net/1xnku3sg/2/
 */
class ZeroLenStrokesGM : public skiagm::GM {
    SkPath fMoveHfPath, fMoveZfPath, fDashedfPath, fRefPath[4];
    SkPath fCubicPath, fQuadPath, fLinePath;
protected:
    void onOnceBeforeDraw() override {

        SkAssertResult(SkParsePath::FromSVGString("M0,0h0M10,0h0M20,0h0", &fMoveHfPath));
        SkAssertResult(SkParsePath::FromSVGString("M0,0zM10,0zM20,0z", &fMoveZfPath));
        SkAssertResult(SkParsePath::FromSVGString("M0,0h25", &fDashedfPath));
        SkAssertResult(SkParsePath::FromSVGString("M 0 0 C 0 0 0 0 0 0", &fCubicPath));
        SkAssertResult(SkParsePath::FromSVGString("M 0 0 Q 0 0 0 0", &fQuadPath));
        SkAssertResult(SkParsePath::FromSVGString("M 0 0 L 0 0", &fLinePath));

        for (int i = 0; i < 3; ++i) {
            fRefPath[0].addCircle(i * 10.f, 0, 5);
            fRefPath[1].addCircle(i * 10.f, 0, 10);
            fRefPath[2].addRect(i * 10.f - 4, -2, i * 10.f + 4, 6);
            fRefPath[3].addRect(i * 10.f - 10, -10, i * 10.f + 10, 10);
        }
    }

    SkString getName() const override { return SkString("zeroPath"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

    void onDraw(SkCanvas* canvas) override {
        SkPaint fillPaint, strokePaint, dashPaint;
        fillPaint.setAntiAlias(true);
        strokePaint = fillPaint;
        strokePaint.setStyle(SkPaint::kStroke_Style);
        for (int i = 0; i < 2; ++i) {
            fillPaint.setAlphaf(1.0f);
            strokePaint.setAlphaf(1.0f);
            strokePaint.setStrokeWidth(i ? 8.f : 10.f);
            strokePaint.setStrokeCap(i ? SkPaint::kSquare_Cap : SkPaint::kRound_Cap);
            canvas->save();
            canvas->translate(10 + i * 100.f, 10);
            canvas->drawPath(fMoveHfPath, strokePaint);
            canvas->translate(0, 20);
            canvas->drawPath(fMoveZfPath, strokePaint);
            dashPaint = strokePaint;
            const SkScalar intervals[] = { 0, 10 };
            dashPaint.setPathEffect(SkDashPathEffect::Make(intervals, 0));
            (void)skpathutils::FillPathWithPaint(fDashedfPath, dashPaint);
            canvas->translate(0, 20);
            canvas->drawPath(fDashedfPath, dashPaint);
            canvas->translate(0, 20);
            canvas->drawPath(fRefPath[i * 2], fillPaint);
            strokePaint.setStrokeWidth(20);
            strokePaint.setAlphaf(0.5f);
            canvas->translate(0, 50);
            canvas->drawPath(fMoveHfPath, strokePaint);
            canvas->translate(0, 30);
            canvas->drawPath(fMoveZfPath, strokePaint);
            canvas->translate(0, 30);
            fillPaint.setAlphaf(0.5f);
            canvas->drawPath(fRefPath[1 + i * 2], fillPaint);
            canvas->translate(0, 30);
            canvas->drawPath(fCubicPath, strokePaint);
            canvas->translate(0, 30);
            canvas->drawPath(fQuadPath, strokePaint);
            canvas->translate(0, 30);
            canvas->drawPath(fLinePath, strokePaint);
            canvas->restore();
        }
    }

private:
    using INHERITED = skiagm::GM;
};

class TeenyStrokesGM : public skiagm::GM {
    SkString getName() const override { return SkString("teenyStrokes"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

    static void line(SkScalar scale, SkCanvas* canvas, SkColor color) {
        SkPaint p;
        p.setAntiAlias(true);
        p.setStyle(SkPaint::kStroke_Style);
        p.setColor(color);
        canvas->translate(50, 0);
        canvas->save();
        p.setStrokeWidth(scale * 5);
        canvas->scale(1 / scale, 1 / scale);
        canvas->drawLine(20 * scale, 20 * scale, 20 * scale, 100 * scale, p);
        canvas->drawLine(20 * scale, 20 * scale, 100 * scale, 100 * scale, p);
        canvas->restore();
    }

    void onDraw(SkCanvas* canvas) override {
        line(0.00005f, canvas, SK_ColorBLACK);
        line(0.000045f, canvas, SK_ColorRED);
        line(0.0000035f, canvas, SK_ColorGREEN);
        line(0.000003f, canvas, SK_ColorBLUE);
        line(0.000002f, canvas, SK_ColorBLACK);
    }
private:
    using INHERITED = skiagm::GM;
};

DEF_SIMPLE_GM(CubicStroke, canvas, 384, 384) {
    SkPaint p;
    p.setAntiAlias(true);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(1.0720f);
    SkPath path;
    path.moveTo(-6000,-6000);
    path.cubicTo(-3500,5500,-500,5500,2500,-6500);
    canvas->drawPath(path, p);
    p.setStrokeWidth(1.0721f);
    canvas->translate(10, 10);
    canvas->drawPath(path, p);
    p.setStrokeWidth(1.0722f);
    canvas->translate(10, 10);
    canvas->drawPath(path, p);
}

DEF_SIMPLE_GM(zerolinestroke, canvas, 90, 120) {
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(20);
    paint.setAntiAlias(true);
    paint.setStrokeCap(SkPaint::kRound_Cap);

    SkPath path;
    path.moveTo(30, 90);
    path.lineTo(30, 90);
    path.lineTo(60, 90);
    path.lineTo(60, 90);
    canvas->drawPath(path, paint);

    path.reset();
    path.moveTo(30, 30);
    path.lineTo(60, 30);
    canvas->drawPath(path, paint);

    path.reset();
    path.moveTo(30, 60);
    path.lineTo(30, 60);
    path.lineTo(60, 60);
    canvas->drawPath(path, paint);
}

DEF_SIMPLE_GM(quadcap, canvas, 200, 200) {
    SkPaint p;
    p.setAntiAlias(true);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(0);
    SkPath path;
    SkPoint pts[] = {{105.738571f,13.126318f},
            {105.738571f,13.126318f},
            {123.753784f,1.f}};
    SkVector tangent = pts[1] - pts[2];
    tangent.normalize();
    SkPoint pts2[3];
    memcpy(pts2, pts, sizeof(pts));
    const SkScalar capOutset = SK_ScalarPI / 8;
    pts2[0].fX += tangent.fX * capOutset;
    pts2[0].fY += tangent.fY * capOutset;
    pts2[1].fX += tangent.fX * capOutset;
    pts2[1].fY += tangent.fY * capOutset;
    pts2[2].fX += -tangent.fX * capOutset;
    pts2[2].fY += -tangent.fY * capOutset;
    path.moveTo(pts2[0]);
    path.quadTo(pts2[1], pts2[2]);
    canvas->drawPath(path, p);

    path.reset();
    path.moveTo(pts[0]);
    path.quadTo(pts[1], pts[2]);
    p.setStrokeCap(SkPaint::kRound_Cap);
    canvas->translate(30, 0);
    canvas->drawPath(path, p);
}

class Strokes2GM : public skiagm::GM {
    SkPath fPath;
protected:
    void onOnceBeforeDraw() override {
        SkRandom rand;
        fPath.moveTo(0, 0);
        for (int i = 0; i < 13; i++) {
            SkScalar x = rand.nextUScalar1() * (W >> 1);
            SkScalar y = rand.nextUScalar1() * (H >> 1);
            fPath.lineTo(x, y);
        }
    }

    SkString getName() const override { return SkString("strokes_poly"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

    void onDraw(SkCanvas* canvas) override {
        canvas->drawColor(SK_ColorWHITE);

        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(9)/2);

        for (int y = 0; y < 2; y++) {
            paint.setAntiAlias(!!y);
            SkAutoCanvasRestore acr(canvas, true);
            canvas->translate(0, SH * y);
            canvas->clipRect(SkRect::MakeLTRB(SkIntToScalar(2),
                                              SkIntToScalar(2),
                                              SW - SkIntToScalar(2),
                                              SH - SkIntToScalar(2)));

            SkRandom rand;
            for (int i = 0; i < N/2; i++) {
                SkRect r;
                rnd_rect(&r, &paint, rand);
                canvas->rotate(SkIntToScalar(15), SW/2, SH/2);
                canvas->drawPath(fPath, paint);
            }
        }
    }

private:
    using INHERITED = skiagm::GM;
};

//////////////////////////////////////////////////////////////////////////////

static SkRect inset(const SkRect& r) {
    SkRect rr(r);
    rr.inset(r.width()/10, r.height()/10);
    return rr;
}

class Strokes3GM : public skiagm::GM {
    static void make0(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addRect(bounds, SkPathDirection::kCW);
        path->addRect(inset(bounds), SkPathDirection::kCW);
        title->set("CW CW");
    }

    static void make1(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addRect(bounds, SkPathDirection::kCW);
        path->addRect(inset(bounds), SkPathDirection::kCCW);
        title->set("CW CCW");
    }

    static void make2(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addOval(bounds, SkPathDirection::kCW);
        path->addOval(inset(bounds), SkPathDirection::kCW);
        title->set("CW CW");
    }

    static void make3(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addOval(bounds, SkPathDirection::kCW);
        path->addOval(inset(bounds), SkPathDirection::kCCW);
        title->set("CW CCW");
    }

    static void make4(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addRect(bounds, SkPathDirection::kCW);
        SkRect r = bounds;
        r.inset(bounds.width() / 10, -bounds.height() / 10);
        path->addOval(r, SkPathDirection::kCW);
        title->set("CW CW");
    }

    static void make5(SkPath* path, const SkRect& bounds, SkString* title) {
        path->addRect(bounds, SkPathDirection::kCW);
        SkRect r = bounds;
        r.inset(bounds.width() / 10, -bounds.height() / 10);
        path->addOval(r, SkPathDirection::kCCW);
        title->set("CW CCW");
    }

public:
    Strokes3GM() {}

protected:
    SkString getName() const override { return SkString("strokes3"); }

    SkISize getISize() override { return SkISize::Make(1500, 1500); }

    void onDraw(SkCanvas* canvas) override {
        SkPaint origPaint;
        origPaint.setAntiAlias(true);
        origPaint.setStyle(SkPaint::kStroke_Style);
        SkPaint fillPaint(origPaint);
        fillPaint.setColor(SK_ColorRED);
        SkPaint strokePaint(origPaint);
        strokePaint.setColor(ToolUtils::color_to_565(0xFF4444FF));

        void (*procs[])(SkPath*, const SkRect&, SkString*) = {
            make0, make1, make2, make3, make4, make5
        };

        canvas->translate(SkIntToScalar(20), SkIntToScalar(80));

        SkRect bounds = SkRect::MakeWH(SkIntToScalar(50), SkIntToScalar(50));
        SkScalar dx = bounds.width() * 4/3;
        SkScalar dy = bounds.height() * 5;

        for (size_t i = 0; i < std::size(procs); ++i) {
            SkPath orig;
            SkString str;
            procs[i](&orig, bounds, &str);

            canvas->save();
            for (int j = 0; j < 13; ++j) {
                strokePaint.setStrokeWidth(SK_Scalar1 * j * j);
                canvas->drawPath(orig, strokePaint);
                canvas->drawPath(orig, origPaint);
                SkPath fill = skpathutils::FillPathWithPaint(orig, strokePaint);
                canvas->drawPath(fill, fillPaint);
                canvas->translate(dx + strokePaint.getStrokeWidth(), 0);
            }
            canvas->restore();
            canvas->translate(0, dy);
        }
    }

private:
    using INHERITED = skiagm::GM;
};

class Strokes4GM : public skiagm::GM {
public:
    Strokes4GM() {}

protected:
    SkString getName() const override { return SkString("strokes_zoomed"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

    void onDraw(SkCanvas* canvas) override {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(0.055f);

        canvas->scale(1000, 1000);
        canvas->drawCircle(0, 2, 1.97f, paint);
    }

private:
    using INHERITED = skiagm::GM;
};

// Test stroking for curves that produce degenerate tangents when t is 0 or 1 (skbug.com/40035337)
class Strokes5GM : public skiagm::GM {
public:
    Strokes5GM() {}

protected:
    SkString getName() const override { return SkString("zero_control_stroke"); }

    SkISize getISize() override { return SkISize::Make(W, H * 2); }

    void onDraw(SkCanvas* canvas) override {
        SkPaint p;
        p.setColor(SK_ColorRED);
        p.setAntiAlias(true);
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(40);
        p.setStrokeCap(SkPaint::kButt_Cap);

        SkPath path;
        path.moveTo(157.474f,111.753f);
        path.cubicTo(128.5f,111.5f,35.5f,29.5f,35.5f,29.5f);
        canvas->drawPath(path, p);
        path.reset();
        path.moveTo(250, 50);
        path.quadTo(280, 80, 280, 80);
        canvas->drawPath(path, p);
        path.reset();
        path.moveTo(150, 50);
        path.conicTo(180, 80, 180, 80, 0.707f);
        canvas->drawPath(path, p);

        path.reset();
        path.moveTo(157.474f,311.753f);
        path.cubicTo(157.474f,311.753f,85.5f,229.5f,35.5f,229.5f);
        canvas->drawPath(path, p);
        path.reset();
        path.moveTo(280, 250);
        path.quadTo(280, 250, 310, 280);
        canvas->drawPath(path, p);
        path.reset();
        path.moveTo(180, 250);
        path.conicTo(180, 250, 210, 280, 0.707f);
        canvas->drawPath(path, p);
    }

private:
    using INHERITED = skiagm::GM;
};


//////////////////////////////////////////////////////////////////////////////

DEF_GM( return new StrokesGM; )
DEF_GM( return new Strokes2GM; )
DEF_GM( return new Strokes3GM; )
DEF_GM( return new Strokes4GM; )
DEF_GM( return new Strokes5GM; )

DEF_GM( return new ZeroLenStrokesGM; )
DEF_GM( return new TeenyStrokesGM; )

DEF_SIMPLE_GM(zerolinedash, canvas, 256, 256) {
    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SkColorSetARGB(255, 0, 0, 0));
    paint.setStrokeWidth(11);
    paint.setStrokeCap(SkPaint::kRound_Cap);
    paint.setStrokeJoin(SkPaint::kBevel_Join);

    SkScalar dash_pattern[] = {1, 5};
    paint.setPathEffect(SkDashPathEffect::Make(dash_pattern, 0));

    canvas->drawLine(100, 100, 100, 100, paint);
}

#ifdef PDF_IS_FIXED_SO_THIS_DOESNT_BREAK_IT
DEF_SIMPLE_GM(longrect_dash, canvas, 250, 250) {
    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SkColorSetARGB(255, 0, 0, 0));
    paint.setStrokeWidth(5);
    paint.setStrokeCap(SkPaint::kRound_Cap);
    paint.setStrokeJoin(SkPaint::kBevel_Join);
    paint.setStyle(SkPaint::kStroke_Style);
    SkScalar dash_pattern[] = {1, 5};
    paint.setPathEffect(SkDashPathEffect::Make(dash_pattern, 2, 0));
    // try all combinations of stretching bounds
    for (auto left : { 20.f, -100001.f } ) {
        for (auto top : { 20.f, -100001.f } ) {
            for (auto right : { 40.f, 100001.f } ) {
                for (auto bottom : { 40.f, 100001.f } ) {
                    canvas->save();
                    canvas->clipRect({10, 10, 50, 50});
                    canvas->drawRect({left, top, right, bottom}, paint);
                    canvas->restore();
                    canvas->translate(60, 0);
               }
            }
            canvas->translate(-60 * 4, 60);
        }
    }
}
#endif

DEF_SIMPLE_GM(inner_join_geometry, canvas, 1000, 700) {
    // These paths trigger cases where we must add inner join geometry.
    // skbug.com/40043052
    const SkPoint pathPoints[] = {
        /*moveTo*/  /*lineTo*/  /*lineTo*/
        {119,  71}, {129, 151}, {230,  24},
        {200, 144}, {129, 151}, {230,  24},
        {192, 176}, {224, 175}, {281, 103},
        {233, 205}, {224, 175}, {281, 103},
        {121, 216}, {234, 189}, {195, 147},
        {141, 216}, {254, 189}, {238, 250},
        {159, 202}, {269, 197}, {289, 165},
        {159, 202}, {269, 197}, {287, 227},
    };

    SkPaint pathPaint;
    pathPaint.setStroke(true);
    pathPaint.setAntiAlias(true);
    pathPaint.setStrokeWidth(100);

    SkPaint skeletonPaint;
    skeletonPaint.setStroke(true);
    skeletonPaint.setAntiAlias(true);
    skeletonPaint.setStrokeWidth(0);
    skeletonPaint.setColor(SK_ColorRED);

    canvas->translate(0, 50);
    for (size_t i = 0; i < std::size(pathPoints) / 3; i++) {
        auto path = SkPath::Polygon({pathPoints + i * 3, 3}, false);
        canvas->drawPath(path, pathPaint);

        SkPath fillPath = skpathutils::FillPathWithPaint(path, pathPaint);
        canvas->drawPath(fillPath, skeletonPaint);

        canvas->translate(200, 0);
        if ((i + 1) % 4 == 0) {
            canvas->translate(-800, 200);
        }
    }
}

DEF_SIMPLE_GM(skbug12244, canvas, 150, 150) {
    // Should look like a stroked triangle; these vertices are the results of the SkStroker
    // but we draw as a filled path in order to highlight that it's the GPU triangulating path
    // renderer that's the source of the problem, and not the stroking operation. The original
    // path was a simple:
    // m(0,0), l(100, 40), l(0, 80), l(0,0) with a stroke width of 15px
    SkPath path;
    path.moveTo(2.7854299545288085938, -6.9635753631591796875);
    path.lineTo( 120.194366455078125,                   40);
    path.lineTo(-7.5000004768371582031, 91.07775115966796875);
    path.lineTo(-7.5000004768371582031, -11.077748298645019531);
    path.lineTo(2.7854299545288085938, -6.9635753631591796875);
    path.moveTo(-2.7854299545288085938, 6.9635753631591796875);
    path.lineTo(                   0,                    0);
    path.lineTo(                 7.5,                    0);
    path.lineTo(7.5000004768371582031, 68.92224884033203125);
    path.lineTo(  79.805633544921875,                   40);
    path.lineTo(-2.7854299545288085938, 6.9635753631591796875);

    SkPaint p;
    p.setColor(SK_ColorGREEN);

    canvas->translate(20.f, 20.f);
    canvas->drawPath(path, p);
}

DEF_SIMPLE_GM(b_340982297, canvas, 80, 50) {
    SkPaint paint;
    paint.setAntiAlias(true);

    SkPath path;
    path.moveTo(30.23983f, 48.5674667f);
    path.lineTo(1.30884242f, 45.5222702f);
    path.lineTo(2.97688866f, 29.6749554f);
    path.lineTo(17.4423828f, 31.1975555f);
    path.lineTo(2.94269657f, 30.0452003f);
    path.lineTo(4.38597536f, 11.8849154f);
    path.lineTo(33.3853493f, 14.1896257f);
    path.close();

    canvas->drawPath(path, paint);

    path.reset();
    path.moveTo(73.3853455f, 4.18963623f);
    path.lineTo(69.995636f, 39.1360626f);
    path.lineTo(42.83145142f, 21.056778f);
    path.lineTo(42.97689819f, 19.6749573f);
    path.lineTo(57.4423828f, 21.1975555f);
    path.lineTo(42.94268799f, 20.0451965f);
    path.lineTo(44.38595581f, 1.88491821f);
    path.close();

    canvas->drawPath(path, paint);
}
