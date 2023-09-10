CanvasKit._extraInitializations = CanvasKit._extraInitializations || [];
CanvasKit._extraInitializations.push(function() {
    CanvasKit.Document.prototype.beginPage = function(width, height, rect) {
        var rPtr = copyRectToWasm(rect);
        return this._beginPage(width, height, rPtr);
      };    
});