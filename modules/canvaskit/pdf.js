CanvasKit._extraInitializations = CanvasKit._extraInitializations || [];
CanvasKit._extraInitializations.push(function() {
    CanvasKit.MakePDFDocument = function(metadata) {
        // Fills out all optional fields with defaults. The emscripten bindings complain if there
        // is a field undefined and it was expecting a float (for example).
        let pdfMetadata = initPDFMetadata(metadata);
        let doc = CanvasKit._MakePDFDocument(pdfMetadata);
        return doc;
    };

    CanvasKit.Document.prototype.beginPage = function(width, height, rect) {
        let rPtr = copyRectToWasm(rect);
        return this._beginPage(width, height, rPtr);
      };

    function copyAttribute(tag, source) {
        let owner = cacheOrCopyString(source['owner'] || '');
        let name = cacheOrCopyString(source['name'] || '');
        if(source['type'] === 'name') {
            tag.appendNameAttribute(owner, name, cacheOrCopyString(source['value'] || ''));
        } else if(source['type'] === 'int') {
            tag.appendIntAttribute(owner, name, source['value'] || 0);
        } else if(source['type'] === 'float') {
            tag.appendFloatAttribute(owner, name, source['value'] || 0.0);
        } else {
            throw new Error('PDFTagAttribute: Unknown type: ' + source['type']);
        }
    }

    function initPDFTag(source) {
      if (!source) {
        return null;
      }
      let pdftag = CanvasKit._PDFTagNode.Make();
      pdftag.setNodeId(source['id'] || 0);
      pdftag.setTypeString(source['type'] || 'NonStruct');
      pdftag.setAlt(source['alt'] || '');
      pdftag.setLang(source['language'] || '');
      source['attributes'] = source['attributes'] || [];
      for (let i = 0; i < source['attributes'].length; i++) {
          copyAttribute(pdftag, source['attributes'][i]);
      }
      source['children'] = source['children'] || [];
      for (let i = 0; i < source['children'].length; i++) {
          pdftag.appendChild(initPDFTag(source['children'][i]));
      }

      return pdftag;
    }

    function initPDFMetadata(metadata) {
        // Fills out all optional fields with defaults. The emscripten bindings complain if there
        // is a field undefined and it was expecting a float (for example).
        // Use [''] to tell closure not to minify the names
        metadata = metadata || {};
        metadata['title'] = metadata['title'] || '';
        metadata['author'] = metadata['author'] || '';
        metadata['subject'] = metadata['subject'] || '';
        metadata['keywords'] = metadata['keywords'] || '';
        metadata['creator'] = metadata['creator'] || '';
        metadata['producer'] = metadata['producer'] || '';
        metadata['language'] = metadata['language'] || '';
        metadata['rasterDPI'] = metadata['rasterDPI'] || 72;
        metadata['PDFA'] = !!metadata['PDFA'];
        metadata['compressionLevel'] = metadata['compressionLevel'] || CanvasKit.PDFCompressionLevel.Default;
        if (metadata['rootTag']) {
            metadata['_rootTag'] = metadata['_rootTag'] || initPDFTag(metadata['rootTag']);
        } 
        return metadata;
    };

     // maps string -> malloc'd pointer
    let stringCache = {};

    // cacheOrCopyString copies a string from JS into WASM on the heap and returns the pointer
    // to the memory of the string. It is expected that a caller to this helper will *not* free
    // that memory, so it is cached. Thus, if a future call to this function with the same string
    // will return the cached pointer, preventing the memory usage from growing unbounded (in
    // a normal use case).
    // We expect that these strings will be used multiple times so would help to cache them.
    function cacheOrCopyString(str) {
      if (stringCache[str]) {
        return stringCache[str];
      }
      // Add 1 for null terminator, which we need when copying/converting
      let strLen = lengthBytesUTF8(str) + 1;
      let strPtr = CanvasKit._malloc(strLen);
      stringToUTF8(str, strPtr, strLen);
      stringCache[str] = strPtr;
      return strPtr;
    }
});
