/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "bench/Benchmark.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkString.h"
#include "include/private/base/SkTemplates.h"
#include "src/core/SkChecksum.h"
#include "tools/fonts/FontToolUtils.h"

#include "bench/gUniqueGlyphIDs.h"

#define gUniqueGlyphIDs_Sentinel    0xFFFF

static int count_glyphs(const uint16_t start[]) {
    const uint16_t* curr = start;
    while (*curr != gUniqueGlyphIDs_Sentinel) {
        curr += 1;
    }
    return static_cast<int>(curr - start);
}

class FontCacheBench : public Benchmark {
public:
    FontCacheBench()  {}

protected:
    const char* onGetName() override {
        return "fontcache";
    }

    void onDraw(int loops, SkCanvas* canvas) override {
        SkFont font = ToolUtils::DefaultFont();
        font.setEdging(SkFont::Edging::kAntiAlias);

        const uint16_t* array = gUniqueGlyphIDs;
        while (*array != gUniqueGlyphIDs_Sentinel) {
            int count = count_glyphs(array);
            for (int i = 0; i < loops; ++i) {
                (void)font.measureText(array, count * sizeof(uint16_t), SkTextEncoding::kGlyphID);
            }
            array += count + 1;    // skip the sentinel
        }
    }

private:
    using INHERITED = Benchmark;
};

///////////////////////////////////////////////////////////////////////////////

static uint32_t rotr(uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32 - bits));
}

typedef uint32_t (*HasherProc)(uint32_t);

static uint32_t hasher0(uint32_t value) {
    value = value ^ (value >> 16);
    return value ^ (value >> 8);
}

static const struct {
    const char* fName;
    HasherProc  fHasher;
} gRec[] = {
    { "hasher0",  hasher0 },
    { "hasher2",  SkChecksum::Mix },
};

#define kMaxHashBits   12
#define kMaxHashCount  (1 << kMaxHashBits)

static int count_collisions(const uint16_t array[], int count, HasherProc proc,
                            unsigned hashMask) {
    char table[kMaxHashCount];
    sk_bzero(table, sizeof(table));

    int collisions = 0;
    for (int i = 0; i < count; ++i) {
        int index = proc(array[i]) & hashMask;
        collisions += table[index];
        table[index] = 1;
    }
    return collisions;
}

static void dump_array(const uint16_t array[], int count) {
    for (int i = 0; i < count; ++i) {
        SkDebugf(" %d,", array[i]);
    }
    SkDebugf("\n");
}

class FontCacheEfficiency : public Benchmark {
public:
    FontCacheEfficiency()  {
        if ((false)) dump_array(nullptr, 0);
        if ((false)) rotr(0, 0);
    }

protected:
    const char* onGetName() override {
        return "fontefficiency";
    }

    void onDraw(int loops, SkCanvas* canvas) override {
        static bool gDone;
        if (gDone) {
            return;
        }
        gDone = true;

        for (int hashBits = 6; hashBits <= 12; hashBits += 1) {
            int hashMask = ((1 << hashBits) - 1);
            for (int limit = 32; limit <= 1024; limit <<= 1) {
                for (size_t i = 0; i < std::size(gRec); ++i) {
                    int collisions = 0;
                    int glyphs = 0;
                    const uint16_t* array = gUniqueGlyphIDs;
                    while (*array != gUniqueGlyphIDs_Sentinel) {
                        int count = std::min(count_glyphs(array), limit);
                        collisions += count_collisions(array, count, gRec[i].fHasher, hashMask);
                        glyphs += count;
                        array += count + 1;    // skip the sentinel
                    }
                    SkDebugf("hashBits [%d] limit [%d] collisions [%d / %d = %1.2g%%] using %s\n", hashBits, limit, collisions, glyphs,
                             collisions * 100.0 / glyphs, gRec[i].fName);
                }
            }
        }
    }

private:
    using INHERITED = Benchmark;
};
DEF_BENCH( return new FontCacheBench(); )

// undefine this to run the efficiency test
//DEF_BENCH( return new FontCacheEfficiency(); )

///////////////////////////////////////////////////////////////////////////////

class FontPathBench : public Benchmark {
    SkFont fFont;
    SkGlyphID fGlyphs[100];
    SkString fName;
    const bool fOneAtATime;

public:
    FontPathBench(bool oneAtATime) : fOneAtATime(oneAtATime) {
        fName.printf("font-path-%s", oneAtATime ? "loop" : "batch");
    }

protected:
    const char* onGetName() override {
        return fName.c_str();
    }

    bool isSuitableFor(Backend backend) override {
        return backend == Backend::kNonRendering;
    }

    void onDelayedSetup() override {
        fFont.setSize(32);
        for (size_t i = 0; i < std::size(fGlyphs); ++i) {
            fGlyphs[i] = i;
        }
    }

    void onDraw(int loops, SkCanvas* canvas) override {
        for (int loop = 0; loop < loops; ++loop) {
            if (fOneAtATime) {
                for (size_t i = 0; i < std::size(fGlyphs); ++i) {
                    (void)fFont.getPath(fGlyphs[i]);
                }
            } else {
                fFont.getPaths(fGlyphs,
                               [](const SkPath* src, const SkMatrix& mx, void*) {
                                   if (src) {
                                       (void)src->makeTransform(mx);
                                   }
                               }, nullptr);
            }
        }
    }

private:
    using INHERITED = Benchmark;
};
DEF_BENCH( return new FontPathBench(true); )
DEF_BENCH( return new FontPathBench(false); )
