// Optional UI verification in a fresh browser context. Never touches the user's browser records.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const root=fileURLToPath(new URL('../../',import.meta.url));
const runId=process.env.P0_HTML_TEST_RUN||`html-lab-${Date.now()}`;
if(!/^[\w-]+$/.test(runId))throw Error('Use a simple, new P0_HTML_TEST_RUN.');
const output=path.join(root,'Saved/P0Runs',runId);
if(fs.existsSync(output))throw Error('Preserve previous test evidence: choose a new run ID.');
fs.mkdirSync(output,{recursive:true});
const {chromium}=await import(process.env.P0_PLAYWRIGHT_MODULE?pathToFileURL(path.resolve(process.env.P0_PLAYWRIGHT_MODULE)).href:'playwright');
const browser=await chromium.launch({headless:true});
const context=await browser.newContext({viewport:{width:1480,height:1060},reducedMotion:'reduce',acceptDownloads:true});
const page=await context.newPage();
const result={scope:'HTML reader UI only; fresh isolated browser storage, not learner completion or Unreal execution',result:'Running',browser:browser.version(),started:new Date().toISOString(),checks:[],errors:[],networkRequests:[],screenshots:[]};
const source=path.join(root,'learning/html/index.html'),base=pathToFileURL(source).href,key='p0-learning-lab:v1';
const manifest=JSON.parse(fs.readFileSync(path.join(root,'learning/html/manifest.json'),'utf8'));
result.indexSha256=createHash('sha256').update(fs.readFileSync(source)).digest('hex');
const check=(condition,message)=>{if(!condition)throw Error(message);};
const pass=name=>result.checks.push(name);
page.on('pageerror',error=>result.errors.push(error.message));
page.on('request',request=>{if(/^https?:/.test(request.url()))result.networkRequests.push(request.url());});
const documentId='P0/A/G2_01_UNIT.md';
async function go(hash){await page.goto(base+hash);await page.locator('#main h1').waitFor();}
async function state(){return page.evaluate(key=>JSON.parse(localStorage.getItem(key))?.state,key);}
async function shot(name){await page.screenshot({path:path.join(output,name),animations:'disabled'});result.screenshots.push(name);}
async function importPayload(payload){await page.evaluate(()=>document.querySelector('#toast').textContent='');await page.locator('#import-file').setInputFiles({name:'records.json',mimeType:'application/json',buffer:Buffer.from(JSON.stringify(payload))});await page.waitForFunction(()=>document.querySelector('#confirm-dialog').open||document.querySelector('#toast').textContent.startsWith('백업을 읽지 못했습니다.'));}
try {
  await go('');
  check(await page.evaluate(()=>window.P0_LAB.documents.length)===manifest.documents.length,'All source documents expected');
  check(await page.evaluate(()=>window.P0_LAB.documents.filter(doc=>doc.course).length)===27,'27 practice units expected');
  check(await page.locator('#nav-count').textContent()==='0 / 27','Reference checklists must not create personal completion');
  await shot('dashboard-desktop.png');
  pass(`Fresh dashboard: ${manifest.documents.length} documents / 27 practices / zero personal completion`);
  await page.locator('#search').fill('UnitColor');
  check(await page.locator('#main .lesson-card').count()>0,'Body-only symbol must be searchable');
  check(await page.locator('#main .lesson-card').filter({hasText:'준비된 유닛과 표시'}).count()===1,'UnitColor should find its lesson');
  await page.locator('#search').fill('no-such-source-symbol-xyz');
  check(await page.locator('#main .empty').count()===1,'No-result feedback');
  await page.locator('#search').fill('');
  check(await page.locator('#main .lesson-card').count()===manifest.documents.length,'Clear search restores all documents');
  await go('#view=library&group=A');
  check(await page.locator('#main .lesson-card').filter({hasText:'P0/A/G1_ROUTE_NOTE.md'}).count()===1,'Role library must include its reference notes, not only numbered practices');
  pass('Full-text search, no-results and reset');
  await go('#doc='+encodeURIComponent(documentId));
  check(await page.locator('.complete-button').isDisabled(),'Fresh personal completion must be disabled');
  check(await page.locator('.lesson-body input[disabled][checked]').count()>0,'Original checked reference checklist preserved');
  await page.locator('[data-personal-check="0"]').check();await page.locator('[data-personal-check="1"]').check();
  check(await page.locator('.complete-button').isDisabled(),'Two checks are not completion');
  await page.locator('[data-personal-check="2"]').check();
  check(await page.locator('#nav-count').textContent()==='0 / 27','Checkmarks alone must not auto-complete');
  await page.locator('.complete-button').click();
  check(await page.locator('#nav-count').textContent()==='1 / 27','Explicit completion should count exactly once');
  const note='내 테스트 SHA: abc123\n<img src=x onerror="window.noteInjected=true">';
  await page.locator('#lesson-notes').fill(note);await page.locator('[data-action="bookmark"]').click();
  await page.reload();await page.locator('#lesson-notes').waitFor();
  check(await page.locator('#lesson-notes').inputValue()===note,'Note persists across reload');
  check(await page.locator('[data-action="bookmark"]').getAttribute('aria-pressed')==='true','Bookmark persists');
  check(await page.locator('#nav-count').textContent()==='1 / 27','Completion persists');
  check(await page.evaluate(()=>!window.noteInjected && !document.querySelector('img[src="x"]')),'Notes must be inert text');
  await page.locator('[data-personal-check="0"]').uncheck();
  check(await page.locator('#nav-count').textContent()==='0 / 27' && await page.locator('.complete-button').isDisabled(),'Unchecking a prerequisite revokes personal completion');
  await page.locator('[data-personal-check="0"]').check();await page.locator('.complete-button').click();
  await go('#view=home');
  check((await page.getByRole('link',{name:'이어서 학습 →',exact:true}).getAttribute('href')).includes('G2_02_COMBAT.md'),'Continue after completion should advance to next pending lesson');
  await go('#doc='+encodeURIComponent(documentId));
  pass('Completion gate, explicit action, reload, uncheck revocation, safe notes and bookmark');
  await shot('lesson-desktop.png');
  await page.getByRole('link',{name:/다음 실습/}).click();await page.waitForURL(/G2_02_COMBAT/);
  await page.goBack();await page.waitForURL(/G2_01_UNIT/);
  check(await page.locator('#lesson-notes').inputValue()===note,'Back navigation retains note');
  await go('#doc=P0%2FCOMMON.md&anchor=g0-replay');
  check(await page.locator('#g0-replay').count()===1,'Explicit source anchor must exist');
  const firstToc=page.locator('[data-anchor]').first();const headingId=await firstToc.getAttribute('data-anchor');await firstToc.click();
  await page.waitForFunction(id=>new URLSearchParams(location.hash.slice(1)).get('anchor')===id && document.getElementById(id).getBoundingClientRect().top>=76,headingId);
  check(await page.evaluate(id=>document.getElementById(id).getBoundingClientRect().top>=76,headingId),'Sticky bar must not cover targeted heading');
  pass('Previous/next, browser Back and source/heading deep links');
  await go('#doc=P0%2FG3_INTEGRATION.md');
  await page.evaluate(()=>{window.copied='';Object.defineProperty(navigator,'clipboard',{configurable:true,value:{writeText:async text=>window.copied=text}});});
  const expectedCopy=await page.locator('pre code').first().textContent();await page.locator('.copy-button').first().click();
  check(await page.evaluate(()=>window.copied)===expectedCopy,'Code copy must retain exact text');
  pass('Code copy button passes exact source text to clipboard');
  await go('#view=progress');
  const downloadEvent=page.waitForEvent('download');await page.locator('[data-action="export"]').click();const download=await downloadEvent;
  const exportedPath=path.join(output,'exported-records.json');await download.saveAs(exportedPath);const exported=JSON.parse(fs.readFileSync(exportedPath,'utf8'));
  check(exported.app==='p0-learning-lab'&&exported.version===1&&exported.state.notes[documentId]===note,'Export preserves personal notes and format');
  const before=await state();
  await importPayload({...exported,version:99});check(!(await page.locator('#confirm-dialog').evaluate(el=>el.open)),'Wrong version must not show import confirmation');check(JSON.stringify(await state())===JSON.stringify(before),'Wrong version must not mutate records');
  const bad=structuredClone(exported);bad.state.checks[documentId]=[false,false,false];
  await importPayload(bad);check(!(await page.locator('#confirm-dialog').evaluate(el=>el.open)),'Inconsistent completion must be rejected');check(JSON.stringify(await state())===JSON.stringify(before),'Invalid completion leaves records unchanged');
  const valid=structuredClone(exported);valid.state.notes[documentId]='복원 확인 메모';
  await importPayload(valid);check(await page.locator('#confirm-dialog').evaluate(el=>el.open),'Valid backup requires confirmation');await page.locator('#cancel-confirm').click();check((await state()).notes[documentId]===note,'Cancel import preserves note');
  await importPayload(valid);await page.locator('#accept-confirm').click();check((await state()).notes[documentId]==='복원 확인 메모','Confirmed import restores note');
  await page.locator('[data-action="reset"]').click();await page.locator('#cancel-confirm').click();check(Object.keys((await state()).done).length===1,'Cancel reset preserves completion');
  await page.locator('[data-action="reset"]').click();await page.locator('#accept-confirm').click();check(Object.keys((await state()).notes).length===0&&Object.keys((await state()).done).length===0,'Confirmed reset clears personal records');
  pass('JSON export/import, invalid backup rejection, cancellation and explicit reset');
  await page.evaluate(()=>{window.print=()=>window.dispatchEvent(new Event('beforeprint'));});
  await page.locator('[data-action="print-all"]').click();check(await page.locator('#print-area .print-document').count()===27,'Whole-practice print includes all 27 lessons');
  await page.emulateMedia({media:'print'});check(await page.locator('.app').isHidden(),'Printing should hide application chrome');
  await page.emulateMedia({media:'screen'});await page.evaluate(()=>window.dispatchEvent(new Event('afterprint')));
  await go('#doc='+encodeURIComponent(documentId));await page.evaluate(()=>{window.print=()=>window.dispatchEvent(new Event('beforeprint'));});await page.locator('[data-action="print"]').click();
  check(await page.locator('#print-area .print-document').count()===1,'Current document print includes one document');
  await page.pdf({path:path.join(output,'current-lesson.pdf'),format:'A4',printBackground:true});
  await page.evaluate(()=>window.dispatchEvent(new Event('afterprint')));check(await page.locator('#print-area').isHidden(),'Print cleanup restores normal view');
  pass('Current/whole-practice print, PDF generation and cleanup');

  const allIds=new Map(),allLinks=[];
  for(const width of [1480,390]){
    await page.setViewportSize({width,height:width===1480?1060:844});
    for(const doc of manifest.documents){
      await go('#doc='+encodeURIComponent(doc.source));
      const inspection=await page.evaluate(()=>{
        const doc=window.P0_LAB.documents.find(d=>new URLSearchParams(location.hash.slice(1)).get('doc')===d.source);
        const template=document.createElement('template');template.innerHTML=doc.body;template.content.querySelector('h1')?.remove();
        const normalize=text=>text.replace(/\s+/g,' ').trim();const actual=normalize(document.querySelector('.lesson-body').textContent);
        const missing=[...template.content.querySelectorAll('p,li,table,h2,h3,pre')].map(el=>normalize(el.textContent)).filter(text=>text&&!actual.includes(text));
        return {overflow:document.documentElement.scrollWidth>innerWidth+1,missing,links:[...document.querySelectorAll('a[href],img[src]')].map(el=>el.href||el.src)};
      });
      check(!inspection.overflow,`App overflow at ${width}: ${doc.source}`);check(inspection.missing.length===0,`Source content lost: ${doc.source} ${inspection.missing[0]}`);
      if(width===1480)allLinks.push(...inspection.links);
    }
    const staticFiles=[path.join(root,'learning/html/index.html'),...manifest.documents.map(doc=>path.join(root,'learning',doc.output))];
    for(const file of staticFiles){
      await page.goto(pathToFileURL(file).href);
      const inspection=await page.evaluate(()=>({overflow:document.documentElement.scrollWidth>innerWidth+1,ids:[...document.querySelectorAll('[id]')].map(el=>el.id),links:[...document.querySelectorAll('a[href],img[src]')].map(el=>el.href||el.src)}));
      check(!inspection.overflow,`Static overflow at ${width}: ${file}`);check(new Set(inspection.ids).size===inspection.ids.length,`Duplicate IDs: ${file}`);
      if(width===1480){allIds.set(fileURLToPath(pathToFileURL(file)),new Set(inspection.ids));allLinks.push(...inspection.links);}
    }
  }
  let fileLinks=0;
  for(const href of allLinks){
    if(!href.startsWith('file:'))continue;const uri=new URL(href),target=fileURLToPath(uri);check(fs.existsSync(target),`Missing target: ${href}`);fileLinks++;
    if(uri.hash&&!uri.hash.startsWith('#doc=')&&!uri.hash.startsWith('#view=')&&allIds.has(target))check(allIds.get(target).has(decodeURIComponent(uri.hash.slice(1))),`Missing HTML anchor: ${href}`);
  }
  result.localLinks=fileLinks;result.appDocuments=manifest.documents.length;result.standalonePages=manifest.documents.length+1;result.widths=[1480,390];
  pass(`${manifest.documents.length} app documents and ${manifest.documents.length+1} independent pages at desktop/mobile widths; source body content and local links`);
  await go('#doc='+encodeURIComponent(documentId));await shot('lesson-mobile.png');
  await page.locator('#menu-toggle').click();check(await page.locator('#menu-toggle').getAttribute('aria-expanded')==='true','Mobile drawer open');
  check(await page.evaluate(()=>document.querySelector('#sidebar').contains(document.activeElement)),'Mobile menu receives focus');
  await shot('menu-mobile.png');await page.keyboard.press('Escape');check(await page.locator('#menu-toggle').getAttribute('aria-expanded')==='false','Escape closes drawer');check(await page.evaluate(()=>document.activeElement.id==='menu-toggle'),'Close restores focus');
  await page.locator('[data-action="personal"]').click();check(await page.locator('.reading-rail').evaluate(el=>el.getBoundingClientRect().top<100),'Mobile personal-record shortcut');
  await go('#view=home');await shot('dashboard-mobile.png');
  pass('Mobile drawer, focus/Escape handling, personal-record shortcut');
  const blocked=await browser.newContext({viewport:{width:1100,height:850}});await blocked.addInitScript(()=>{Storage.prototype.setItem=function(){throw new DOMException('Full','QuotaExceededError');};});const blockedPage=await blocked.newPage();await blockedPage.goto(base+'#doc='+encodeURIComponent(documentId));await blockedPage.locator('#lesson-notes').fill('저장 거절 상황에서도 메모 유지');
  check((await blockedPage.locator('#save-status').textContent()).includes('자동 저장을 사용할 수 없습니다'),'Storage failure must be visible');check(await blockedPage.locator('#lesson-notes').inputValue()==='저장 거절 상황에서도 메모 유지','Storage failure keeps current input');await blocked.close();
  pass('Storage failure feedback without losing current input');
  check(!result.errors.length&&!result.networkRequests.length,'No script errors or external requests');result.result='Pass';
}catch(error){result.result='Fail';result.error=error.stack;}
finally{result.finished=new Date().toISOString();await browser.close();fs.writeFileSync(path.join(output,'result.json'),JSON.stringify(result,null,2));}
console.log(JSON.stringify(result,null,2));if(result.result!=='Pass')process.exitCode=1;
