// Offline fixture input selection only. It does not change game data or reroll a running match.
// UE 5.8 Core/Public/Math/RandomStream.h: MutateSeed, GetFraction (Seed >> 9), RandHelper.
// Confirm the returned sequence against the actual EconomyService before using it as evidence.
const count = Number(process.argv[2] ?? 7);
if (!Number.isInteger(count) || count < 1 || count > 10) throw new Error('Expected a count from 1 to 10.');
function sequence(initial, length) {
  let state = initial >>> 0;
  const draw = n => {
    state = (Math.imul(state, 196314165) + 907633515) >>> 0;
    return Math.trunc(Math.fround((state >>> 9) / 8388608 * n));
  };
  const rows = [];
  for (let i = 0; i < length; ++i) {
    const gradeDraw = draw(10000);
    const grade = gradeDraw < 9743 ? 'C' : gradeDraw < 9941 ? 'R' : gradeDraw < 9990 ? 'E' : 'L';
    rows.push({unit: `${grade}0${draw(4) + 1}`, gradeDraw, state});
  }
  return rows;
}
let found = false;
for (let seed = 0; seed < 10000000; ++seed) {
  const rows = sequence(seed, count);
  if (rows.every(row => row.unit === 'C01')) {
    console.log(JSON.stringify({scope: 'Offline candidate; actual UE execution not yet verified', engine: 'UE 5.8', seed, player0: rows, player1: sequence((seed + 2654435761) >>> 0, count)}, null, 2));
    found = true;
    break;
  }
}
if (!found) throw new Error('No candidate within the fixed search bound.');
