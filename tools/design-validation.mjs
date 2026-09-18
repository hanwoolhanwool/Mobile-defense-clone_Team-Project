// Static design contracts only. This does not implement or simulate UE gameplay.
export function validateDesign(data){
 let checks=0;const errors=[];const ok=(v,m)=>{checks++;if(!v)errors.push(m);};
 const r=data.GameRules, tables=Object.entries(data).filter(([n])=>n.startsWith('DT_'));
 for(const [name,rows] of tables){
  ok(Array.isArray(rows)&&rows.length>0,name+': rows required');
  ok(new Set(rows.map(x=>x.Name)).size===rows.length,name+': duplicate Name');
  const keys=Object.keys(rows[0]).sort().join(',');
  for(const row of rows){
   ok(/^[A-Za-z0-9_]+$/.test(row.Name),name+': invalid Name');
   ok(Object.keys(row).sort().join(',')===keys,name+'/'+row.Name+': inconsistent fields');
   for(const [key,value] of Object.entries(row))if(typeof value==='number')ok(Number.isFinite(value)&&value>=0,name+'/'+row.Name+'/'+key+': invalid number');
  }
 }
 const units=data.DT_Units, waves=data.DT_Waves, profiles=data.DT_SummonProfiles, grades=['Common','Rare','Epic','Legendary'];
 ok(r.SchemaVersion===2,'schema version');
 ok(units.filter(u=>u.EnabledInP0).length===16,'P0 must have 16 units');
 for(const g of grades)ok(units.filter(u=>u.Grade===g&&u.EnabledInP0).length===4,g+': P0 roster count');
 for(const u of units){
  ok(u.MaxStack===3,u.Name+': stack capacity');
  ok(u.RangeCm===u.RangeCells*r.Board.CellSizeCm&&[1.25,2.5,3.5].includes(u.RangeCells),u.Name+': cell-based range');
  ok(u.BaseAttack>0&&u.AttackIntervalSeconds>0,u.Name+': basic stats');
  ok(u.AttackPresentation===(u.RangeCells===1.25?'Melee':'Projectile'),u.Name+': presentation');
  ok(u.SkillId==='None',u.Name+': skills must await concept');
  ok(u.LegacySkillId==='None'||data.DT_Skills.some(s=>s.Name===u.LegacySkillId),u.Name+': legacy reference');
  if(u.Grade==='Common')ok(u.SalePolicy==='HalfNextPaidSummonFloor'&&u.SaleCurrency==='Gold',u.Name+': sale policy');
  if(grades.includes(u.Grade)&&u.Grade!=='Common')ok(u.SaleCurrency==='Stars'&&u.SaleAmount===({Rare:1,Epic:2,Legendary:4})[u.Grade],u.Name+': sale amount');
 }
 for(const row of [...data.DT_Skills,...data.DT_Recipes])ok(row.Enabled===false&&row.DesignStatus==='DeferredConcept',row.Name+': deferred content enabled');
 for(const recipe of data.DT_Recipes)for(const [key,grade]of [['OutputUnitId','Mythic'],['LegendaryUnitId','Legendary'],['EpicUnitId','Epic'],['RareUnitId','Rare']])ok(units.some(u=>u.Name===recipe[key]&&u.Grade===grade),recipe.Name+': reference '+key);
 const outcomes=['Fail','Common','Rare','Epic','Legendary','Mythic'];
 const roll=(p,n)=>{let cumulative=0;for(const x of outcomes){cumulative+=p[x+'Weight'];if(n<cumulative)return x;}return 'Invalid';};
 ok(profiles.length===14,'11 gold levels and 3 roulette profiles required');
 for(const p of profiles){
  const weights=outcomes.map(x=>p[x+'Weight']);
  ok(weights.every(w=>Number.isInteger(w)&&w>=0)&&weights.reduce((a,b)=>a+b,0)===10000,p.Name+': weights');
  ok(p.MythicWeight===0&&p.PityThreshold===0,p.Name+': direct mythic or pity');
  const counts=Object.fromEntries(outcomes.map(o=>[o,0]));
  for(let n=0;n<10000;n++){const outcome=roll(p,n);counts[outcome]=(counts[outcome]??0)+1;}
  ok(outcomes.every(o=>counts[o]===p[o+'Weight']),p.Name+': exhaustive interval distribution');
  if(p.Source==='Gold'){
   const L=p.Level;
   ok(Number.isInteger(L)&&L>=0&&L<=10,p.Name+': level');
   ok(p.CommonWeight===9743-150*L&&p.RareWeight===198+100*L&&p.EpicWeight===49+45*L&&p.LegendaryWeight===10+5*L,p.Name+': upgrade odds');
   ok(p.CostStars===0&&p.FailWeight===0,p.Name+': gold failure/cost');
  }
 }
 for(const [name,cost,weight] of [['Rare',1,6000],['Epic',1,2000],['Legendary',2,1000]]){
  const p=profiles.find(x=>x.Name==='Roulette_'+name);
  ok(p?.CostStars===cost&&p[name+'Weight']===weight&&p.FailWeight===10000-weight,'roulette '+name);
 }
 for(const s of data.DT_SpawnProfiles)ok(['N01','N02','N03','N04'].reduce((a,id)=>a+s[id+'Weight'],0)===10000,s.Name+': weights');
 let normals=0,bosses=0,killGold=0;
 ok(waves.length===80,'80 wave fixtures required');
 for(const [i,w]of waves.entries()){
  const boss=(i+1)%10===0;
  ok(w.WaveIndex===i+1,w.Name+': sequence');
  ok(w.NormalCountPerGate===(boss?0:20)&&w.BossCountPerGate===(boss?1:0),w.Name+': quantities');
  ok(w.DurationSeconds===(boss?60:20)&&w.SpawnIntervalSeconds===(boss?0:1),w.Name+': timing');
  ok(w.GoldRewardPerPlayer===0&&w.StarRewardPerPlayer===0,w.Name+': wave reward prohibited');
  ok(boss?data.DT_EnemyTypes.some(e=>e.Name===w.BossId&&e.Kind==='Boss'):data.DT_SpawnProfiles.some(p=>p.Name===w.SpawnProfileId),w.Name+': enemy reference');
  if(boss)ok(w.BossDeadlineSeconds===60&&w.PostBossDelaySeconds===2,w.Name+': boss timing');
  else ok(w.FirstSpawnOffsetSeconds+(w.NormalCountPerGate-1)*w.SpawnIntervalSeconds<w.DurationSeconds,w.Name+': last spawn outside wave');
  normals+=2*w.NormalCountPerGate;bosses+=2*w.BossCountPerGate;killGold+=2*w.NormalCountPerGate*(w.WaveIndex>=21?2:1);
 }
 ok(normals===2880&&bosses===16&&killGold===5040,'full-match totals');
 const b=r.Board,e=r.Economy;
 ok(b.Columns===6&&b.Rows===3&&b.CellsPerPlayer===18&&b.MaxUnitsPerPlayer===20,'board / population');
 ok(b.MaxPartialStacksPerUnitPerArea===1&&!b.MergeUsesSelectedCell&&b.TransferPreservesInstanceAndTimers,'stack invariants');
 for(let p=0;p<2;p++){
  const order=b.PlacementOrderByPlayer[p];
  ok(order.length===18&&new Set(order).size===18&&order.every(id=>Math.floor(id/18)===p),'placement order permutation '+p);
  const expected=Array.from({length:6},(_,c)=>5-c).flatMap(c=>(p===0?[2,1,0]:[0,1,2]).map(row=>18*p+6*row+c));
  ok(order.join()===expected.join(),'screen top-down columns '+p);
 }
 ok(r.Paths.PointsByGateCm.length===2,'two routes');
 for(const points of r.Paths.PointsByGateCm){
  const length=points.reduce((sum,p,i)=>sum+Math.hypot(...p.map((v,j)=>v-points[(i+1)%points.length][j])),0);
  ok(length===r.Paths.LengthPerGateCm,'route length');
  ok(JSON.stringify(points.slice(1,3))===JSON.stringify(r.Paths.SharedSegmentCm),'shared direction');
 }
 ok(e.StartingGold===100&&e.SummonMaxGold===null,'starting gold/no cap');
 const price=n=>e.SummonBaseGold+e.SummonIncrementGold*n;
 ok([0,1,2,3].reduce((sum,n)=>sum+price(n),0)===92&&price(23)===66,'summon price arithmetic');
 ok(Math.floor(price(4)*e.CommonSaleFractionOfNextPaidPrice)===14,'common sale arithmetic');
 ok(!e.WaveBaseRewardsEnabled&&e.WaveEntryGold===0&&e.WaveEndGold===0,'no wave reward');
 ok(e.BossKillGoldPerPlayer===100&&e.BossKillStarsPerPlayer===2&&e.FastBossBonusPerBoss&&e.FastBossBonusStarsPerPlayer===1,'boss reward units');
 for(const [id,total]of [['Attack_CommonRare',1625],['Attack_Epic',3250],['Attack_LegendaryMythic',65],['SummonOdds',3250]]){
  const u=data.DT_Upgrades.find(x=>x.Name===id);
  ok(!!u&&u.MaxLevel===10&&u.SuccessWeight===10000,id+': upgrade contract');
  if(u)ok(Array.from({length:u.MaxLevel},(_,L)=>u.BaseCost+u.CostIncrement*L).reduce((a,b)=>a+b,0)===total,id+': cumulative cost');
 }
 ok(r.Defeat.CountedEnemyKinds.join()==='Normal'&&r.Defeat.ActiveEnemyThreshold===100&&r.Defeat.ContinuousSeconds===0,'counter defeat');
 ok(r.Defeat.BossDeadlineSeconds===60&&r.Defeat.RequireBothWaveBossesDead&&!r.Defeat.HuntTimeoutDefeatsTeam,'boss vs hunt');
 ok(r.Timing.DamageAtDeadlineCounts&&!r.Timing.RejectedRequestConsumesRng,'time/RNG boundary');
 ok(r.Combat.SlowCap===.8&&r.Combat.ArmorFloor===-50&&r.Combat.MaximumAttacksPerSecond===8,'combat caps');
 ok(r.Difficulty.Dungeon.HPPercentDamageFactor===.3&&r.Difficulty.Dungeon.ReplacesHardHPPercentFactor,'dungeon factor override');
 ok(r.Hunt.TimeoutSeconds===60&&r.Hunt.RetryFromSummonSeconds===90,'hunt timing');
 ok(r.Dungeon.ReturnUnit==='WholeCell'&&r.Dungeon.ReturnAllButton&&r.Dungeon.ReturnTransaction==='AllOrNothing','dungeon return');
 ok(r.Dungeon.PopulationIncludesFieldAndDungeon&&r.Dungeon.MaxStack===3,'dungeon capacity');
 ok(r.Missions.AutoClaim&&!r.Missions.Repeatable&&r.Missions.CompletionLatched,'mission lifecycle');
 ok(new Set(r.Missions.Entries.map(m=>m.Id)).size===r.Missions.Entries.length,'mission IDs');
 ok(r.P0Overrides.FinalWave===10&&r.P0Overrides.AllowedGrades.join()===grades.join()&&r.P0Overrides.EnableStars,'P0 scope');
 ok(!r.P0Overrides.EnableSkills&&!r.P0Overrides.EnableUpgrades&&!r.P0Overrides.EnableDungeon&&!r.P0Overrides.EnableMeta,'P0 phase gates');
 return {checks,errors,totalNormalEnemies:normals,totalBosses:bosses,totalKillGoldPerPlayer:killGold};
}
