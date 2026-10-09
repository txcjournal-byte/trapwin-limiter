const { chromium } = require('playwright');
(async () => { const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 480, height: 768 }, deviceScaleFactor: 1 });
  await p.goto('file://' + __dirname + '/render/'+(process.argv[2]||'s1')+'.html'); await p.waitForTimeout(200); await p.screenshot({ path: __dirname + '/render/'+(process.argv[2]||'s1')+'.png' }); await b.close(); })();
