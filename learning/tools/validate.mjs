// Optional, read-only checks. Kept inside learning so removal needs no CI change.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

const learningRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const projectRoot = path.dirname(learningRoot);
const errors = [];
let links = 0;

function filesIn(directory) {
  if (!fs.existsSync(directory)) return [];
  return fs.readdirSync(directory, {withFileTypes: true}).flatMap(entry => {
    const target = path.join(directory, entry.name);
    return entry.isDirectory() ? filesIn(target) : entry.isFile() ? [target] : [];
  });
}

const relative = file => path.relative(projectRoot, file).replaceAll('\\', '/');
const inside = (parent, target) => {
  const rel = path.relative(parent, target);
  return rel === '' || (!rel.startsWith('..' + path.sep) && rel !== '..' && !path.isAbsolute(rel));
};
const read = file => fs.readFileSync(file, 'utf8');
const withoutFences = text => text.replace(/^```[^\n]*\n[\s\S]*?^```\s*$/gm, '');

function localLinks(file) {
  return [...withoutFences(read(file)).matchAll(/!?\[[^\]\n]*\]\(([^)\n]+)\)/g)].flatMap(match => {
    const raw = match[1].trim().replace(/^<([^>]+)>$/, '$1');
    if (/^[a-z][\w+.-]*:/i.test(raw) || raw.startsWith('//')) return [];
    try {
      const [urlPath, anchor = ''] = raw.split('#');
      const target = urlPath ? path.resolve(path.dirname(file), decodeURIComponent(urlPath)) : file;
      return [{target, anchor: decodeURIComponent(anchor), raw}];
    } catch {
      errors.push(`${relative(file)}: invalid link ${raw}`);
      return [];
    }
  });
}

// P0 was intentionally removed by DEC-035. Validate it again when recreated.
const phases = ['P1', 'P2'];
if (fs.existsSync(path.join(learningRoot, 'P0'))) phases.unshift('P0');
for (const phase of phases) {
  for (const file of ['README.md', 'COMMON.md', 'A/README.md', 'B/README.md', 'INTEGRATION.md']) {
    if (!fs.existsSync(path.join(learningRoot, phase, file))) errors.push(`Missing ${phase}/${file}`);
  }
}

const markdown = filesIn(learningRoot).filter(file => file.endsWith('.md'));
for (const file of markdown) {
  for (const link of localLinks(file)) {
    links++;
    if (!inside(projectRoot, link.target) || !fs.existsSync(link.target)) {
      errors.push(`${relative(file)}: missing/outside target ${link.raw}`);
    } else if (link.anchor && link.target.endsWith('.md')) {
      const ids = [...read(link.target).matchAll(/<a\s+id=["']([^"']+)["']/g)].map(match => match[1]);
      if (!ids.includes(link.anchor)) errors.push(`${relative(file)}: missing explicit anchor ${link.raw}`);
    }
  }
  let columns = null;
  for (const [index, line] of withoutFences(read(file)).split(/\r?\n/).entries()) {
    if (!line.trim().startsWith('|')) { columns = null; continue; }
    const count = line.trim().split(/(?<!\\)\|/).length - 2;
    if (columns !== null && count !== columns) errors.push(`${relative(file)}:${index + 1}: table column mismatch`);
    columns = count;
  }
}

const rootFiles = fs.readdirSync(projectRoot, {withFileTypes: true})
  .filter(entry => entry.isFile()).map(entry => path.join(projectRoot, entry.name));
const permanentDocs = [...rootFiles, ...filesIn(path.join(projectRoot, 'docs')),
  ...filesIn(path.join(projectRoot, '.github'))].filter(file => file.endsWith('.md'));
for (const file of permanentDocs) {
  for (const link of localLinks(file)) {
    if (inside(learningRoot, link.target)) errors.push(`${relative(file)}: permanent Markdown link depends on learning: ${link.raw}`);
  }
}

const codeFiles = [...rootFiles, ...['Source', 'Config', 'tools', '.github'].flatMap(dir => filesIn(path.join(projectRoot, dir)))];
const textExtensions = /\.(?:h|hpp|cpp|cs|ini|uproject|json|mjs|js|ts|ps1|py|yml|yaml|toml|sh|bat|cmd)$/i;
for (const file of codeFiles.filter(file => textExtensions.test(file))) {
  if (/(?:^|[\/\\\s"'`(])learning[\/\\]/im.test(read(file))) {
    errors.push(`${relative(file)}: permanent source/config/tool text references learning/`);
  }
}

console.log(JSON.stringify({result: errors.length ? 'FAIL' : 'PASS', phases: phases.length,
  markdownFiles: markdown.length, localLinks: links, errors}, null, 2));
// Text checks only: this does not inspect Unreal binary references or execute gameplay.
process.exitCode = errors.length ? 1 : 0;
