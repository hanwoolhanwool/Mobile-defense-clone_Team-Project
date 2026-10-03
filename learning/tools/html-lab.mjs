// Presentation only. Markdown remains the source of the lesson and its verified status.
export const escapeHtml = value => String(value).replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;').replaceAll("'", '&#39;');
export const route = (id, anchor = '') => '#doc=' + encodeURIComponent(id) + (anchor ? '&anchor=' + encodeURIComponent(anchor) : '');
export const courses = [
  {id:'A', title:'개발자 A', subtitle:'데이터에서 전투와 웨이브까지', description:'공통 상태·유닛·피해·승패를 구현하고 Android 검수로 이어갑니다.', color:'green'},
  {id:'B', title:'개발자 B', subtitle:'보드에서 조작과 경제까지', description:'좌표·입력·명령·소환·합성·판매와 HUD를 연결합니다.', color:'blue'},
  {id:'integration', title:'함께 통합', subtitle:'같은 계약, 하나의 게임', description:'각 게이트에서 서로의 API와 구현을 비교하고 실제 실행으로 검증합니다.', color:'purple'},
];
export function classify(source) {
  if (/^P0\/A\/G\d_\d\d_/.test(source)) return {course:'A', gate:/G\d/.exec(source)[0]};
  if (/^P0\/B\/G\d-\d\d-/.test(source)) return {course:'B', gate:/G\d/.exec(source)[0]};
  if (/^P0\/(G\d_)?INTEGRATION\.md$/.test(source)) return {course:'integration', gate:/G\d/.exec(source)?.[0] || 'G0'};
  return {course:'', gate:''};
}
export function plain(html) {
  return html.replace(/<[^>]*>/g,' ').replace(/&#(\d+);/g,(_,n)=>String.fromCodePoint(Number(n)))
    .replace(/&(amp|lt|gt|quot|#39);/g,(_,s)=>({amp:'&',lt:'<',gt:'>',quot:'"','#39':"'"})[s]).replace(/\s+/g,' ').trim();
}
export function shell({title, marker, css, client, catalog, home = '', source = '', body = '', toc = '', original = '', data = null}) {
  const e = escapeHtml;
  const link = hash => home + hash;
  const nav = courses.map(course => `<details class="course-nav" ${!source || classify(source).course === course.id ? 'open' : ''}><summary>${course.title}<span>${catalog.filter(d=>d.course===course.id).length}</span></summary>${catalog.filter(d=>d.course===course.id).sort((a,b)=>a.gate.localeCompare(b.gate,undefined,{numeric:true})||a.source.localeCompare(b.source)).map((doc,i)=>`<a href="${link(route(doc.source))}" data-nav-doc="${e(doc.source)}"><span class="nav-number">${String(i+1).padStart(2,'0')}</span><span>${e(doc.shortTitle)}</span><span class="nav-done" aria-label="개인 실습 확인"></span></a>`).join('')}</details>`).join('');
  return `${marker}
<!doctype html><html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>${e(title)} · P0 Learning Lab</title><style>${css}</style></head>
<body${data ? ' data-lab="true"' : ''}><a class="skip" href="#main">본문으로 이동</a>
<button class="backdrop" aria-label="학습 메뉴 닫기" tabindex="-1" hidden></button>
<aside class="sidebar" id="sidebar" aria-label="학습 탐색"><a class="brand" href="${link('#view=home')}"><span class="brand-mark">P0</span><span>Learning Lab<small>A · B REFERENCE WORKBOOK</small></span></a>
<nav class="side-main" aria-label="주요 문서"><a href="${link('#view=home')}" data-view="home">학습 대시보드 <span>↗</span></a><a href="${link('#view=progress')}" data-view="progress">나의 학습 기록 <span id="nav-count">0 / 27</span></a><a href="${link('#view=library')}" data-view="library">전체 문서 <span>${catalog.length}</span></a></nav>
<div class="side-scroll"><p class="nav-label">P0 · 단계별 실습</p>${nav}<p class="nav-label">공통 · 참고 자료</p><a class="side-link" href="${link(route('GIT_GUIDE.md'))}">Git · 프로젝트 사용 안내 ↗</a><a class="side-link" href="${link(route('P0/COMMON.md'))}">공통 계약과 출발점 ↗</a><a class="side-link" href="${link(route('WORKFLOW.md'))}">학습 운영과 브랜치 ↗</a><a class="side-link" href="${link('#view=library&group=reference')}">검수 증거 · 기존 계획 ↗</a></div>
<div class="side-bottom"><div><span>내 실습 기록</span><b id="side-percent">0%</b></div><div class="progress-track"><span id="side-progress"></span></div><small>개인 기록과 공식 검증 상태는 별도입니다.</small></div></aside>
<div class="app"><header class="topbar"><button id="menu-toggle" class="icon-button" aria-label="학습 메뉴 열기" aria-expanded="false" aria-controls="sidebar">☰</button><div class="breadcrumb">P0 WORKBOOK <span>/</span> <b id="breadcrumb">${source ? '개별 문서' : '대시보드'}</b></div>${data ? '<label class="global-search"><span aria-hidden="true">⌕</span><input id="search" type="search" aria-label="수업 본문 검색" placeholder="수업·본문·코드 검색" autocomplete="off"><kbd>/</kbd></label>' : `<a class="btn small" href="${link(route(source))}">학습실에서 열기 ↗</a>`}<span class="offline"><i></i> 오프라인</span></header>
<main id="main" tabindex="-1">${data ? '<noscript><h1>P0 학습 자료</h1><p>학습실은 JavaScript가 필요합니다. <a href="README.html">개별 HTML 문서</a>는 JavaScript 없이도 읽을 수 있습니다.</p></noscript>' : `<div class="standalone-notice">개별 HTML 읽기본입니다. 메모·북마크·개인 진도는 <a href="${link(route(source))}">학습실</a>에서 함께 관리합니다.</div><div class="reader-tools"><a href="${original}">Markdown 원본</a><button class="btn secondary small" data-action="print">인쇄 / PDF</button></div><div class="reader-grid"><article class="lesson-body">${body}</article><aside class="reading-rail"><section class="panel toc"><h2>이 문서의 목차</h2>${toc}</section></aside></div>`}</main></div>
<div id="toast" role="status" aria-live="polite"></div><div id="print-area"></div>
${data ? `<dialog id="confirm-dialog"><h2 id="confirm-title"></h2><p id="confirm-description"></p><div class="button-row"><button class="btn secondary" id="cancel-confirm">취소</button><button class="btn" id="accept-confirm">확인</button></div></dialog><input type="file" id="import-file" accept="application/json,.json" hidden><script>window.P0_LAB=${JSON.stringify(data).replaceAll('<','\\u003c')};</script>` : ''}
<script>${client}</script></body></html>
`;
}
