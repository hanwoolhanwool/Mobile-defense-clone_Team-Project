import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import {validatePlanning} from './planning-checks.mjs';
import {projectRoot} from './build-planning.mjs';

// Mutations happen only in disposable fixtures, never in the user's checkout.
function fixture(run){
 const dir=fs.mkdtempSync(path.join(os.tmpdir(),'defense-doc-check-'));
 try{
  for(const name of ['docs','tools','.github','data','Source','.gitignore','README.md','PLANNING_README.md','COMMIT_CONVENTION.md','Mobile_defense_clone.uproject']){
   fs.cpSync(path.join(projectRoot,name),path.join(dir,name),{recursive:true});
  }
  fs.mkdirSync(path.join(dir,'Content'),{recursive:true});
  fs.mkdirSync(path.join(dir,'Config'),{recursive:true});
  for(const name of ['DefaultEngine.ini','DefaultGame.ini','DefaultInput.ini','DefaultEditor.ini']){
   fs.copyFileSync(path.join(projectRoot,'Config',name),path.join(dir,'Config',name));
  }
  const edit=(file,transform)=>{
   const target=path.join(dir,file);
   fs.writeFileSync(target,transform(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')));
  };
  const json=(file,change)=>edit(file,text=>{const value=JSON.parse(text);change(value);return JSON.stringify(value,null,2)+'\n';});
  run({dir,edit,json});
 }finally{
  // mkdtemp returns a unique absolute directory under the system temp directory.
  const resolved=path.resolve(dir),temp=path.resolve(os.tmpdir())+path.sep;
  assert.ok(resolved.startsWith(temp)&&path.basename(resolved).startsWith('defense-doc-check-'));
  fs.rmSync(resolved,{recursive:true,force:true});
 }
}

test('a clean checkout of validation inputs passes',()=>fixture(({dir})=>{
 assert.deepEqual(validatePlanning(dir).errors,[]);
}));

const rejected=(name,mutate,expected)=>test(name,()=>fixture(context=>{
 mutate(context);
 assert.ok(validatePlanning(context.dir).errors.some(error=>error.includes(expected)),`Expected rejection: ${expected}`);
}));
rejected('a new technical document without metadata fails',({dir})=>{
 fs.writeFileSync(path.join(dir,'docs/technical/NEW_GUIDE.md'),'# New guide\n');
},'NEW_GUIDE.md: missing metadata');
rejected('metadata is checked outside the GDD source list',({edit})=>{
 edit('docs/technical/UNREAL_CONSOLE_COMMANDS.md',text=>text.replace('status: Draft','status: Finished'));
},'UNREAL_CONSOLE_COMMANDS.md: status invalid');
rejected('a fabricated run cannot complete a task',({json})=>{
 json('docs/production/verification.json',value=>{value.runs[1].id='RUN-20990101-01';});
},'verification: unknown run RUN-20990101-01');
rejected('a failing run cannot justify Done',({json})=>{
 json('docs/production/verification.json',value=>{value.runs[1].result='Fail';});
},'needs a passing run');
rejected('the run result must match the execution record',({edit})=>{
 edit('docs/production/TEST_RUNS.md',text=>text.replace('- 기록 판정: Pass','- 기록 판정: Fail'));
},'result differs from TEST_RUNS record');
rejected('an unexecuted acceptance criterion cannot justify Done',({json})=>{
 json('docs/production/verification.json',value=>{value.completions[0].criteria[0].result='NotRun';});
},'must Pass');
rejected('Done requires a completion record',({json})=>{
 json('docs/production/verification.json',value=>{value.completions=[];});
},'Done needs structured completion evidence');
rejected('missing evidence fails',({json})=>{
 json('docs/production/verification.json',value=>{value.runs[1].evidence.push('docs/production/evidence/missing.log');});
},'missing file docs/production/evidence/missing.log');
rejected('altered archived evidence fails its hash',({edit})=>{
 edit('docs/production/evidence/archived/RUN-20260913-03/Logs/editor.log',text=>text+'tampered\n');
},'Evidence hash mismatch');
rejected('a stale status summary fails',({edit})=>{
 edit('docs/production/PROJECT_STATUS.md',text=>text.replace('| P0 | 11 | 10 |','| P0 | 11 | 9 |'));
},'P0 counts differ from board');
rejected('a completed task cannot be the next action',({edit})=>{
 edit('docs/production/PROJECT_STATUS.md',text=>text.replace('| 다음 착수 후보 | TASK-MOB-01','| 다음 착수 후보 | TASK-CORE-01'));
},'next task TASK-CORE-01');
rejected('a stale GDD reading copy fails',({edit})=>{
 edit('docs/GDD_행운공방디펜스_UE5.md',text=>text+'stale\n');
},'GDD reading copy is stale');

for(const [label,file,script] of [
 ['data','data/GameRules.json','tools/build-design-data.mjs'],
 ['report','docs/VALIDATION_REPORT.md','tools/validate-design-data.mjs'],
]){
 test(`check-only ${label} detects drift without repairing the file`,()=>fixture(({dir,edit})=>{
  edit(file,text=>file.endsWith('.json')?text.replace('"RulesVersion": "0.1.0"','"RulesVersion": "stale"'):text+'stale\n');
  const before=fs.readFileSync(path.join(dir,file));
  const result=spawnSync(process.execPath,[script,'--check'],{cwd:dir,encoding:'utf8'});
  assert.equal(result.status,1,result.stdout+result.stderr);
  assert.deepEqual(fs.readFileSync(path.join(dir,file)),before);
 }));
}
