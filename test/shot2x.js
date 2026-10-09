const { chromium } = require('playwright');
(async () => { const b = await chromium.launch({args:['--allow-file-access-from-files']});
  for (const [n,w] of process.argv.slice(2).map(s=>s.split(':'))) { const W=+w, H=Math.round(W*356/1980);
    const p = await b.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 2 });
    await p.goto('file://' + __dirname + '/render/' + n + '.html'); await p.waitForFunction(()=>document.title==='done',{timeout:10000}); await p.screenshot({ path: __dirname + '/render/' + n + '.png' }); await p.close(); }
  await b.close(); })();
