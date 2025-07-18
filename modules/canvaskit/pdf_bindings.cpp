#include "include/core/SkDocument.h"
#include "include/core/SkStream.h"
#include "include/docs/SkPDFDocument.h"
#include "include/docs/SkPDFJpegHelpers.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include "modules/canvaskit/WasmCommon.h"

using namespace emscripten;

struct SimplePDFTagAttribute {
    std::string owner;
    std::string name;
    std::string type;       // "name", "int", "float", "float-array" "node-id-array"
    std::string nameValue;  // This is the value of the name, if type is "name".
    int intValue;
    float floatValue;                // This is the value of the float, if type is "float".
    std::vector<float> floatValues;  // This is the value of the float, if type is "float".
    std::vector<int32_t>
            nodeIdArray;  // This is the value of the node ID array, if type is "node-id-array".

    void AppendToAttributeList(SkPDF::AttributeList& list) const {
        if (type == "name") {
            list.appendName(SkString(owner), SkString(name), SkString(nameValue));
        } else if (type == "float") {
            list.appendFloat(SkString(owner), SkString(name), floatValue);
        } else if (type == "int") {
            list.appendInt(SkString(owner), SkString(name), intValue);
        } else if (type == "float" && floatValues.size() > 0) {
            list.appendFloatArray(SkString(owner), SkString(name), floatValues);
        } else if (type == "node-id-array" && nodeIdArray.size() > 0) {
            list.appendNodeIdArray(SkString(owner), SkString(name), nodeIdArray);
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

namespace emscripten {
namespace internal {
template <> void raw_destructor<SkDocument>(SkDocument* ptr) {
    if (ptr) {
        ptr->unref();
    }
}
}  // namespace internal
}  // namespace emscripten

// toBytes returns a Uint8Array that has a copy of the data in the given SkData.
// defined in canvaskit_bindings.cpp
extern Uint8Array toBytes(sk_sp<SkData> data);

EMSCRIPTEN_BINDINGS(Pdf) {
    class_<SkDocument>("Document")
            .smart_ptr<sk_sp<SkDocument>>("sk_sp<SkDocument>")
            .function("_beginPage",
                      optional_override([](SkDocument& self,
                                           SkScalar width,
                                           SkScalar height,
                                           WASMPointerF32 fPtr) -> SkCanvas* {
                          const SkRect* rect = reinterpret_cast<const SkRect*>(fPtr);
                          return self.beginPage(width, height, rect);
                      }),
                      allow_raw_pointers())
            .function("endPage", &SkDocument::endPage)
            .function("close", &SkDocument::close)
            .function("abort", &SkDocument::abort);

    class_<SkWStream>("WStream")
            .function("bytesWritten", &SkWStream::bytesWritten)
            .function("flush", &SkWStream::flush);

    class_<SkDynamicMemoryWStream, base<SkWStream>>("DynamicMemoryStream")
            .constructor<>()
            .function("detachAsBytes",
                      optional_override([](SkDynamicMemoryWStream& self) -> Uint8Array {
                          auto data = self.detachAsData();
                          return toBytes(data);
                      }));

    function("MakePDFDocument",
             optional_override([](SkWStream& stream,
                                  SimplePDFMetadata metadata) -> sk_sp<SkDocument> {
                 SkPDF::Metadata pdfMetadata;
                 pdfMetadata.jpegDecoder = SkPDF::JPEG::Decode;
                 pdfMetadata.jpegEncoder = SkPDF::JPEG::Encode;
                 metadata.to(pdfMetadata);
                 auto document = SkPDF::MakeDocument(&stream, pdfMetadata);
                 delete pdfMetadata
                         .fStructureElementTreeRoot;  // Clean up the root tag as SkPDF::Metadata
                                                      // does not take ownership of it.
                 return document;
             }));

    function("SetPDFTagId", optional_override([](SkCanvas& canvas, int32_t tagId) {
                 SkPDF::SetNodeId(&canvas, tagId);
             }));

    value_object<SimplePDFTagAttribute>("PDFTagAttribute")
            .field("owner", &SimplePDFTagAttribute::owner)
            .field("name", &SimplePDFTagAttribute::name)
            .field("type", &SimplePDFTagAttribute::type)
            .field("floatValues", &SimplePDFTagAttribute::floatValues)
            .field("nodeIdArray", &SimplePDFTagAttribute::nodeIdArray)
            .field("nameValue", &SimplePDFTagAttribute::nameValue)
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
