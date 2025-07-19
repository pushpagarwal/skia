#include "include/core/SkDocument.h"
#include "include/core/SkStream.h"
#include "include/docs/SkPDFDocument.h"
#include "include/docs/SkPDFJpegHelpers.h"
#include "src/pdf/SkPDFUtils.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include "modules/canvaskit/WasmCommon.h"

#include <iostream>

using namespace emscripten;

struct SimplePDFMetadata {
    std::string title;
    std::string author;
    std::string subject;
    std::string keywords;
    std::string creator;
    std::string producer;
    std::string language;
    SkScalar rasterDPI = 0;
    bool PDFA = false;
    SkPDF::Metadata::CompressionLevel compressionLevel = SkPDF::Metadata::CompressionLevel::Default;

    void to(SkPDF::Metadata& meta) const {
        meta.fTitle = SkString(title);
        meta.fAuthor = SkString(author);
        meta.fSubject = SkString(subject);
        meta.fKeywords = SkString(keywords);
        meta.fCreator = SkString(creator);
        meta.fProducer = SkString(producer);
        meta.fLang = SkString(language);
        meta.fRasterDPI = rasterDPI;
        meta.fPDFA = PDFA;
        meta.fCompressionLevel = compressionLevel;
    }
};

// toBytes returns a Uint8Array that has a copy of the data in the given SkData.
// defined in canvaskit_bindings.cpp
extern Uint8Array toBytes(sk_sp<SkData> data);

class CkDocument {
public:
    static CkDocument* MakePDFDocument(SimplePDFMetadata metadata,
                                       SkPDF::StructureElementNode* rootTag) {
        SkPDF::Metadata pdfMetadata;
        pdfMetadata.jpegDecoder = SkPDF::JPEG::Decode;
        pdfMetadata.jpegEncoder = SkPDF::JPEG::Encode;
        SkPDF::DateTime now;
        SkPDFUtils::GetDateTime(&now);
        pdfMetadata.fCreation = now;
        pdfMetadata.fModified = now;
        metadata.to(pdfMetadata);
        pdfMetadata.fStructureElementTreeRoot = rootTag;
        auto stream = std::make_unique<SkDynamicMemoryWStream>();
        auto document = SkPDF::MakeDocument(stream.get(), pdfMetadata);
        if (rootTag) {
            // pdfMetadata does not take ownership of the rootTag, so we need to delete it here.
            delete rootTag;
        }
        return new CkDocument(std::move(document), std::move(stream));
    }
    SkCanvas* beginPage(SkScalar width, SkScalar height, WASMPointerF32 fPtr) {
        const SkRect* rect = reinterpret_cast<const SkRect*>(fPtr);
        return fDoc->beginPage(width, height, rect);
    }
    void endPage() { fDoc->endPage(); }
    Uint8Array close() {
        fDoc->close();
        fStream->flush();
        auto data = fStream->detachAsData();
        return toBytes(data);
    }
    void abort() {
        fDoc->abort();
        fStream->flush();
    }

private:
    CkDocument(sk_sp<SkDocument> doc, std::unique_ptr<SkDynamicMemoryWStream> fStream)
            : fDoc(std::move(doc)), fStream(std::move(fStream)) {}
    sk_sp<SkDocument> fDoc;
    std::unique_ptr<SkDynamicMemoryWStream> fStream;
};

EMSCRIPTEN_BINDINGS(Pdf) {
    class_<CkDocument>("Document")
            .function("_beginPage", &CkDocument::beginPage, allow_raw_pointers())
            .function("endPage", &CkDocument::endPage)
            .function("close", &CkDocument::close)
            .function("abort", &CkDocument::abort);

    function("_MakePDFDocument", &CkDocument::MakePDFDocument, allow_raw_pointers());

    function("SetPDFTagId", optional_override([](SkCanvas& canvas, int32_t tagId) {
                 SkPDF::SetNodeId(&canvas, tagId);
             }));

    class_<SkPDF::StructureElementNode>("_PDFTagNode")
            .class_function("Make",
                            optional_override([]() {
                                return std::make_unique<SkPDF::StructureElementNode>();
                            }),
                            allow_raw_pointers())
            .function("setNodeId",
                      optional_override([](SkPDF::StructureElementNode& node, int32_t id) {
                          node.fNodeId = id;
                      }))
            .function("setTypeString",
                      optional_override([](SkPDF::StructureElementNode& node, std::string type) {
                          node.fTypeString = SkString(type);
                      }))
            .function("setAlt",
                      optional_override([](SkPDF::StructureElementNode& node, std::string alt) {
                          node.fAlt = SkString(alt);
                      }))
            .function("setLang",
                      optional_override([](SkPDF::StructureElementNode& node, std::string lang) {
                          node.fLang = SkString(lang);
                      }))
            .function("appendChild",
                      optional_override([](SkPDF::StructureElementNode& node,
                                           std::unique_ptr<SkPDF::StructureElementNode> child) {
                          node.fChildVector.push_back(std::move(child));
                      }))
            .function("appendIntAttribute",
                      optional_override([](SkPDF::StructureElementNode& node,
                                           WASMPointerU8 ownerPtr,
                                           WASMPointerU8 namePtr,
                                           int32_t value) {
                          node.fAttributes.appendInt(reinterpret_cast<const char*>(ownerPtr),
                                                     reinterpret_cast<const char*>(namePtr),
                                                     value);
                      }))
            .function("appendFloatAttribute",
                      optional_override([](SkPDF::StructureElementNode& node,
                                           WASMPointerU8 ownerPtr,
                                           WASMPointerU8 namePtr,
                                           float value) {
                          node.fAttributes.appendFloat(reinterpret_cast<const char*>(ownerPtr),
                                                       reinterpret_cast<const char*>(namePtr),
                                                       value);
                      }))
            .function("appendNameAttribute",
                      optional_override([](SkPDF::StructureElementNode& node,
                                           WASMPointerU8 ownerPtr,
                                           WASMPointerU8 namePtr,
                                           WASMPointerU8 valuePtr) {
                          node.fAttributes.appendName(reinterpret_cast<const char*>(ownerPtr),
                                                      reinterpret_cast<const char*>(namePtr),
                                                      reinterpret_cast<const char*>(valuePtr));
                      }));

    value_object<SimplePDFMetadata>("PDFMetadata")
            .field("title", &SimplePDFMetadata::title)
            .field("author", &SimplePDFMetadata::author)
            .field("subject", &SimplePDFMetadata::subject)
            .field("keywords", &SimplePDFMetadata::keywords)
            .field("creator", &SimplePDFMetadata::creator)
            .field("producer", &SimplePDFMetadata::producer)
            .field("language", &SimplePDFMetadata::language)
            .field("rasterDPI", &SimplePDFMetadata::rasterDPI)
            .field("PDFA", &SimplePDFMetadata::PDFA)
            .field("compressionLevel", &SimplePDFMetadata::compressionLevel);

    constant("pdf", true);
    enum_<SkPDF::Metadata::CompressionLevel>("PDFCompressionLevel")
            .value("Default", SkPDF::Metadata::CompressionLevel::Default)
            .value("None", SkPDF::Metadata::CompressionLevel::None)
            .value("LowButFast", SkPDF::Metadata::CompressionLevel::LowButFast)
            .value("Average", SkPDF::Metadata::CompressionLevel::Average)
            .value("HighButSlow", SkPDF::Metadata::CompressionLevel::HighButSlow);
}
