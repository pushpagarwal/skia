/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/pdf/SkPDFDevice.h"

#include "include/codec/SkCodec.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkClipOp.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkColorType.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkM44.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkPathTypes.h"
#include "include/core/SkPathUtils.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkShader.h"
#include "include/core/SkSize.h"
#include "include/core/SkSpan.h"
#include "include/core/SkString.h"
#include "include/core/SkStrokeRec.h"
#include "include/core/SkSurface.h"
#include "include/core/SkSurfaceProps.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkTypes.h"
#include "include/docs/SkPDFDocument.h"
#include "include/pathops/SkPathOps.h"
#include "include/private/base/SkDebug.h"
#include "include/private/base/SkTemplates.h"
#include "include/private/base/SkTo.h"
#include "src/base/SkScopeExit.h"
#include "src/base/SkTLazy.h"
#include "src/base/SkUTF.h"
#include "src/core/SkAdvancedTypefaceMetrics.h"
#include "src/core/SkAnnotationKeys.h"
#include "src/core/SkBitmapDevice.h"
#include "src/core/SkBlendModePriv.h"
#include "src/core/SkColorSpacePriv.h"
#include "src/core/SkDevice.h"
#include "src/core/SkDraw.h"
#include "src/core/SkGlyph.h"
#include "src/core/SkMask.h"
#include "src/core/SkMaskFilterBase.h"
#include "src/core/SkPaintPriv.h"
#include "src/core/SkRasterClip.h"
#include "src/core/SkSpecialImage.h"
#include "src/core/SkStrikeSpec.h"
#include "src/pdf/SkBitmapKey.h"
#include "src/pdf/SkClusterator.h"
#include "src/pdf/SkKeyedImage.h"
#include "src/pdf/SkPDFBitmap.h"
#include "src/pdf/SkPDFDocumentPriv.h"
#include "src/pdf/SkPDFFont.h"
#include "src/pdf/SkPDFFormXObject.h"
#include "src/pdf/SkPDFGraphicState.h"
#include "src/pdf/SkPDFResourceDict.h"
#include "src/pdf/SkPDFShader.h"
#include "src/pdf/SkPDFTag.h"
#include "src/pdf/SkPDFTypes.h"
#include "src/pdf/SkPDFUnion.h"
#include "src/pdf/SkPDFUtils.h"
#include "src/shaders/SkColorShader.h"
#include "src/shaders/SkShaderBase.h"
#include "src/text/GlyphRun.h"
#include "src/utils/SkClipStackUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

class SkBlender;
class SkMesh;
class SkVertices;

using namespace skia_private;

SkPDFDevice::MarkedContentManager::MarkedContentManager(SkPDFDocument* document,
                                                        SkDynamicMemoryWStream* out)
    : fDoc(document)
    , fOut(out)
    , fCurrentlyActiveMark()
    , fNextMarksElemId(0)
    , fCurrentMarksElemId(0)
    , fMadeMarks(false)
{}

SkPDFDevice::MarkedContentManager::~MarkedContentManager() {
    // This does not close the last open mark, that is done in SkPDFDevice::content.
    SkASSERT(fNextMarksElemId == 0);
};

void SkPDFDevice::MarkedContentManager::setNextMarksElemId(int nextMarksElemId) {
    fNextMarksElemId = nextMarksElemId;
}
int SkPDFDevice::MarkedContentManager::elemId() const { return fNextMarksElemId; }

void SkPDFDevice::MarkedContentManager::beginMark() {
    if (fNextMarksElemId == fCurrentMarksElemId) {
        return;
    }
    if (fCurrentMarksElemId) {
        // End this mark
        fOut->writeText("EMC\n");
        fCurrentlyActiveMark = SkPDFStructTree::Mark();
        fCurrentMarksElemId = 0;
    }
    if (fNextMarksElemId) {
        fCurrentlyActiveMark = fDoc->createMarkForElemId(fNextMarksElemId);
        if (fCurrentlyActiveMark) {
            // Begin this mark
            SkPDFUnion::Name(fCurrentlyActiveMark.structType()).emitObject(fOut);
            fOut->writeText(" <</MCID ");
            fOut->writeDecAsText(fCurrentlyActiveMark.mcid());
            fOut->writeText(" >>BDC\n");
            fCurrentMarksElemId = fCurrentlyActiveMark.elemId();
            fMadeMarks = true;
        } else if (SkPDF::NodeID::BackgroundArtifact <= fNextMarksElemId &&
                   fNextMarksElemId <= SkPDF::NodeID::OtherArtifact &&
                   fDoc->hasCurrentPage())
        {
            fOut->writeText("/Artifact");
            if (fNextMarksElemId == SkPDF::NodeID::OtherArtifact) {
                fOut->writeText(" BMC\n");
            } else if (fNextMarksElemId == SkPDF::NodeID::PaginationArtifact ||
                       fNextMarksElemId == SkPDF::NodeID::PaginationHeaderArtifact ||
                       fNextMarksElemId == SkPDF::NodeID::PaginationFooterArtifact ||
                       fNextMarksElemId == SkPDF::NodeID::PaginationWatermarkArtifact)
            {
                fOut->writeText(" <</Type /Pagination");
                if (fNextMarksElemId == SkPDF::NodeID::PaginationHeaderArtifact) {
                    fOut->writeText(" /Subtype /Header");
                } else if  (fNextMarksElemId == SkPDF::NodeID::PaginationFooterArtifact) {
                    fOut->writeText(" /Subtype /Footer");
                } else if (fNextMarksElemId == SkPDF::NodeID::PaginationWatermarkArtifact) {
                    fOut->writeText(" /Subtype /Watermark");
                }
                fOut->writeText(" >>BDC\n");
            } else if (fNextMarksElemId == SkPDF::NodeID::LayoutArtifact) {
                fOut->writeText(" <</Type /Layout >>BDC\n");
            } else if (fNextMarksElemId == SkPDF::NodeID::PageArtifact) {
                fOut->writeText(" <</Type /Page >>BDC\n");
            } else if (fNextMarksElemId == SkPDF::NodeID::BackgroundArtifact) {
                fOut->writeText(" <</Type /Background >>BDC\n");
            }
            fCurrentMarksElemId = fNextMarksElemId;
        }
    }
}

bool SkPDFDevice::MarkedContentManager::hasActiveMark() const { return bool(fCurrentlyActiveMark); }

void SkPDFDevice::MarkedContentManager::accumulate(const SkPoint& p) {
    SkASSERT(fCurrentlyActiveMark);
    fCurrentlyActiveMark.accumulate(p);
}

// This function destroys the mask and either frees or takes the pixels.
sk_sp<SkImage> mask_to_greyscale_image(SkMaskBuilder* mask, SkPDFDocument* doc) {
    sk_sp<SkImage> img;
    SkPixmap pm(SkImageInfo::Make(mask->fBounds.width(), mask->fBounds.height(),
                                  kGray_8_SkColorType, kOpaque_SkAlphaType),
                mask->fImage, mask->fRowBytes);
    constexpr int imgQuality = SK_PDF_MASK_QUALITY;
    if constexpr (imgQuality <= 100 && imgQuality >= 0) {
        SkPDF::EncodeJpegCallback encodeJPEG = doc->metadata().jpegEncoder;
        SkPDF::DecodeJpegCallback decodeJPEG = doc->metadata().jpegDecoder;
        if (encodeJPEG && decodeJPEG) {
            SkDynamicMemoryWStream buffer;
            // By encoding this into jpeg, it be embedded efficiently during drawImage.
            if (encodeJPEG(&buffer, pm, imgQuality)) {
                std::unique_ptr<SkCodec> codec = decodeJPEG(buffer.detachAsData());
                SkASSERT(codec);
                img = SkCodecs::DeferredImage(std::move(codec));
                SkASSERT(img);
                if (img) {
                    SkMaskBuilder::FreeImage(mask->image());
                }
            }
        }
    }
    if (!img) {
        img = SkImages::RasterFromPixmap(
                pm, [](const void* p, void*) { SkMaskBuilder::FreeImage(const_cast<void*>(p)); }, nullptr);
    }
    *mask = SkMaskBuilder();  // destructive;
    return img;
}

sk_sp<SkImage> alpha_image_to_greyscale_image(const SkImage* mask) {
    int w = mask->width(), h = mask->height();
    SkBitmap greyBitmap;
    greyBitmap.allocPixels(SkImageInfo::Make(w, h, kGray_8_SkColorType, kOpaque_SkAlphaType));
    // TODO: support gpu images in pdf
    if (!mask->readPixels(nullptr, SkImageInfo::MakeA8(w, h),
                          greyBitmap.getPixels(), greyBitmap.rowBytes(), 0, 0)) {
        return nullptr;
    }
    greyBitmap.setImmutable();
    return greyBitmap.asImage();
}

static int add_resource(THashSet<SkPDFIndirectReference>& resources, SkPDFIndirectReference ref) {
    resources.add(ref);
    return ref.fValue;
}

static void draw_points(SkCanvas::PointMode mode,
                        SkSpan<const SkPoint> points,
                        const SkPaint& paint,
                        const SkIRect& bounds,
                        SkDevice* device) {
    SkRasterClip rc(bounds);
    SkDraw draw;
    draw.fDst = SkPixmap(SkImageInfo::MakeUnknown(bounds.right(), bounds.bottom()), nullptr, 0);
    draw.fCTM = &device->localToDevice();
    draw.fRC = &rc;
    draw.drawPoints(mode, points, paint, device);
}

static void transform_shader(SkPaint* paint, const SkMatrix& ctm) {
    SkASSERT(!ctm.isIdentity());
#if defined(SK_BUILD_FOR_ANDROID_FRAMEWORK)
    // A shader's matrix is:  CTM x LocalMatrix x WrappingLocalMatrix.  We want to
    // switch to device space, where CTM = I, while keeping the original behavior.
    //
    //               I * LocalMatrix * NewWrappingMatrix = CTM * LocalMatrix
    //                   LocalMatrix * NewWrappingMatrix = CTM * LocalMatrix
    //  InvLocalMatrix * LocalMatrix * NewWrappingMatrix = InvLocalMatrix * CTM * LocalMatrix
    //                                 NewWrappingMatrix = InvLocalMatrix * CTM * LocalMatrix
    //
    SkMatrix lm = SkPDFUtils::GetShaderLocalMatrix(paint->getShader());
    SkMatrix lmInv;
    if (lm.invert(&lmInv)) {
        SkMatrix m = SkMatrix::Concat(SkMatrix::Concat(lmInv, ctm), lm);
        paint->setShader(paint->getShader()->makeWithLocalMatrix(m));
    }
    return;
#endif
    paint->setShader(paint->getShader()->makeWithLocalMatrix(ctm));
}


static SkTCopyOnFirstWrite<SkPaint> clean_paint(const SkPaint& srcPaint) {
    SkTCopyOnFirstWrite<SkPaint> paint(srcPaint);
    // If the paint will definitely draw opaquely, replace kSrc with
    // kSrcOver.  http://crbug.com/473572
    if (!paint->isSrcOver() &&
        SkBlendFastPath::kSrcOver == CheckFastPath(*paint, false))
    {
        paint.writable()->setBlendMode(SkBlendMode::kSrcOver);
    }
    if (paint->getColorFilter()) {
        // We assume here that PDFs all draw in sRGB.
        SkPaintPriv::RemoveColorFilter(paint.writable(), sk_srgb_singleton());
    }
    SkASSERT(!paint->getColorFilter());
    return paint;
}

static void set_style(SkTCopyOnFirstWrite<SkPaint>* paint, SkPaint::Style style) {
    if (paint->get()->getStyle() != style) {
        paint->writable()->setStyle(style);
    }
}

/* Calculate an inverted path's equivalent non-inverted path, given the
 * canvas bounds.
 * outPath may alias with invPath (since this is supported by PathOps).
 */
static bool calculate_inverse_path(const SkRect& bounds, const SkPath& invPath,
                                   SkPath* outPath) {
    SkASSERT(invPath.isInverseFillType());
    return Op(SkPath::Rect(bounds), invPath, kIntersect_SkPathOp, outPath);
}

sk_sp<SkDevice> SkPDFDevice::createDevice(const CreateInfo& cinfo, const SkPaint* layerPaint) {
    // PDF does not support image filters, so render them on CPU.
    // Note that this rendering is done at "screen" resolution (100dpi), not
    // printer resolution.

    // TODO: It may be possible to express some filters natively using PDF
    // to improve quality and file size (skbug.com/40034150)
    if ((layerPaint && (layerPaint->getImageFilter() || layerPaint->getColorFilter()))
        || (cinfo.fInfo.colorSpace() && !cinfo.fInfo.colorSpace()->isSRGB())) {
        // need to return a raster device, which we will detect in drawDevice()
        return SkBitmapDevice::Create(cinfo.fInfo,
                                      SkSurfaceProps());
    }
    return sk_make_sp<SkPDFDevice>(cinfo.fInfo.dimensions(), fDocument);
}

// A helper class to automatically finish a ContentEntry at the end of a
// drawing method and maintain the state needed between set up and finish.
class ScopedContentEntry {
public:
    ScopedContentEntry(SkPDFDevice* device,
                       const SkClipStack* clipStack,
                       const SkMatrix& matrix,
                       const SkPaint& paint,
                       SkScalar textScale = 0)
        : fDevice(device)
        , fBlendMode(SkBlendMode::kSrcOver)
        , fClipStack(clipStack)
    {
        if (matrix.hasPerspective()) {
            NOT_IMPLEMENTED(!matrix.hasPerspective(), false);
            return;
        }
        fBlendMode = paint.getBlendMode_or(SkBlendMode::kSrcOver);
        fContentStream =
            fDevice->setUpContentEntry(clipStack, matrix, paint, textScale, &fDstFormXObject);
    }
    ScopedContentEntry(SkPDFDevice* dev, const SkPaint& paint, SkScalar textScale = 0)
        : ScopedContentEntry(dev, &dev->cs(), dev->localToDevice(), paint, textScale) {}

    ~ScopedContentEntry() {
        if (fContentStream) {
            SkPath* shape = &fShape;
            if (shape->isEmpty()) {
                shape = nullptr;
            }
            fDevice->finishContentEntry(fClipStack, fBlendMode, fDstFormXObject, shape);
        }
    }

    explicit operator bool() const { return fContentStream != nullptr; }
    SkDynamicMemoryWStream* stream() { return fContentStream; }

    /* Returns true when we explicitly need the shape of the drawing. */
    bool needShape() {
        switch (fBlendMode) {
            case SkBlendMode::kClear:
            case SkBlendMode::kSrc:
            case SkBlendMode::kSrcIn:
            case SkBlendMode::kSrcOut:
            case SkBlendMode::kDstIn:
            case SkBlendMode::kDstOut:
            case SkBlendMode::kSrcATop:
            case SkBlendMode::kDstATop:
            case SkBlendMode::kModulate:
                return true;
            default:
                return false;
        }
    }

    /* Returns true unless we only need the shape of the drawing. */
    bool needSource() {
        if (fBlendMode == SkBlendMode::kClear) {
            return false;
        }
        return true;
    }

    /* If the shape is different than the alpha component of the content, then
     * setShape should be called with the shape.  In particular, images and
     * devices have rectangular shape.
     */
    void setShape(const SkPath& shape) {
        fShape = shape;
    }

private:
    SkPDFDevice* fDevice = nullptr;
    SkDynamicMemoryWStream* fContentStream = nullptr;
    SkBlendMode fBlendMode;
    SkPDFIndirectReference fDstFormXObject;
    SkPath fShape;
    const SkClipStack* fClipStack;
};

////////////////////////////////////////////////////////////////////////////////

SkPDFDevice::SkPDFDevice(SkISize pageSize, SkPDFDocument* doc, const SkMatrix& transform)
        : SkClipStackDevice(SkImageInfo::MakeUnknown(pageSize.width(), pageSize.height()),
                            SkSurfaceProps())
        , fInitialTransform(transform)
        , fMarkManager(doc, &fContent)
        , fDocument(doc) {
    SkASSERT(!pageSize.isEmpty());
}

SkPDFDevice::~SkPDFDevice() = default;

void SkPDFDevice::reset() {
    fGraphicStateResources.reset();
    fXObjectResources.reset();
    fShaderResources.reset();
    fFontResources.reset();
    fContent.reset();
    fActiveStackState = SkPDFGraphicStackState();
}

void SkPDFDevice::drawAnnotation(const SkRect& rect, const char key[], SkData* value) {
    if (!value || !fDocument->hasCurrentPage()) {
        return;
    }
    // Annotations are specified in absolute coordinates, so the page xform maps from device space
    // to the global space, and applies the document transform.
    SkMatrix pageXform = this->deviceToGlobal().asM33();
    pageXform.postConcat(fDocument->currentPageTransform());
    if (rect.isEmpty()) {
        if (!strcmp(key, SkPDFGetElemIdKey())) {
            int elemId;
            if (value->size() != sizeof(elemId)) { return; }
            memcpy(&elemId, value->data(), sizeof(elemId));
            fMarkManager.setNextMarksElemId(elemId);
            return;
        }
        if (!strcmp(SkAnnotationKeys::Define_Named_Dest_Key(), key)) {
            SkPoint p = this->localToDevice().mapPoint({rect.x(), rect.y()});
            p = pageXform.mapPoint(p);
            auto pg = fDocument->currentPage();
            fDocument->fNamedDestinations.push_back(SkPDFNamedDestination{sk_ref_sp(value), p, pg});
        }
        return;
    }
    // Convert to path to handle non-90-degree rotations.
    SkPath path = SkPath::Rect(rect).makeTransform(this->localToDevice());
    SkPath clip;
    SkClipStack_AsPath(this->cs(), &clip);
    Op(clip, path, kIntersect_SkPathOp, &path);
    // PDF wants a rectangle only.
    SkRect transformedRect = pageXform.mapRect(path.getBounds());
    if (transformedRect.isEmpty()) {
        return;
    }

    SkPDFLink::Type linkType = SkPDFLink::Type::kNone;
    if (!strcmp(SkAnnotationKeys::URL_Key(), key)) {
        linkType = SkPDFLink::Type::kUrl;
    } else if (!strcmp(SkAnnotationKeys::Link_Named_Dest_Key(), key)) {
        linkType = SkPDFLink::Type::kNamedDestination;
    }

    if (linkType != SkPDFLink::Type::kNone) {
        std::unique_ptr<SkPDFLink> link = std::make_unique<SkPDFLink>(
            linkType, value, transformedRect, fMarkManager.elemId());
        fDocument->fCurrentPageLinks.push_back(std::move(link));
    }
}

void SkPDFDevice::drawPaint(const SkPaint& srcPaint) {
    auto inverse = this->localToDevice().invert();
    if (!inverse) {
        return;
    }
    SkRect bbox = this->cs().bounds(this->bounds());
    inverse->mapRect(&bbox);
    bbox.roundOut(&bbox);
    if (this->hasEmptyClip()) {
        return;
    }
    SkPaint newPaint = srcPaint;
    newPaint.setStyle(SkPaint::kFill_Style);
    this->drawRect(bbox, newPaint);
}

void SkPDFDevice::drawPoints(SkCanvas::PointMode mode,
                             SkSpan<const SkPoint> points,
                             const SkPaint& srcPaint) {
    if (this->hasEmptyClip()) {
        return;
    }
    if (points.size() == 0) {
        return;
    }
    SkTCopyOnFirstWrite<SkPaint> paint(clean_paint(srcPaint));



    if (SkCanvas::kPoints_PointMode != mode) {
        set_style(&paint, SkPaint::kStroke_Style);
    }

    // SkDraw::drawPoints converts to multiple calls to fDevice->drawPath.
    // We only use this when there's a path effect or perspective because of the overhead
    // of multiple calls to setUpContentEntry it causes.
    if (paint->getPathEffect() || this->localToDevice().hasPerspective()) {
        draw_points(mode, points, *paint, this->devClipBounds(), this);
        return;
    }


    if (mode == SkCanvas::kPoints_PointMode && paint->getStrokeCap() != SkPaint::kRound_Cap) {
        if (paint->getStrokeWidth()) {
            // PDF won't draw a single point with square/butt caps because the
            // orientation is ambiguous.  Draw a rectangle instead.
            set_style(&paint, SkPaint::kFill_Style);
            SkScalar strokeWidth = paint->getStrokeWidth();
            SkScalar halfStroke = SkScalarHalf(strokeWidth);
            for (auto&& pt : points) {
                SkRect r = SkRect::MakeXYWH(pt.fX, pt.fY, 0, 0);
                r.inset(-halfStroke, -halfStroke);
                this->drawRect(r, *paint);
            }
            return;
        } else {
            if (paint->getStrokeCap() != SkPaint::kRound_Cap) {
                paint.writable()->setStrokeCap(SkPaint::kRound_Cap);
            }
        }
    }

    ScopedContentEntry content(this, *paint);
    if (!content) {
        return;
    }
    SkDynamicMemoryWStream* contentStream = content.stream();
    fMarkManager.beginMark();
    if (fMarkManager.hasActiveMark()) {
        // Destinations are in absolute coordinates.
        SkMatrix pageXform = this->deviceToGlobal().asM33();
        pageXform.postConcat(fDocument->currentPageTransform());
        // The points do not already have localToDevice applied.
        pageXform.preConcat(this->localToDevice());

        for (auto&& userPoint : points) {
            fMarkManager.accumulate(pageXform.mapPoint(userPoint));
        }
    }
    const size_t count = points.size();
    switch (mode) {
        case SkCanvas::kPolygon_PointMode:
            SkPDFUtils::MoveTo(points[0].fX, points[0].fY, contentStream);
            for (size_t i = 1; i < count; i++) {
                SkPDFUtils::AppendLine(points[i].fX, points[i].fY, contentStream);
            }
            SkPDFUtils::StrokePath(contentStream);
            break;
        case SkCanvas::kLines_PointMode:
            for (size_t i = 0; i < count/2; i++) {
                SkPDFUtils::MoveTo(points[i * 2].fX, points[i * 2].fY, contentStream);
                SkPDFUtils::AppendLine(points[i * 2 + 1].fX, points[i * 2 + 1].fY, contentStream);
                SkPDFUtils::StrokePath(contentStream);
            }
            break;
        case SkCanvas::kPoints_PointMode:
            SkASSERT(paint->getStrokeCap() == SkPaint::kRound_Cap);
            for (size_t i = 0; i < count; i++) {
                SkPDFUtils::MoveTo(points[i].fX, points[i].fY, contentStream);
                SkPDFUtils::ClosePath(contentStream);
                SkPDFUtils::StrokePath(contentStream);
            }
            break;
        default:
            SkASSERT(false);
    }
}

void SkPDFDevice::drawRect(const SkRect& rect, const SkPaint& paint) {
    SkRect r = rect;
    r.sort();
    this->internalDrawPath(this->cs(), this->localToDevice(), SkPath::Rect(r), paint, true);
}

void SkPDFDevice::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
    this->internalDrawPath(this->cs(), this->localToDevice(), SkPath::RRect(rrect), paint, true);
}

void SkPDFDevice::drawOval(const SkRect& oval, const SkPaint& paint) {
    this->internalDrawPath(this->cs(), this->localToDevice(), SkPath::Oval(oval), paint, true);
}

void SkPDFDevice::drawPath(const SkPath& path, const SkPaint& paint, bool pathIsMutable) {
    this->internalDrawPath(this->cs(), this->localToDevice(), path, paint, pathIsMutable);
}

void SkPDFDevice::internalDrawPathWithFilter(const SkClipStack& clipStack,
                                             const SkMatrix& ctm,
                                             const SkPath& origPath,
                                             const SkPaint& origPaint) {
    SkASSERT(origPaint.getMaskFilter());
    SkPathBuilder builder;
    SkTCopyOnFirstWrite<SkPaint> paint(origPaint);

    SkStrokeRec::InitStyle initStyle = skpathutils::FillPathWithPaint(origPath, *paint, &builder)
                                     ? SkStrokeRec::kFill_InitStyle
                                     : SkStrokeRec::kHairline_InitStyle;
    builder.transform(ctm);
    SkPath path = builder.detach();

    SkIRect bounds = clipStack.bounds(this->bounds()).roundOut();
    SkMaskBuilder sourceMask;
    if (!SkDraw::DrawToMask(path, bounds, paint->getMaskFilter(), &SkMatrix::I(),
                            &sourceMask, SkMaskBuilder::kComputeBoundsAndRenderImage_CreateMode,
                            initStyle)) {
        return;
    }
    SkAutoMaskFreeImage srcAutoMaskFreeImage(sourceMask.image());
    SkMaskBuilder dstMask;
    SkIPoint margin;
    if (!as_MFB(paint->getMaskFilter())->filterMask(&dstMask, sourceMask, ctm, &margin)) {
        return;
    }
    SkIRect dstMaskBounds = dstMask.fBounds;
    sk_sp<SkImage> mask = mask_to_greyscale_image(&dstMask, fDocument);
    // PDF doesn't seem to allow masking vector graphics with an Image XObject.
    // Must mask with a Form XObject.
    sk_sp<SkPDFDevice> maskDevice = this->makeCongruentDevice();
    {
        SkCanvas canvas(maskDevice);
        canvas.drawImage(mask, dstMaskBounds.x(), dstMaskBounds.y());
    }
    if (!ctm.isIdentity() && paint->getShader()) {
        transform_shader(paint.writable(), ctm); // Since we are using identity matrix.
    }
    ScopedContentEntry content(this, &clipStack, SkMatrix::I(), *paint);
    if (!content) {
        return;
    }
    this->setGraphicState(SkPDFGraphicState::GetSMaskGraphicState(
            maskDevice->makeFormXObjectFromDevice(dstMaskBounds, true), false,
            SkPDFGraphicState::kLuminosity_SMaskMode, fDocument), content.stream());
    SkPDFUtils::AppendRectangle(SkRect::Make(dstMaskBounds), content.stream());
    SkPDFUtils::PaintPath(SkPaint::kFill_Style, path.getFillType(), content.stream());
    this->clearMaskOnGraphicState(content.stream());
}

void SkPDFDevice::setGraphicState(SkPDFIndirectReference gs, SkDynamicMemoryWStream* content) {
    SkPDFUtils::ApplyGraphicState(add_resource(fGraphicStateResources, gs), content);
}

void SkPDFDevice::clearMaskOnGraphicState(SkDynamicMemoryWStream* contentStream) {
    // The no-softmask graphic state is used to "turn off" the mask for later draw calls.
    SkPDFIndirectReference& noSMaskGS = fDocument->fNoSmaskGraphicState;
    if (!noSMaskGS) {
        SkPDFDict tmp("ExtGState");
        tmp.insertName("SMask", "None");
        noSMaskGS = fDocument->emit(tmp);
    }
    this->setGraphicState(noSMaskGS, contentStream);
}

void SkPDFDevice::internalDrawPath(const SkClipStack& clipStack,
                                   const SkMatrix& ctm,
                                   const SkPath& origPath,
                                   const SkPaint& srcPaint,
                                   bool pathIsMutable) {
    if (clipStack.isEmpty(this->bounds())) {
        return;
    }
    SkTCopyOnFirstWrite<SkPaint> paint(clean_paint(srcPaint));
    SkPath modifiedPath;
    SkPath* pathPtr = const_cast<SkPath*>(&origPath);

    if (paint->getMaskFilter()) {
        this->internalDrawPathWithFilter(clipStack, ctm, origPath, *paint);
        return;
    }

    SkMatrix matrix = ctm;

    if (paint->getPathEffect()) {
        if (clipStack.isEmpty(this->bounds())) {
            return;
        }
        SkPathBuilder builder;
        if (skpathutils::FillPathWithPaint(*pathPtr, *paint, &builder)) {
            set_style(&paint, SkPaint::kFill_Style);
        } else {
            set_style(&paint, SkPaint::kStroke_Style);
            if (paint->getStrokeWidth() != 0) {
                paint.writable()->setStrokeWidth(0);
            }
        }
        modifiedPath = builder.detach();
        pathPtr = &modifiedPath;
        pathIsMutable = true;
        paint.writable()->setPathEffect(nullptr);
    }

    if (this->handleInversePath(*pathPtr, *paint, pathIsMutable)) {
        return;
    }
    if (matrix.getType() & SkMatrix::kPerspective_Mask) {
        if (!pathIsMutable) {
            modifiedPath = origPath;
            pathPtr = &modifiedPath;
            pathIsMutable = true;
        }
        pathPtr->transform(matrix);
        if (paint->getShader()) {
            transform_shader(paint.writable(), matrix);
        }
        matrix = SkMatrix::I();
    }

    ScopedContentEntry content(this, &clipStack, matrix, *paint);
    if (!content) {
        return;
    }
    fMarkManager.beginMark();
    if (fMarkManager.hasActiveMark()) {
        // Destinations are in absolute coordinates.
        SkMatrix pageXform = this->deviceToGlobal().asM33();
        pageXform.postConcat(fDocument->currentPageTransform());
        // The path does not already have localToDevice / ctm / matrix applied.
        pageXform.preConcat(matrix);

        SkRect pathBounds = pathPtr->computeTightBounds();
        pageXform.mapRect(&pathBounds);
        fMarkManager.accumulate({pathBounds.fLeft, pathBounds.fBottom}); // y-up
    }
    constexpr SkScalar kToleranceScale = 0.0625f;  // smaller = better conics (circles).
    SkScalar matrixScale = matrix.mapRadius(1.0f);
    SkScalar tolerance = matrixScale > 0.0f ? kToleranceScale / matrixScale : kToleranceScale;
    bool consumeDegeratePathSegments =
           paint->getStyle() == SkPaint::kFill_Style ||
           (paint->getStrokeCap() != SkPaint::kRound_Cap &&
            paint->getStrokeCap() != SkPaint::kSquare_Cap);
    SkPDFUtils::EmitPath(*pathPtr, paint->getStyle(), consumeDegeratePathSegments, content.stream(),
                         tolerance);
    SkPDFUtils::PaintPath(paint->getStyle(), pathPtr->getFillType(), content.stream());
}

////////////////////////////////////////////////////////////////////////////////

void SkPDFDevice::drawImageRect(const SkImage* image,
                                const SkRect* src,
                                const SkRect& dst,
                                const SkSamplingOptions& sampling,
                                const SkPaint& paint,
                                SkCanvas::SrcRectConstraint) {
    SkASSERT(image);
    this->internalDrawImageRect(SkKeyedImage(sk_ref_sp(const_cast<SkImage*>(image))),
                                src, dst, sampling, paint, this->localToDevice());
}

void SkPDFDevice::drawSprite(const SkBitmap& bm, int x, int y, const SkPaint& paint) {
    SkASSERT(!bm.drawsNothing());
    auto r = SkRect::MakeXYWH(x, y, bm.width(), bm.height());
    this->internalDrawImageRect(SkKeyedImage(bm), nullptr, r, SkSamplingOptions(), paint,
                                SkMatrix::I());
}

////////////////////////////////////////////////////////////////////////////////

namespace {
class GlyphPositioner {
public:
    GlyphPositioner(SkDynamicMemoryWStream* content,
                    SkScalar textSkewX,
                    SkPoint origin)
        : fContent(content)
        , fCurrentMatrixOrigin(origin)
        , fTextSkewX(textSkewX) {
    }
    ~GlyphPositioner() { this->flush(); }
    void flush() {
        if (fInText) {
            fContent->writeText("> Tj\n");
            fInText = false;
        }
    }
    void setFont(SkPDFFont* pdfFont) {
        this->flush();
        fPDFFont = pdfFont;
        // Reader 2020.013.20064 incorrectly advances some Type3 fonts https://crbug.com/1226960
        bool convertedToType3 = fPDFFont->getType() == SkAdvancedTypefaceMetrics::kOther_Font;
        bool thousandEM = fPDFFont->strike().fPath.fUnitsPerEM == 1000;
        fViewersAgreeOnAdvancesInFont = thousandEM || !convertedToType3;
    }
    void writeGlyph(SkGlyphID glyph, SkScalar advanceWidth, SkPoint xy) {
        SkASSERT(fPDFFont);
        if (!fInitialized) {
            // Flip the text about the x-axis to account for origin swap and include
            // the passed parameters.
            fContent->writeText("1 0 ");
            SkPDFUtils::AppendScalar(-fTextSkewX, fContent);
            fContent->writeText(" -1 ");
            SkPDFUtils::AppendScalar(fCurrentMatrixOrigin.x(), fContent);
            fContent->writeText(" ");
            SkPDFUtils::AppendScalar(fCurrentMatrixOrigin.y(), fContent);
            fContent->writeText(" Tm\n");
            fCurrentMatrixOrigin.set(0.0f, 0.0f);
            fInitialized = true;
        }
        SkPoint position = xy - fCurrentMatrixOrigin;
        if (!fViewersAgreeOnXAdvance || position != SkPoint{fXAdvance, 0}) {
            this->flush();
            SkPDFUtils::AppendScalar(position.x() - position.y() * fTextSkewX, fContent);
            fContent->writeText(" ");
            SkPDFUtils::AppendScalar(-position.y(), fContent);
            fContent->writeText(" Td ");
            fCurrentMatrixOrigin = xy;
            fXAdvance = 0;
            fViewersAgreeOnXAdvance = true;
        }
        fXAdvance += advanceWidth;
        if (!fViewersAgreeOnAdvancesInFont) {
            fViewersAgreeOnXAdvance = false;
        }
        if (!fInText) {
            fContent->writeText("<");
            fInText = true;
        }
        if (fPDFFont->multiByteGlyphs()) {
            SkPDFUtils::WriteUInt16BE(fContent, glyph);
        } else {
            SkASSERT(0 == glyph >> 8);
            SkPDFUtils::WriteUInt8(fContent, static_cast<uint8_t>(glyph));
        }
    }

private:
    SkDynamicMemoryWStream* fContent;
    SkPDFFont* fPDFFont = nullptr;
    SkPoint fCurrentMatrixOrigin;
    SkScalar fXAdvance = 0.0f;
    bool fViewersAgreeOnAdvancesInFont = true;
    bool fViewersAgreeOnXAdvance = true;
    SkScalar fTextSkewX;
    bool fInText = false;
    bool fInitialized = false;
};
}  // namespace

namespace {
struct PositionedGlyph {
    SkPoint fPos;
    SkGlyphID fGlyph;
};
}  // namespace

static SkRect get_glyph_bounds_device_space(const SkGlyph* glyph,
                                            SkScalar xScale, SkScalar yScale,
                                            SkPoint xy, const SkMatrix& ctm) {
    SkRect glyphBounds = SkMatrix::Scale(xScale, yScale).mapRect(glyph->rect());
    glyphBounds.offset(xy);
    ctm.mapRect(&glyphBounds); // now in dev space.
    return glyphBounds;
}

static bool contains(const SkRect& r, SkPoint p) {
   return r.left() <= p.x() && p.x() <= r.right() &&
          r.top()  <= p.y() && p.y() <= r.bottom();
}

void SkPDFDevice::drawGlyphRunAsPath(
        const sktext::GlyphRun& glyphRun, SkPoint offset, const SkPaint& runPaint) {
    const SkFont& font = glyphRun.font();
    SkPath path;

    struct Rec {
        SkPath* fPath;
        SkPoint fOffset;
        const SkPoint* fPos;
    } rec = {&path, offset, glyphRun.positions().data()};

    font.getPaths(glyphRun.glyphsIDs(),
                  [](const SkPath* path, const SkMatrix& mx, void* ctx) {
                      Rec* rec = reinterpret_cast<Rec*>(ctx);
                      if (path) {
                          SkMatrix total = mx;
                          total.postTranslate(rec->fPos->fX + rec->fOffset.fX,
                                              rec->fPos->fY + rec->fOffset.fY);
                          rec->fPath->addPath(*path, total);
                      }
                      rec->fPos += 1; // move to the next glyph's position
                  }, &rec);
    this->internalDrawPath(this->cs(), this->localToDevice(), path, runPaint, true);

    SkFont transparentFont = glyphRun.font();
    transparentFont.setEmbolden(false); // Stop Recursion
    sktext::GlyphRun tmpGlyphRun(glyphRun, transparentFont);

    SkPaint transparent;
    transparent.setColor(SK_ColorTRANSPARENT);

    if (this->localToDevice().hasPerspective()) {
        SkAutoDeviceTransformRestore adr(this, SkM44());
        this->internalDrawGlyphRun(tmpGlyphRun, offset, transparent);
    } else {
        this->internalDrawGlyphRun(tmpGlyphRun, offset, transparent);
    }
}

static bool needs_new_font(SkPDFFont* font, const SkGlyph* glyph,
                           SkAdvancedTypefaceMetrics::FontType initialFontType) {
    if (!font || !font->hasGlyph(glyph->getGlyphID())) {
        return true;
    }
    if (initialFontType == SkAdvancedTypefaceMetrics::kOther_Font) {
        return false;
    }
    if (glyph->isEmpty()) {
        return false;
    }

    bool hasUnmodifiedPath = glyph->path() && !glyph->pathIsModified();
    bool convertedToType3 = font->getType() == SkAdvancedTypefaceMetrics::kOther_Font;
    return convertedToType3 == hasUnmodifiedPath;
}

void SkPDFDevice::internalDrawGlyphRun(
        const sktext::GlyphRun& glyphRun, SkPoint offset, const SkPaint& runPaint) {

    const SkGlyphID* glyphIDs = glyphRun.glyphsIDs().data();
    uint32_t glyphCount = SkToU32(glyphRun.glyphsIDs().size());
    const SkFont& glyphRunFont = glyphRun.font();

    if (!glyphCount || !glyphIDs || glyphRunFont.getSize() <= 0 || this->hasEmptyClip()) {
        return;
    }

    // TODO: SkPDFFont has code to handle paints with mask filters, but the viewers do not.
    // See https://crbug.com/362796158 for Pdfium and b/325266484 for Preview
    if (this->localToDevice().hasPerspective() || runPaint.getMaskFilter()) {
        this->drawGlyphRunAsPath(glyphRun, offset, runPaint);
        return;
    }

    sk_sp<SkPDFStrike> pdfStrike = SkPDFStrike::Make(fDocument, glyphRunFont, runPaint);
    if (!pdfStrike) {
        return;
    }
    const SkTypeface& typeface = pdfStrike->fPath.fStrikeSpec.typeface();

    const SkAdvancedTypefaceMetrics* metrics = SkPDFFont::GetMetrics(typeface, fDocument);
    if (!metrics) {
        return;
    }

    const std::vector<SkUnichar>& glyphToUnicode = SkPDFFont::GetUnicodeMap(typeface, fDocument);
    THashMap<SkGlyphID, SkString>& glyphToUnicodeEx=SkPDFFont::GetUnicodeMapEx(typeface, fDocument);

    // TODO: FontType should probably be on SkPDFStrike?
    SkAdvancedTypefaceMetrics::FontType initialFontType = SkPDFFont::FontType(*pdfStrike, *metrics);

    SkClusterator clusterator(glyphRun);

    // The size, skewX, and scaleX are applied here.
    SkScalar textSize = glyphRunFont.getSize();
    SkScalar advanceScale = textSize * glyphRunFont.getScaleX() / pdfStrike->fPath.fUnitsPerEM;

    // textScaleX and textScaleY are used to get a conservative bounding box for glyphs.
    SkScalar textScaleY = textSize / pdfStrike->fPath.fUnitsPerEM;
    SkScalar textScaleX = advanceScale + glyphRunFont.getSkewX() * textScaleY;

    SkRect clipStackBounds = this->cs().bounds(this->bounds());

    // Clear everything from the runPaint that will be applied by the strike.
    SkPaint fillPaint(runPaint);
    if (fillPaint.getStrokeWidth() > 0) {
        fillPaint.setStroke(false);
    }
    fillPaint.setPathEffect(nullptr);
    fillPaint.setMaskFilter(nullptr);
    SkTCopyOnFirstWrite<SkPaint> paint(clean_paint(fillPaint));
    ScopedContentEntry content(this, *paint, glyphRunFont.getScaleX());
    if (!content) {
        return;
    }
    SkDynamicMemoryWStream* out = content.stream();

    // Destinations are in absolute coordinates.
    // The glyphs bounds go through the localToDevice separately for clipping.
    SkMatrix pageXform = this->deviceToGlobal().asM33();
    pageXform.postConcat(fDocument->currentPageTransform());

    fMarkManager.beginMark();
    if (!glyphRun.text().empty()) {
        fDocument->addStructElemTitle(fMarkManager.elemId(), glyphRun.text());
    }

    out->writeText("BT\n");
    SK_AT_SCOPE_EXIT(out->writeText("ET\n"));

    const int numGlyphs = typeface.countGlyphs();

    if (clusterator.reversedChars()) {
        out->writeText("/ReversedChars BMC\n");
    }
    SK_AT_SCOPE_EXIT(if (clusterator.reversedChars()) { out->writeText("EMC\n"); } );
    GlyphPositioner glyphPositioner(out, glyphRunFont.getSkewX(), offset);
    SkPDFFont* font = nullptr;

    SkBulkGlyphMetricsAndPaths paths{pdfStrike->fPath.fStrikeSpec};
    auto glyphs = paths.glyphs(glyphRun.glyphsIDs());

    while (SkClusterator::Cluster c = clusterator.next()) {
        int glyphIndex = c.fGlyphIndex;
        int glyphLimit = glyphIndex + c.fGlyphCount;

        bool actualText = false;
        SK_AT_SCOPE_EXIT(if (actualText) {
                             glyphPositioner.flush();
                             out->writeText("EMC\n");
                         });
        if (c.fUtf8Text) {
            bool toUnicode = false;
            const char* textPtr = c.fUtf8Text;
            const char* textEnd = c.fUtf8Text + c.fTextByteLength;
            SkUnichar clusterUnichar = SkUTF::NextUTF8(&textPtr, textEnd);
            // ToUnicode can only handle one glyph in a cluster.
            if (clusterUnichar >= 0 && c.fGlyphCount == 1) {
                SkGlyphID gid = glyphIDs[glyphIndex];
                SkUnichar fontUnichar = gid < glyphToUnicode.size() ? glyphToUnicode[gid] : 0;

                // The regular cmap can handle this if there is one glyph in the cluster,
                // one code point in the cluster, and the glyph maps to the code point.
                toUnicode = textPtr == textEnd && clusterUnichar == fontUnichar;

                // The extended cmap can handle this if there is one glyph in the cluster,
                // the font has no code point for the glyph,
                // there are less than 512 bytes in the UTF-16,
                // and the mapping matches or can be added.
                // UTF-16 uses at most 2x space of UTF-8; 64 code points seems enough.
                if (!toUnicode && fontUnichar <= 0 && c.fTextByteLength < 256) {
                    SkString* unicodes = glyphToUnicodeEx.find(gid);
                    if (!unicodes) {
                        glyphToUnicodeEx.set(gid, SkString(c.fUtf8Text, c.fTextByteLength));
                        toUnicode = true;
                    } else if (unicodes->equals(c.fUtf8Text, c.fTextByteLength)) {
                        toUnicode = true;
                    }
                }
            }
            if (!toUnicode) {
                glyphPositioner.flush();
                // Begin marked-content sequence with associated property list.
                out->writeText("/Span<</ActualText ");
                SkPDFWriteTextString(out, c.fUtf8Text, c.fTextByteLength);
                out->writeText(" >> BDC\n");
                actualText = true;
            }
        }
        for (; glyphIndex < glyphLimit; ++glyphIndex) {
            SkGlyphID gid = glyphIDs[glyphIndex];
            if (numGlyphs <= gid) {
                continue;
            }
            SkPoint xy = glyphRun.positions()[glyphIndex];
            // Do a glyph-by-glyph bounds-reject if positions are absolute.
            SkRect glyphBounds = get_glyph_bounds_device_space(
                    glyphs[glyphIndex], textScaleX, textScaleY,
                    xy + offset, this->localToDevice());
            if (glyphBounds.isEmpty()) {
                if (!contains(clipStackBounds, {glyphBounds.x(), glyphBounds.y()})) {
                    continue;
                }
            } else {
                if (!clipStackBounds.intersects(glyphBounds)) {
                    continue;  // reject glyphs as out of bounds
                }
            }
            if (needs_new_font(font, glyphs[glyphIndex], initialFontType)) {
                // Not yet specified font or need to switch font.
                font = pdfStrike->getFontResource(glyphs[glyphIndex]);
                SkASSERT(font);  // All preconditions for SkPDFFont::GetFontResource are met.
                glyphPositioner.setFont(font);
                SkPDFWriteResourceName(out, SkPDFResourceType::kFont,
                                       add_resource(fFontResources, font->indirectReference()));
                out->writeText(" ");
                SkPDFUtils::AppendScalar(textSize, out);
                out->writeText(" Tf\n");

            }
            font->noteGlyphUsage(gid);
            SkGlyphID encodedGlyph = font->glyphToPDFFontEncoding(gid);
            SkScalar advance = advanceScale * glyphs[glyphIndex]->advanceX();
            if (fMarkManager.hasActiveMark()) {
                SkRect pageGlyphBounds = pageXform.mapRect(glyphBounds);
                fMarkManager.accumulate({pageGlyphBounds.fLeft, pageGlyphBounds.fBottom}); // y-up
            }
            glyphPositioner.writeGlyph(encodedGlyph, advance, xy);
        }
    }
}

void SkPDFDevice::onDrawGlyphRunList(SkCanvas*,
                                     const sktext::GlyphRunList& glyphRunList,
                                     const SkPaint& paint) {
    SkASSERT(!glyphRunList.hasRSXForm());
    for (const sktext::GlyphRun& glyphRun : glyphRunList) {
        this->internalDrawGlyphRun(glyphRun, glyphRunList.origin(), paint);
    }
}

void SkPDFDevice::drawVertices(const SkVertices*, sk_sp<SkBlender>, const SkPaint&, bool) {
    if (this->hasEmptyClip()) {
        return;
    }
    // TODO: implement drawVertices
}

void SkPDFDevice::drawMesh(const SkMesh&, sk_sp<SkBlender>, const SkPaint&) {
    if (this->hasEmptyClip()) {
        return;
    }
    // TODO: implement drawMesh
}

void SkPDFDevice::drawFormXObject(SkPDFIndirectReference xObject, SkDynamicMemoryWStream* content,
                                  SkPath* shape) {
    fMarkManager.beginMark();
    if (fMarkManager.hasActiveMark() && shape) {
        // Destinations are in absolute coordinates.
        SkMatrix pageXform = this->deviceToGlobal().asM33();
        pageXform.postConcat(fDocument->currentPageTransform());
        // The shape already has localToDevice applied.

        SkRect shapeBounds = shape->computeTightBounds();
        pageXform.mapRect(&shapeBounds);
        fMarkManager.accumulate({shapeBounds.fLeft, shapeBounds.fBottom}); // y-up
    }

    SkASSERT(xObject);
    SkPDFWriteResourceName(content, SkPDFResourceType::kXObject,
                           add_resource(fXObjectResources, xObject));
    content->writeText(" Do\n");
}

sk_sp<SkSurface> SkPDFDevice::makeSurface(const SkImageInfo& info, const SkSurfaceProps& props) {
    return SkSurfaces::Raster(info, &props);
}

static std::vector<SkPDFIndirectReference> sort(const THashSet<SkPDFIndirectReference>& src) {
    std::vector<SkPDFIndirectReference> dst;
    dst.reserve(src.count());
    for (SkPDFIndirectReference ref : src) {
        dst.push_back(ref);
    }
    std::sort(dst.begin(), dst.end(),
            [](SkPDFIndirectReference a, SkPDFIndirectReference b) { return a.fValue < b.fValue; });
    return dst;
}

std::unique_ptr<SkPDFDict> SkPDFDevice::makeResourceDict() {
    return SkPDFMakeResourceDict(sort(fGraphicStateResources),
                                 sort(fShaderResources),
                                 sort(fXObjectResources),
                                 sort(fFontResources));
}

std::unique_ptr<SkStreamAsset> SkPDFDevice::content() {
    if (fActiveStackState.fContentStream) {
        fActiveStackState.drainStack();
        fActiveStackState = SkPDFGraphicStackState();
    }
    if (fContent.bytesWritten() == 0) {
        return std::make_unique<SkMemoryStream>();
    }

    // Implicitly close any still active marked-content sequence.
    // Must do this before fContent is written to buffer.
    fMarkManager.setNextMarksElemId(0);
    fMarkManager.beginMark();

    SkDynamicMemoryWStream buffer;
    if (fInitialTransform.getType() != SkMatrix::kIdentity_Mask) {
        SkPDFUtils::AppendTransform(fInitialTransform, &buffer);
    }
    if (fNeedsExtraSave) {
        buffer.writeText("q\n");
    }
    fContent.writeToAndReset(&buffer);
    if (fNeedsExtraSave) {
        buffer.writeText("Q\n");
    }
    fNeedsExtraSave = false;
    return std::unique_ptr<SkStreamAsset>(buffer.detachAsStream());
}

/* Draws an inverse filled path by using Path Ops to compute the positive
 * inverse using the current clip as the inverse bounds.
 * Return true if this was an inverse path and was properly handled,
 * otherwise returns false and the normal drawing routine should continue,
 * either as a (incorrect) fallback or because the path was not inverse
 * in the first place.
 */
bool SkPDFDevice::handleInversePath(const SkPath& origPath,
                                    const SkPaint& paint,
                                    bool pathIsMutable) {
    if (!origPath.isInverseFillType()) {
        return false;
    }

    if (this->hasEmptyClip()) {
        return false;
    }

    SkPath modifiedPath;
    SkPath* pathPtr = const_cast<SkPath*>(&origPath);
    SkPaint noInversePaint(paint);

    // Merge stroking operations into final path.
    if (SkPaint::kStroke_Style == paint.getStyle() ||
        SkPaint::kStrokeAndFill_Style == paint.getStyle()) {
        SkPathBuilder builder;
        bool doFillPath = skpathutils::FillPathWithPaint(origPath, paint, &builder);
        modifiedPath = builder.detach();
        pathPtr = &modifiedPath;

        if (doFillPath) {
            noInversePaint.setStyle(SkPaint::kFill_Style);
            noInversePaint.setStrokeWidth(0);
        } else {
            // To be consistent with the raster output, hairline strokes
            // are rendered as non-inverted.
            modifiedPath.toggleInverseFillType();
            this->internalDrawPath(this->cs(), this->localToDevice(), modifiedPath, paint, true);
            return true;
        }
    }

    // Get bounds of clip in current transform space
    // (clip bounds are given in device space).
    auto transformInverse = this->localToDevice().invert();
    if (!transformInverse) {
        return false;
    }
    SkRect bounds = this->cs().bounds(this->bounds());
    transformInverse->mapRect(&bounds);

    // Extend the bounds by the line width (plus some padding)
    // so the edge doesn't cause a visible stroke.
    bounds.outset(paint.getStrokeWidth() + SK_Scalar1,
                  paint.getStrokeWidth() + SK_Scalar1);

    if (!calculate_inverse_path(bounds, *pathPtr, &modifiedPath)) {
        return false;
    }

    this->internalDrawPath(this->cs(), this->localToDevice(), modifiedPath, noInversePaint, true);
    return true;
}

SkPDFIndirectReference SkPDFDevice::makeFormXObjectFromDevice(SkIRect bounds, bool alpha) {
    SkMatrix inverseTransform = SkMatrix::I();
    if (auto inv = fInitialTransform.invert()) {
        inverseTransform = *inv;
    } else {
        SkDEBUGFAIL("Layer initial transform should be invertible.");
    }
    const char* colorSpace = alpha ? "DeviceGray" : nullptr;

    SkPDFIndirectReference xobject =
        SkPDFMakeFormXObject(fDocument, this->content(),
                             SkPDFMakeArray(bounds.left(), bounds.top(),
                                            bounds.right(), bounds.bottom()),
                             this->makeResourceDict(), inverseTransform, colorSpace);
    // We always draw the form xobjects that we create back into the device, so
    // we simply preserve the font usage instead of pulling it out and merging
    // it back in later.
    this->reset();
    return xobject;
}

SkPDFIndirectReference SkPDFDevice::makeFormXObjectFromDevice(bool alpha) {
    return this->makeFormXObjectFromDevice(SkIRect{0, 0, this->width(), this->height()}, alpha);
}

void SkPDFDevice::drawFormXObjectWithMask(SkPDFIndirectReference xObject,
                                          SkPDFIndirectReference sMask,
                                          SkBlendMode mode,
                                          bool invertClip) {
    SkASSERT(sMask);
    SkPaint paint;
    paint.setBlendMode(mode);
    ScopedContentEntry content(this, nullptr, SkMatrix::I(), paint);
    if (!content) {
        return;
    }
    this->setGraphicState(SkPDFGraphicState::GetSMaskGraphicState(
            sMask, invertClip, SkPDFGraphicState::kAlpha_SMaskMode,
            fDocument), content.stream());
    this->drawFormXObject(xObject, content.stream(), nullptr);
    this->clearMaskOnGraphicState(content.stream());
}


static bool treat_as_regular_pdf_blend_mode(SkBlendMode blendMode) {
    return nullptr != SkPDFUtils::BlendModeName(blendMode);
}

static void populate_graphic_state_entry_from_paint(
        SkPDFDocument* doc,
        const SkMatrix& matrix,
        const SkClipStack* clipStack,
        SkIRect deviceBounds,
        const SkPaint& paint,
        const SkMatrix& initialTransform,
        SkScalar textScale,
        SkPDFGraphicStackState::Entry* entry,
        THashSet<SkPDFIndirectReference>* shaderResources,
        THashSet<SkPDFIndirectReference>* graphicStateResources) {
    NOT_IMPLEMENTED(paint.getPathEffect() != nullptr, false);
    NOT_IMPLEMENTED(paint.getMaskFilter() != nullptr, false);
    NOT_IMPLEMENTED(paint.getColorFilter() != nullptr, false);

    entry->fMatrix = matrix;
    entry->fClipStackGenID = clipStack ? clipStack->getTopmostGenID()
                                       : SkClipStack::kWideOpenGenID;
    SkColor4f color = paint.getColor4f();
    entry->fColor = {color.fR, color.fG, color.fB, 1};
    entry->fShaderIndex = -1;

    // PDF treats a shader as a color, so we only set one or the other.
    SkShader* shader = paint.getShader();
    if (shader) {
        if (as_SB(shader)->type() == SkShaderBase::ShaderType::kColor) {
            auto colorShader = static_cast<SkColorShader*>(shader);
            // We don't have to set a shader just for a color.
            color = colorShader->color();
            color.fA *= paint.getAlphaf();
            entry->fColor = colorShader->color().makeOpaque();
        } else {
            // PDF positions patterns relative to the initial transform, so
            // we need to apply the current transform to the shader parameters.
            SkMatrix transform = matrix;
            transform.postConcat(initialTransform);

            // PDF doesn't support kClamp_TileMode, so we simulate it by making
            // a pattern the size of the current clip.
            SkRect clipStackBounds = clipStack ? clipStack->bounds(deviceBounds)
                                               : SkRect::Make(deviceBounds);

            // We need to apply the initial transform to bounds in order to get
            // bounds in a consistent coordinate system.
            initialTransform.mapRect(&clipStackBounds);
            SkIRect bounds;
            clipStackBounds.roundOut(&bounds);

            // Use alpha 1 for the shader, the paint alpha is applied with newGraphicsState (below)
            auto c = paint.getColor4f();
            SkPDFIndirectReference pdfShader = SkPDFMakeShader(doc, shader, transform, bounds,
                                                               {c.fR, c.fG, c.fB, 1.0f});

            if (pdfShader) {
                // pdfShader has been canonicalized so we can directly compare pointers.
                entry->fShaderIndex = add_resource(*shaderResources, pdfShader);
            }
        }
    }

    SkPDFIndirectReference newGraphicState;
    if (color == paint.getColor4f()) {
        newGraphicState = SkPDFGraphicState::GetGraphicStateForPaint(doc, paint);
    } else {
        SkPaint newPaint = paint;
        newPaint.setColor4f(color, nullptr);
        newGraphicState = SkPDFGraphicState::GetGraphicStateForPaint(doc, newPaint);
    }
    entry->fGraphicStateIndex = add_resource(*graphicStateResources, newGraphicState);
    entry->fTextScaleX = textScale;
}

SkDynamicMemoryWStream* SkPDFDevice::setUpContentEntry(const SkClipStack* clipStack,
                                                       const SkMatrix& matrix,
                                                       const SkPaint& paint,
                                                       SkScalar textScale,
                                                       SkPDFIndirectReference* dst) {
    SkASSERT(!*dst);
    SkBlendMode blendMode = paint.getBlendMode_or(SkBlendMode::kSrcOver);

    // Dst xfer mode doesn't draw source at all.
    if (blendMode == SkBlendMode::kDst) {
        return nullptr;
    }

    // For the following modes, we want to handle source and destination
    // separately, so make an object of what's already there.
    if (!treat_as_regular_pdf_blend_mode(blendMode) && blendMode != SkBlendMode::kDstOver) {
        if (!isContentEmpty()) {
            *dst = this->makeFormXObjectFromDevice();
            SkASSERT(isContentEmpty());
        } else if (blendMode != SkBlendMode::kSrc &&
                   blendMode != SkBlendMode::kSrcOut) {
            // Except for Src and SrcOut, if there isn't anything already there,
            // then we're done.
            return nullptr;
        }
    }
    // TODO(vandebo): Figure out how/if we can handle the following modes:
    // Xor, Plus.  For now, we treat them as SrcOver/Normal.

    if (treat_as_regular_pdf_blend_mode(blendMode)) {
        if (!fActiveStackState.fContentStream) {
            if (fContent.bytesWritten() != 0) {
                fContent.writeText("Q\nq\n");
                fNeedsExtraSave = true;
            }
            fActiveStackState = SkPDFGraphicStackState(&fContent);
        } else {
            SkASSERT(fActiveStackState.fContentStream = &fContent);
        }
    } else {
        fActiveStackState.drainStack();
        fActiveStackState = SkPDFGraphicStackState(&fContentBuffer);
    }
    SkASSERT(fActiveStackState.fContentStream);
    SkPDFGraphicStackState::Entry entry;
    populate_graphic_state_entry_from_paint(
            fDocument,
            matrix,
            clipStack,
            this->bounds(),
            paint,
            fInitialTransform,
            textScale,
            &entry,
            &fShaderResources,
            &fGraphicStateResources);
    fActiveStackState.updateClip(clipStack, this->bounds());
    fActiveStackState.updateMatrix(entry.fMatrix);
    fActiveStackState.updateDrawingState(entry);

    return fActiveStackState.fContentStream;
}

void SkPDFDevice::finishContentEntry(const SkClipStack* clipStack,
                                     SkBlendMode blendMode,
                                     SkPDFIndirectReference dst,
                                     SkPath* shape) {
    SkASSERT(blendMode != SkBlendMode::kDst);
    if (treat_as_regular_pdf_blend_mode(blendMode)) {
        SkASSERT(!dst);
        return;
    }

    SkASSERT(fActiveStackState.fContentStream);

    fActiveStackState.drainStack();
    fActiveStackState = SkPDFGraphicStackState();

    if (blendMode == SkBlendMode::kDstOver) {
        SkASSERT(!dst);
        if (fContentBuffer.bytesWritten() != 0) {
            if (fContent.bytesWritten() != 0) {
                fContentBuffer.writeText("Q\nq\n");
                fNeedsExtraSave = true;
            }
            fContentBuffer.prependToAndReset(&fContent);
            SkASSERT(fContentBuffer.bytesWritten() == 0);
        }
        return;
    }
    if (fContentBuffer.bytesWritten() != 0) {
        if (fContent.bytesWritten() != 0) {
            fContent.writeText("Q\nq\n");
            fNeedsExtraSave = true;
        }
        fContentBuffer.writeToAndReset(&fContent);
        SkASSERT(fContentBuffer.bytesWritten() == 0);
    }

    if (!dst) {
        SkASSERT(blendMode == SkBlendMode::kSrc ||
                 blendMode == SkBlendMode::kSrcOut);
        return;
    }

    SkASSERT(dst);
    // Changing the current content into a form-xobject will destroy the clip
    // objects which is fine since the xobject will already be clipped. However
    // if source has shape, we need to clip it too, so a copy of the clip is
    // saved.

    SkPaint stockPaint;

    SkPDFIndirectReference srcFormXObject;
    if (this->isContentEmpty()) {
        // If nothing was drawn and there's no shape, then the draw was a
        // no-op, but dst needs to be restored for that to be true.
        // If there is shape, then an empty source with Src, SrcIn, SrcOut,
        // DstIn, DstAtop or Modulate reduces to Clear and DstOut or SrcAtop
        // reduces to Dst.
        if (shape == nullptr || blendMode == SkBlendMode::kDstOut ||
                blendMode == SkBlendMode::kSrcATop) {
            ScopedContentEntry content(this, nullptr, SkMatrix::I(), stockPaint);
            this->drawFormXObject(dst, content.stream(), nullptr);
            return;
        } else {
            blendMode = SkBlendMode::kClear;
        }
    } else {
        srcFormXObject = this->makeFormXObjectFromDevice();
    }

    // TODO(vandebo) srcFormXObject may contain alpha, but here we want it
    // without alpha.
    if (blendMode == SkBlendMode::kSrcATop) {
        // TODO(vandebo): In order to properly support SrcATop we have to track
        // the shape of what's been drawn at all times. It's the intersection of
        // the non-transparent parts of the device and the outlines (shape) of
        // all images and devices drawn.
        this->drawFormXObjectWithMask(srcFormXObject, dst, SkBlendMode::kSrcOver, true);
    } else {
        if (shape != nullptr) {
            // Draw shape into a form-xobject.
            SkPaint filledPaint;
            filledPaint.setColor(SK_ColorBLACK);
            filledPaint.setStyle(SkPaint::kFill_Style);
            SkClipStack empty;
            SkPDFDevice shapeDev(this->size(), fDocument, fInitialTransform);
            shapeDev.internalDrawPath(clipStack ? *clipStack : empty,
                                      SkMatrix::I(), *shape, filledPaint, true);
            this->drawFormXObjectWithMask(dst, shapeDev.makeFormXObjectFromDevice(),
                                          SkBlendMode::kSrcOver, true);
        } else {
            this->drawFormXObjectWithMask(dst, srcFormXObject, SkBlendMode::kSrcOver, true);
        }
    }

    if (blendMode == SkBlendMode::kClear) {
        return;
    } else if (blendMode == SkBlendMode::kSrc ||
            blendMode == SkBlendMode::kDstATop) {
        ScopedContentEntry content(this, nullptr, SkMatrix::I(), stockPaint);
        if (content) {
            this->drawFormXObject(srcFormXObject, content.stream(), nullptr);
        }
        if (blendMode == SkBlendMode::kSrc) {
            return;
        }
    } else if (blendMode == SkBlendMode::kSrcATop) {
        ScopedContentEntry content(this, nullptr, SkMatrix::I(), stockPaint);
        if (content) {
            this->drawFormXObject(dst, content.stream(), nullptr);
        }
    }

    SkASSERT(blendMode == SkBlendMode::kSrcIn   ||
             blendMode == SkBlendMode::kDstIn   ||
             blendMode == SkBlendMode::kSrcOut  ||
             blendMode == SkBlendMode::kDstOut  ||
             blendMode == SkBlendMode::kSrcATop ||
             blendMode == SkBlendMode::kDstATop ||
             blendMode == SkBlendMode::kModulate);

    if (blendMode == SkBlendMode::kSrcIn ||
            blendMode == SkBlendMode::kSrcOut ||
            blendMode == SkBlendMode::kSrcATop) {
        this->drawFormXObjectWithMask(srcFormXObject, dst, SkBlendMode::kSrcOver,
                                      blendMode == SkBlendMode::kSrcOut);
        return;
    } else {
        SkBlendMode mode = SkBlendMode::kSrcOver;
        if (blendMode == SkBlendMode::kModulate) {
            this->drawFormXObjectWithMask(srcFormXObject, dst, SkBlendMode::kSrcOver, false);
            mode = SkBlendMode::kMultiply;
        }
        this->drawFormXObjectWithMask(dst, srcFormXObject, mode, blendMode == SkBlendMode::kDstOut);
        return;
    }
}

bool SkPDFDevice::isContentEmpty() {
    return fContent.bytesWritten() == 0 && fContentBuffer.bytesWritten() == 0;
}

static SkSize rect_to_size(const SkRect& r) { return {r.width(), r.height()}; }

static sk_sp<SkImage> color_filter(const SkImage* image,
                                   SkColorFilter* colorFilter) {
    auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(image->dimensions()));
    SkASSERT(surface);
    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);
    SkPaint paint;
    paint.setColorFilter(sk_ref_sp(colorFilter));
    canvas->drawImage(image, 0, 0, SkSamplingOptions(), &paint);
    return surface->makeImageSnapshot();
}

////////////////////////////////////////////////////////////////////////////////

static bool is_integer(SkScalar x) {
    return x == SkScalarTruncToScalar(x);
}

static bool is_integral(const SkRect& r) {
    return is_integer(r.left()) &&
           is_integer(r.top()) &&
           is_integer(r.right()) &&
           is_integer(r.bottom());
}

void SkPDFDevice::internalDrawImageRect(SkKeyedImage imageSubset,
                                        const SkRect* src,
                                        const SkRect& dst,
                                        const SkSamplingOptions& sampling,
                                        const SkPaint& srcPaint,
                                        const SkMatrix& ctm) {
    if (this->hasEmptyClip()) {
        return;
    }
    if (!imageSubset) {
        return;
    }

    SkKeyedImage originalImage = imageSubset;
    bool canUseOriginal = true;
    bool didSubset = false;

    // First, figure out the src->dst transform and subset the image if needed.
    SkIRect bounds = imageSubset.image()->bounds();
    SkRect srcRect = src ? *src : SkRect::Make(bounds);
    SkMatrix transform = SkMatrix::RectToRect(srcRect, dst);
    SkMatrix originalTransform = transform;
    if (src && *src != SkRect::Make(bounds)) {
        if (!srcRect.intersect(SkRect::Make(bounds))) {
            return;
        }
        srcRect.roundOut(&bounds);
        transform.preTranslate(SkIntToScalar(bounds.x()),
                               SkIntToScalar(bounds.y()));
        if (bounds != imageSubset.image()->bounds()) {
            imageSubset = imageSubset.subset(bounds);
            didSubset = true;
        }
        if (!imageSubset) {
            return;
        }
    }

    // If the image is opaque and the paint's alpha is too, replace
    // kSrc blendmode with kSrcOver.  http://crbug.com/473572
    SkTCopyOnFirstWrite<SkPaint> paint(srcPaint);
    if (!paint->isSrcOver() &&
        imageSubset.image()->isOpaque() &&
        SkBlendFastPath::kSrcOver == CheckFastPath(*paint, false))
    {
        paint.writable()->setBlendMode(SkBlendMode::kSrcOver);
    }

    // Alpha-only images need to get their color from the shader, before
    // applying the colorfilter.
    if (imageSubset.image()->isAlphaOnly() && paint->getColorFilter()) {
        // must blend alpha image and shader before applying colorfilter.
        auto surface =
                SkSurfaces::Raster(SkImageInfo::MakeN32Premul(imageSubset.image()->dimensions()));
        SkCanvas* canvas = surface->getCanvas();
        SkPaint tmpPaint;
        // In the case of alpha images with shaders, the shader's coordinate
        // system is the image's coordiantes.
        tmpPaint.setShader(sk_ref_sp(paint->getShader()));
        tmpPaint.setColor4f(paint->getColor4f(), nullptr);
        canvas->clear(0x00000000);
        canvas->drawImage(imageSubset.image().get(), 0, 0, sampling, &tmpPaint);
        if (paint->getShader() != nullptr) {
            paint.writable()->setShader(nullptr);
        }
        imageSubset = SkKeyedImage(surface->makeImageSnapshot());
        canUseOriginal = false;
        SkASSERT(!imageSubset.image()->isAlphaOnly());
    }

    if (imageSubset.image()->isAlphaOnly()) {
        // The ColorFilter applies to the paint color/shader, not the alpha layer.
        SkASSERT(nullptr == paint->getColorFilter());

        sk_sp<SkImage> mask = alpha_image_to_greyscale_image(imageSubset.image().get());
        if (!mask) {
            return;
        }
        // PDF doesn't seem to allow masking vector graphics with an Image XObject.
        // Must mask with a Form XObject.
        sk_sp<SkPDFDevice> maskDevice = this->makeCongruentDevice();
        {
            SkCanvas canvas(maskDevice);
            // This clip prevents the mask image shader from covering
            // entire device if unnecessary.
            canvas.clipRect(this->cs().bounds(this->bounds()));
            canvas.concat(ctm);
            if (paint->getMaskFilter()) {
                SkPaint tmpPaint;
                tmpPaint.setShader(mask->makeShader(SkSamplingOptions(), transform));
                tmpPaint.setMaskFilter(sk_ref_sp(paint->getMaskFilter()));
                canvas.drawRect(dst, tmpPaint);
            } else {
                if (src && !is_integral(*src)) {
                    canvas.clipRect(dst);
                }
                canvas.concat(transform);
                canvas.drawImage(mask, 0, 0);
            }
        }
        SkIRect maskDeviceBounds = maskDevice->cs().bounds(maskDevice->bounds()).roundOut();
        if (!ctm.isIdentity() && paint->getShader()) {
            transform_shader(paint.writable(), ctm); // Since we are using identity matrix.
        }
        ScopedContentEntry content(this, &this->cs(), SkMatrix::I(), *paint);
        if (!content) {
            return;
        }
        this->setGraphicState(SkPDFGraphicState::GetSMaskGraphicState(
                maskDevice->makeFormXObjectFromDevice(maskDeviceBounds, true), false,
                SkPDFGraphicState::kLuminosity_SMaskMode, fDocument), content.stream());
        SkPDFUtils::AppendRectangle(SkRect::Make(this->size()), content.stream());
        SkPDFUtils::PaintPath(SkPaint::kFill_Style, SkPathFillType::kWinding, content.stream());
        this->clearMaskOnGraphicState(content.stream());
        return;
    }
    if (paint->getMaskFilter()) {
        paint.writable()->setShader(imageSubset.image()->makeShader(SkSamplingOptions(),
                                                                    transform));
        SkPath path = SkPath::Rect(dst); // handles non-integral clipping.
        this->internalDrawPath(this->cs(), this->localToDevice(), path, *paint, true);
        return;
    }
    transform.postConcat(ctm);
    originalTransform.postConcat(ctm);

    SkMatrix matrix = transform;

    // Rasterize the bitmap using perspective in a new bitmap.
    if (transform.hasPerspective()) {
        // Transform the bitmap in the new space, without taking into
        // account the initial transform.
        SkRect imageBounds = SkRect::Make(imageSubset.image()->bounds());
        SkPath perspectiveOutline = SkPath::Rect(imageBounds).makeTransform(transform);

        // Retrieve the bounds of the new shape.
        SkRect outlineBounds = perspectiveOutline.getBounds();
        if (!outlineBounds.intersect(SkRect::Make(this->devClipBounds()))) {
            return;
        }

        // Transform the bitmap in the new space to the final space, to account for DPI
        SkRect physicalBounds = fInitialTransform.mapRect(outlineBounds);
        SkScalar scaleX = physicalBounds.width() / outlineBounds.width();
        SkScalar scaleY = physicalBounds.height() / outlineBounds.height();

        // TODO(edisonn): A better approach would be to use a bitmap shader
        // (in clamp mode) and draw a rect over the entire bounding box. Then
        // intersect perspectiveOutline to the clip. That will avoid introducing
        // alpha to the image while still giving good behavior at the edge of
        // the image.  Avoiding alpha will reduce the pdf size and generation
        // CPU time some.

        SkISize wh = rect_to_size(physicalBounds).toCeil();

        auto surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(wh));
        if (!surface) {
            return;
        }
        SkCanvas* canvas = surface->getCanvas();
        canvas->clear(SK_ColorTRANSPARENT);

        SkScalar deltaX = outlineBounds.left();
        SkScalar deltaY = outlineBounds.top();

        SkMatrix offsetMatrix = transform;
        offsetMatrix.postTranslate(-deltaX, -deltaY);
        offsetMatrix.postScale(scaleX, scaleY);

        // Translate the draw in the new canvas, so we perfectly fit the
        // shape in the bitmap.
        canvas->setMatrix(offsetMatrix);
        canvas->drawImage(imageSubset.image(), 0, 0);

        // In the new space, we use the identity matrix translated
        // and scaled to reflect DPI.
        matrix.setScale(1 / scaleX, 1 / scaleY);
        matrix.postTranslate(deltaX, deltaY);

        imageSubset = SkKeyedImage(surface->makeImageSnapshot());
        canUseOriginal = false;
        if (!imageSubset) {
            return;
        }
    }

    if (SkColorFilter* colorFilter = paint->getColorFilter()) {
        sk_sp<SkImage> img = color_filter(imageSubset.image().get(), colorFilter);
        imageSubset = SkKeyedImage(std::move(img));
        canUseOriginal = false;
        if (!imageSubset) {
            return;
        }
        // TODO(halcanary): de-dupe this by caching filtered images.
        // (maybe in the resource cache?)
    }

    bool useCroppedOriginal = false;
    if (didSubset && canUseOriginal) {
        size_t originalSize = SkPDFSerializeImageSize(
            originalImage.image().get(), fDocument, fDocument->metadata().fEncodingQuality);
        size_t subsetSize = SkPDFSerializeImageSize(
            imageSubset.image().get(), fDocument, fDocument->metadata().fEncodingQuality);
        if (originalSize <= subsetSize) {
            matrix = originalTransform;
            imageSubset = originalImage;
            useCroppedOriginal = true;
        }
    }

    // Need sub-pixel clipping to fix skbug.com/40035524
    SkClipStack::AutoRestore ar(&this->cs(), false);
    if ((src && !is_integral(*src)) || useCroppedOriginal) {
        this->cs().save();
        this->cs().clipRect(dst, ctm, SkClipOp::kIntersect, true);
    }

    SkMatrix scaled;
    // Adjust for origin flip.
    scaled.setScale(SK_Scalar1, -SK_Scalar1);
    scaled.postTranslate(0, SK_Scalar1);
    // Scale the image up from 1x1 to WxH.
    SkIRect subset = imageSubset.image()->bounds();
    scaled.postScale(SkIntToScalar(subset.width()),
                     SkIntToScalar(subset.height()));
    scaled.postConcat(matrix);
    ScopedContentEntry content(this, &this->cs(), scaled, *paint);
    if (!content) {
        return;
    }
    SkPath shape = SkPath::Rect(SkRect::Make(subset)).makeTransform(matrix);
    if (content.needShape()) {
        content.setShape(shape);
    }
    if (!content.needSource()) {
        return;
    }

    SkBitmapKey key = imageSubset.key();
    SkPDFIndirectReference* pdfimagePtr = fDocument->fPDFBitmapMap.find(key);
    SkPDFIndirectReference pdfimage = pdfimagePtr ? *pdfimagePtr : SkPDFIndirectReference();
    if (!pdfimagePtr) {
        SkASSERT(imageSubset);
        pdfimage = SkPDFSerializeImage(imageSubset.image().get(), fDocument,
                                       fDocument->metadata().fEncodingQuality);
        SkASSERT((key != SkBitmapKey{{0, 0, 0, 0}, 0}));
        fDocument->fPDFBitmapMap.set(key, pdfimage);
    }
    SkASSERT(pdfimage != SkPDFIndirectReference());
    this->drawFormXObject(pdfimage, content.stream(), &shape);
}

///////////////////////////////////////////////////////////////////////////////////////////////////


void SkPDFDevice::drawDevice(SkDevice* device, const SkSamplingOptions& sampling,
                             const SkPaint& paint) {
    SkASSERT(!paint.getImageFilter());
    SkASSERT(!paint.getMaskFilter());

    // Check if SkPDFDevice::createDevice returned an SkBitmapDevice.
    // SkPDFDevice::createDevice creates SkBitmapDevice for color filters.
    // Image filters generally go through makeSpecial and drawSpecial.
    SkPixmap pmap;
    if (device->peekPixels(&pmap)) {
        this->SkClipStackDevice::drawDevice(device, sampling, paint);
        return;
    }

    // Otherwise SkPDFDevice::createDevice() creates SkPDFDevice subclasses.
    SkPDFDevice* pdfDevice = static_cast<SkPDFDevice*>(device);

    if (pdfDevice->isContentEmpty()) {
        return;
    }

    SkMatrix matrix = device->getRelativeTransform(*this).asM33();
    ScopedContentEntry content(this, &this->cs(), matrix, paint);
    if (!content) {
        return;
    }
    SkPath shape = SkPath::Rect(SkRect::Make(device->imageInfo().dimensions()));
    shape.transform(matrix);
    if (content.needShape()) {
        content.setShape(shape);
    }
    if (!content.needSource()) {
        return;
    }
    // This XObject may contain its own marks, which are hidden if emitted inside an outer mark.
    // If it does have its own marks we need to pause the current mark and then re-set it after.
    int currentStructElemId = fMarkManager.elemId();
    if (pdfDevice->fMarkManager.madeMarks()) {
        fMarkManager.setNextMarksElemId(0);
        fMarkManager.beginMark();
    }
    this->drawFormXObject(pdfDevice->makeFormXObjectFromDevice(), content.stream(), &shape);
    fMarkManager.setNextMarksElemId(currentStructElemId);
}

void SkPDFDevice::drawSpecial(SkSpecialImage* srcImg, const SkMatrix& localToDevice,
                              const SkSamplingOptions& sampling, const SkPaint& paint,
                              SkCanvas::SrcRectConstraint) {
    if (this->hasEmptyClip()) {
        return;
    }
    SkASSERT(!srcImg->isGaneshBacked() && !srcImg->isGraphiteBacked());
    SkASSERT(!paint.getMaskFilter() && !paint.getImageFilter());

    SkBitmap resultBM;
    if (SkSpecialImages::AsBitmap(srcImg, &resultBM)) {
        auto r = SkRect::MakeWH(resultBM.width(), resultBM.height());
        this->internalDrawImageRect(SkKeyedImage(resultBM), nullptr, r, sampling, paint,
                                    localToDevice);
    }
}
