// Offline UI. Personal records never write to Markdown, Git, or Unreal.
(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const esc = value => String(value ?? '').replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;').replaceAll('"','&quot;').replaceAll("'",'&#39;');
  const toDoc = (id, anchor='') => '#doc=' + encodeURIComponent(id) + (anchor ? '&anchor=' + encodeURIComponent(anchor) : '');
  let toastTimer;
  function toast(message) { $('toast').textContent=message; $('toast').classList.add('visible'); clearTimeout(toastTimer); toastTimer=setTimeout(()=>$('toast').classList.remove('visible'),3500); }
  function closeMenu(focus=false) { $('sidebar').classList.remove('open'); document.querySelector('.backdrop').hidden=true; $('menu-toggle').setAttribute('aria-expanded','false'); if(innerWidth<=900)$('sidebar').inert=true; if(focus)$('menu-toggle').focus(); }
  function toggleMenu() { const open=!$('sidebar').classList.contains('open'); if(!open){closeMenu(true);return;} $('sidebar').inert=false; $('sidebar').classList.add('open'); document.querySelector('.backdrop').hidden=false; $('menu-toggle').setAttribute('aria-expanded','true'); $('sidebar').querySelector('a').focus(); }
  $('menu-toggle').addEventListener('click',toggleMenu);
  document.querySelector('.backdrop').addEventListener('click',()=>closeMenu(true));
  const resizeMenu=()=>{if(innerWidth>900){$('sidebar').inert=false;closeMenu();}else if(!$('sidebar').classList.contains('open'))$('sidebar').inert=true;};
  window.addEventListener('resize',resizeMenu); resizeMenu();
  document.addEventListener('keydown',event=>{
    if(event.key==='Escape' && $('sidebar').classList.contains('open')){event.preventDefault();closeMenu(true);}
    if(event.key==='Tab' && innerWidth<=900 && $('sidebar').classList.contains('open')){
      const candidates=[...$('sidebar').querySelectorAll('a,summary,button')].filter(el=>el.getClientRects().length);
      const first=candidates[0],last=candidates.at(-1);
      if(event.shiftKey && document.activeElement===first){event.preventDefault();last.focus();}
      else if(!event.shiftKey && document.activeElement===last){event.preventDefault();first.focus();}
    }
  });
  function addCopyButtons(container) {
    container.querySelectorAll('pre').forEach(pre=>{
      if(pre.querySelector('.copy-button'))return;
      const button=document.createElement('button');button.type='button';button.className='copy-button';button.textContent='코드 복사';
      button.addEventListener('click',async()=>{
        const text=pre.querySelector('code')?.textContent || pre.textContent.replace(/코드 복사$/,'');
        try { if(!navigator.clipboard?.writeText)throw Error('Clipboard unavailable'); await navigator.clipboard.writeText(text);toast('코드를 복사했습니다.'); }
        catch { const area=document.createElement('textarea');area.value=text;area.style.cssText='position:fixed;left:-9999px';document.body.append(area);area.select();const copied=document.execCommand('copy');area.remove();toast(copied?'코드를 복사했습니다.':'복사를 허용하지 않는 브라우저입니다. 코드를 선택해 복사해 주세요.'); }
      });pre.append(button);
    });
  }
  if(!window.P0_LAB){document.querySelector('[data-action="print"]')?.addEventListener('click',()=>window.print());addCopyButtons($('main'));return;}

  const {documents:docs,courses}=window.P0_LAB;
  const byId=new Map(docs.map(doc=>[doc.source,doc]));
  const lessonDocs=docs.filter(doc=>doc.course);
  const courseDocs=id=>lessonDocs.filter(doc=>doc.course===id).sort((a,b)=>a.gate.localeCompare(b.gate,undefined,{numeric:true})||a.source.localeCompare(b.source));
  const orderedLessons=courses.flatMap(course=>courseDocs(course.id));
  const KEY='p0-learning-lab:v1',APP='p0-learning-lab',MAX_NOTE=20000;
  const blank=()=>({last:null,notes:{},checks:{},done:{},bookmarks:{}});
  let storageBlocked=false, storageMessage='', pendingConfirm=null, current={view:'home'}, observer=null;
  function validateState(value) {
    if(!value || typeof value!=='object' || Array.isArray(value))throw Error('기록 형식이 올바르지 않습니다.');
    const result=blank(),allowed=new Set(Object.keys(result));
    if(Object.keys(value).some(key=>!allowed.has(key)))throw Error('알 수 없는 기록 항목입니다.');
    if(value.last!==null && value.last!==undefined && !byId.has(value.last))throw Error('알 수 없는 최근 문서입니다.');
    result.last=value.last || null;
    for(const field of ['notes','checks','done','bookmarks']){
      const map=value[field] ?? {};
      if(typeof map!=='object' || !map || Array.isArray(map))throw Error('기록 항목의 형식이 올바르지 않습니다.');
      for(const [id,item] of Object.entries(map)){
        if(!byId.has(id))throw Error('이 교재에 없는 문서 기록입니다.');
        if(field==='notes'){if(typeof item!=='string'||item.length>MAX_NOTE)throw Error('메모는 문서당 20,000자 이하여야 합니다.');result.notes[id]=item;}
        else if(field==='checks'){if(!byId.get(id).course||!Array.isArray(item)||item.length!==3||item.some(v=>typeof v!=='boolean'))throw Error('실습 체크 항목이 올바르지 않습니다.');result.checks[id]=[...item];}
        else {if(typeof item!=='boolean'||(field==='done'&&!byId.get(id).course))throw Error('완료·북마크 형식이 올바르지 않습니다.');if(item)result[field][id]=true;}
      }
    }
    for(const id of Object.keys(result.done))if(!result.checks[id]?.every(Boolean))throw Error('확인 항목이 없는 완료 기록입니다.');
    return result;
  }
  let state=blank();
  try {const raw=localStorage.getItem(KEY);if(raw){const saved=JSON.parse(raw);if(saved.app!==APP||saved.version!==1)throw Error('지원하지 않는 저장 버전');state=validateState(saved.state);}}
  catch {storageBlocked=true;storageMessage='기존 기록을 읽을 수 없어 원본을 유지합니다. 이번 탭의 기록은 백업으로 보관해 주세요.';}
  function persist() {
    let saved=false;
    if(!storageBlocked)try{localStorage.setItem(KEY,JSON.stringify({app:APP,version:1,state}));saved=true;storageMessage='';}catch{storageMessage='자동 저장을 사용할 수 없습니다. 기록 백업으로 현재 내용을 보관해 주세요.';}
    const status=$('save-status');if(status)status.textContent=saved?'이 브라우저에 저장됨':storageMessage;
    const warning=$('storage-warning');if(warning){warning.hidden=!storageMessage;warning.textContent=storageMessage;}
    return saved;
  }
  const count=(course='')=>lessonDocs.filter(doc=>(!course||doc.course===course)&&state.done[doc.source]).length;
  const checksFor=id=>state.checks[id]||[false,false,false];
  const courseName=id=>courses.find(course=>course.id===id)?.title || '참고 문서';
  function updateSidebar(){
    $('nav-count').textContent=`${count()} / ${lessonDocs.length}`;
    const percent=Math.round(count()/lessonDocs.length*100);$('side-percent').textContent=`${percent}%`;$('side-progress').style.width=`${percent}%`;
    document.querySelectorAll('[data-nav-doc]').forEach(a=>{a.classList.toggle('active',current.doc===a.dataset.navDoc);a.querySelector('.nav-done').textContent=state.done[a.dataset.navDoc]?'✓':'';if(current.doc===a.dataset.navDoc){a.setAttribute('aria-current','page');a.closest('details').open=true;}else a.removeAttribute('aria-current');});
    document.querySelectorAll('[data-view]').forEach(a=>{a.classList.toggle('active',!current.doc&&a.dataset.view===current.view);});
  }
  const footer=()=>'<footer class="page-footer"><span>P0 LEARNING LAB · 직접 만들고, 설명하고, 검증하기</span><span>원문·SHA·공식 상태는 그대로 유지합니다.</span></footer>';
  const metadata=doc=>`${doc.course?`${courseName(doc.course)} · ${doc.gate}`:doc.source.startsWith('P1/')||doc.source.startsWith('P2/')?'기존 계획':'공통 · 참고 자료'}`;
  function tile(doc){return `<a class="lesson-card" href="${toDoc(doc.source)}"><div class="card-meta"><span>${esc(metadata(doc))}</span><span>${state.done[doc.source]?'✓ 내 실습 확인':state.bookmarks[doc.source]?'★ 북마크':'↗'}</span></div><h3>${esc(doc.shortTitle)}</h3>${doc.goal?`<p>${esc(doc.goal.slice(0,130))}${doc.goal.length>130?'…':''}</p>`:''}<small>${esc(doc.source)}</small></a>`;}
  function heroArt(){const cells=Array.from({length:18},(_,i)=>`<rect x="${i%6*29}" y="${Math.floor(i/6)*29}" width="25" height="25" rx="3" fill="${[1,5,7,10,14].includes(i)?['#bde7cc','#9dc6dd','#c6aae4'][i%3]:'#274f50'}" stroke="#51847a"/>`).join('');return `<svg class="hero-art" viewBox="0 0 370 290" aria-hidden="true"><g transform="translate(102 48) rotate(-9 85 110)"><rect x="-18" y="-16" width="208" height="241" rx="16" fill="none" stroke="#8dbf9f" stroke-dasharray="5 7"/><g>${cells}</g><path d="M-16 99H187" stroke="#bde7cc" stroke-width="2"/><g transform="translate(0 121)">${cells}</g></g><rect x="12" y="75" width="102" height="37" rx="7" fill="#173f3e" stroke="#749e8d"/><text x="28" y="99">A · COMBAT</text><rect x="236" y="187" width="117" height="37" rx="7" fill="#bde7cc"/><text x="252" y="211" fill="#173f3e">B · BOARD</text><circle cx="302" cy="51" r="6" fill="#e6a475"/></svg>`;}
  function home(){
    const last=byId.get(state.last),lastIndex=orderedLessons.findIndex(doc=>doc.source===state.last);
    const next=last&&!state.done[last.source]?last:orderedLessons.slice(lastIndex+1).find(doc=>!state.done[doc.source])||orderedLessons.find(doc=>!state.done[doc.source])||last||orderedLessons[0];
    return `<section class="hero"><div><p class="eyebrow">P0 REFERENCE · BUILD TOGETHER</p><h1>각자의 기능을 만들고,<br><em>하나의 게임으로 연결하세요.</em></h1><p>16종 기본 공격 · 10웨이브 · 2인 협동.<br>공통 계약부터 역할별 구현과 게이트 통합까지.</p><div class="button-row"><a class="btn mint" href="${toDoc(next.source)}">${state.last?'이어서 학습':'첫 실습 시작'} <span>→</span></a><a class="btn hero-secondary" href="${toDoc('P0/README.md')}">출발점과 읽는 순서</a></div></div>${heroArt()}</section>
    <section class="metrics" aria-label="교재와 개인 기록"><div><strong>${docs.length}</strong><span>전체 학습 문서</span></div><div><strong>${lessonDocs.length}</strong><span>역할별 실습·통합</span></div><div><strong>${count()}<small> / ${lessonDocs.length}</small></strong><span>내 실습 확인</span></div><div><strong>${Object.keys(state.bookmarks).length}</strong><span>나의 북마크</span></div></section>
    <div class="section-heading"><h2>담당 역할에서 시작하세요</h2><span>공통 계약 → 작은 구현 → 함께 통합</span></div>
    <div class="course-grid">${courses.map((course,i)=>`<section class="course-card ${course.color}"><div class="card-meta"><span>0${i+1} / ${course.title}</span><span>${courseDocs(course.id).length}개 실습</span></div><h2>${course.subtitle}</h2><p>${course.description}</p><div class="progress-label"><span>내 실습 확인</span><b>${count(course.id)} / ${courseDocs(course.id).length}</b></div><div class="progress-track"><span style="width:${count(course.id)/courseDocs(course.id).length*100}%"></span></div><a href="#view=course&course=${course.id}">학습 순서 보기 <span>↗</span></a></section>`).join('')}</div>
    <div class="section-heading"><h2>지금 이어 볼 문서</h2><a href="#view=library">전체 문서 →</a></div><div class="lesson-grid">${tile(next)}${tile(byId.get('P0/COMMON.md'))}</div>
    <div class="callout"><b>참고 제작과 나의 학습을 구분합니다.</b><p>원문에는 참고 구현의 Verified·Draft와 실제 학습자의 Planned가 기록되어 있습니다. 여기서 체크한 진도와 메모는 나의 브라우저 기록이며 원문·제품 검수·Git 브랜치를 변경하지 않습니다. Android G4는 원문의 미검증 범위를 확인하세요.</p></div>${footer()}`;
  }
  function coursePage(id){const course=courses.find(item=>item.id===id);if(!course)return home();return `<header class="page-heading"><p class="eyebrow">P0 · ${esc(course.title)}</p><h1>${course.subtitle}</h1><p>${course.description}</p><div class="tags"><span>${courseDocs(id).length}개 실습</span><span>내 실습 확인 ${count(id)}개</span><a href="${toDoc(id==='integration'?'P0/COMMON.md':`P0/${id}/README.md`)}">원문의 순서·선행 조건 ↗</a></div></header><div class="lesson-grid">${courseDocs(id).map(tile).join('')}</div>${footer()}`;}
  function libraryPage(){
    const words=(current.q||'').trim().toLocaleLowerCase().split(/\s+/).filter(Boolean),group=current.group||'all';
    const matches=docs.filter(doc=> (group==='all'||group==='reference'&&!doc.course||['A','B'].includes(group)&&doc.source.startsWith(`P0/${group}/`)||doc.course===group)&&words.every(word=>`${doc.title} ${doc.source} ${doc.text}`.toLocaleLowerCase().includes(word)));
    return `<header class="page-heading"><p class="eyebrow">REFERENCE LIBRARY</p><h1>${words.length?'본문 검색 결과':'전체 학습 문서'}</h1><p>제목·본문·코드·경로에서 검색합니다. 수업을 열어 원문과 검수 범위를 함께 확인하세요.</p></header><div class="filter-row">${[['all','전체'],['A','개발자 A'],['B','개발자 B'],['integration','통합'],['reference','공통·증거·계획']].map(([id,label])=>`<a class="chip${group===id?' selected':''}" href="#view=library&group=${id}${current.q?'&q='+encodeURIComponent(current.q):''}"${group===id?' aria-current="true"':''}>${label}</a>`).join('')}</div><p class="result-count" role="status">${matches.length}개 문서${current.q?' · “'+esc(current.q)+'”':''}</p><div class="lesson-grid">${matches.map(tile).join('')}</div>${matches.length?'':'<div class="empty"><h2>일치하는 문서가 없습니다.</h2><p>검색어를 줄이거나 다른 단어를 입력해 보세요.</p><a class="btn secondary" href="#view=library">검색 초기화</a></div>'}${footer()}`;
  }
  function panels(html){
    const template=document.createElement('template');template.innerHTML=html;template.content.querySelector('h1')?.remove();
    const result=document.createElement('div');let panel=null,number=0;
    [...template.content.childNodes].forEach(node=>{
      if(node.nodeType===3&&!node.textContent.trim())return;
      if(node.nodeName==='H2'||!panel){panel=document.createElement('section');panel.className='content-panel';result.append(panel);if(node.nodeName==='H2'){const label=document.createElement('p');label.className='eyebrow';label.textContent=`${String(++number).padStart(2,'0')} / ${/코드|설정|작성/.test(node.textContent)?'BUILD':/실행|검증|완료|실패/.test(node.textContent)?'VERIFY':/이해|변형/.test(node.textContent)?'EXPLAIN':'READ'}`;panel.append(label);}}
      panel.append(node);
    });return result.innerHTML;
  }
  const personalCriteria=['이 수업의 선행 계약과 상태 소유권을 설명할 수 있다.','내 환경에서 구현하고 정상·실패 사례를 직접 확인했다.','내 시작·완료 SHA, 실행 증거와 남은 문제를 기록했다.'];
  function documentPage(doc){
    const sequence=doc.course?courseDocs(doc.course):[],pos=sequence.findIndex(item=>item.source===doc.source),prev=sequence[pos-1],next=sequence[pos+1];
    const toc=doc.headings.filter(h=>h.depth>1&&h.depth<4).map(h=>`<a class="level-${h.depth}" href="${toDoc(doc.source,h.id)}" data-anchor="${esc(h.id)}">${h.label}</a>`).join('');
    return `<header class="page-heading lesson-heading"><p class="eyebrow">${esc(metadata(doc))} ${pos>=0?`/ ${String(pos+1).padStart(2,'0')} OF ${sequence.length}`:''}</p><h1 id="${esc(doc.headings.find(h=>h.depth===1)?.id||'document-title')}">${esc(doc.shortTitle)}</h1>${doc.goal?`<p class="goal">${esc(doc.goal)}</p>`:''}<div class="tags"><span>${esc(doc.source)}</span>${doc.gate==='G4'?'<span class="tag-warning">G4 · 원문 Draft</span>':''}<span>원문 상태·SHA 보존</span></div><div class="reader-tools"><button class="btn secondary small" data-action="bookmark" aria-pressed="${!!state.bookmarks[doc.source]}">${state.bookmarks[doc.source]?'★ 북마크됨':'☆ 북마크'}</button><button class="btn secondary small" data-action="print">현재 문서 인쇄</button><button class="btn secondary small" data-action="personal">체크·메모 ↓</button><a href="${esc(doc.standalone)}">개별 HTML ↗</a><a href="${esc(doc.original)}">Markdown 원본 ↗</a></div></header>
    <div class="reader-grid"><article class="lesson-body">${panels(doc.body)}<nav class="lesson-pagination" aria-label="이전 다음 실습">${prev?`<a href="${toDoc(prev.source)}"><small>← 이전 실습</small>${esc(prev.shortTitle)}</a>`:'<span></span>'}${next?`<a href="${toDoc(next.source)}"><small>다음 실습 →</small>${esc(next.shortTitle)}</a>`:doc.course?'<a href="#view=progress">나의 학습 기록 →</a>':'<span></span>'}</nav></article>
    <aside class="reading-rail">${doc.course?`<section class="panel personal-checks"><p class="eyebrow">MY PRACTICE</p><h2>내 실습 확인</h2><p class="small-note">원문의 체크 표시는 참고 제작 기록입니다. 아래는 내가 직접 수행한 항목만 체크하세요.</p>${personalCriteria.map((label,i)=>`<label class="check-item"><input type="checkbox" data-personal-check="${i}" ${checksFor(doc.source)[i]?'checked':''}><span>${label}</span></label>`).join('')}<button class="btn complete-button" data-action="complete" ${!state.done[doc.source]&&!checksFor(doc.source).every(Boolean)?'disabled':''}>${state.done[doc.source]?'✓ 내 실습 확인됨 · 되돌리기':'내 실습 확인 기록'}</button><p class="small-note" id="complete-help">세 항목 확인 후 기록합니다. 공식 Verified 상태를 변경하지 않습니다.</p></section>`:''}
    <section class="panel"><h2><label for="lesson-notes">나의 제작 메모</label></h2><textarea id="lesson-notes" class="notepad" maxlength="${MAX_NOTE}" placeholder="내 SHA / 실행 환경 / 막힌 지점 / 해결 과정 / 남은 문제">${esc(state.notes[doc.source]||'')}</textarea><p class="small-note" id="save-status" role="status">${esc(storageMessage||'입력할 때 자동 저장 · 최대 20,000자')}</p></section><section class="panel toc"><h2>이 문서의 목차</h2>${toc||'<p class="small-note">하위 제목이 없는 문서입니다.</p>'}</section></aside></div>${footer()}`;
  }
  function progressPage(){const bookmarked=docs.filter(doc=>state.bookmarks[doc.source]);const noted=docs.filter(doc=>state.notes[doc.source]?.trim());return `<header class="page-heading"><p class="eyebrow">MY LEARNING</p><h1>나의 학습 기록</h1><p>이 시작 페이지에서 남긴 개인 기록입니다. 공식 문서의 제작·학습 상태와는 별개입니다.</p></header><div class="course-grid">${courses.map(course=>`<section class="panel"><p class="eyebrow">${course.title}</p><h2>${count(course.id)} <small>/ ${courseDocs(course.id).length} 실습 확인</small></h2><div class="progress-track"><span style="width:${count(course.id)/courseDocs(course.id).length*100}%"></span></div><a class="text-link" href="#view=course&course=${course.id}">과정 이어 보기 →</a></section>`).join('')}</div><section class="panel record-tools"><h2>기록 보관과 이동</h2><p>메모·체크·북마크는 이 브라우저에 저장합니다. 다른 브라우저나 파일 경로로 옮길 때는 JSON 백업을 내보내고 새 학습실에서 불러오세요.</p><div class="button-row"><button class="btn" data-action="export">기록 백업</button><button class="btn secondary" data-action="import">백업 불러오기</button><button class="btn secondary" data-action="print-all">전체 ${lessonDocs.length}개 실습 인쇄</button><button class="btn text-danger" data-action="reset">개인 기록 초기화</button></div></section><div class="section-heading"><h2>북마크</h2><span>${bookmarked.length}개</span></div><div class="lesson-grid">${bookmarked.map(tile).join('')||'<p class="empty-inline">다시 볼 문서에서 ☆ 북마크를 눌러 주세요.</p>'}</div><div class="section-heading"><h2>메모를 남긴 문서</h2><span>${noted.length}개</span></div><div class="lesson-grid">${noted.map(tile).join('')||'<p class="empty-inline">수업 화면에서 메모를 남기면 여기에 모입니다.</p>'}</div>${footer()}`;}
  function observeToc(){observer?.disconnect();const headings=[...$('main').querySelectorAll('.lesson-body h2')];if(!headings.length)return;observer=new IntersectionObserver(entries=>{const visible=entries.filter(entry=>entry.isIntersecting).sort((a,b)=>a.boundingClientRect.top-b.boundingClientRect.top);if(!visible.length)return;document.querySelectorAll('[data-anchor]').forEach(a=>a.classList.toggle('active',a.dataset.anchor===visible[0].target.id));},{rootMargin:'-80px 0px -55% 0px'});headings.forEach(el=>observer.observe(el));}
  function render({scroll=true,focus=false}={}){
    const doc=current.doc&&byId.get(current.doc);$('breadcrumb').textContent=doc?metadata(doc):current.view==='progress'?'나의 학습 기록':current.view==='library'?'전체 문서':current.view==='course'?courseName(current.course):'대시보드';
    $('main').innerHTML=`<div id="storage-warning" class="callout warning" role="status" ${storageMessage?'':'hidden'}>${esc(storageMessage)}</div>`+(doc?documentPage(doc):current.view==='progress'?progressPage():current.view==='library'?libraryPage():current.view==='course'?coursePage(current.course):home());
    document.title=(doc?doc.shortTitle:$('breadcrumb').textContent)+' · P0 Learning Lab';updateSidebar();addCopyButtons($('main'));observeToc();
    if(scroll){if(current.anchor&&doc){const target=document.getElementById(current.anchor);target?.scrollIntoView({block:'start'});}else window.scrollTo(0,0);}
    if(focus)$('main').focus({preventScroll:true});
  }
  function readRoute(focus=false){
    const params=new URLSearchParams(location.hash.slice(1));const requested=params.get('doc');
    current=requested&&byId.has(requested)?{doc:requested,anchor:params.get('anchor')||''}:{view:['home','progress','library','course'].includes(params.get('view'))?params.get('view'):'home',course:params.get('course')||'',group:params.get('group')||'all',q:params.get('q')||''};
    $('search').value=current.q||'';closeMenu();if(current.doc){state.last=current.doc;persist();}render({focus});
  }
  const confirmation=(title,description,action)=>{pendingConfirm=action;$('confirm-title').textContent=title;$('confirm-description').textContent=description;$('confirm-dialog').showModal();$('cancel-confirm').focus();};
  $('cancel-confirm').onclick=()=>{pendingConfirm=null;$('confirm-dialog').close();};
  $('confirm-dialog').addEventListener('cancel',()=>pendingConfirm=null);
  $('accept-confirm').onclick=()=>{const action=pendingConfirm;pendingConfirm=null;$('confirm-dialog').close();action?.();};
  function exportState(){const blob=new Blob([JSON.stringify({app:APP,version:1,exportedAt:new Date().toISOString(),state},null,2)],{type:'application/json;charset=utf-8'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=`p0-learning-records-${new Date().toISOString().slice(0,10)}.json`;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);toast('개인 학습 기록을 백업했습니다.');}
  $('import-file').addEventListener('change',async event=>{
    const file=event.target.files[0];event.target.value='';if(!file)return;
    try{if(file.size>5000000)throw Error('5MB 이하 파일을 선택하세요.');const data=JSON.parse(await file.text());if(data.app!==APP||data.version!==1)throw Error('P0 Learning Lab의 버전 1 백업이 아닙니다.');const imported=validateState(data.state);confirmation('백업 기록을 불러올까요?',`실습 확인 ${Object.keys(imported.done).length}개, 메모 ${Object.keys(imported.notes).length}개를 읽었습니다. 현재 개인 기록을 이 백업으로 바꿉니다.`,()=>{state=imported;storageBlocked=false;persist();location.hash='view=progress';render();toast('백업 기록을 복원했습니다.');});}
    catch(error){toast('백업을 읽지 못했습니다. '+error.message);}
  });
  function preparePrint(all=false){
    const selected=all?orderedLessons:current.doc?[byId.get(current.doc)]:[];
    if(!selected.length){$('print-area').innerHTML='';return;}
    $('print-area').innerHTML=selected.map(doc=>`<article class="print-document">${doc.body}<section><h2>나의 제작 메모</h2><p class="print-note">${esc(state.notes[doc.source]||'기록 없음')}</p>${doc.course?`<p>개인 실습 확인: ${state.done[doc.source]?'확인 기록 있음':'미확인'} · 공식 검증 상태와 별도</p><ul>${personalCriteria.map((label,i)=>`<li>${checksFor(doc.source)[i]?'[✓]':'[ ]'} ${esc(label)}</li>`).join('')}</ul>`:''}</section></article>`).join('');document.body.classList.add('printing-documents');
  }
  document.addEventListener('click',event=>{
    const button=event.target.closest('[data-action]');if(!button)return;
    const action=button.dataset.action;
    if(action==='export')exportState();
    if(action==='import')$('import-file').click();
    if(action==='reset')confirmation('개인 학습 기록을 초기화할까요?','이 브라우저의 메모·체크·북마크를 지웁니다. 필요하면 취소하고 먼저 기록을 백업하세요. 원문과 공식 상태는 그대로 유지됩니다.',()=>{state=blank();storageBlocked=false;persist();render();toast('개인 기록을 초기화했습니다.');});
    if(action==='print'||action==='print-all'){preparePrint(action==='print-all');window.print();}
    if(action==='personal')document.querySelector('.reading-rail')?.scrollIntoView({block:'start'});
    if(action==='bookmark'&&current.doc){if(state.bookmarks[current.doc])delete state.bookmarks[current.doc];else state.bookmarks[current.doc]=true;persist();button.setAttribute('aria-pressed',String(!!state.bookmarks[current.doc]));button.textContent=state.bookmarks[current.doc]?'★ 북마크됨':'☆ 북마크';toast(state.bookmarks[current.doc]?'북마크에 추가했습니다.':'북마크를 해제했습니다.');}
    if(action==='complete'&&current.doc&&byId.get(current.doc).course){if(state.done[current.doc])delete state.done[current.doc];else if(checksFor(current.doc).every(Boolean))state.done[current.doc]=true;persist();updateCompletion();updateSidebar();toast(state.done[current.doc]?'내 실습 확인을 기록했습니다.':'개인 실습 확인을 되돌렸습니다.');}
  });
  function updateCompletion(){const button=document.querySelector('[data-action="complete"]');if(!button)return;button.disabled=!state.done[current.doc]&&!checksFor(current.doc).every(Boolean);button.textContent=state.done[current.doc]?'✓ 내 실습 확인됨 · 되돌리기':'내 실습 확인 기록';}
  document.addEventListener('change',event=>{if(event.target.matches('[data-personal-check]')&&current.doc){const checks=[...checksFor(current.doc)];checks[Number(event.target.dataset.personalCheck)]=event.target.checked;state.checks[current.doc]=checks;if(!checks.every(Boolean))delete state.done[current.doc];persist();updateCompletion();updateSidebar();}});
  document.addEventListener('input',event=>{if(event.target.id==='lesson-notes'&&current.doc){state.notes[current.doc]=event.target.value.slice(0,MAX_NOTE);persist();}});
  $('search').addEventListener('input',event=>{current={view:'library',group:'all',q:event.target.value};history.replaceState(null,'','#view=library'+(current.q?'&q='+encodeURIComponent(current.q):''));render();});
  document.addEventListener('keydown',event=>{if(event.key==='/'&&!$('confirm-dialog').open&&!/INPUT|TEXTAREA|SELECT/.test(document.activeElement.tagName)&&!event.ctrlKey&&!event.metaKey){event.preventDefault();$('search').focus();}});
  window.addEventListener('hashchange',()=>readRoute(true));
  window.addEventListener('beforeprint',()=>{if(!$('print-area').childElementCount)preparePrint();});
  window.addEventListener('afterprint',()=>{document.body.classList.remove('printing-documents');$('print-area').innerHTML='';});
  readRoute();
})();
