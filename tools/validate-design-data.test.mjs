import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {validateDesign} from './design-validation.mjs';
const names=['GameRules',...['Units','Skills','Recipes','EnemyTypes','SpawnProfiles','Waves','SummonProfiles','Upgrades'].map(n=>'DT_'+n)];
const load=()=>Object.fromEntries(names.map(n=>[n,JSON.parse(fs.readFileSync(new URL('../data/'+n+'.json',import.meta.url),'utf8'))]));
test('accepted design tables satisfy static contracts',()=>assert.deepEqual(validateDesign(load()).errors,[]));
for(const [label,mutate,expected]of [
 ['probability mass',d=>{d.DT_SummonProfiles[0].CommonWeight++;},'weights'],
 ['wave-entry reward regression',d=>{d.GameRules.Economy.WaveEntryGold=20;},'no wave reward'],
 ['old legendary stack capacity',d=>{d.DT_Units.find(u=>u.Grade==='Legendary').MaxStack=1;},'stack capacity'],
 ['deadline equality regression',d=>{d.GameRules.Timing.DamageAtDeadlineCounts=false;},'time/RNG boundary'],
 ['partial dungeon return regression',d=>{d.GameRules.Dungeon.ReturnTransaction='Partial';},'dungeon return'],
 ['selected-cell merge regression',d=>{d.GameRules.Board.MergeUsesSelectedCell=true;},'stack invariants'],
 ['wave spawn overflow',d=>{d.DT_Waves[0].FirstSpawnOffsetSeconds=2;},'last spawn outside wave'],
 ['reversed central route',d=>{d.GameRules.Paths.PointsByGateCm[1].reverse();},'shared direction'],
 ['accidental concept skill activation',d=>{d.DT_Skills[0].Enabled=true;},'deferred content enabled'],
]){
 test('rejects '+label,()=>{const d=load();mutate(d);assert.ok(validateDesign(d).errors.some(e=>e.includes(expected)));});
}
