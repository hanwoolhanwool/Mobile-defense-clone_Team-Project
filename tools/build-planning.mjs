// Rebuild the reading copy of the GDD from its editable topic documents.
// Topic documents are authoritative. The generated GDD must not be edited directly.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

export const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
export const documentSources = [
  { id:'PRD-001', file:'docs/product/OVERVIEW.md', sections:[1,2,3], title:'제품 방향과 범위' },
  { id:'SPEC-BOARD', file:'docs/design/BOARD_UI.md', sections:[4,12], title:'전장·배치·모바일 UI' },
  { id:'SPEC-BATTLE', file:'docs/design/BATTLE.md', sections:[5,8,10], title:'전투·웨이브·적·승패' },
  { id:'SPEC-SUMMON', file:'docs/design/SUMMON_ECONOMY.md', sections:[6,7], title:'소환·합성·재화' },
  { id:'SPEC-UNITS', file:'docs/design/UNITS.md', sections:[9], title:'수호자·스킬 콘텐츠' },
  { id:'SPEC-COOP', file:'docs/design/COOP_META.md', sections:[11,13], title:'협동·접속·성장·저장' },
  { id:'ART-001', file:'docs/art/ART_DIRECTION.md', sections:[14], title:'3D 아트·연출·사운드' },
  { id:'TECH-001', file:'docs/technical/ARCHITECTURE.md', sections:[15,16,17,18], title:'UE5 구현·데이터·성능' },
  { id:'PLAN-001', file:'docs/production/ROADMAP.md', sections:[19], title:'마일스톤·일정·완료 기준' },
  { id:'DEC-LOG', file:'docs/DECISIONS.md', sections:[20], title:'의사결정과 가정' },
];

export function sectionBodies(body) {
  const headings=[...body.matchAll(/^## (.+)$/gm)];
  const result=new Map();
  for(let i=0;i<headings.length;i++){
    const h=headings[i],match=h[1].match(/^(\d+)\. /);
    if(!match)continue;
    const content=body.slice(h.index,headings[i+1]?.index??body.length).trim();
    const num=Number(match[1]);
    if(result.has(num))throw new Error(`Duplicate section ${num}`);
    result.set(num,content);
  }
  return result;
}

function rebaseLinks(body,fromFile,toFile){
  return body.replace(/\]\(([^)]+)\)/g,(full,href)=>{
    if(/^(?:[a-z]+:|#)/i.test(href))return full;
    const [file,anchor]=href.split('#');
    const target=path.resolve(projectRoot,path.dirname(fromFile),file);
    const relative=path.relative(path.resolve(projectRoot,path.dirname(toFile)),target).replaceAll('\\','/');
    return `](${relative}${anchor?'#'+anchor:''})`;
  });
}

export function buildReadingCopy(root=projectRoot){
  const chapters=new Map();
  const output='docs/GDD_행운공방디펜스_UE5.md';
  for(const source of documentSources){
    const body=fs.readFileSync(path.join(root,source.file),'utf8').replaceAll('\r\n','\n');
    const sections=sectionBodies(body);
    if(sections.size!==source.sections.length)throw new Error(`${source.file}: unexpected section count`);
    for(const num of source.sections){
      if(!sections.has(num)||chapters.has(num))throw new Error(`Missing or duplicate section ${num}`);
      chapters.set(num,{
        source,
        body:rebaseLinks(sections.get(num),source.file,output),
      });
    }
  }
  if(chapters.size!==20)throw new Error('Expected all 20 original GDD chapters');
  const lines=[
    '# 행운 공방 디펜스 — 전체 기획 읽기본','',
    '> 자동 생성 문서입니다. 수정은 아래 각 장의 「편집 원본」에서 하고 `node tools/build-planning.mjs`로 이 파일을 갱신하세요.','',
    '[기획 허브](README.md) · [관리 방식](WORKFLOW.md) · [개발 현황](production/BOARD.md) · [결정 기록](DECISIONS.md)','',
    '문서 체계: 0.2 · 모바일 우선 확정. 세부 시스템은 프로토타입 설계안이며 구현·밸런스 검증 완료를 뜻하지 않습니다. 개별 문서의 상태와 작업 보드가 최신 진행 상태입니다.','',
    '## 목차','',
  ];
  for(let n=1;n<=20;n++){
    const {body}=chapters.get(n);
    lines.push(`- [${body.split('\n')[0].replace(/^## /,'')}](#chapter-${String(n).padStart(2,'0')})`);
  }
  lines.push('');
  for(let n=1;n<=20;n++){
    const {source,body}=chapters.get(n);
    const rel=path.relative('docs',source.file).replaceAll('\\','/');
    lines.push(`<a id="chapter-${String(n).padStart(2,'0')}"></a>`,'',`편집 원본: [${source.id} · ${source.title}](${rel})`,'',body,'');
  }
  return lines.join('\n');
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  const output=path.join(projectRoot,'docs','GDD_행운공방디펜스_UE5.md');
  fs.writeFileSync(output,buildReadingCopy(),'utf8');
  console.log('Generated GDD reading copy from 10 editable topic documents / 20 chapters.');
}
