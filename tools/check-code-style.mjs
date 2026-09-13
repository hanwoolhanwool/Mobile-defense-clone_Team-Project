import {fileURLToPath} from 'node:url';
import {checkCodeStyle, fixCodeStyle} from './style/code-style.mjs';

const root = fileURLToPath(new URL('../', import.meta.url));
try {
  const args = process.argv.slice(2);
  if (args[0] === '--fix' && args.length > 1) {
    console.log(`Formatted ${fixCodeStyle(root, args.slice(1))} explicit file(s). Review the diff and run the check again.`);
  } else if (args.length === 0) {
    const result = checkCodeStyle(root);
    console.log(`Code style: ${result.checked} checked (includes formatting sample), ${result.legacy} unchanged legacy files, ${result.errors.length} errors.`);
    for (const error of result.errors) console.error(error);
    if (result.errors.length) process.exitCode = 1;
  } else {
    throw new Error('Usage: node tools/check-code-style.mjs [--fix Source/path/File.h ...]');
  }
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
