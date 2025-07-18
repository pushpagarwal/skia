#include "include/core/SkDocument.h"
#include "include/core/SkStream.h"
#include "include/docs/SkPDFDocument.h"
#include "include/docs/SkPDFJpegHelpers.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include "modules/canvaskit/WasmCommon.h"

using namespace emscripten;

struct SimplePDFTagAttribute {
    WASMPointerU8 ownerPtr;
    WASMPointerU8 namePtr;
    WASMPointerU8 typePtr;       // "name", "int", "float", "float-array" "node-id-array"
    WASMPointerU8 nameValuePtr;  // This is the value of the name, if type is "name".
    int intValue;
    float floatValue;                // This is the value of the float, if type is "float".
    std::vector<float> floatValues;  // This is the value of the float, if type is "float".
    std::vector<int32_t>
            nodeIdArray;  // This is the value of the node ID array, if type is "node-id-array".

    void AppendToAttributeList(SkPDF::AttributeList& list) const {
        std::string type(reinterpret_cast<const char*>(typePtr));
        if (type == "name") {
            list.appendName(reinterpret_cast<const char*>(ownerPtr),
                            reinterpret_cast<const char*>(namePtr),
                            reinterpret_cast<const char*>(nameValuePtr));
        } else if (type == "float") {
            list.appendFloat(reinterpret_cast<const char*>(ownerPtr),
                             reinterpret_cast<const char*>(namePtr),
                             floatValue);
        } else if (type == "int") {
            list.appendInt(reinterpret_cast<const char*>(ownerPtr),
                           reinterpret_cast<const char*>(namePtr),
                           intValue);
        } else if (type == "float" && floatValues.size() > 0) {
            list.appendFloatArray(reinterpret_cast<const char*>(ownerPtr),
                                  reinterpret_cast<const char*>(namePtr),
                                  floatValues);
        } else if (type == "node-id-array" && nodeIdArray.size() > 0) {
            list.appendNodeIdArray(reinterpret_cast<const char*>(ownerPtr),
                                   reinterpret_cast<const char*>(namePtr),
                                   nodeIdArray);
        }
    }
};

struct SimplePDFTag {
    int id = 0;
    std::string type;
    std::string alt;
    std::string language;
    std::vector<SimplePDFTagAttribute> attributes;
    std::vector<SimplePDFTag> children;
    std::unique_ptr<SkPDF::StructureElementNode> toStructureElementNode() const {
        auto tag = std::make_unique<SkPDF::StructureElementNode>();
        tag->fNodeId = id;
        tag->fTypeString = SkString(type);
        tag->fAlt = SkString(alt);
        tag->fLang = SkString(language);
        for (const auto& attr : attributes) {
            attr.AppendToAttributeList(tag->fAttributes);
        }
        for (const auto& child : children) {
            tag->fChildVector.push_back(child.toStructureElementNode());
        }
        return tag;
    }
};

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
    SimplePDFTag rootTag;
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
        meta.fStructureElementTreeRoot = rootTag.toStructureElementNode().release();
    }
};

// toBytes returns a Uint8Array that has a copy of the data in the given SkData.
// defined in canvaskit_bindings.cpp
extern Uint8Array toBytes(sk_sp<SkData> data);

class CkDocument {
public:
    static CkDocument* MakePDFDocument(SimplePDFMetadata metadata) {
        SkPDF::Metadata pdfMetadata;
        pdfMetadata.jpegDecoder = SkPDF::JPEG::Decode;
        pdfMetadata.jpegEncoder = SkPDF::JPEG::Encode;
        metadata.to(pdfMetadata);
        auto stream = std::make_unique<SkDynamicMemoryWStream>();
        auto document = SkPDF::MakeDocument(stream.get(), pdfMetadata);
        // Clean up the root tag as SkPDF::Metadata does not take ownership of it.
        delete pdfMetadata.fStructureElementTreeRoot;
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

namespace emscripten {
namespace internal {
template <> void raw_destructor<SkDocument>(SkDocument* ptr) {
    if (ptr) {
        ptr->unref();
    }
}
}  // namespace internal
}  // namespace emscripten

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

    value_object<SimplePDFTagAttribute>("PDFTagAttribute")
            .field("owner", &SimplePDFTagAttribute::ownerPtr)
            .field("name", &SimplePDFTagAttribute::namePtr)
            .field("type", &SimplePDFTagAttribute::typePtr)
            .field("floatValues", &SimplePDFTagAttribute::floatValues)
            .field("nodeIdArray", &SimplePDFTagAttribute::nodeIdArray)
            .field("nameValue", &SimplePDFTagAttribute::nameValuePtr)
            .field("intValue", &SimplePDFTagAttribute::intValue)
            .field("floatValue", &SimplePDFTagAttribute::floatValue);

    value_object<SimplePDFTag>("PDFTag")
            .field("id", &SimplePDFTag::id)
            .field("type", &SimplePDFTag::type)
            .field("alt", &SimplePDFTag::alt)
            .field("attributes", &SimplePDFTag::attributes)
            .field("children", &SimplePDFTag::children);

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
            .field("rootTag", &SimplePDFMetadata::rootTag)
            .field("compressionLevel", &SimplePDFMetadata::compressionLevel);

    constant("pdf", true);
    enum_<SkPDF::Metadata::CompressionLevel>("PDFCompressionLevel")
            .value("Default", SkPDF::Metadata::CompressionLevel::Default)
            .value("None", SkPDF::Metadata::CompressionLevel::None)
            .value("LowButFast", SkPDF::Metadata::CompressionLevel::LowButFast)
            .value("Average", SkPDF::Metadata::CompressionLevel::Average)
            .value("HighButSlow", SkPDF::Metadata::CompressionLevel::HighButSlow);

    emscripten::register_vector<SimplePDFTagAttribute>("PDFTagAttributeVector");
    emscripten::register_vector<SimplePDFTag>("PDFTagVector");
    emscripten::register_vector<int>("IntVector");
    emscripten::register_vector<float>("FloatVector");
}
