import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';

export const formatterVersion = '20.1.8';
export const samplePath = 'tools/style/LDStyleExample.h';
const extensions = new Set(['.h', '.hpp', '.cpp', '.cc', '.cxx', '.inl']);

// Git may check out LF as CRLF on Windows. Only that conversion is equivalent.
export const sourceHash = text => createHash('sha256').update(text.replaceAll('\r\n', '\n')).digest('hex');

export function formatterPath(root) {
  return process.env.CLANG_FORMAT || path.join(root, 'Saved', 'Tooling', 'code-style',
    process.platform === 'win32' ? 'Scripts/clang-format.exe' : 'bin/clang-format');
}

export function verifyFormatter(binary) {
  const result = spawnSync(binary, ['--version'], {encoding: 'utf8'});
  if (result.error || result.status !== 0) {
    throw new Error(`clang-format unavailable: ${binary}. See docs/technical/CODE_STYLE.md. ${result.error?.message || result.stderr}`);
  }
  const actual = result.stdout.match(/clang-format version (\d+\.\d+\.\d+)(?:\s|$)/)?.[1];
  if (actual !== formatterVersion) {
    throw new Error(`clang-format ${formatterVersion} required; received: ${result.stdout.trim()}`);
  }
}

export function sourceFiles(root) {
  const files = [];
  function walk(relative) {
    for (const entry of fs.readdirSync(path.join(root, relative), {withFileTypes: true})) {
      const name = `${relative}/${entry.name}`;
      if (entry.isSymbolicLink()) throw new Error(`Source symlinks are not supported: ${name}`);
      if (entry.isDirectory()) walk(name);
      else if (extensions.has(path.extname(name).toLowerCase())) files.push(name);
    }
  }
  walk('Source');
  return files.sort();
}

function readSource(root, relative) {
  const buffer = fs.readFileSync(path.join(root, relative));
  // Preserve a BOM for comparison; new/modified files must be UTF-8 without BOM.
  return new TextDecoder('utf-8', {fatal: true, ignoreBOM: true}).decode(buffer);
}

function readBaseline(root) {
  const baseline = JSON.parse(fs.readFileSync(path.join(root, 'tools/style/legacy-baseline.json'), 'utf8'));
  if (baseline.schemaVersion !== 1 || baseline.formatterVersion !== formatterVersion || !baseline.files) {
    throw new Error('Invalid style baseline schema/version');
  }
  for (const [name, hash] of Object.entries(baseline.files)) {
    if (!name.startsWith('Source/') || name.includes('\\') || name.split('/').includes('..') ||
        !extensions.has(path.extname(name).toLowerCase()) || !/^[a-f0-9]{64}$/.test(hash)) {
      throw new Error(`Invalid style baseline entry: ${name}`);
    }
  }
  return baseline.files;
}

function format(root, binary, relative, input) {
  const result = spawnSync(binary, [
    `--style=file:${path.join(root, '.clang-format')}`,
    '--fail-on-incomplete-format', `--assume-filename=${relative}`,
  ], {cwd: root, input, encoding: 'utf8', maxBuffer: 16 * 1024 * 1024});
  if (result.error || result.status !== 0) {
    throw new Error(`${relative}: formatter failed: ${result.error?.message || result.stderr}`);
  }
  return result.stdout;
}

export function checkCodeStyle(root, binary = formatterPath(root)) {
  verifyFormatter(binary);
  const baseline = readBaseline(root);
  const errors = [];
  let checked = 0, legacy = 0;
  for (const relative of [...sourceFiles(root), samplePath]) {
    const input = readSource(root, relative);
    if (relative !== samplePath && Object.hasOwn(baseline, relative) && sourceHash(input) === baseline[relative]) {
      legacy++;
      continue;
    }
    checked++;
    if (input.startsWith('\uFEFF')) errors.push(`${relative}: UTF-8 BOM is not allowed`);
    if (/\/[/\*]\s*clang-format\s+off\b/.test(input)) errors.push(`${relative}: clang-format off requires a policy change; remove the bypass`);
    if (format(root, binary, relative, input) !== input) {
      errors.push(`${relative}: format differs; run node tools/check-code-style.mjs --fix ${relative}`);
    }
  }
  return {checked, legacy, errors};
}

export function fixCodeStyle(root, files, binary = formatterPath(root)) {
  verifyFormatter(binary);
  const allowed = new Set([...sourceFiles(root), samplePath]);
  // Resolve every input before writing anything. Never expand a folder or glob.
  const selected = files.map(file => path.relative(root, path.resolve(root, file)).split(path.sep).join('/'));
  for (const relative of selected) {
    if (!allowed.has(relative)) throw new Error(`Expected an explicit C++ file under Source or ${samplePath}: ${relative}`);
  }
  for (const relative of new Set(selected)) {
    const input = readSource(root, relative).replace(/^\uFEFF/, '');
    fs.writeFileSync(path.join(root, relative), format(root, binary, relative, input), 'utf8');
  }
  return selected.length;
}
