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
  [
    "C01",
    "나무 병정",
    "Common",
    15,
    1,
    "Physical",
    "None"
  ],
  [
    "C02",
    "견습 궁수",
    "Common",
    12,
    0.8,
    "Physical",
    "None"
  ],
  [
    "C03",
    "촛불 정령",
    "Common",
    19,
    1.2,
    "Magic",
    "None"
  ],
  [
    "C04",
    "태엽 조수",
    "Common",
    10,
    0.7,
    "Physical",
    "None"
  ],
  [
    "R01",
    "강철 파수병",
    "Rare",
    55,
    1.1,
    "Physical",
    "S_R01"
  ],
  [
    "R02",
    "톱니 궁수",
    "Rare",
    40,
    0.8,
    "Physical",
    "S_R02"
  ],
  [
    "R03",
    "불씨 마도사",
    "Rare",
    64,
    1.2,
    "Magic",
    "S_R03"
  ],
  [
    "R04",
    "태엽 기술자",
    "Rare",
    38,
    1,
    "Physical",
    "S_R04"
  ],
  [
    "E01",
    "서리 기사",
    "Epic",
    165,
    1,
    "Magic",
    "S_E01"
  ],
  [
    "E02",
    "폭풍 사수",
    "Epic",
    120,
    0.75,
    "Physical",
    "S_E02"
  ],
  [
    "E03",
    "화약 연금술사",
    "Epic",
    215,
    1.25,
    "Magic",
    "S_E03"
  ],
  [
    "E04",
    "태엽 악사",
    "Epic",
    110,
    1,
    "Magic",
    "S_E04"
  ],
  [
    "L01",
    "용광로 거인",
    "Legendary",
    650,
    1.2,
    "Magic",
    "S_L01"
  ],
  [
    "L02",
    "서리 여왕",
    "Legendary",
    460,
    1,
    "Magic",
    "S_L02"
  ],
  [
    "L03",
    "천둥 지휘관",
    "Legendary",
    480,
    0.8,
    "Physical",
    "S_L03"
  ],
  [
    "L04",
    "별의 설계사",
    "Legendary",
    390,
    1,
    "Magic",
    "S_L04"
  ],
  [
    "M01",
    "태양 용광로",
    "Mythic",
    1800,
    1.2,
    "Magic",
    "S_M01"
  ],
  [
    "M02",
    "폭풍 지휘자",
    "Mythic",
    1400,
    0.8,
    "Physical",
    "S_M02"
  ],
  [
    "M03",
    "겨울의 파수꾼",
    "Mythic",
    1300,
    1,
    "Magic",
    "S_M03"
  ],
  [
    "M04",
    "별빛 설계자",
    "Mythic",
    950,
    1,
    "Magic",
    "S_M04"
  ]
];

const cellSizeCm=140;
const units=unitSeeds.map(([Name,DisplayName,Grade,BaseAttack,AttackIntervalSeconds,DamageType,LegacySkillId])=>{
 const ordinal=Number(Name.slice(1)), RangeCells=ordinal===1?1.25:ordinal===2?3.5:2.5;
 return {Name,DisplayName,Grade,DesignStatus:Grade==='Mythic'?'DeferredConcept':'UnplaytestedFixture',EnabledInP0:Grade!=='Mythic',BaseAttack,AttackIntervalSeconds,AttackPresentation:ordinal===1?'Melee':'Projectile',RangeCells,RangeCm:RangeCells*cellSizeCm,DamageType,SkillId:'None',LegacySkillId,MaxStack:3,SaleCurrency:Grade==='Common'?'Gold':'Stars',SalePolicy:Grade==='Common'?'HalfNextPaidSummonFloor':Grade==='Mythic'?'DisabledPendingConcept':'Fixed',SaleAmount:({Rare:1,Epic:2,Legendary:4})[Grade]??0,VisualId:`V_${Name}`};
});
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
write('DT_Skills.json',skills.map(s=>({...s,Enabled:false,DesignStatus:'DeferredConcept'})));
const recipes=[
 ['Recipe_M01','M01','L01','E03','R03'],
 ['Recipe_M02','M02','L03','E02','R02'],
 ['Recipe_M03','M03','L02','E01','R01'],
 ['Recipe_M04','M04','L04','E04','R04']
].map(([Name,OutputUnitId,LegendaryUnitId,EpicUnitId,RareUnitId])=>({Name,OutputUnitId,LegendaryUnitId,EpicUnitId,RareUnitId,LegendaryCount:1,EpicCount:1,RareCount:1,GoldCost:0}));
write('DT_Recipes.json',recipes.map(r=>({...r,Enabled:false,DesignStatus:'DeferredConcept'})));

const enemy=(Name,Kind,HPScale,FixedHP,SpeedCmPerSec,Armor,MagicResistance)=>({Name,DisplayName:Name,Kind,DesignStatus:'UnplaytestedFixture',HPScale,FixedHP,SpeedCmPerSec,Armor,MagicResistance,StunImmune:false,SlowCap:.8,AbilityId:'None'});
write('DT_EnemyTypes.json',[
 enemy('N01','Normal',1,0,150,0,0),
 enemy('N02','Normal',.75,0,220,0,0),
 enemy('N03','Normal',1.6,0,120,60,0),
 enemy('N04','Normal',1.2,0,150,0,.35),
 ...Array.from({length:8},(_,i)=>enemy('B0'+(i+1),'Boss',0,Math.round(6000*1.6**i),100,20+5*i,.1)),
]);
write('DT_SpawnProfiles.json',[
 {Name:'Early',N01Weight:10000,N02Weight:0,N03Weight:0,N04Weight:0},
 {Name:'Runner',N01Weight:7500,N02Weight:2500,N03Weight:0,N04Weight:0},
 {Name:'Armored',N01Weight:5000,N02Weight:2500,N03Weight:2500,N04Weight:0},
 {Name:'Mixed',N01Weight:2500,N02Weight:2500,N03Weight:2500,N04Weight:2500},
]);
const waves=Array.from({length:80},(_,i)=>{
 const w=i+1,boss=w%10===0;
 return {Name:'W'+String(w).padStart(2,'0'),WaveIndex:w,DesignStatus:'UnplaytestedFixture',DurationSeconds:boss?60:20,SpawnWindowSeconds:boss?0:20,SpawnIntervalSeconds:boss?0:1,FirstSpawnOffsetSeconds:0,NormalCountPerGate:boss?0:20,NormalBaseHP:Math.round(70*1.06**(w-1)),SpawnProfileId:boss?'None':'Early',BossId:boss?'B0'+w/10:'None',BossCountPerGate:boss?1:0,BossDeadlineSeconds:boss?60:0,PostBossDelaySeconds:boss?2:0,GoldRewardPerPlayer:0,StarRewardPerPlayer:0};
});
write('DT_Waves.json',waves);
const profile=(Name,Source,Level,CostStars,FailWeight,CommonWeight,RareWeight,EpicWeight,LegendaryWeight)=>({Name,Source,Level,CostStars,FailWeight,CommonWeight,RareWeight,EpicWeight,LegendaryWeight,MythicWeight:0,PityThreshold:0});
write('DT_SummonProfiles.json',[
 ...Array.from({length:11},(_,L)=>profile(L===0?'Gold_Default':'Gold_Lv'+String(L).padStart(2,'0'),'Gold',L,0,0,9743-150*L,198+100*L,49+45*L,10+5*L)),
 profile('Roulette_Rare','Star',0,1,4000,0,6000,0,0),
 profile('Roulette_Epic','Star',0,1,8000,0,0,2000,0),
 profile('Roulette_Legendary','Star',0,2,9000,0,0,0,1000),
]);
write('DT_Upgrades.json',[
 {Name:'Attack_CommonRare',Currency:'Gold',Grades:'Common,Rare',MaxLevel:10,BaseCost:50,CostIncrement:25,Stat:'BaseAttackPct',ValuePerLevel:.5,SuccessWeight:10000},
 {Name:'Attack_Epic',Currency:'Gold',Grades:'Epic',MaxLevel:10,BaseCost:100,CostIncrement:50,Stat:'BaseAttackPct',ValuePerLevel:.5,SuccessWeight:10000},
 {Name:'Attack_LegendaryMythic',Currency:'Stars',Grades:'Legendary,Mythic',MaxLevel:10,BaseCost:2,CostIncrement:1,Stat:'BaseAttackPct',ValuePerLevel:.5,SuccessWeight:10000},
 {Name:'SummonOdds',Currency:'Gold',Grades:'AllSummonable',MaxLevel:10,BaseCost:100,CostIncrement:50,Stat:'SummonProfileLevel',ValuePerLevel:1,SuccessWeight:10000},
]);
const boardColumns=6,boardRows=3;
const boardXCenters=Array.from({length:boardColumns},(_,column)=>(column-(boardColumns-1)/2)*cellSizeCm);
const placementOrder=player=>Array.from({length:6},(_,c)=>5-c).flatMap(c=>(player===0?[2,1,0]:[0,1,2]).map(r=>player*18+r*6+c));
write('GameRules.json',{
 SchemaVersion:2,RulesVersion:'0.3.0',DesignStatus:'Unplaytested',
 Session:{HumanPlayerCount:2,GateCount:2,PreparationSeconds:10,LoadingTimeoutSeconds:30,FinalWave:80,LogicHz:20,ResultFinalWaveRewards:false,PauseOnline:false},
 Board:{Columns:boardColumns,Rows:boardRows,CellsPerPlayer:18,MaxUnitsPerPlayer:20,PopulationIncludesDungeon:true,MaxStack:3,CellSizeCm:cellSizeCm,XCentersCm:boardXCenters,YCentersByPlayer:[[-420,-280,-140],[140,280,420]],PlacementOrderByPlayer:[placementOrder(0),placementOrder(1)],PlacementPolicy:'SameUnitPartialThenLocalColumnTopDown',MergeUsesSelectedCell:false,MaxPartialStacksPerUnitPerArea:1,SaleCompaction:'TransferFromExistingPartialToSoldStack',MoveMode:'WholeStackDragSwap',ManualIndividualSplit:false,TransferPreservesInstanceAndTimers:true,MoveLockSeconds:.3,VisualMoveSeconds:.15,InitialAttackDelaySeconds:.25},
 Paths:{Closed:true,LinearSegments:true,WidthCm:140,SharedSegmentCm:[[490,0,0],[-490,0,0]],PointsByGateCm:[[[490,-560,0],[490,0,0],[-490,0,0],[-490,-560,0]],[[490,560,0],[490,0,0],[-490,0,0],[-490,560,0]]],LengthPerGateCm:3080,GateStartDistancesCm:[0,0]},
 Defeat:{CountedEnemyKinds:['Normal'],ActiveEnemyThreshold:100,ContinuousSeconds:0,BossDeadlineSeconds:60,RequireBothWaveBossesDead:true,HuntTimeoutDefeatsTeam:false,EnemyPoolStressCount:128},
 Victory:{RequiresFinalSpawnsComplete:true,RequiresNoNormalEnemies:true,RequiresBothFinalBossesDead:true,HuntBlocksVictory:false,HardRequiresDungeonComplete:true},
 Timing:{DamageAtDeadlineCounts:true,Order:'CommandsThenDueDamageThenDeathsAndRewardsThenSpawnsAndCountDefeatThenDeadlinesThenVictoryThenNextWave',CommandsSerial:true,RequestDedupKey:'MatchId+PlayerId+ConnectionEpoch+RequestId',RewardDedupKey:'MatchId+EventId+RecipientId',RejectedRequestConsumesRng:false},
 Economy:{StartingGold:100,StartingStars:0,NormalKillGoldPerPlayer:1,NormalKillGoldFromWave21:2,KillRateUsesSpawnWave:true,RewardsToBothPlayers:true,BossKillGoldPerPlayer:100,BossKillStarsPerPlayer:2,FastBossKillSeconds:30,FastBossBonusStarsPerPlayer:1,FastBossBonusPerBoss:true,HuntKillStarsPerPlayer:2,WaveEntryGold:0,WaveEndGold:0,WaveBaseRewardsEnabled:false,SummonBaseGold:20,SummonIncrementGold:2,SummonMaxGold:null,FreeSummonIncrementsPaidCounter:false,SummonAdmission:'BeforeRngRequirePopulationAndCapacityForEveryPossibleUnit',CommonSaleFractionOfNextPaidPrice:.5,CommonSaleRounding:'Floor',RareSaleStars:1,EpicSaleStars:2,LegendarySaleStars:4,GoldProfile:'Gold_Default',RouletteProfiles:['Roulette_Rare','Roulette_Epic','Roulette_Legendary'],RouletteFailureConsumesCost:true},
 Combat:{MaximumAttacksPerSecond:8,AuraRefreshSeconds:.25,SlowCap:.8,ArmorFloor:-50,ResistanceCap:.75,CritEnabled:false,RoundMode:'PositiveHalfUp',StunStacking:'LatestEndTime',SlowStacking:'SumDistinctSourcesAfterDifficultyThenCap',ArmorBreakStacking:'SumDistinctSourcesRefreshSameSource',BuffStacking:'SumDistinctInstancesRefreshSameSource',DamageOrder:['BaseAndPermanentGrowth','BattleUpgrade','AttackBuff','SkillCoefficient','TargetMitigation'],HPPercentUsesAttackOrCrit:false,DefaultManaCap:100,DefaultManaPerSecond:2,DefaultManaPerBasicAttack:1,ManaPerTargetHit:false,MaxCooldownReduction:.5,HasteAffectsSkillCooldown:false,DotSameSource:'Refresh',DotDistinctSources:'Coexist',SupportsTargetPriorityBoss:true},
 Difficulty:{Normal:{StunDurationFactor:1,SlowStrengthFactor:1,HPPercentDamageFactor:1},Hard:{StunDurationFactor:.5,SlowStrengthFactor:.5,HPPercentDamageFactor:.7},Dungeon:{HPPercentDamageFactor:.3,ReplacesHardHPPercentFactor:true}},
 Hunt:{FirstAvailableWave:6,FirstAvailableWaveOffsetSeconds:10,TimeoutSeconds:60,RetryFromSummonSeconds:90,MaxActivePerPlayer:1,SuccessAdvancesStage:true,FailureKeepsStage:true},
 Dungeon:{EnabledIn:'Hard',CellsPerPlayer:3,MaxStack:3,EnterCount:1,ReturnUnit:'WholeCell',ReturnAllButton:true,ReturnTransaction:'AllOrNothing',PopulationIncludesFieldAndDungeon:true,GlobalPassiveScope:'OwnerFieldAndDungeon',LocalAuraScope:'SameArea',BossStatsAndRewardsStatus:'PendingContent'},
 Missions:{Repeatable:false,AutoClaim:true,Reset:'NewMatch',HoldingsScope:'OwnerFieldAndDungeon',CompletionLatched:true,Entries:[
  {Id:'CollectCommon',Metric:'CurrentAllDistinctCommon',Target:4,Gold:100,Stars:0,Scope:'Player'},
  {Id:'CollectRare',Metric:'CurrentAllDistinctRare',Target:4,Gold:100,Stars:0,Scope:'Player'},
  {Id:'CollectEpic',Metric:'CurrentAllDistinctEpic',Target:4,Gold:0,Stars:1,Scope:'Player'},
  {Id:'HoldLegendary',Metric:'CurrentLegendaryCount',Target:3,Gold:0,Stars:2,Scope:'Player'},
  {Id:'TeamKills40',Metric:'CumulativeNormalKills',Target:40,Gold:100,Stars:0,Scope:'TeamEachPlayer'},
  {Id:'Sell5',Metric:'CumulativeIndividualsSold',Target:5,Gold:50,Stars:0,Scope:'Player'},
  {Id:'Roulette20',Metric:'CumulativePaidRouletteAttempts',Target:20,Gold:0,Stars:1,Scope:'Player'},
  {Id:'Upgrade2',Metric:'CumulativeSuccessfulUpgrades',Target:2,Gold:50,Stars:0,Scope:'Player'},
 ]},
 Meta:{Phase:'P1',Systems:['Treasure','Relic','Pet'],Acquisition:true,Inventory:true,Growth:true,DuplicateConversion:true,Persistence:'VersionedLocalSave',CatalogAndCostsStatus:'PendingContent',ServerAuthorityPhase:'P2'},
 Connectivity:{BotStartAfterDisconnectSeconds:5,ReconnectGraceSeconds:60,CommandRatePerSecond:8,CommandBurst:12,RequestResultCacheCount:256,PingCooldownSeconds:2},
 Progression:{MaxLevel:20,NextLevelBaseXP:100,NextLevelXPIncrement:50,XPPerCompletedWave:10,WinBonusXP:100,PracticeXP:0,TutorialXP:0,DesignStatus:'ProvisionalLegacyNotActiveP0'},
 TestProfiles:[{Id:'Baseline',EnemyLimit:100,PopulationPerPlayer:20,MetaEffects:false},{Id:'AdjustedFixture',EnemyLimit:113,PopulationPerPlayer:26,MetaEffects:false}],
 P0Overrides:{FinalWave:10,AllowedGrades:['Common','Rare','Epic','Legendary'],AllowedNormalEnemyIds:['N01'],NormalSpawnProfileId:'Early',GoldProfile:'Gold_Default',MergeInputGrades:['Common','Rare','Epic'],EnableStars:true,EnableSkills:false,EnableRoulette:false,EnableUpgrades:false,EnableCraft:false,EnableExchange:false,EnableAccountXP:false,EnableHunt:false,EnableDungeon:false,EnableMissions:false,EnableMeta:false},
});
console.log((checkOnly?'Checked':'Generated')+' 8 DataTables and GameRules.json'+(process.exitCode?' (FAIL)':''));
