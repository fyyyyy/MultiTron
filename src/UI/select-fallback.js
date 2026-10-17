(function () {
  var open = null;
  var activeSelect = null;

  function close() {
    if (open) open.classList.remove('select-fallback-open');
    open = null;
  }

  function moveSelection(select, direction) {
    var index = select.selectedIndex;
    do {
      index += direction;
    } while (index >= 0 && index < select.options.length && select.options[index].disabled);
    if (index < 0 || index >= select.options.length) return;
    select.selectedIndex = index;
    select.dispatchEvent(new Event('change', { bubbles: true }));
    refresh(select);
  }

  function toggleSelectionMenu(select) {
    var wrapper = select.parentNode;
    if (open === wrapper) {
      close();
      return;
    }
    close();
    refresh(select);
    wrapper.classList.add('select-fallback-open');
    open = wrapper;
  }

  function refresh(select) {
    var wrapper = select.parentNode;
    if (!wrapper || !wrapper.classList.contains('select-fallback')) return;
    var trigger = wrapper.querySelector('.select-fallback-trigger');
    var menu = wrapper.querySelector('.select-fallback-menu');
    var option = select.options[select.selectedIndex];
    trigger.textContent = option ? option.textContent : '';
    menu.innerHTML = '';
    Array.prototype.forEach.call(select.options, function (entry) {
      var choice = document.createElement('button');
      choice.type = 'button';
      choice.className = 'select-fallback-option';
      choice.textContent = entry.textContent;
      choice.disabled = entry.disabled;
      choice.classList.toggle('selected', entry.selected);
      choice.onclick = function () {
        if (entry.disabled) return;
        select.value = entry.value;
        select.dispatchEvent(new Event('change', { bubbles: true }));
        refresh(select);
        close();
      };
      menu.appendChild(choice);
    });
  }

  function enhance(select) {
    if (select.dataset.selectFallback || select.id === 'xboxAxis') return;
    select.dataset.selectFallback = 'true';
    var wrapper = document.createElement('span');
    wrapper.className = 'select-fallback';
    var trigger = document.createElement('button');
    trigger.type = 'button';
    trigger.className = 'select-fallback-trigger';
    trigger.setAttribute('aria-haspopup', 'listbox');
    var menu = document.createElement('span');
    menu.className = 'select-fallback-menu';
    menu.setAttribute('role', 'listbox');
    select.parentNode.insertBefore(wrapper, select);
    wrapper.appendChild(select);
    wrapper.appendChild(trigger);
    wrapper.appendChild(menu);
    trigger.onclick = function () {
      activeSelect = select;
      toggleSelectionMenu(select);
    };
    trigger.onkeydown = function (event) {
      var direction = event.key === 'ArrowDown' ? 1 : event.key === 'ArrowUp' ? -1 : 0;
      if (!direction) return;
      event.preventDefault();
      event.stopPropagation();
      moveSelection(select, direction);
    };
    select.addEventListener('change', function () { refresh(select); });
    refresh(select);
  }

  function enhanceAll(root) {
    Array.prototype.forEach.call((root || document).querySelectorAll('select'), enhance);
  }

  document.addEventListener('click', function (event) {
    if (open && !open.contains(event.target)) close();
  });
  document.addEventListener('keydown', function (event) {
    if (event.key === 'Escape') close();
    if ((event.key === 'Enter' || event.key === ' ') && activeSelect) {
      event.preventDefault();
      toggleSelectionMenu(activeSelect);
      return;
    }
    var direction = event.key === 'ArrowDown' ? 1 : event.key === 'ArrowUp' ? -1 : 0;
    if (!direction || !activeSelect) return;
    event.preventDefault();
    moveSelection(activeSelect, direction);
  });
  window.RouterRefreshSelects = function (root) {
    enhanceAll(root);
    Array.prototype.forEach.call(document.querySelectorAll('select[data-select-fallback]'), refresh);
  };

  enhanceAll();
  setTimeout(function () { window.RouterRefreshSelects(); }, 0);
  var originalMappings = window.RouterMappings;
  window.RouterMappings = function (rows) {
    originalMappings(rows);
    window.RouterRefreshSelects(document.querySelector('.keymap'));
  };
  var originalMice = window.RouterMice;
  window.RouterMice = function (mice, selectedId) {
    originalMice(mice, selectedId);
    window.RouterRefreshSelects(document.getElementById('mouseDevice').parentNode);
  };
  var originalConfig = window.RouterConfig;
  window.RouterConfig = function (config) {
    originalConfig(config);
    window.RouterRefreshSelects();
  };
}());
