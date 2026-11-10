// Optional: npm install --no-save playwright; npx playwright install chromium
// Serve build-web, then: node tests/browser_smoke.cjs http://localhost:8000
const fs = require('node:fs');
const {chromium} = require(process.env.PLAYWRIGHT_PATH || 'playwright');
(async () => {
  const browser = await chromium.launch({
    headless: true,
    ...(process.env.CHROME_PATH ? {executablePath: process.env.CHROME_PATH} : {}),
    args: ['--enable-unsafe-swiftshader', '--use-angle=swiftshader']
  });
  try {
    const page = await browser.newPage({viewport: {width: 1100, height: 650}, hasTouch: true});
    const errors = [], messages = [], assets = [];
    page.on('response', r => {if(/\.(wasm|data)$/.test(r.url()) && r.ok()) assets.push(r.url());});
    page.on('pageerror', e => errors.push(String(e)));
    page.on('console', m => {
      messages.push(m.text());
      if (m.type() === 'error') errors.push(m.text());
    });
    const base = (process.argv[2] || 'http://localhost:8000').replace(/\/$/, '');
    await page.goto(base + '/notation-check.html');
    await page.waitForFunction(() => document.querySelector('#status').textContent === '');
    await page.waitForTimeout(1500);
    if (!messages.some(m => m.includes('Lomse rendered D4 bass'))) throw Error('No Lomse render confirmation');
    const box = await page.locator('canvas').boundingBox();
    const crop = {x:box.x+box.width*.04,y:box.y+box.height*.28,width:box.width*.92,height:box.height*.40};
    const first = await page.screenshot({clip:crop});
    await page.mouse.click(250, 550);
    await page.waitForTimeout(300);
    if (!messages.some(m => m.includes('Lomse rendered D4 treble'))) throw Error('Clef switch did not render');
    const clef = await page.screenshot({clip:crop});
    if (first.equals(clef)) throw Error('Clef switch left identical pixels');
    await page.touchscreen.tap(800, 550);
    await page.waitForTimeout(300);
    if (!messages.some(m => m.includes('Lomse rendered C-sharp 4 treble'))) throw Error('Pitch switch did not render');
    fs.mkdirSync('build-browser-artifacts', {recursive:true});
    await page.screenshot({path:'build-browser-artifacts/notation-check.png'});
    await page.setViewportSize({width:740,height:360});
    await page.waitForTimeout(300);
    await page.screenshot({path:'build-browser-artifacts/notation-small.png'});
    if(!assets.some(u=>u.endsWith('.wasm')) || !assets.some(u=>u.endsWith('.data'))) throw Error('WebAssembly or packaged font assets were not loaded');
    if(errors.length) throw Error(errors.join('\n'));
    console.log('PASS: Lomse initialized, pitch/clef controls changed rendered pixels; screenshots captured.');
  } finally { await browser.close(); }
})().catch(e=>{console.error(e);process.exitCode=1;});
