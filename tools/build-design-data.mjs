// Initial design data, not a gameplay simulator. Edit this file and regenerate.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const out = path.join(root, 'data');
const checkOnly = process.argv.includes('--check');
if (!checkOnly) fs.mkdirSync(out, { recursive: true });
const write = (name, value) => {
 const file=path.join(out,name), expected=JSON.stringify(value,null,2)+'\n';
 if(checkOnly){
  if(!fs.existsSync(file)||fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')!==expected){
   console.error(`Stale generated data: data/${name}; run node tools/build-design-data.mjs`);
   process.exitCode=1;
  }
 }else fs.writeFileSync(file,expected,'utf8');
};

const unitSeeds = [
  ['C01','나무 병정','Common',15,1,320,'Physical','None',3,5],
  ['C02','견습 궁수','Common',12,.8,430,'Physical','None',3,5],
  ['C03','촛불 정령','Common',19,1.2,380,'Magic','None',3,5],
  ['C04','태엽 조수','Common',10,.7,350,'Physical','None',3,5],
  ['R01','강철 파수병','Rare',55,1.1,330,'Physical','S_R01',3,15],
  ['R02','톱니 궁수','Rare',40,.8,470,'Physical','S_R02',3,15],
  ['R03','불씨 마도사','Rare',64,1.2,410,'Magic','S_R03',3,15],
  ['R04','태엽 기술자','Rare',38,1,380,'Physical','S_R04',3,15],
  ['E01','서리 기사','Epic',165,1,360,'Magic','S_E01',3,45],
  ['E02','폭풍 사수','Epic',120,.75,520,'Physical','S_E02',3,45],
  ['E03','화약 연금술사','Epic',215,1.25,440,'Magic','S_E03',3,45],
  ['E04','태엽 악사','Epic',110,1,420,'Magic','S_E04',3,45],
  ['L01','용광로 거인','Legendary',650,1.2,400,'Magic','S_L01',1,120],
  ['L02','서리 여왕','Legendary',460,1,480,'Magic','S_L02',1,120],
  ['L03','천둥 지휘관','Legendary',480,.8,560,'Physical','S_L03',1,120],
  ['L04','별의 설계사','Legendary',390,1,440,'Magic','S_L04',1,120],
  ['M01','태양 용광로','Mythic',1800,1.2,460,'Magic','S_M01',1,300],
  ['M02','폭풍 지휘자','Mythic',1400,.8,620,'Physical','S_M02',1,300],
  ['M03','겨울의 파수꾼','Mythic',1300,1,500,'Magic','S_M03',1,300],
  ['M04','별빛 설계자','Mythic',950,1,500,'Magic','S_M04',1,300],
];
const units=unitSeeds.map(([Name,DisplayName,Grade,BaseAttack,AttackIntervalSeconds,RangeCm,DamageType,SkillId,MaxStack,SellGold])=>({Name,DisplayName,Grade,BaseAttack,AttackIntervalSeconds,RangeCm,DamageType,SkillId,MaxStack,SellGold,VisualId:`V_${Name}`}));
write('DT_Units.json',units);

const skill=(Name,values)=>({Name,Trigger:'Cooldown',EffectType:'AreaDamage',CooldownSeconds:0,TriggerAttackCount:0,DamageCoefficient:0,DamageType:'Magic',RadiusCm:0,MaxTargets:1,ChainRadiusCm:0,SlowPct:0,StunSeconds:0,ArmorBreak:0,DurationSeconds:0,DotCoefficient:0,DotTicks:0,DotIntervalSeconds:0,AuraAttackPct:0,AuraHastePct:0,BossBonusCoefficient:0,...values});
const skills=[
 skill('S_R01',{Trigger:'AttackCount',EffectType:'Stun',TriggerAttackCount:4,StunSeconds:.4,DurationSeconds:.4}),
 skill('S_R02',{Trigger:'AttackCount',EffectType:'ExtraShot',TriggerAttackCount:4,DamageCoefficient:1,DamageType:'Physical'}),
 skill('S_R03',{CooldownSeconds:6,DamageCoefficient:.8,RadiusCm:150,MaxTargets:5}),
 skill('S_R04',{Trigger:'Aura',EffectType:'Aura',RadiusCm:300,MaxTargets:72,AuraHastePct:.08}),
 skill('S_E01',{EffectType:'AreaDebuff',CooldownSeconds:5,RadiusCm:170,MaxTargets:6,SlowPct:.30,DurationSeconds:2}),
 skill('S_E02',{Trigger:'AttackCount',EffectType:'Chain',TriggerAttackCount:5,DamageCoefficient:.8,DamageType:'Physical',MaxTargets:3,ChainRadiusCm:250}),
 skill('S_E03',{CooldownSeconds:5,DamageCoefficient:1.5,RadiusCm:180,MaxTargets:6}),
 skill('S_E04',{Trigger:'Aura',EffectType:'Aura',RadiusCm:380,MaxTargets:72,AuraAttackPct:.12}),
 skill('S_L01',{CooldownSeconds:5,DamageCoefficient:2,RadiusCm:210,MaxTargets:8}),
 skill('S_L02',{EffectType:'AreaDebuff',CooldownSeconds:4,RadiusCm:220,MaxTargets:8,SlowPct:.45,DurationSeconds:3}),
 skill('S_L03',{Trigger:'AttackCount',EffectType:'Chain',TriggerAttackCount:4,DamageCoefficient:1,DamageType:'Physical',MaxTargets:4,ChainRadiusCm:250}),
 skill('S_L04',{Trigger:'Aura',EffectType:'Aura',RadiusCm:450,MaxTargets:72,AuraAttackPct:.20}),
 skill('S_M01',{EffectType:'AreaBurn',CooldownSeconds:5,DamageCoefficient:2.2,RadiusCm:240,MaxTargets:10,DurationSeconds:3,DotCoefficient:.35,DotTicks:3,DotIntervalSeconds:1}),
 skill('S_M02',{Trigger:'AttackCount',EffectType:'ChainBoss',TriggerAttackCount:3,DamageCoefficient:1,DamageType:'Physical',MaxTargets:5,ChainRadiusCm:250,BossBonusCoefficient:1.5}),
 skill('S_M03',{EffectType:'AreaDebuff',CooldownSeconds:4,DamageCoefficient:.8,RadiusCm:240,MaxTargets:10,SlowPct:.50,ArmorBreak:30,DurationSeconds:3}),
 skill('S_M04',{Trigger:'Aura',EffectType:'Aura',RadiusCm:600,MaxTargets:72,AuraAttackPct:.30,AuraHastePct:.15}),
];
write('DT_Skills.json',skills);
const recipes=[
 ['Recipe_M01','M01','L01','E03','R03'],
 ['Recipe_M02','M02','L03','E02','R02'],
 ['Recipe_M03','M03','L02','E01','R01'],
 ['Recipe_M04','M04','L04','E04','R04']
].map(([Name,OutputUnitId,LegendaryUnitId,EpicUnitId,RareUnitId])=>({Name,OutputUnitId,LegendaryUnitId,EpicUnitId,RareUnitId,LegendaryCount:1,EpicCount:1,RareCount:1,GoldCost:0}));
write('DT_Recipes.json',recipes);

const enemy=(Name,DisplayName,Kind,HPScale,FixedHP,SpeedCmPerSec,Armor,MagicResistance,values={})=>({Name,DisplayName,Kind,HPScale,FixedHP,SpeedCmPerSec,Armor,MagicResistance,StunImmune:Kind==='Boss',SlowCap:Kind==='Boss'?.25:.5,AbilityId:'None',AbilityIntervalSeconds:0,AbilityDurationSeconds:0,SpeedBuffPct:0,...values});
write('DT_EnemyTypes.json',[
 enemy('N01','먼지 인형','Normal',1,0,150,0,0),
 enemy('N02','질주 팽이','Normal',.75,0,220,0,0),
 enemy('N03','철갑 장난감','Normal',1.6,0,120,60,0),
 enemy('N04','절연 인형','Normal',1.2,0,150,0,.35),
 enemy('B01','고철 골렘','Boss',0,6000,100,20,.10),
 enemy('B02','폭주 기관장','Boss',0,30000,150,35,.15,{AbilityId:'SelfHaste',AbilityIntervalSeconds:8,AbilityDurationSeconds:3,SpeedBuffPct:.5}),
 enemy('B03','균열 감독관','Boss',0,90000,120,40,.20,{AbilityId:'NormalEnemyHaste',AbilityIntervalSeconds:10,AbilityDurationSeconds:4,SpeedBuffPct:.2}),
]);
write('DT_SpawnProfiles.json',[
 {Name:'Early',N01Weight:10000,N02Weight:0,N03Weight:0,N04Weight:0},
 {Name:'Runner',N01Weight:7500,N02Weight:2500,N03Weight:0,N04Weight:0},
 {Name:'Armored',N01Weight:5000,N02Weight:2500,N03Weight:2500,N04Weight:0},
 {Name:'Mixed',N01Weight:2500,N02Weight:2500,N03Weight:2500,N04Weight:2500},
]);
const waves=Array.from({length:30},(_,i)=>{
 const w=i+1,boss=w%10===0;
 return {Name:`W${String(w).padStart(2,'0')}`,WaveIndex:w,DurationSeconds:30,SpawnWindowSeconds:boss?0:20,NormalCountPerGate:boss?0:12+Math.floor((w-1)/3),NormalBaseHP:Math.round(70*1.12**(w-1)),SpawnProfileId:boss?'None':w<=3?'Early':w<=9?'Runner':w<=19?'Armored':'Mixed',BossId:boss?`B0${w/10}`:'None',BossCountPerGate:boss?1:0,GoldRewardPerPlayer:20+3*w,StarRewardPerPlayer:(w%2===0?1:0)+(boss?4:0)};
});
write('DT_Waves.json',waves);
const profile=(Name,Source,CostStars,FailWeight,CommonWeight,RareWeight,EpicWeight,LegendaryWeight,CounterMode,PityThreshold,GuaranteedProfileId)=>({Name,Source,CostStars,FailWeight,CommonWeight,RareWeight,EpicWeight,LegendaryWeight,MythicWeight:0,CounterMode,PityThreshold,GuaranteedProfileId});
write('DT_SummonProfiles.json',[
 profile('Gold_Default','Gold',0,0,8200,1500,300,0,'ConsecutiveCommon',14,'Gold_Pity'),
 profile('Gold_Pity','Gold',0,0,0,8500,1500,0,'ConsecutiveCommon',0,'None'),
 profile('Star_Default','Star',2,5000,0,3000,1700,300,'ConsecutiveBelowEpic',4,'Star_Pity'),
 profile('Star_Pity','Star',2,0,0,0,9000,1000,'ConsecutiveBelowEpic',0,'None'),
 profile('P0_Gold_Default','Gold',0,0,8500,1500,0,0,'ConsecutiveCommon',14,'P0_Gold_Pity'),
 profile('P0_Gold_Pity','Gold',0,0,0,10000,0,0,'ConsecutiveCommon',0,'None'),
]);
write('DT_Upgrades.json',[
 {Name:'AttackTraining',MaxLevel:8,BaseCost:60,CostIncrement:40,Stat:'AttackPct',ValuePerLevel:.10},
 {Name:'DeviceHaste',MaxLevel:5,BaseCost:80,CostIncrement:60,Stat:'AttackSpeedPct',ValuePerLevel:.05},
]);
write('GameRules.json',{
 SchemaVersion:1,RulesVersion:'0.1.0',DesignStatus:'Unplaytested',
 Session:{HumanPlayerCount:2,GateCount:2,PreparationSeconds:10,LoadingTimeoutSeconds:30,FinalWave:30,LogicHz:20,ResultFinalWaveRewards:true,PauseOnline:false},
 Board:{Columns:4,Rows:3,CellsPerPlayer:12,MaxUnitsPerPlayer:36,CellSizeCm:140,XCentersCm:[-210,-70,70,210],YCentersByPlayer:[[-470,-330,-190],[190,330,470]],MoveLockSeconds:.3,VisualMoveSeconds:.15,InitialAttackDelaySeconds:.25},
 Path:{PointsCm:[[-350,-650,0],[350,-650,0],[350,650,0],[-350,650,0]],Closed:true,LinearSegments:true,LengthCm:4000,GateDistancesCm:[0,2000]},
 Defeat:{ActiveEnemyThreshold:100,ContinuousSeconds:3,FinalWaveRequiresNoEnemies:true,BossDeadlineIsWaveEnd:true,EnemyPoolStressCount:128},
 Economy:{StartingGold:140,StartingStars:0,GoldPerTeamNormalKillPerPlayer:1,GoldPerBossKill:0,SummonBaseGold:20,SummonIncrementGold:2,SummonMaxGold:60,RequireEmptyCellForAnySummon:true,GoldProfile:'Gold_Default',StarProfile:'Star_Default',ExchangeEpicGold:100,ExchangeLegendaryGold:200,ExchangeLimitPerPlayer:2},
 Combat:{MinimumAttackIntervalSeconds:.25,AuraRefreshSeconds:.25,NormalSlowCap:.5,BossSlowCap:.25,ResistanceCap:.75,ArmorFloor:0,CritEnabled:false,RoundMode:'PositiveHalfUp',SupportsTargetPriorityBoss:true},
 Connectivity:{BotStartAfterDisconnectSeconds:5,ReconnectGraceSeconds:60,CommandRatePerSecond:8,CommandBurst:12,RequestResultCacheCount:256,PingCooldownSeconds:2},
 Progression:{MaxLevel:20,NextLevelBaseXP:100,NextLevelXPIncrement:50,XPPerCompletedWave:10,WinBonusXP:100,PracticeXP:0,TutorialXP:0},
 P0Overrides:{FinalWave:10,AllowedGrades:['Common','Rare'],AllowedNormalEnemyIds:['N01'],NormalSpawnProfileId:'Early',GoldProfile:'P0_Gold_Default',AllowRareMerge:false,EnableStars:false,EnableCraft:false,EnableExchange:false,EnableAccountXP:false},
});
console.log(`${checkOnly?'Checked':'Generated'} 8 DataTables and GameRules.json${process.exitCode?' (FAIL)':''}`);
