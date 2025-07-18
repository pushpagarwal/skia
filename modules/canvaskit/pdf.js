CanvasKit._extraInitializations = CanvasKit._extraInitializations || [];
CanvasKit._extraInitializations.push(function() {
    CanvasKit.MakePDFDocument = function(metadata) {
        // Fills out all optional fields with defaults. The emscripten bindings complain if there
        // is a field undefined and it was expecting a float (for example).
        metadata = initPDFMetadata(metadata);
        var doc = CanvasKit._MakePDFDocument(metadata);
        // Free the tags after the document is created.
        // This is to free vectors and other memory that is not needed anymore.
        freeTags(metadata.rootTag);
        return doc;
    };

    CanvasKit.Document.prototype.beginPage = function(width, height, rect) {
        var rPtr = copyRectToWasm(rect);
        return this._beginPage(width, height, rPtr);
      };

    function initAttribute(attr) {
        // Fills out all optional fields with defaults. The emscripten bindings complain if
        // there is a field undefined and it was expecting a float (for example).
        // Use [''] to tell closure not to minify the names 
        attr = attr || {};
        attr['name'] = cacheOrCopyString(attr['name'] || '');
        attr['type'] = cacheOrCopyString(attr['type'] || '');
        attr['owner'] = cacheOrCopyString(attr['owner'] || '');
        if(attr['type'] === 'name') {
            attr['nameValue'] = cacheOrCopyString(attr['nameValue'] || '');
        } else if(attr['type'] === 'int') {
            attr['intValue'] = attr['intValue'] || 0;
        } else if(attr['type'] === 'float') {
            attr['floatValue'] = attr['floatValue'] || 0.0;
        } else if(attr['type'] === 'float-array') {
            var floatValues = new CanvasKit.FloatVector();
            for (var i = 0; i < attr['floatValues'].length; i++) {
                floatValues.push_back(attr['floatValues'][i]);
            }
            attr['floatValues'] = floatValues;
        } else if(attr['type'] === 'node-id-array') {
            var nodeIdArray = new CanvasKit.IntVector();
            for (var i = 0; i < attr['nodeIdArray'].length; i++) {
                nodeIdArray.push_back(attr['nodeIdArray'][i]);
            } 
            attr['nodeIdArray'] = nodeIdArray;
        } else {
            throw new Error('PDFTagAttribute: Unknown type: ' + attr['type']);
        }
        attr['nameValue'] = attr['nameValue'] || '';
        attr['floatValues'] = attr['floatValues'] || new CanvasKit.FloatVector();
        attr['nodeIdArray'] = attr['nodeIdArray'] || new CanvasKit.IntVector();
        attr['intValue'] = attr['intValue'] || 0;
        attr['floatValue'] = attr['floatValue'] || 0.0;


        return attr;
    }

    function freeAttribute(attr) {
        // Free the floatValues array if it exists.
        if (attr.floatValues) {
            attr.floatValues.delete();
        }
        // Free the nodeIdArray if it exists.
        if (attr.nodeIdArray) {
            attr.nodeIdArray.delete();
        }
    }

    function initPDFTag(pdfTag) {
      // Fills out all optional fields with defaults. The emscripten bindings complain if
      // there is a field undefined and it was expecting a float (for example).
      // Use [''] to tell closure not to minify the names
      pdfTag = pdfTag || {};
      pdfTag['id'] = pdfTag['id'] || 0;
      pdfTag['type'] = pdfTag['type'] || 'NonStruct';
      pdfTag['alt'] = pdfTag['alt'] || '';
      pdfTag['lang'] = pdfTag['lang'] || '';
      let children = new CanvasKit.PDFTagVector();
      if (pdfTag['children'] && pdfTag['children'].length > 0) {
          for (let i = 0; i < pdfTag['children'].length; i++) {
              children.push_back(initPDFTag(pdfTag['children'][i]));
          }
      }
      pdfTag['children'] = children;
      let attributes = new CanvasKit.PDFTagAttributeVector();
      if (pdfTag['attributes'] && pdfTag['attributes'].length > 0) {
          for (let i = 0; i < pdfTag['attributes'].length; i++) {
              attributes.push_back(initAttribute(pdfTag['attributes'][i]));
          }
      }
      pdfTag['attributes'] = attributes;
      return pdfTag;
    }

    function freeTags(tag) {
        if (!tag) {
            return;
        }
        // Free the children first, so that we don't have dangling pointers.
        if (tag.children) {
            for (let i = 0; i < tag.children.length; i++) {
                freeTags(tag.children[i]);
            }
        }
        tag.children.delete();
        // Free the attributes.
        if (tag.attributes) {
            for (let i = 0; i < tag.attributes.length; i++) {
                freeAttribute(tag.attributes[i]);
            }
        }
        tag.attributes.delete();
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
        metadata['rootTag'] = initPDFTag(metadata['rootTag']);
        metadata['compressionLevel'] = metadata['compressionLevel'] || CanvasKit.PDFCompressionLevel.Default;
        return metadata;
    };

     // maps string -> malloc'd pointer
    var stringCache = {};

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
      var strLen = lengthBytesUTF8(str) + 1;
      var strPtr = CanvasKit._malloc(strLen);
      stringToUTF8(str, strPtr, strLen);
      stringCache[str] = strPtr;
      return strPtr;
    }
});
