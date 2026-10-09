const { chromium } = require('playwright');
(async () => { const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 600, height: 960 }, deviceScaleFactor: 1.5 });
  for (const n of process.argv.slice(2)) { await p.goto('file://' + __dirname + '/render/' + n + '.html'); await p.waitForTimeout(200); await p.screenshot({ path: __dirname + '/render/' + n + '.png' }); }
  await b.close(); })();
