describe('PDF Behavior', () => {
    let container;

    beforeEach(async () => {
        await EverythingLoaded;
    });

    it('should be able to create a PDF', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const stream = new CanvasKit.DynamicMemoryStream();
        const metadata = new CanvasKit.PDFMetadata({
            title: 'Test PDF',
        });
        const pdf = CanvasKit.MakePDFDocument(stream, metadata);
        const canvas = pdf.beginPage(612, 792);
        const paint = new CanvasKit.Paint();
        paint.setColor(CanvasKit.Color(0, 0, 0));
        canvas.drawRRect(CanvasKit.RRectXY(CanvasKit.LTRBRect(10, 10, 100, 100), 10, 10), paint);
        paint.delete();
        pdf.endPage();
        pdf.close();
        pdf.delete();
        const data = stream.detachAsBytes();
        stream.delete();
        expect(data.length).toBeGreaterThan(5);
        var string = new TextDecoder().decode(data).slice(0, 4);
        expect(string).toEqual('%PDF');
    });
    it('stream should be empty if close is called before adding a page', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const stream = new CanvasKit.DynamicMemoryStream();
        const metadata = new CanvasKit.PDFMetadata({
            title: 'Test PDF',
        });
        const pdf = CanvasKit.MakePDFDocument(stream, metadata);
        pdf.close();
        pdf.delete();
        expect(stream.bytesWritten()).toBe(0);
        stream.delete();
    });
    it('abort should cause only headers to be written', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const stream = new CanvasKit.DynamicMemoryStream();
        const metadata = new CanvasKit.PDFMetadata({
            title: 'Test PDF',
        });
        const pdf = CanvasKit.MakePDFDocument(stream, metadata);
        const canvas = pdf.beginPage(612, 792);
        const paint = new CanvasKit.Paint();
        paint.setColor(CanvasKit.Color(0, 0, 0));
        canvas.drawRRect(CanvasKit.RRectXY(CanvasKit.LTRBRect(10, 10, 100, 100), 10, 10), paint);
        paint.delete();
        pdf.abort();
        pdf.delete();
        expect(stream.bytesWritten()).toBeLessThan(256); // PDF header + some metadata
        stream.delete();
    });
});