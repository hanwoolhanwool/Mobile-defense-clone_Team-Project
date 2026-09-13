// Read-only checks: a stale generated file fails instead of being repaired by CI.
import {spawnSync} from 'node:child_process';
import {projectRoot} from './build-planning.mjs';

for(const args of [
 ['tools/build-design-data.mjs','--check'],
 ['tools/validate-design-data.mjs','--check'],
 ['tools/validate-planning.mjs'],
 ['tools/check-code-style.mjs'],
]){
 console.log(`> node ${args.join(' ')}`);
 const result=spawnSync(process.execPath,args,{cwd:projectRoot,stdio:'inherit'});
 if(result.error||result.status!==0){
  if(result.error)console.error(result.error.message);
  process.exit(result.status||1);
 }
}
