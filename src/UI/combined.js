(function () {
  function byId(id) { return document.getElementById(id); }
  function position(value) { return 'calc(50% - ' + (value * .34) + '%)'; }
  function setSource(source, config) {
    byId(source + 'MixMinimum').value = config[source + 'MixMinimum'];
    byId(source + 'MixMaximum').value = config[source + 'MixMaximum'];
    byId(source + 'MixPriority').value = config[source + 'MixPriority'];
  }
  var originalConfig = window.RouterConfig;
  window.RouterConfig = function (config) {
    originalConfig(config);
    ['mouse', 'audio', 'loudness'].forEach(function (source) { setSource(source, config); });
  };
  var originalState = window.RouterState;
  window.RouterState = function (state) {
    originalState(state);
    var loudness = byId('loudnessDot');
    if (!loudness) return;
    [['mouseDot', state.mouse], ['audioDot', state.audio], ['loudnessDot', state.loudness], ['outputDot', state.output]].forEach(function (entry) {
      var dot = byId(entry[0]), value = entry[1] || 0;
      if (state.targetAxis === 0) { dot.style.left = position(value); dot.style.top = '50%'; }
      else { dot.style.left = '50%'; dot.style.top = position(value); }
    });
  };
  if (window.__routerConfig) window.RouterConfig(window.__routerConfig);
  if (window.__routerState) window.RouterState(window.__routerState);
}());
