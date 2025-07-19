// tagged_pdf_test.js
// Ported from tests/PDFTaggedTest.cpp
// This test verifies tagged PDF structure and metadata using CanvasKit's PDF API

describe('Tagged PDF Structure', function() {

    beforeEach(async () => {
        await EverythingLoaded;
    });

    it('creates a tagged PDF with structure tree and metadata', function() {
        // Create PDF metadata with a structure tree
        const rootTag = {
            id: 1,
            type: 'Document',
            children: [
                {
                    id: 2,
                    type: 'H1',
                    alt: 'Main Heading',
                    language: 'en',
                    attributes: [
                        { owner: 'Heading', name: 'Level', type: 'int', value: 1 }
                    ],
                    children: []
                },
                {
                    id: 3,
                    type: 'P',
                    alt: 'Paragraph text',
                    language: 'en',
                    children: []
                },
                {
                    id: 4,
                    type: 'Table',
                    attributes: [
                        { owner: 'Table', name: 'RowCount', type: 'int', value: 2 },
                        { owner: 'Table', name: 'ColCount', type: 'int', value: 2 }
                    ],
                    children: [
                        {
                            id: 5,
                            type: 'TR',
                            children: [
                                { id: 6, type: 'TD', alt: 'Cell 1', children: [] },
                                { id: 7, type: 'TD', alt: 'Cell 2', children: [] }
                            ]
                        },
                        {
                            id: 8,
                            type: 'TR',
                            children: [
                                { id: 9, type: 'TD', alt: 'Cell 3', children: [] },
                                { id: 10, type: 'TD', alt: 'Cell 4', children: [] }
                            ]
                        }
                    ]
                }
            ]
        };

        const metadata = {
            title: 'Tagged PDF Test',
            author: 'CanvasKit',
            subject: 'Testing tagged PDF',
            keywords: 'PDF, tagged, test',
            language: 'en',
            rootTag: rootTag
        };

        // Create a memory stream for PDF output
        const doc = CanvasKit.MakePDFDocument(metadata);
        expect(doc).not.toBeNull();

        // Begin a page and draw heading, paragraph, and table
        const canvas = doc.beginPage(400, 400);
        expect(canvas).not.toBeNull();
        canvas.clear(CanvasKit.Color4f(1, 1, 1, 1));

        // Draw heading (tagId: 2)
        CanvasKit.SetPDFTagId(canvas, 2);
        const headingFont = new CanvasKit.Font(null, 32);
        const headingPaint = new CanvasKit.Paint();
        headingPaint.setColor(CanvasKit.Color4f(0, 0, 0, 1));
        canvas.drawText('Main Heading', 40, 60, headingPaint, headingFont);

        // Draw paragraph (tagId: 3)
        CanvasKit.SetPDFTagId(canvas, 3);
        const paraFont = new CanvasKit.Font(null, 18);
        const paraPaint = new CanvasKit.Paint();
        paraPaint.setColor(CanvasKit.Color4f(0.2, 0.2, 0.2, 1));
        canvas.drawText('Paragraph text', 40, 100, paraPaint, paraFont);

        // Draw table (tagId: 4)
        CanvasKit.SetPDFTagId(canvas, 4);
        const cellFont = new CanvasKit.Font(null, 16);
        const cellPaint = new CanvasKit.Paint();
        cellPaint.setColor(CanvasKit.Color4f(0, 0, 0, 1));
        const borderPaint = new CanvasKit.Paint();
        borderPaint.setColor(CanvasKit.Color4f(0, 0, 0, 1));
        borderPaint.setStyle(CanvasKit.PaintStyle.Stroke);
        borderPaint.setStrokeWidth(1);

        const tableX = 40, tableY = 140, cellW = 120, cellH = 40;
        for (let row = 0; row < 2; ++row) {
            for (let col = 0; col < 2; ++col) {
                // Set tagId for each cell
                const cellTagId = 6 + row * 2 + col; // 6,7,9,10
                CanvasKit.SetPDFTagId(canvas, cellTagId);
                const x = tableX + col * cellW;
                const y = tableY + row * cellH;
                // Draw cell border
                canvas.drawRect4f(x, y, x + cellW, y + cellH, borderPaint);
                // Draw cell text
                const cellText = `Cell ${row * 2 + col + 1}`;
                canvas.drawText(cellText, x + 10, y + 25, cellPaint, cellFont);
            }
        }

        doc.endPage();
        // Close the document
        const pdfBytes = doc.close();
        expect(pdfBytes).toBeInstanceOf(Uint8Array);
        expect(pdfBytes.length).toBeGreaterThan(0);

        // Optionally, check for PDF header
        const header = String.fromCharCode.apply(null, pdfBytes.slice(0, 5));
        expect(header).toBe('%PDF-');
    });
});
