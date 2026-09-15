import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const read=name=>JSON.parse(fs.readFileSync(path.join(root,'data',name),'utf8'));
let checks=0;
const errors=[];
const ok=(value,message)=>{checks++;if(!value)errors.push(message);};
const tableNames=['Units','Skills','Recipes','EnemyTypes','SpawnProfiles','Waves','SummonProfiles','Upgrades'];
const tables=Object.fromEntries(tableNames.map(n=>[n,read(`DT_${n}.json`)]));
const rules=read('GameRules.json');
for(const [name,rows] of Object.entries(tables)){
 ok(Array.isArray(rows)&&rows.length>0,`${name}: empty or invalid table`);
 ok(new Set(rows.map(r=>r.Name)).size===rows.length,`${name}: duplicate row name`);
 const keys=Object.keys(rows[0]).sort().join(',');
 for(const row of rows){
  ok(typeof row.Name==='string'&&/^[A-Za-z0-9_]+$/.test(row.Name),`${name}: invalid Name`);
  ok(Object.keys(row).sort().join(',')===keys,`${name}/${row.Name}: inconsistent fields`);
  for(const [key,value] of Object.entries(row)) if(typeof value==='number')ok(Number.isFinite(value)&&value>=0,`${name}/${row.Name}/${key}: invalid numeric value`);
 }
}
const index=(rows)=>new Map(rows.map(r=>[r.Name,r]));
const unitMap=index(tables.Units),skillMap=index(tables.Skills),enemyMap=index(tables.EnemyTypes),spawnMap=index(tables.SpawnProfiles),summonMap=index(tables.SummonProfiles);
ok(tables.Units.length===20,'unit count != 20');
ok(tables.Skills.length===16,'skill count != 16');
const grades=['Common','Rare','Epic','Legendary','Mythic'];
for(const grade of grades)ok(tables.Units.filter(u=>u.Grade===grade).length===4,`${grade}: expected 4 units`);
for(const u of tables.Units){
 ok(grades.includes(u.Grade),`${u.Name}: unknown grade`);
 ok(u.SkillId==='None'||skillMap.has(u.SkillId),`${u.Name}: missing skill`);
 ok(u.BaseAttack>0&&u.AttackIntervalSeconds>0&&u.RangeCm>0,`${u.Name}: invalid combat stats`);
 ok(['Physical','Magic'].includes(u.DamageType),`${u.Name}: invalid damage type`);
 ok(u.MaxStack===(['Legendary','Mythic'].includes(u.Grade)?1:3),`${u.Name}: invalid stack capacity`);
 ok(u.SellGold==={Common:5,Rare:15,Epic:45,Legendary:120,Mythic:300}[u.Grade],`${u.Name}: sale mismatch`);
}
for(const s of tables.Skills){
 ok(['Cooldown','AttackCount','Aura'].includes(s.Trigger),`${s.Name}: invalid trigger`);
 ok(['Stun','ExtraShot','AreaDamage','AreaDebuff','Chain','Aura','AreaBurn','ChainBoss'].includes(s.EffectType),`${s.Name}: invalid effect`);
 ok(Number.isInteger(s.MaxTargets)&&s.MaxTargets>0,`${s.Name}: target count`);
 if(s.Trigger==='Cooldown')ok(s.CooldownSeconds>0,`${s.Name}: zero cooldown`);
 if(s.Trigger==='AttackCount')ok(Number.isInteger(s.TriggerAttackCount)&&s.TriggerAttackCount>0,`${s.Name}: attack count`);
 if(s.Trigger==='Aura')ok(s.RadiusCm>0&&(s.AuraAttackPct>0||s.AuraHastePct>0),`${s.Name}: empty aura`);
 if(s.EffectType==='AreaBurn')ok(s.DotTicks*s.DotIntervalSeconds===s.DurationSeconds,`${s.Name}: DoT duration mismatch`);
 if(s.EffectType.startsWith('Chain'))ok(s.ChainRadiusCm>0,`${s.Name}: no chain radius`);
 ok(s.SlowPct<=.5,`${s.Name}: slow above cap`);
}
ok(tables.Recipes.length===4,'recipe count != 4');
for(const r of tables.Recipes){
 for(const [field,grade] of [['OutputUnitId','Mythic'],['LegendaryUnitId','Legendary'],['EpicUnitId','Epic'],['RareUnitId','Rare']])ok(unitMap.get(r[field])?.Grade===grade,`${r.Name}: invalid ${field}`);
 ok(r.LegendaryCount===1&&r.EpicCount===1&&r.RareCount===1&&r.GoldCost===0,`${r.Name}: quantity mismatch`);
 const inputRefund=['LegendaryUnitId','EpicUnitId','RareUnitId'].reduce((s,k)=>s+(unitMap.get(r[k])?.SellGold??0),0);
 // Craft sale may add value, but it destroys rare materials and has no reverse loop.
 ok(inputRefund===180,`${r.Name}: material refund baseline changed`);
}
for(const e of tables.EnemyTypes){
 ok(['Normal','Boss'].includes(e.Kind),`${e.Name}: enemy kind`);
 ok(e.MagicResistance>=0&&e.MagicResistance<=.75,`${e.Name}: resistance`);
 if(e.Kind==='Boss')ok(e.FixedHP>0&&e.StunImmune===true&&e.SlowCap===.25,`${e.Name}: boss rule mismatch`);
 else ok(e.HPScale>0&&e.FixedHP===0&&e.SlowCap===.5,`${e.Name}: normal rule mismatch`);
}
for(const p of tables.SpawnProfiles)ok(['N01Weight','N02Weight','N03Weight','N04Weight'].reduce((n,k)=>n+p[k],0)===10000,`${p.Name}: spawn weights`);
const outcomeNames=['Fail','Common','Rare','Epic','Legendary','Mythic'];
for(const p of tables.SummonProfiles){
 ok(outcomeNames.every(x=>Number.isInteger(p[`${x}Weight`])),`${p.Name}: noninteger weight`);
 ok(outcomeNames.reduce((sum,x)=>sum+p[`${x}Weight`],0)===10000,`${p.Name}: summon weights`);
 ok(p.GuaranteedProfileId==='None'||summonMap.has(p.GuaranteedProfileId),`${p.Name}: guarantee reference`);
 ok(p.Source==='Star'?p.CostStars===2:p.CostStars===0,`${p.Name}: cost`);
}
const roll=(p,r)=>{let total=0;for(const n of outcomeNames){total+=p[`${n}Weight`];if(r<total)return n;}throw new Error('invalid roll');};
const gold=summonMap.get('Gold_Default'),gp=summonMap.get('Gold_Pity'),sp=summonMap.get('Star_Pity');
for(const [r,expected] of [[0,'Common'],[8199,'Common'],[8200,'Rare'],[9699,'Rare'],[9700,'Epic'],[9999,'Epic']])ok(roll(gold,r)===expected,`gold boundary ${r}`);
for(let r=0;r<10000;r++){ok(['Rare','Epic'].includes(roll(gp,r)),`gold guarantee invalid ${r}`);ok(['Epic','Legendary'].includes(roll(sp,r)),`star guarantee invalid ${r}`);}
ok(gold.PityThreshold===14&&summonMap.get('Star_Default').PityThreshold===4,'pity thresholds mismatch');

function allocation(K,p){
 const ids=['N01','N02','N03','N04'];
 const rows=ids.map(id=>{const raw=K*p[`${id}Weight`]/10000;return {id,count:Math.floor(raw),fraction:raw-Math.floor(raw)};});
 let rest=K-rows.reduce((s,r)=>s+r.count,0);
 const ranked=[...rows].sort((a,b)=>b.fraction-a.fraction||a.id.localeCompare(b.id));
 for(let i=0;i<rest;i++)ranked[i].count++;
 return Object.fromEntries(rows.map(r=>[r.id,r.count]));
}
ok(allocation(13,spawnMap.get('Runner')).N01===10,'allocation runner N01');
ok(allocation(13,spawnMap.get('Runner')).N02===3,'allocation runner N02');
ok(tables.Waves.length===30,'waves != 30');
let normalCount=0,bossCount=0,waveGold=0,stars=0,preFinalGold=rules.Economy.StartingGold,preFinalStars=0;
const waveLines=[];
for(let i=0;i<tables.Waves.length;i++){
 const w=tables.Waves[i],n=i+1,boss=n%10===0;
 ok(w.WaveIndex===n,`wave order ${n}`);
 ok(w.DurationSeconds===30,`${w.Name}: duration`);
 ok(w.NormalBaseHP===Math.round(70*1.12**(n-1)),`${w.Name}: hp formula`);
 ok(w.GoldRewardPerPlayer===20+3*n,`${w.Name}: gold formula`);
 ok(w.StarRewardPerPlayer===(n%2===0?1:0)+(boss?4:0),`${w.Name}: stars formula`);
 if(boss){
  ok(w.NormalCountPerGate===0&&w.BossCountPerGate===1&&enemyMap.get(w.BossId)?.Kind==='Boss',`${w.Name}: boss contents`);
 }else{
  ok(w.NormalCountPerGate===12+Math.floor((n-1)/3)&&w.BossCountPerGate===0&&w.BossId==='None',`${w.Name}: normal contents`);
  const profile=spawnMap.get(w.SpawnProfileId);ok(!!profile,`${w.Name}: profile ref`);
  const counts=allocation(w.NormalCountPerGate,profile);
  ok(Object.values(counts).reduce((s,c)=>s+c,0)===w.NormalCountPerGate,`${w.Name}: allocation sum`);
 }
 const enemies=w.NormalCountPerGate*2;
 normalCount+=enemies;bossCount+=w.BossCountPerGate*2;waveGold+=w.GoldRewardPerPlayer;stars+=w.StarRewardPerPlayer;
 if(n<30){preFinalGold+=enemies+w.GoldRewardPerPlayer;preFinalStars+=w.StarRewardPerPlayer;}
 waveLines.push(`| ${n} | ${boss?'보스':w.SpawnProfileId} | ${enemies} | ${boss?`${w.BossId} ×2`:'—'} | ${w.NormalBaseHP} | ${w.GoldRewardPerPlayer} | ${w.StarRewardPerPlayer} |`);
}
ok(normalCount===882,'total normal count != 882');ok(bossCount===6,'boss count != 6');ok(waveGold===1995,'wave gold != 1995');ok(stars===27,'stars != 27');ok(preFinalStars===22,'pre-final stars != 22');
const totalGold=rules.Economy.StartingGold+normalCount+waveGold;
const summonCost=n=>Math.min(rules.Economy.SummonBaseGold+rules.Economy.SummonIncrementGold*n,rules.Economy.SummonMaxGold);
const costFor=count=>Array.from({length:count},(_,n)=>summonCost(n)).reduce((a,b)=>a+b,0);
const maxSummons=budget=>{let count=0,spent=0;while(spent+summonCost(count)<=budget){spent+=summonCost(count);count++;}return {count,spent,remaining:budget-spent};};
ok(costFor(5)===120,'first five costs');ok(summonCost(20)===60,'21st cost');
ok(rules.Board.CellsPerPlayer===rules.Board.Columns*rules.Board.Rows,'board dimensions');
ok(rules.Board.MaxUnitsPerPlayer===rules.Board.CellsPerPlayer*3,'unit limit');
const pts=rules.Path.PointsCm;
const length=pts.reduce((sum,p,i)=>sum+Math.hypot(...p.map((v,j)=>v-pts[(i+1)%pts.length][j])),0);
ok(length===4000&&length===rules.Path.LengthCm,'path length');
ok(Math.round(100*1.2*1.1*100/130)===102,'damage example');
const upperBefore=maxSummons(preFinalGold),upperAll=maxSummons(totalGold);
const upgradeTotal=u=>Array.from({length:u.MaxLevel},(_,i)=>u.BaseCost+u.CostIncrement*i).reduce((a,b)=>a+b,0);
const sampleSpend=costFor(40)+(60+100+140+180)+(80+140)+200;
ok(sampleSpend<=preFinalGold,'sample spending exceeds pre-final budget');

const report=[
 '# 초기 데이터 검증 결과','',
 `검증 스크립트: tools/validate-design-data.mjs · RulesVersion: ${rules.RulesVersion}`,'',
 `결과: **${errors.length?'실패':'통과'}** · 총 검사 ${checks.toLocaleString('en-US')}개 · 실패 ${errors.length}개`,'',
 '이 결과는 JSON의 내부 일관성·참조·수식·확률 구간을 검사한 것이다. UE 임포트, 게임 코드 컴파일, 실제 플레이, 시드별 승률, 네트워크, 모바일 성능을 검증한 결과가 아니다. 확률 보장의 전체 10,000개 정수 구간을 검사하므로 검사 수가 크다. 독립된 게임 테스트가 그만큼 있다는 의미는 아니다.','',
 '## 검사한 내용','',
 '- 8개 DataTable의 행 키 중복, 필드 일관성, 수치 범위.',
 '- 수호자 20종, 스킬16종, 신화 레시피4종의 참조와 등급.',
 '- 소환 6개 프로파일의 가중치 합계10,000, 기본 확률 경계, 두 보장 프로파일의 모든 결과.',
 '- 30웨이브 수량·HP·보상·보스 구성·스폰 정수 배분.',
 '- 경로4,000cm, 보드24칸, 팀 수호자72개체, 피해 계산 예제.',
 '- 일반 소환 가격, 종료/전투 중 재화 구분, 강화 비용.','',
 '## 재화 계산','',
 '| 항목 | 계산 결과 |','|---|---:|',
 `| 일반 적 총수 | ${normalCount} |`,
 `| 보스 총수 | ${bossCount} |`,
 `| 웨이브 골드 합계 / 개인 | ${waveGold} |`,
 `| 시작+모든 일반 적 처치+웨이브 골드 / 개인 | ${totalGold} |`,
 `| 30웨이브 시작 전 확보 가능한 골드 상한 / 개인 | ${preFinalGold} |`,
 `| 별조각 전체 / 개인 | ${stars} |`,
 `| 결과 전 사용 가능한 별조각 / 개인 | ${preFinalStars} |`,
 `| 전투 중 도전 횟수 상한 | ${Math.floor(preFinalStars/2)} |`,
 `| 종료 보상까지 포함한 소환 구매력 계산 | ${upperAll.count}회 / ${upperAll.spent}골드 |`,
 `| 전투 중 소환만 샀을 때 상한 | ${upperBefore.count}회 / ${upperBefore.spent}골드 |`,
 `| 공격 훈련8레벨 누적 비용 | ${upgradeTotal(tables.Upgrades[0])} |`,
 `| 장치 가속5레벨 누적 비용 | ${upgradeTotal(tables.Upgrades[1])} |`,'',
 '모든 일반 적을 제때 처치했다는 이상적 수입 상한이다. 전투 중 실제 수입은 잔여 적과 실패 웨이브에 따라 줄어든다. 판매 수입은 포함하지 않았으며, 수호자 판매는 군세를 줄인다. 골드 상한을 도달 가능한 승률·유닛 구성으로 해석하지 않는다.','',
 `예산 예시: 일반 소환40회 ${costFor(40)} + 공격훈련4레벨480 + 장치가속2레벨220 + 전설교환1회200 = ${sampleSpend}골드. 최종 웨이브 직전 이상적 수입 ${preFinalGold}보다 ${preFinalGold-sampleSpend} 적다. 이 예시는 자원상 가능한 지출이고, 그 구매 순서로 생존하거나 신화가 보장된다는 의미는 아니다.`,
 '', '30웨이브 완료의 별조각5개·골드110은 전투가 끝난 뒤 지급된다. 이를 최종 보스 공략에 사용할 수입으로 계산하지 않았다.','',
 '## 웨이브 전체 표','',
 '| 웨이브 | 구성 | 새 일반 적 전체 | 보스 | 일반 기준HP | 골드/개인 | 별조각/개인 |',
 '|---:|---|---:|---|---:|---:|---:|',...waveLines,'',
 '보스 웨이브의 일반 기준HP는 수식상 기준값으로만 남으며 새 일반 적은 생성하지 않는다. 실제 일반 적 HP는 유형별 HPScale을 적용한다.','',
 '## 아직 필요한 검증','',
 '- UE Row Struct 구현 후 JSON 실제 임포트 및 데이터 참조 에디터 검사.',
 '- 보드 수용량·합성 예약·중복 명령에 대한 게임 코드 테스트.',
 '- 최악/중앙/최선 시드의 전설·신화 획득 시각과 보스 클리어율.',
 '- 사람 2인과 봇 포함 세션의 서로 다른 난이도.',
 '- 타깃 기기 장시간 FPS·메모리·발열 및 지연·손실 네트워크 테스트.','',
 ...(errors.length?['## 실패 목록','',...errors.map(e=>`- ${e}`),'']:[]),
 ];
const reportPath=path.join(root,'docs','VALIDATION_REPORT.md');
if(process.argv.includes('--check')){
 ok(fs.existsSync(reportPath)&&fs.readFileSync(reportPath,'utf8').replaceAll('\r\n','\n')===report.join('\n'),
  'VALIDATION_REPORT.md is stale; run node tools/validate-design-data.mjs');
}else fs.writeFileSync(reportPath,report.join('\n'),'utf8');
console.log(JSON.stringify({checks,errors,totalNormalEnemies:normalCount,totalGoldPerPlayer:totalGold,preFinalGoldPerPlayer:preFinalGold,usableStars:preFinalStars,preFinalSummonsOnly:upperBefore.count},null,2));
if(errors.length)process.exitCode=1;
