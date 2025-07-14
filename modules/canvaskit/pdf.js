CanvasKit._extraInitializations = CanvasKit._extraInitializations || [];
CanvasKit._extraInitializations.push(function() {
    CanvasKit.Document.prototype.beginPage = function(width, height, rect) {
        var rPtr = copyRectToWasm(rect);
        return this._beginPage(width, height, rPtr);
      };

    function initAttribute(attr) {
        // Fills out all optional fields with defaults. The emscripten bindings complain if
        // there is a field undefined and it was expecting a float (for example).
        // Use [''] to tell closure not to minify the names 
        attr = attr || {};
        attr['name'] = attr['name'] || '';
        attr['type'] = attr['type'] || '';
        attr['owner'] = attr['owner'] || '';
        if(attr['type'] === 'name') {
            attr['nameValue'] = attr['nameValue'] || '';
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

    CanvasKit.PDFMetadata = function(metadata) {
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
        metadata['freeTags'] = metadata['freeTags'] || function() {
          if (this.rootTag) {
            freeTags(this.rootTag);
          }
        };
        return metadata;
    };
});
