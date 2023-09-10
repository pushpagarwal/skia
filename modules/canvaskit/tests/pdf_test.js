describe('PDF Behavior', () => {
    let container;

    beforeEach(async () => {
        await EverythingLoaded;
        container = document.createElement('div');
        container.innerHTML = `
            <canvas width=600 height=600 id=test></canvas>
            <canvas width=600 height=600 id=report></canvas>`;
        document.body.appendChild(container);
    });

    afterEach(() => {
        document.body.removeChild(container);
    });

    it('should be able to create a PDF', () => {
        if (!CanvasKit.pdf) {
            console.warn('Skipping test because not compiled with pdf support');
            return;
        }
        const stream = new CanvasKit.DynamicMemoryStream();
        const pdf = CanvasKit.MakePDFDocument(stream);
        const canvas = pdf.beginPage(612, 792);
        const paint = new CanvasKit.Paint();
        paint.setColor(CanvasKit.Color(0, 0, 0));
        canvas.drawRRect(CanvasKit.RRectXY(CanvasKit.LTRBRect(10, 10, 100, 100), 10, 10), paint);
        paint.delete();
        pdf.endPage();
        pdf.close();
        pdf.delete();
        const data = stream.detachAsBytes();
        expect(data.length).toBeGreaterThan(5);
        var string = new TextDecoder().decode(data).slice(0, 4);
        expect(string).toEqual('%PDF');
    });
});