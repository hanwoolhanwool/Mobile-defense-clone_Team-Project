// Runs offline from file://; no network, telemetry, or stored learner state.
document.querySelector('.print-button')?.addEventListener('click', () => window.print());
const search = document.querySelector('#document-search');
if (search) {
  const items = [...document.querySelectorAll('[data-search]')];
  const groups = [...document.querySelectorAll('.catalog-group')];
  search.addEventListener('input', () => {
    const words = search.value.trim().toLowerCase().split(/\s+/).filter(Boolean);
    let count = 0;
    for (const item of items) {
      item.hidden = !words.every(word => item.dataset.search.includes(word));
      if (!item.hidden) count++;
    }
    groups.forEach(group => { group.hidden = !group.querySelector('li:not([hidden])'); });
    document.querySelector('#search-count').textContent = `${count}개 문서`;
    document.querySelector('#search-empty').hidden = count !== 0;
  });
}
