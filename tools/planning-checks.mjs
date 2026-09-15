import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {projectRoot,documentSources,buildReadingCopy} from './build-planning.mjs';

export function validatePlanning(root=projectRoot){
 let checks=0;
 const errors=[];
 const check=(condition,message)=>{checks++;if(!condition)errors.push(message);};
 const read=file=>fs.readFileSync(path.join(root,file),'utf8').replaceAll('\r\n','\n');
 const walk=dir=>fs.readdirSync(path.join(root,dir),{withFileTypes:true}).flatMap(e=>
  e.isDirectory()?walk(`${dir}/${e.name}`):e.name.endsWith('.md')?[`${dir}/${e.name}`]:[]);
 const files=['README.md','PLANNING_README.md','COMMIT_CONVENTION.md',...walk('docs'),...walk('.github'),'data/README.md'];
 const cache=new Map(files.map(f=>[f,read(f)]));
 const withoutCode=body=>body.replace(/^```[^\n]*\n[\s\S]*?^```\s*$/gm,'');
 const anchors=body=>new Set([...withoutCode(body).matchAll(/<a id="([^"]+)"><\/a>/g)].map(m=>m[1]));
 const dateValid=value=>/^\d{4}-\d{2}-\d{2}$/.test(value??'')&&Number.isFinite(Date.parse(value))&&new Date(value).toISOString().slice(0,10)===value;
 const localFile=(file,label)=>{
  if(typeof file!=='string'||!file){check(false,`${label}: missing file path`);return null;}
  const absolute=path.resolve(root,file),relative=path.relative(root,absolute);
  const inside=relative!==''&&!relative.startsWith('..')&&!path.isAbsolute(relative);
  check(inside,`${label}: file outside project ${file}`);
  if(!inside)return null;
  const exists=fs.existsSync(absolute)&&fs.statSync(absolute).isFile();
  check(exists,`${label}: missing file ${file}`);
  return exists?absolute:null;
 };
 const parseJson=file=>{
  try{return JSON.parse(read(file));}catch(error){check(false,`${file}: ${error.message}`);return {};}
 };
 const definitions=cache.get('docs/BACKLOG_QA.md');
 const allAnchors=anchors(definitions);
 const tasks=[...allAnchors].filter(x=>x.startsWith('TASK-'));
 const qa=[...allAnchors].filter(x=>x.startsWith('QA-'));
 const runText=withoutCode(cache.get('docs/production/TEST_RUNS.md'));
 const runSections=new Map();
 for(const match of runText.matchAll(/^## (RUN-\d{8}-\d+)\b[^\n]*\n([\s\S]*?)(?=^## |$(?![\s\S]))/gm)){
  check(!runSections.has(match[1]),`Duplicate run ${match[1]}`);
  runSections.set(match[1],match[0]);
 }

 // GDD source selection controls assembly, not metadata coverage.
 const requiredMetadata=file=>/^docs\/(technical|design|product|art)\//.test(file)||
  ['docs/DATA_SCHEMA.md','docs/WORKFLOW.md','docs/TEMPLATES.md','docs/production/DEVELOPMENT_READINESS.md','docs/production/ROADMAP.md','docs/DECISIONS.md'].includes(file);
 const metadataDocs=files.filter(file=>file.startsWith('docs/')&&(requiredMetadata(file)||/^---\n/.test(cache.get(file))));
 const ids=new Set();
 for(const file of metadataDocs){
  const front=cache.get(file).match(/^---\n([\s\S]*?)\n---/);
  check(!!front,`${file}: missing metadata`);
  const metadata=Object.fromEntries((front?.[1]??'').split('\n').filter(line=>line.includes(':')).map(line=>{
   const i=line.indexOf(':');return [line.slice(0,i),line.slice(i+1).trim()];
  }));
  check(!!metadata.id&&!ids.has(metadata.id),`${file}: missing or duplicate document ID`);ids.add(metadata.id);
  const source=documentSources.find(s=>s.file===file);
  if(source)check(metadata.id===source.id,`${file}: ID mismatch`);
  check(['Draft','Review','Baseline','Deprecated'].includes(metadata.status),`${file}: status invalid`);
  check(!!metadata.owner,`${file}: owner missing`);
  check(dateValid(metadata.updated),`${file}: date invalid`);
  check(/^\d+\.\d+(?:\.\d+)?$/.test(metadata.version??''),`${file}: version invalid`);
  if(['Review','Baseline'].includes(metadata.status)){
   check(!['unassigned','미지정',''].includes(metadata.owner??''),`${file}: reviewed document needs owner`);
   check(!!metadata.reviewed&&!!metadata.review_run,`${file}: reviewed document needs review record`);
  }
  if(metadata.status==='Baseline')check(!!metadata.baseline_basis,`${file}: Baseline needs decision basis`);
  for(const [dateKey,runKey,scopeKey] of [['reviewed','review_run','applies_to'],['verified','verified_run','verification_scope']]){
   if(!metadata[dateKey]&&!metadata[runKey])continue;
   check(dateValid(metadata[dateKey]),`${file}: ${dateKey} invalid`);
   check(!!metadata[scopeKey],`${file}: ${scopeKey} missing`);
   check(runSections.has(metadata[runKey]),`${file}: unknown ${runKey} ${metadata[runKey]}`);
   check(metadata[runKey]?.slice(4,12)===metadata[dateKey]?.replaceAll('-',''),`${file}: ${dateKey} and run date differ`);
  }
 }
 for(const source of documentSources)check(cache.has(source.file),`Missing source ${source.file}`);

 for(const [file,raw] of cache){
  const body=withoutCode(raw),seen=new Set();
  for(const match of body.matchAll(/<a id="([^"]+)"><\/a>/g)){
   check(!seen.has(match[1]),`${file}: duplicate anchor ${match[1]}`);seen.add(match[1]);
  }
  for(const match of body.matchAll(/\]\(([^)]+)\)/g)){
   const href=match[1];if(/^[a-z]+:/i.test(href))continue;
   const [pathname,hash]=href.split('#');
   const absolute=pathname?path.resolve(root,path.dirname(file),decodeURIComponent(pathname)):path.join(root,file);
   const relative=path.relative(root,absolute).replaceAll('\\','/');
   check(!relative.startsWith('..')&&!path.isAbsolute(relative),`${file}: external local link ${href}`);
   check(fs.existsSync(absolute),`${file}: missing link ${href}`);
   if(hash&&fs.existsSync(absolute)&&fs.statSync(absolute).isFile()){
    const target=cache.get(relative)??fs.readFileSync(absolute,'utf8');
    check(anchors(target).has(decodeURIComponent(hash)),`${file}: missing anchor ${href}`);
   }
  }
  let cols=0;
  for(const [i,line] of body.split('\n').entries()){
   if(line.startsWith('|')){
    const current=line.split('|').length-2;
    if(cols)check(cols===current,`${file}:${i+1}: table column mismatch`);
    cols=current;
   }else cols=0;
  }
 }

 const board=withoutCode(cache.get('docs/production/BOARD.md'));
 const cells=line=>line.split('|').slice(1,-1).map(x=>x.trim());
 const boardRows=board.split('\n').filter(l=>l.startsWith('| [TASK-')).map(cells);
 const taskStatus=['Backlog','Ready','InProgress','Review','QA','Done','Blocked'];
 const boardTaskIds=boardRows.map(row=>row[0].match(/\[(TASK-[^\]]+)\]/)?.[1]);
 check(tasks.length>0,'No task definitions found');check(qa.length>0,'No QA cases found');
 check(boardRows.length===tasks.length,'Board and backlog task counts differ');
 check(new Set(boardTaskIds).size===tasks.length,'Duplicate task on board');
 const done=[];
 for(const row of boardRows){
  const id=row[0].match(/\[(TASK-[^\]]+)\]/)?.[1];
  check(tasks.includes(id),`Undefined board task ${id}`);
  check(['P0','P1','P2'].includes(row[1]),`${id}: invalid milestone`);
  check(['Must','Should','Could'].includes(row[2]),`${id}: invalid priority`);
  check(taskStatus.includes(row[3]),`${id}: invalid task status`);
  check(!!row[4],`${id}: missing owner value`);
  if(row[3]!=='Backlog')check(!['미지정','unassigned'].includes(row[4]),`${id}: active task needs owner`);
  if(row[3]==='Done')done.push(id);
 }
 for(const line of definitions.split('\n').filter(l=>l.startsWith('| <a id="TASK-'))){
  for(const dep of cells(line)[4].matchAll(/TASK-[A-Z][A-Z0-9]*-\d+(?:\.\d+)?/g))check(tasks.includes(dep[0]),`Unknown dependency ${dep[0]}`);
 }

 const status=withoutCode(cache.get('docs/production/PROJECT_STATUS.md'));
 const summary=status.match(/^### A3\.[\s\S]*?(?=^### A4\.)/m)?.[0]??'';
 for(const stage of ['P0','P1','P2','합계']){
  const actual=boardRows.filter(row=>stage==='합계'||row[1]===stage);
  const expected=[actual.length,...taskStatus.map(s=>actual.filter(row=>row[3]===s).length)];
  const line=summary.split('\n').find(l=>l.startsWith(`| ${stage} |`));
  check(!!line&&JSON.stringify(cells(line).slice(1).map(Number))===JSON.stringify(expected),`PROJECT_STATUS: ${stage} counts differ from board`);
 }
 const inProgress=boardRows.filter(row=>row[3]==='InProgress').length;
 const activeLine=status.split('\n').find(l=>l.startsWith('| 진행 중 작업 |'))??'';
 check(activeLine.includes(`InProgress ${inProgress}개`),'PROJECT_STATUS: InProgress summary differs from board');
 const next=status.match(/^### A4\.[\s\S]*?(?=^### A5\.)/m)?.[0]??'';
 const nextLine=status.split('\n').find(l=>l.startsWith('| 다음 착수 후보 |'))??'';
 for(const match of (next+'\n'+nextLine).matchAll(/TASK-[A-Z][A-Z0-9]*-\d+(?:\.\d+)?/g)){
  check(tasks.includes(match[0])&&!done.includes(match[0]),`PROJECT_STATUS: next task ${match[0]} is undefined or already Done`);
 }

 // Completion claims require scoped, passing evidence; sufficiency is reviewed by a person.
 const verification=parseJson('docs/production/verification.json');
 check(verification.schemaVersion===1,'verification: schemaVersion invalid');
 const runs=new Map();
 for(const run of verification.runs??[]){
  check(!runs.has(run.id),`verification: duplicate run ${run.id}`);runs.set(run.id,run);
  check(runSections.has(run.id),`verification: unknown run ${run.id}`);
  check(tasks.includes(run.task),`${run.id}: undefined task ${run.task}`);
  check(runSections.get(run.id)?.includes(run.task),`${run.id}: run text does not link task ${run.task}`);
  check(['Pass','Fail','NotRun'].includes(run.result),`${run.id}: invalid result`);
  const recordedResult=runSections.get(run.id)?.match(/^- 기록 판정: (Pass|Fail|NotRun)\b/m)?.[1];
  check(recordedResult===run.result,`${run.id}: result differs from TEST_RUNS record`);
  check(!!run.scope,`${run.id}: scope missing`);
  check(run.evidence?.length>0,`${run.id}: evidence missing`);
  for(const file of run.evidence??[])localFile(file,run.id);
 }
 const completionIds=new Set();
 for(const completion of verification.completions??[]){
  const id=completion.task;
  check(!completionIds.has(id),`Duplicate completion ${id}`);completionIds.add(id);
  check(done.includes(id),`${id}: completion does not match Done board state`);
  check(!!completion.scope&&!!completion.reviewedBy,`${id}: completion scope/reviewer missing`);
  check(dateValid(completion.reviewed),`${id}: completion review date invalid`);
  check(completion.criteria?.length>0,`${id}: completion criteria missing`);
  const note=board.match(new RegExp(`^### ${id}\\n([\\s\\S]*?)(?=^##|$(?![\\s\\S]))`,'m'))?.[1]??'';
  const criterionIds=new Set();
  for(const criterion of completion.criteria??[]){
   check(!!criterion.id&&!criterionIds.has(criterion.id),`${id}: missing or duplicate criterion ID`);criterionIds.add(criterion.id);
   check(!!criterion.description&&criterion.result==='Pass',`${id}: completion criterion ${criterion.id} must Pass`);
   const run=runs.get(criterion.run);
   check(!!run&&run.task===id&&run.result==='Pass',`${id}: criterion ${criterion.id} needs a passing run for the same task`);
   check(note.includes(criterion.run),`${id}: completion run missing from board notes`);
   check(criterion.evidence?.length>0,`${id}: criterion evidence missing`);
   for(const file of criterion.evidence??[]){
    localFile(file,`${id}/${criterion.id}`);
    check(run?.evidence?.includes(file),`${id}: criterion evidence is not registered to run`);
   }
  }
 }
 for(const id of done)check(completionIds.has(id),`${id}: Done needs structured completion evidence`);

 const archive=parseJson('docs/production/evidence/archive-manifest.json');
 check(archive.schemaVersion===1&&archive.files?.length>0,'Evidence archive manifest missing or invalid');
 const archivePaths=new Set();
 for(const entry of archive.files??[]){
  check(!archivePaths.has(entry.path),`Evidence archive duplicate ${entry.path}`);archivePaths.add(entry.path);
  check(entry.path?.startsWith('docs/production/evidence/archived/'),`Evidence archive path invalid ${entry.path}`);
  const absolute=localFile(entry.path,'Evidence archive');
  if(absolute){
   const bytes=fs.readFileSync(absolute);
   check(bytes.length===entry.bytes&&createHash('sha256').update(bytes).digest('hex')===entry.sha256,`Evidence hash mismatch ${entry.path}`);
  }
 }
 for(const run of runs.values())for(const file of run.evidence??[]){
  if(file.startsWith('docs/production/evidence/archived/'))check(archivePaths.has(file),`${run.id}: evidence missing from archive manifest ${file}`);
 }
 try{check(buildReadingCopy(root)===cache.get('docs/GDD_행운공방디펜스_UE5.md'),'GDD reading copy is stale; run node tools/build-planning.mjs');}
 catch(error){check(false,error.message);}
 return {result:errors.length?'FAIL':'PASS',checks,markdownFiles:files.length,metadataDocuments:metadataDocs.length,editableTopicDocuments:documentSources.length,tasks:tasks.length,qaCases:qa.length,errors};
}
