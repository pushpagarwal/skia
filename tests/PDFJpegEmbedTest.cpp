/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkTypes.h"

#ifdef SK_SUPPORT_PDF

#include "include/codec/SkEncodedOrigin.h"
#include "include/codec/SkJpegDecoder.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkData.h"
#include "include/core/SkDocument.h"
#include "include/core/SkImage.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSize.h"
#include "include/core/SkStream.h"
#include "include/docs/SkPDFDocument.h"
#include "include/docs/SkPDFJpegHelpers.h"
#include "include/private/SkEncodedInfo.h"
#include "src/codec/SkCodecPriv.h"
#include "tests/Test.h"
#include "tools/Resources.h"

#include <array>
#include <cstdint>
#include <cstring>

static bool is_subset_of(SkData* smaller, SkData* larger) {
    SkASSERT(smaller && larger);
    if (smaller->size() > larger->size()) {
        return false;
    }
    size_t size = smaller->size();
    size_t size_diff = larger->size() - size;
    for (size_t i = 0; i <= size_diff; ++i) {
        if (0 == memcmp(larger->bytes() + i, smaller->bytes(), size)) {
            return true;
        }
    }
    return false;
}


static sk_sp<SkData> load_resource(
        skiatest::Reporter* r, const char* test, const char* filename) {
    sk_sp<SkData> data = GetResourceAsData(filename);
    if (!data) {
        INFOF(r, "\n%s: Resource '%s' can not be found.\n",
              test, filename);
    }
    return data;  // May return nullptr.
}

/**
 *  Test that for Jpeg files that use the JFIF colorspace, they are
 *  directly embedded into the PDF (without re-encoding) when that
 *  makes sense.
 */
DEF_TEST(SkPDF_JpegEmbedTest, r) {
    REQUIRE_PDF_DOCUMENT(SkPDF_JpegEmbedTest, r);
    const char test[] = "SkPDF_JpegEmbedTest";
    sk_sp<SkData> mandrillData(load_resource(r, test, "images/mandrill_512_q075.jpg"));
    sk_sp<SkData> cmykData(load_resource(r, test, "images/CMYK.jpg"));
    sk_sp<SkData> yuvICCData(load_resource(r, test, "images/crbug999986.jpeg"));
    sk_sp<SkData> cmykICCData(load_resource(r, test, "images/crbug1465627.jpeg"));

    if (!mandrillData || !cmykData || !yuvICCData || !cmykICCData) {
        REPORTER_ASSERT(r, false, "Could not load all images");
        return;
    }
    ////////////////////////////////////////////////////////////////////////////
    SkDynamicMemoryWStream pdf;
    auto document = SkPDF::MakeDocument(&pdf, SkPDF::JPEG::MetadataWithCallbacks());
    SkCanvas* canvas = document->beginPage(642, 2048);

    canvas->clear(SK_ColorLTGRAY);
    canvas->clipIRect(SkIRect::MakeLTRB(0, 0, 642, 2048));

    sk_sp<SkImage> im1(SkImages::DeferredFromEncodedData(mandrillData));
    canvas->drawImage(im1, 65.0, 0.0);
    sk_sp<SkImage> im2(SkImages::DeferredFromEncodedData(cmykData));
    canvas->drawImage(im2, 0.0, 512.0);

    // This should be the same blue as the dark spot seen in the image.
    SkPaint bluePaint;
    bluePaint.setColor(SkColorSetARGB(255, 0, 59, 103));
    canvas->drawRect(SkRect::MakeXYWH(0, 1000, 642, 24), bluePaint);
    sk_sp<SkImage> im3(SkImages::DeferredFromEncodedData(yuvICCData));
    canvas->drawImageRect(
        im3,
        SkRect::MakeXYWH(0, 0, 1024, 800),
        SkRect::MakeXYWH(0, 1024, 1024, 800),
        SkSamplingOptions(), nullptr, SkCanvas::kFast_SrcRectConstraint);

    sk_sp<SkImage> im4(SkImages::DeferredFromEncodedData(cmykICCData));
    canvas->drawImage(im4, 0.0, 1536.0);

    document->endPage();
    document->close();
    sk_sp<SkData> pdfData = pdf.detachAsData();
    SkASSERT(pdfData);

    // This JPEG uses a nonstandard colorspace - it can not be
    // embedded into the PDF directly.
    REPORTER_ASSERT(r, !is_subset_of(cmykData.get(), pdfData.get()));

    // Part of this JPEG was drawn with drawImageRect.
    // However, the original data is smaller than the subset data so the original should be present.
    REPORTER_ASSERT(r, is_subset_of(yuvICCData.get(), pdfData.get()));

    if ((false)) {
        SkFILEWStream s("/tmp/jpegembed.pdf");
        REPORTER_ASSERT(r, s.write(pdfData->data(), pdfData->size()));
        s.flush();
        s.fsync();
    }
}

struct SkJFIFInfo {
    SkISize fSize;
    enum Type {
        kGrayscale,
        kYCbCr,
    } fType;
};
bool SkIsJFIF(const SkData* data, SkJFIFInfo* info) {
    static constexpr const SkCodecs::Decoder decoders[] = {
        SkJpegDecoder::Decoder(),
    };

    if (!data) {
        return false;
    }
    std::unique_ptr<SkCodec> codec = SkCodec::MakeFromData(sk_ref_sp(data), decoders);
    if (!codec) {
        return false;
    }

    SkISize jpegSize = codec->dimensions();
    SkEncodedInfo::Color jpegColorType = SkCodecPriv::GetEncodedInfo(codec.get()).color();
    SkEncodedOrigin exifOrientation = codec->getOrigin();

    bool yuv = jpegColorType == SkEncodedInfo::kYUV_Color;
    bool goodColorType = yuv || jpegColorType == SkEncodedInfo::kGray_Color;
    if (goodColorType && kTopLeft_SkEncodedOrigin == exifOrientation) {
        if (info) {
            *info = {jpegSize, yuv ? SkJFIFInfo::kYCbCr : SkJFIFInfo::kGrayscale};
        }
        return true;
    }
    return false;
}

DEF_TEST(SkPDF_JpegIdentification, r) {
    static struct {
        const char* path;
        bool isJfif;
        SkJFIFInfo::Type type;
    } kTests[] = {{"images/CMYK.jpg", false, SkJFIFInfo::kGrayscale},
                  {"images/color_wheel.jpg", true, SkJFIFInfo::kYCbCr},
                  {"images/grayscale.jpg", true, SkJFIFInfo::kGrayscale},
                  {"images/mandrill_512_q075.jpg", true, SkJFIFInfo::kYCbCr},
                  {"images/randPixels.jpg", true, SkJFIFInfo::kYCbCr}};
    for (size_t i = 0; i < std::size(kTests); ++i) {
        sk_sp<SkData> data(load_resource(r, "JpegIdentification", kTests[i].path));
        if (!data) {
            continue;
        }
        SkJFIFInfo info;
        bool isJfif = SkIsJFIF(data.get(), &info);
        if (isJfif != kTests[i].isJfif) {
            ERRORF(r, "%s failed isJfif test", kTests[i].path);
            continue;
        }
        if (!isJfif) {
            continue;  // not applicable
        }
        if (kTests[i].type != info.fType) {
            ERRORF(r, "%s failed jfif type test", kTests[i].path);
            continue;
        }
        INFOF(r, "\nJpegIdentification: %s [%d x %d]\n", kTests[i].path,
              info.fSize.width(), info.fSize.height());
    }

    // Test several malformed jpegs.
    SkJFIFInfo info;
    {
        static const char goodJpeg[] =
            "\377\330\377\340\0\20JFIF\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\20\13\14"
            "\16\14\n\20\16\r\16\22\21\20\23\30(\32\30\26\26\0301#%\35(:3=<9387"
            "@H\\N@DWE78PmQW_bghg>Mqypdx\\egc\377\333\0C\1\21\22\22\30\25\30/\32"
            "\32/cB8Bcccccccccccccccccccccccccccccccccccccccccccccccccc\377\300"
            "\0\21\10\0\10\0\10\3\1\"\0\2\21\1\3\21\1\377\304\0\37\0\0\1\5\1\1\1"
            "\1\1\1\0\0\0\0\0\0\0\0\1\2\3\4\5\6\7\10\t\n\13\377\304\0\265\20\0\2"
            "\1\3\3\2\4\3\5\5\4\4\0\0\1}\1\2\3\0\4\21\5\22!1A\6\23Qa\7\"q\0242\201"
            "\221\241\10#B\261\301\25R\321\360$3br\202\t\n\26\27\30\31\32%&'()*"
            "456789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\203\204\205\206\207\210\211"
            "\212\222\223\224\225\226\227\230\231\232\242\243\244\245\246\247\250"
            "\251\252\262\263\264\265\266\267\270\271\272\302\303\304\305\306\307"
            "\310\311\312\322\323\324\325\326\327\330\331\332\341\342\343\344\345"
            "\346\347\350\351\352\361\362\363\364\365\366\367\370\371\372\377\304"
            "\0\37\1\0\3\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\1\2\3\4\5\6\7\10\t\n\13\377"
            "\304\0\265\21\0\2\1\2\4\4\3\4\7\5\4\4\0\1\2w\0\1\2\3\21\4\5!1\6\22"
            "AQ\7aq\23\"2\201\10\24B\221\241\261\301\t#3R\360\25br\321\n\26$4\341"
            "%\361\27\30\31\32&'()*56789:CDEFGHIJSTUVWXYZcdefghijstuvwxyz\202\203"
            "\204\205\206\207\210\211\212\222\223\224\225\226\227\230\231\232\242"
            "\243\244\245\246\247\250\251\252\262\263\264\265\266\267\270\271\272"
            "\302\303\304\305\306\307\310\311\312\322\323\324\325\326\327\330\331"
            "\332\342\343\344\345\346\347\350\351\352\362\363\364\365\366\367\370"
            "\371\372\377\332\0\14\3\1\0\2\21\3\21\0?\0\216M\352\214\25\271\224"
            "\262\310\253\363tl\22209\35O~\237\\\24QZ\306Mh\216\252i\364ml\177\377"
            "\331";
        size_t goodJpegLength = 659;
        auto data = SkData::MakeWithoutCopy(goodJpeg, goodJpegLength);
        REPORTER_ASSERT(r, SkIsJFIF(data.get(), &info));
        REPORTER_ASSERT(r, info.fSize == (SkISize{8, 8}));
        REPORTER_ASSERT(r, info.fType == SkJFIFInfo::kYCbCr);

        // Not long enough to read first (SOI) segment marker.
        data = SkData::MakeWithoutCopy(goodJpeg, 1);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));

        // Not long enough to read second segment (APP0) marker.
        data = SkData::MakeWithoutCopy(goodJpeg, 3);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));

        // Not long enough to read second segment's length.
        data = SkData::MakeWithoutCopy(goodJpeg, 5);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));

        // APP0 segment is truncated.
        data = SkData::MakeWithoutCopy(goodJpeg, 7);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));

        // Missing SOF segment.
        data = SkData::MakeWithoutCopy(goodJpeg, 89);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
    {
        // JFIF tag missing.
        static const char jpeg[] =
            "\377\330\377\340\0\20JFIX\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\10\6\6\7"
            "\6\5\10\7\7\7\t\t\10\n\14\24\r\14\13\13\14\31\22\23\17\24\35\32\37"
            "\36\35\32\34\34 $.' \",#\34\34(7),01444\37'9=82<.342\377\333\0C\1\t"
            "\t\t\14\13\14\30\r\r\0302!\34!222222222222222222222222222222222222"
            "22222222222222\377\300\0\21\10\2\0\2\0\3\1\"\0\2\21\1\3\21\001";
        size_t jpegLength = 177;
        auto data = SkData::MakeWithoutCopy(jpeg, jpegLength);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
    {
        // APP0 segment short (byte 6 changed).
        static const char jpeg[] =
            "\377\330\377\340\0\5JFIF\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\10\6\6\7"
            "\6\5\10\7\7\7\t\t\10\n\14\24\r\14\13\13\14\31\22\23\17\24\35\32\37"
            "\36\35\32\34\34 $.' \",#\34\34(7),01444\37'9=82<.342\377\333\0C\1\t"
            "\t\t\14\13\14\30\r\r\0302!\34!222222222222222222222222222222222222"
            "22222222222222\377\300\0\21\10\2\0\2\0\3\1\"\0\2\21\1\3\21\001";
        size_t jpegLength = 177;
        auto data = SkData::MakeWithoutCopy(jpeg, jpegLength);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
    {
        // SOF segment short. ('\21' replaced with '\5')
        static const char jpeg[] =
            "\377\330\377\340\0\20JFIF\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\10\6\6\7"
            "\6\5\10\7\7\7\t\t\10\n\14\24\r\14\13\13\14\31\22\23\17\24\35\32\37"
            "\36\35\32\34\34 $.' \",#\34\34(7),01444\37'9=82<.342\377\333\0C\1\t"
            "\t\t\14\13\14\30\r\r\0302!\34!222222222222222222222222222222222222"
            "22222222222222\377\300\0\5\10\2\0\2\0\3\1\"\0\2\21\1\3\21\001";
        size_t jpegLength = 177;
        auto data = SkData::MakeWithoutCopy(jpeg, jpegLength);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
    {
        // Unsupported 12-bit components. ('\10' replaced with '\14')
        static const char jpeg[] =
            "\377\330\377\340\0\20JFIF\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\10\6\6\7"
            "\6\5\10\7\7\7\t\t\10\n\14\24\r\14\13\13\14\31\22\23\17\24\35\32\37"
            "\36\35\32\34\34 $.' \",#\34\34(7),01444\37'9=82<.342\377\333\0C\1\t"
            "\t\t\14\13\14\30\r\r\0302!\34!222222222222222222222222222222222222"
            "22222222222222\377\300\0\21\14\2\0\2\0\3\1\"\0\2\21\1\3\21\001";
        size_t jpegLength = 177;
        auto data = SkData::MakeWithoutCopy(jpeg, jpegLength);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
    {
        // Two color channels.  ('\3' replaced with '\2')
        static const char jpeg[] =
            "\377\330\377\340\0\20JFIF\0\1\1\0\0\1\0\1\0\0\377\333\0C\0\10\6\6\7"
            "\6\5\10\7\7\7\t\t\10\n\14\24\r\14\13\13\14\31\22\23\17\24\35\32\37"
            "\36\35\32\34\34 $.' \",#\34\34(7),01444\37'9=82<.342\377\333\0C\1\t"
            "\t\t\14\13\14\30\r\r\0302!\34!222222222222222222222222222222222222"
            "22222222222222\377\300\0\21\10\2\0\2\0\2\1\"\0\2\21\1\3\21\001";
        size_t jpegLength = 177;
        auto data = SkData::MakeWithoutCopy(jpeg, jpegLength);
        REPORTER_ASSERT(r, !SkIsJFIF(data.get(), &info));
    }
}
#endif
