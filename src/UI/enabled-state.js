(function () {
  function setElementDisabled(element, disabled) {
    if (!element) return;
    element.classList.toggle('disabled-content', disabled);
    element.querySelectorAll('input, select, button, textarea').forEach(function (control) {
      control.disabled = disabled;
    });
  }
  function setDisabled(selector, disabled) {
    document.querySelectorAll(selector).forEach(function (element) {
      setElementDisabled(element, disabled);
    });
  }
  function checkbox(id) { return document.getElementById(id).checked; }
  function update() {
    var mouse = !checkbox('mouseEnabled');
    setDisabled('#mouse .field, #mouse .mouse-graphs', mouse);

    var audio = !checkbox('audioEnabled');
    setDisabled('#audio .audio-rpm, #audio .field', audio);
    var audioSwitches = document.querySelectorAll('#audio .switch');
    setElementDisabled(audioSwitches[1], audio);

    var loudness = !checkbox('loudnessEnabled');
    setDisabled('#loudness .field, #loudness .audio-rpm', loudness);
    var loudnessSwitches = document.querySelectorAll('#loudness .switch');
    for (var i = 1; i < loudnessSwitches.length; ++i) setElementDisabled(loudnessSwitches[i], loudness);

    var vjoy = !checkbox('vjoyEnabled');
    setDisabled('#vjoyPanel .field, #vjoyPanel .metric, #visual', vjoy);

    var xbox = !checkbox('xboxEnabled');
    setDisabled('#xboxPanel .field, #xboxPanel .hint, #xboxPanel .driver-link, #xboxPanel .metric', xbox);

    var keyboard = !checkbox('keyboardEnabled');
    setDisabled('#keyboardPanel canvas, #keyboardPanel .keymap', keyboard);
  }
  document.addEventListener('change', function (event) {
    if (event.target.matches('#mouseEnabled,#audioEnabled,#loudnessEnabled,#vjoyEnabled,#xboxEnabled,#keyboardEnabled')) update();
  });
  document.querySelectorAll('.outtab').forEach(function (tab) {
    tab.addEventListener('click', update);
  });
  window.setTimeout(update, 0);
}());
