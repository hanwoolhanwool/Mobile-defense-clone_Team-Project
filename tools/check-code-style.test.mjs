import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {checkCodeStyle, fixCodeStyle, formatterPath, sourceHash, samplePath, verifyFormatter} from './style/code-style.mjs';

const root = fileURLToPath(new URL('../', import.meta.url));
const binary = formatterPath(root);
const legacyFile = 'Source/Legacy.cpp';
const legacyText = 'int Legacy( ){return 1;}\n';

function fixture(run) {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'defense-code-style-'));
  try {
    fs.mkdirSync(path.join(dir, 'Source'));
    fs.mkdirSync(path.join(dir, 'tools/style'), {recursive: true});
    for (const file of ['.clang-format', samplePath]) fs.copyFileSync(path.join(root, file), path.join(dir, file));
    fs.writeFileSync(path.join(dir, legacyFile), legacyText);
    fs.writeFileSync(path.join(dir, 'tools/style/legacy-baseline.json'), JSON.stringify({
      schemaVersion: 1, formatterVersion: '20.1.8', files: {[legacyFile]: sourceHash(legacyText)},
    }));
    run(dir);
  } finally {
    const resolved = path.resolve(dir);
    assert.ok(resolved.startsWith(path.resolve(os.tmpdir()) + path.sep) && path.basename(resolved).startsWith('defense-code-style-'));
    fs.rmSync(resolved, {recursive: true, force: true});
  }
}

test('unchanged legacy is exempt; Unreal macro sample is always checked', () => fixture(dir => {
  assert.deepEqual(checkCodeStyle(dir, binary), {checked: 1, legacy: 1, errors: []});
}));

test('new malformed code fails read-only; explicit formatting repairs it', () => fixture(dir => {
  const file = 'Source/LDNew.cpp', absolute = path.join(dir, file);
  fs.writeFileSync(absolute, 'int GetValue( ){return 2;}');
  const before = fs.readFileSync(absolute);
  assert.ok(checkCodeStyle(dir, binary).errors.some(error => error.startsWith(file)));
  assert.deepEqual(fs.readFileSync(absolute), before);
  fixCodeStyle(dir, [file], binary);
  assert.deepEqual(checkCodeStyle(dir, binary).errors, []);
  assert.match(fs.readFileSync(absolute, 'utf8'), /\n\{\n\treturn 2;\n\}/);
}));

test('editing an inherited file removes its exemption', () => fixture(dir => {
  fs.writeFileSync(path.join(dir, legacyFile), legacyText.replace('1', '3'));
  const result = checkCodeStyle(dir, binary);
  assert.equal(result.legacy, 0);
  assert.ok(result.errors.some(error => error.startsWith(legacyFile)));
}));

test('renaming an inherited file does not carry its exemption', () => fixture(dir => {
  fs.renameSync(path.join(dir, legacyFile), path.join(dir, 'Source/Renamed.cpp'));
  assert.ok(checkCodeStyle(dir, binary).errors.some(error => error.startsWith('Source/Renamed.cpp')));
}));

test('Git CRLF checkout preserves the unchanged legacy exemption', () => fixture(dir => {
  fs.writeFileSync(path.join(dir, legacyFile), legacyText.replaceAll('\n', '\r\n'));
  assert.equal(checkCodeStyle(dir, binary).legacy, 1);
}));

test('the sample cannot be hidden by a baseline entry', () => fixture(dir => {
  const file = path.join(dir, 'tools/style/legacy-baseline.json');
  const baseline = JSON.parse(fs.readFileSync(file, 'utf8'));
  baseline.files[samplePath] = sourceHash(fs.readFileSync(path.join(dir, samplePath), 'utf8'));
  fs.writeFileSync(file, JSON.stringify(baseline));
  assert.throws(() => checkCodeStyle(dir, binary), /Invalid style baseline entry/);
}));

test('new code cannot disable the formatter', () => fixture(dir => {
  fs.writeFileSync(path.join(dir, 'Source/Bypass.cpp'), '// clang-format off\nint Value=1;\n');
  assert.ok(checkCodeStyle(dir, binary).errors.some(error => error.includes('remove the bypass')));
}));

test('UTF-8 BOM and CRLF in new files fail and are fixed explicitly', () => fixture(dir => {
  const file = 'Source/Encoding.cpp';
  fs.writeFileSync(path.join(dir, file), '\uFEFFint Value = 1;\r\n');
  assert.ok(checkCodeStyle(dir, binary).errors.some(error => error.includes('BOM')));
  fixCodeStyle(dir, [file], binary);
  assert.equal(fs.readFileSync(path.join(dir, file), 'utf8'), 'int Value = 1;\n');
  assert.deepEqual(checkCodeStyle(dir, binary).errors, []);
}));

test('fix validates all paths before writing, and rejects outside paths', () => fixture(dir => {
  assert.throws(() => fixCodeStyle(dir, [legacyFile, '../Outside.cpp'], binary), /Expected an explicit C\+\+ file/);
  assert.equal(fs.readFileSync(path.join(dir, legacyFile), 'utf8'), legacyText);
}));

test('missing and mismatched formatters fail instead of skipping', () => {
  assert.throws(() => verifyFormatter(path.join(root, 'Saved/missing-clang-format')), /unavailable/);
  assert.throws(() => verifyFormatter(process.execPath), /20\.1\.8 required/);
});
