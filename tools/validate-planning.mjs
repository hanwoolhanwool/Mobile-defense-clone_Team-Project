import {validatePlanning} from './planning-checks.mjs';

try {
 const result=validatePlanning();
 console.log(JSON.stringify(result,null,2));
 if(result.errors.length)process.exitCode=1;
} catch(error) {
 console.error(`Planning validation failed: ${error.message}`);
 process.exitCode=1;
}
