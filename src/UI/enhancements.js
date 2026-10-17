(function () {
  var mappings = [], lastState = null, keyboardHistory = [];
  var tooltip = document.createElement('div');
  tooltip.id = 'routerTooltip';
  document.body.appendChild(tooltip);
  function tooltipTarget(node) {
    while (node && node !== document.body) {
      if (node.getAttribute && node.getAttribute('title')) return node;
      node = node.parentNode;
    }
    return null;
  }
  document.addEventListener('mousemove', function (event) {
    var target = tooltipTarget(event.target);
    if (!target) { tooltip.style.display = 'none'; return; }
    tooltip.textContent = target.getAttribute('title');
    tooltip.style.left = event.clientX + 12 + 'px';
    tooltip.style.top = event.clientY + 14 + 'px';
    tooltip.style.display = 'block';
  });
  // Keep the raw mouse trace blue while retaining a signed (zero-centred) scale.
  window.drawMouseRawGraph = function (value) {
    var canvas = document.getElementById('mouseRawGraph'); if (!canvas) return;
    var width = canvas.clientWidth, height = canvas.clientHeight; if (!width || !height) return;
    var scale = window.devicePixelRatio || 1;
    if (canvas.width !== width * scale || canvas.height !== height * scale) { canvas.width = width * scale; canvas.height = height * scale; }
    mouseRawHistory.push(value); if (mouseRawHistory.length > 100) mouseRawHistory.shift();
    var maximum = 20; mouseRawHistory.forEach(function (entry) { maximum = Math.max(maximum, Math.abs(entry)); }); maximum *= 1.15;
    var ctx = canvas.getContext('2d'), left = 30, right = 6, top = 4, bottom = 20, plotWidth = width - left - right, plotHeight = height - top - bottom, centre = top + plotHeight / 2;
    ctx.setTransform(scale, 0, 0, scale, 0, 0); ctx.clearRect(0, 0, width, height); ctx.strokeStyle = '#254b5d'; ctx.lineWidth = 1; ctx.setLineDash([3, 3]);
    for (var i = 0; i < 5; i++) { var y = top + plotHeight * i / 4; ctx.beginPath(); ctx.moveTo(left, y); ctx.lineTo(width - right, y); ctx.stroke(); }
    ctx.setLineDash([]); ctx.strokeStyle = '#7993a7'; ctx.beginPath(); ctx.moveTo(left, centre); ctx.lineTo(width - right, centre); ctx.stroke();
    ctx.strokeStyle = '#248cff'; ctx.lineWidth = 2; ctx.beginPath(); mouseRawHistory.forEach(function (entry, index) { var x = mouseRawHistory.length === 1 ? width - right : left + index * plotWidth / (mouseRawHistory.length - 1), y = centre - entry / maximum * (plotHeight / 2); if (index) ctx.lineTo(x, y); else ctx.moveTo(x, y); }); ctx.stroke();
    document.getElementById('mouseRawMin').textContent = Math.min.apply(null, mouseRawHistory); document.getElementById('mouseRawMax').textContent = Math.max.apply(null, mouseRawHistory);
  };
  var modifiers = ['', 'Ctrl', 'Ctrl', 'Shift', 'Shift', 'Alt', 'Alt', 'Caps'];
  var keyColours = ['#59d975', '#37bde8', '#bd74f5', '#f0aa4d', '#ee6b8d', '#59d5c8', '#f06b4d', '#7c9cff', '#d6c85a', '#e878c4'];
  var modifierColours = ['', '#4b9fff', '#76b8ff', '#a97cff', '#d0a1ff', '#f0a34d', '#f4c66a', '#e76ea5'];
  // Keep the exact configured character.  JavaScript expands ß to "SS" when
  // uppercasing, which made the preview lie about the key being sent.
  function typeFor(row) { return (modifiers[row.modifier] ? modifiers[row.modifier] + '+' : '') + row.key; }
  function keyFor(row) { return row.key; }
  function colourForKey(key) {
    var keys = mappings.map(keyFor).filter(function (mappedKey, index, allKeys) { return allKeys.indexOf(mappedKey) === index; }), index = keys.indexOf(key);
    return keyColours[(index < 0 ? 0 : index) % keyColours.length];
  }
  function modifierColourFor(modifier) { return modifierColours[modifier] || ''; }
  function activeTypes(percent) {
    return mappings.filter(function (row) { return percent !== 0 && row.minimum <= row.maximum && percent >= row.minimum && (percent < row.maximum || row.maximum === 100 || row.maximum === -100); })
      .map(typeFor).sort(function (a, b) { return mappings.map(typeFor).indexOf(a) - mappings.map(typeFor).indexOf(b); });
  }
  function selectedOutput() { var active = document.querySelector('.outtab.active'); return active ? active.dataset.output : 'keyboard'; }
  function renderInputOutputBars() {
    if (!lastState) return;
    ['mouse', 'audio', 'loudness', 'combined'].forEach(function (input) {
      var value = Math.round(lastState[input] || 0);
      document.getElementById(input + 'OutputBar').style.width = Math.min(100, Math.abs(value)) * .36 + 'px';
      document.getElementById(input + 'OutputValue').textContent = value + '%';
    });
  }
  function drawKeyboardGraph() {
    var canvas = document.getElementById('keyboardGraph'); if (!canvas) return;
    var width = canvas.clientWidth, height = canvas.clientHeight; if (!width || !height) return;
    var scale = window.devicePixelRatio || 1;
    if (canvas.width !== width * scale || canvas.height !== height * scale) { canvas.width = width * scale; canvas.height = height * scale; }
    var types = []; keyboardHistory.forEach(function (entry) { if (entry.type && types.indexOf(entry.type) < 0) types.push(entry.type); });
    types.sort(function (a, b) { return mappings.map(typeFor).indexOf(a) - mappings.map(typeFor).indexOf(b); });
    var ctx = canvas.getContext('2d'), left = 54, right = 5, top = 7, bottom = 18, plotWidth = width - left - right, plotHeight = height - top - bottom;
    ctx.setTransform(scale, 0, 0, scale, 0, 0); ctx.clearRect(0, 0, width, height); ctx.font = '10px Segoe UI'; ctx.textAlign = 'right';
    if (!types.length) { ctx.fillStyle = '#8fa7b9'; ctx.textAlign = 'center'; ctx.fillText('No keyboard output in the last 10 seconds', width / 2, height / 2); return; }
    var rowHeight = plotHeight / types.length;
    types.forEach(function (type, row) { var y = top + row * rowHeight; ctx.fillStyle = '#8fa7b9'; ctx.fillText(type, left - 6, y + rowHeight * .65); ctx.fillStyle = '#162f40'; ctx.fillRect(left, y, plotWidth, Math.max(1, rowHeight - 1)); });
    keyboardHistory.forEach(function (entry, index) {
      if (!entry.type) return;
      var row = types.indexOf(entry.type), x = left + index * plotWidth / 100, y = top + row * rowHeight, boxHeight = Math.max(1, rowHeight - 1), boxWidth = Math.ceil(plotWidth / 100) + 1, modifierColour = modifierColourFor(entry.modifier);
      ctx.fillStyle = colourForKey(entry.key);
      ctx.fillRect(x, y, boxWidth, boxHeight);
      if (modifierColour) {
        ctx.fillStyle = modifierColour;
        ctx.fillRect(x, y + Math.floor(boxHeight / 2), boxWidth, Math.ceil(boxHeight / 2));
      }
    });
    ctx.fillStyle = '#8fa7b9'; ctx.textAlign = 'left'; ctx.fillText('-10s', left, height - 4); ctx.textAlign = 'right'; ctx.fillText('NOW', width - right, height - 4);
    var current = keyboardHistory.length ? keyboardHistory[keyboardHistory.length - 1].type : '';
    if (current) { ctx.globalAlpha = .7; ctx.fillStyle = '#e9f1fa'; ctx.font = '700 24px Segoe UI'; ctx.textAlign = 'center'; ctx.fillText('Keyboard output: ' + current, left + plotWidth / 2, top + plotHeight / 2 + 8); ctx.globalAlpha = 1; }
  }
  function renderMetric() {
    if (!lastState) return;
    var keyboard = selectedOutput() === 'keyboard', metric = document.getElementById('outputMetric');
    metric.style.display = keyboard ? 'none' : 'block';
    var unscaled = document.getElementById('outputUnscaled'); unscaled.style.display = keyboard ? 'none' : 'inline';
    unscaled.querySelector('b').textContent = Math.round(selectedOutput() === 'xbox' ? lastState.xboxUnscaledOutput : lastState.vjoyUnscaledOutput);
    document.getElementById('outputNumber').textContent = Math.round(selectedOutput() === 'xbox' ? lastState.xboxOutput : lastState.vjoyOutput);
    document.getElementById('outputUnit').style.display = keyboard ? 'none' : 'inline';
  }
  var originalConfig = window.RouterConfig;
  window.RouterConfig = function (config) {
    originalConfig(config);
    document.getElementById('vjoyStrengthValue').textContent = config.vjoyStrength;
    document.getElementById('loudnessRadius').value = config.loudnessRadius;
    document.getElementById('keyboardRepeatInterval').value = config.keyboardRepeatInterval;
    document.getElementById('keyboardRepeatPause').value = config.keyboardRepeatPause;
    document.getElementById('overlayEnabled').checked = config.overlay;
    document.getElementById('overlayPosition').value = config.overlayPosition;
    document.getElementById('xboxXLeftKey').value = config.xboxXLeftKey || 'A'; document.getElementById('xboxXRightKey').value = config.xboxXRightKey || 'D';
  };
  var audioMeterIds = ['audioRpm', 'audioLevel', 'audioLevelMin', 'audioLevelMax'];
  var savedAudioMeters = {};
  function audioIsEnabled() { var input = document.getElementById('audioEnabled'); return input && input.checked; }
  function saveAudioMeters() { audioMeterIds.forEach(function (id) { savedAudioMeters[id] = document.getElementById(id).textContent; }); }
  function restoreAudioMeters() { audioMeterIds.forEach(function (id) { document.getElementById(id).textContent = savedAudioMeters[id]; }); }
  var originalDrawAudioGraph = window.drawAudioGraph;
  var originalDrawRpmGraph = window.drawRpmGraph;
  window.drawAudioGraph = function (level, event) { if (audioIsEnabled()) originalDrawAudioGraph(level, event); };
  window.drawRpmGraph = function (rpm) { if (audioIsEnabled()) originalDrawRpmGraph(rpm); };
  var originalState = window.RouterState;
  window.RouterState = function (state) {
    var audioEnabled = audioIsEnabled();
    if (!audioEnabled) saveAudioMeters();
    lastState = state; originalState(state);
    if (!audioEnabled) restoreAudioMeters();
    else saveAudioMeters();
    renderInputOutputBars();
    document.getElementById('loudnessMaximumRpm').textContent = Math.round(state.loudnessMaximumRpm || 0);
    var current = document.getElementById('keyboardEnabled').checked ? mappings.filter(function (row) { return activeTypes(Math.round(state.output)).indexOf(typeFor(row)) >= 0; })[0] : null;
    keyboardHistory.push(current ? { type: typeFor(current), key: keyFor(current), modifier: current.modifier } : { type: '' }); if (keyboardHistory.length > 100) keyboardHistory.shift(); drawKeyboardGraph(); renderMetric();
  };
  var originalMappings = window.RouterMappings;
  window.RouterMappings = function (rows) {
    mappings = rows;
    var deleteButton = document.getElementById('keymapDelete'), cloneButton = document.getElementById('keymapClone');
    if (deleteButton) deleteButton.hidden = true;
    if (cloneButton) cloneButton.hidden = true;
    originalMappings(rows);
    rows.forEach(function (row, index) {
      var mapping = document.querySelectorAll('.mapping-row')[index]; if (!mapping) return;
      var minimum = mapping.querySelector('.mapping-min'), maximum = mapping.querySelector('.mapping-max');
      var modifierToggle = mapping.querySelector('.mapping-modifier-toggle'), keyToggle = mapping.querySelector('.mapping-key-toggle');
      var dragHandle = mapping.querySelector('.mapping-drag-handle'), remove = mapping.querySelector('.mapping-remove');
      if (minimum) minimum.title = 'Start range';
      if (maximum) maximum.title = 'End range';
      if (modifierToggle) modifierToggle.parentNode.title = 'Toggle Modifier';
      if (keyToggle) keyToggle.parentNode.title = 'Toggle Key';
      if (dragHandle) dragHandle.title = 'Drag to reorder or select';
      if (remove) remove.title = 'Delete';
      var key = mapping.querySelector('.mapping-key'), keyColour = colourForKey(keyFor(row));
      if (key) { key.style.borderLeft = '2px solid ' + keyColour; key.title = 'Key'; }
      var modifier = mapping.querySelector('.mapping-modifier'), modifierColour = modifierColourFor(row.modifier);
      var modifierTrigger = modifier && modifier.parentNode.querySelector('.select-fallback-trigger');
      if (modifierTrigger) { modifierTrigger.style.borderLeft = modifierColour ? '2px solid ' + modifierColour : ''; modifierTrigger.title = 'Modifier key'; }
    });
    renderMetric();
  };
  document.querySelectorAll('.outtab').forEach(function (tab) { tab.addEventListener('click', renderMetric); });
  document.getElementById('keyboardEnabled').addEventListener('change', renderMetric);
  document.addEventListener('wheel', function (event) { var slider = event.target; if (!(slider instanceof HTMLInputElement) || slider.type !== 'range') return; event.preventDefault(); var next = Number(slider.value) + (event.deltaY < 0 ? Number(slider.step) || 1 : -(Number(slider.step) || 1)); slider.value = Math.min(Number(slider.max), Math.max(Number(slider.min), next)); slider.dispatchEvent(new Event('input', { bubbles: true })); }, { passive: false });
  var lastSessionMetricsRender = 0;
  renderSessionMetrics = function (state) {
    var now = Date.now();
    if (now - lastSessionMetricsRender < 500) return;
    lastSessionMetricsRender = now;
    var imperial = statsUnit === 'mi', distanceUnit = imperial ? 'mi' : 'km', speedUnit = imperial ? 'mph' : 'km/h';
    var speed = imperial ? state.speedKmh * .621371 : state.speedKmh;
    var average = imperial ? state.averageKmh * .621371 : state.averageKmh;
    var distance = imperial ? state.distanceMiles : state.distanceKm;
    var total = imperial ? (state.totalDistanceMiles || distance) : (state.totalDistanceKm || distance);
    document.getElementById('sessionMetrics').innerHTML = '<span><i class="session-label">NOW </i><b class="session-value">' + speed.toFixed(1) + ' ' + speedUnit + '</b></span><span><i class="session-label">AVG </i><b class="session-value">' + average.toFixed(1) + ' ' + speedUnit + '</b></span><span><i class="session-label">DIST </i><b class="session-value">' + distance.toFixed(3) + ' ' + distanceUnit + '</b></span><span><i class="session-label">TOT </i><b class="session-value">' + total.toFixed(3) + ' ' + distanceUnit + '</b></span>';
  };
  var baseRouterConfig = window.RouterConfig;
  window.RouterConfig = function (config) {
    baseRouterConfig(config);
    var rotationDelta = document.getElementById('mouseRotationDelta');
    if (rotationDelta && config.mouseRotationDelta !== undefined) rotationDelta.textContent = config.mouseRotationDelta ? config.mouseRotationDelta + ' pixels' : 'Not measured';
    var distance = document.getElementById('mouseDistancePerRotation');
    if (distance && config.mouseDistancePerRotation !== undefined) distance.value = config.mouseDistancePerRotation;
    var unit = document.getElementById('mouseDistanceUnit');
    if (unit && config.mouseDistanceUnit !== undefined) { unit.value = config.mouseDistanceUnit; unit.dataset.currentUnit = config.mouseDistanceUnit; }
  };
  window.changeMouseDistanceUnit = function (nextUnit) {
    var unit = document.getElementById('mouseDistanceUnit'), distance = document.getElementById('mouseDistancePerRotation');
    var currentUnit = unit.dataset.currentUnit === undefined ? nextUnit : unit.dataset.currentUnit;
    if (distance && currentUnit !== nextUnit) distance.value = (Number(distance.value) * (currentUnit === '0' ? 3.28084 : 0.3048)).toFixed(3);
    unit.dataset.currentUnit = nextUnit;
    if (distance) RouterAction('mouse-distance', distance.value);
    RouterAction('mouse-distance-unit', nextUnit);
  };
  if (window.__routerConfig) window.RouterConfig(window.__routerConfig);
  if (window.__routerState) window.RouterState(window.__routerState);
  if (window.__routerMappings) window.RouterMappings(window.__routerMappings);
}());
