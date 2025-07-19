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
        const metadata = {
            title: 'Test PDF',
        };
        const pdf = CanvasKit.MakePDFDocument(metadata);
        const canvas = pdf.beginPage(612, 792);
        const paint = new CanvasKit.Paint();
        paint.setColor(CanvasKit.Color(0, 0, 0));
        canvas.drawRRect(CanvasKit.RRectXY(CanvasKit.LTRBRect(10, 10, 100, 100), 10, 10), paint);
        paint.delete();
        pdf.endPage();
        const data =pdf.close();
        pdf.delete();
        expect(data.length).toBeGreaterThan(5);
        var string = new TextDecoder().decode(data).slice(0, 4);
        expect(string).toEqual('%PDF');
    });
    it('stream should be empty if close is called before adding a page', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const metadata = {
            title: 'Test PDF',
        };
        const pdf = CanvasKit.MakePDFDocument(metadata);
        const data = pdf.close();
        pdf.delete();
        expect(data.length).toBe(0);
    });
    it('abort should not throw', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const metadata = {
            title: 'Test PDF',
        };
        const pdf = CanvasKit.MakePDFDocument(metadata);
        const canvas = pdf.beginPage(612, 792);
        const paint = new CanvasKit.Paint();
        paint.setColor(CanvasKit.Color(0, 0, 0));
        canvas.drawRRect(CanvasKit.RRectXY(CanvasKit.LTRBRect(10, 10, 100, 100), 10, 10), paint);
        paint.delete();
        pdf.abort();
        pdf.delete();
    });
});