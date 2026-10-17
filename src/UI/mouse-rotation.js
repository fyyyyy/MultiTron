window.startMouseRotationMeasurement = function () {
  RouterAction('mouse-rotation-start', '');
  var modal = document.createElement('div');
  modal.className = 'mouse-rotation-modal';
  modal.innerHTML = '<div class="mouse-rotation-dialog" role="dialog" aria-modal="true" aria-label="Measure a full rotation"><h2>Measure a full rotation</h2><p>Turn the bike belt through one complete rotation, then select Done.</p><div><button class="button secondary" type="button">Cancel</button><button class="button" type="button">Done</button></div></div>';
  var close = function (command) { RouterAction(command, ''); modal.remove(); };
  modal.querySelector('.secondary').onclick = function () { close('mouse-rotation-cancel'); };
  modal.querySelector('.button:not(.secondary)').onclick = function () { close('mouse-rotation-finish'); };
  document.body.appendChild(modal);
};
