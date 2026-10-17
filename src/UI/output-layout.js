(function () {
  function placeMetricInActivePanel() {
    var active = document.querySelector('.outtab.active');
    var metric = document.getElementById('outputMetric');
    if (!active || !metric) return;
    var panelId = active.dataset.output === 'vjoy' ? 'vjoyPanel'
      : active.dataset.output === 'xbox' ? 'xboxPanel' : '';
    if (!panelId) return;
    var panel = document.getElementById(panelId);
    if (panel) panel.insertBefore(metric, panel.children[1] || null);
  }

  document.querySelectorAll('.outtab').forEach(function (tab) {
    tab.addEventListener('click', placeMetricInActivePanel);
  });
  placeMetricInActivePanel();
}());
