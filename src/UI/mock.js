/* Browser-only mock. Loaded exclusively by index.html?mock=1. */
(function () {
  var state = {
    targetAxis: 0,
    mouse: 0,
    audio: 0,
    output: 0,
    pulse: 0,
    distanceKm: 0,
    averageKmh: 0
  };

  window.NativeRouterAction = function (name, value) {
    if (name === 'target') state.targetAxis = Number(value);
    if (name === 'refresh') {
      var select = document.getElementById('mouseDevice');
      select.innerHTML = '<option>Bike belt mouse (mock)</option><option>Logitech USB Receiver (mock)</option>';
    }
    console.info('[MultiTron2 mock]', name, value);
  };

  window.RouterConfig({
    source: 0, target: 0, vjoyStrength: 62, xboxStrength: 62, smoothing: 45, inputUpdateInterval: 15, reverse: false,
    keyboard: true, audio: true, beep: false, threshold: 180, radius: 1000,
    unit: 0, rpm: 110, xbox: false, xboxAxis: 1, loudnessClicks: 10,
    loudnessSmoothing: 55, loudnessRadius: 1000, loudness: false, loudnessReverse: false,
    mouseMixMinimum: 0, mouseMixMaximum: 100, mouseMixPriority: 1,
    audioMixMinimum: 0, audioMixMaximum: 100, audioMixPriority: 1,
    loudnessMixMinimum: 0, loudnessMixMaximum: 100, loudnessMixPriority: 1
  });
  window.RouterInputUpdateInterval(15);
  window.RouterMappings([
    { minimum: 20, maximum: 100, modifier: 0, key: 'W' },
    { minimum: -100, maximum: -20, modifier: 0, key: 'S' }
  ]);
  window.NativeRouterAction('refresh', '');

  setInterval(function () {
    var now = Date.now() / 1000;
    var rpm = 72 + Math.sin(now * 1.4) * 12 + Math.sin(now * 3.7) * 4;
    state.mouse = Math.sin(now * 1.7) * 58;
    state.audio = Math.sin(now * 1.4) * 49;
    state.output = Math.max(-100, Math.min(100, state.mouse * .42 + state.audio * .58));
    state.distanceKm += Math.max(rpm, 0) * 2.105 * .001 / 60 / 8;
    state.averageKmh = 16.2 + Math.sin(now / 8) * .6;
    if (Math.floor(now * 1.4) !== Math.floor((now - .125) * 1.4)) state.pulse++;
    window.RouterState({
      output: state.output, combined: state.output, mouse: state.mouse, audio: state.audio, loudness: 0,
      mouseRaw: Math.abs(Math.round(state.mouse * 1.7)),
      mouseSmoothed: Math.abs(Math.round(state.mouse)), targetAxis: state.targetAxis,
      audioRpm: rpm, audioLevel: 180 + Math.abs(Math.sin(now * 5.6)) * 420,
      audioPulseSequence: state.pulse, speedKmh: rpm * 2.105 * 60 / 1000,
      averageKmh: state.averageKmh, distanceKm: state.distanceKm,
      distanceMiles: state.distanceKm / 1.609344, xbox: false,
      xboxStatus: 'Disabled'
    });
  }, 125);
}());
