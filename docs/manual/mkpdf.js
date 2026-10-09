const { chromium } = require('playwright');
(async () => { const b = await chromium.launch({args:['--allow-file-access-from-files']}); const p = await b.newPage();
 await p.goto('file://' + process.argv[2], {waitUntil:'networkidle'}); await p.evaluate(()=>document.fonts.ready);
 await p.pdf({ path: process.argv[3], format: 'A4', printBackground: true, preferCSSPageSize: true,
   displayHeaderFooter: true, headerTemplate: '<span></span>',
   footerTemplate: '<div style="width:100%;font-size:7pt;color:#888;padding:0 17mm;display:flex;justify-content:space-between;font-family:sans-serif"><span>TrapWin Limiter · User Manual</span><span><span class="pageNumber"></span> / <span class="totalPages"></span></span></div>' });
 await b.close(); })();
