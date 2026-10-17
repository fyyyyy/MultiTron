// RouterDashboard can publish its first state before deferred UI scripts run.
// Replay that cached bridge data after the Lit tree and legacy bridge exist.
(function () {
  if (window.__routerConfig) window.RouterConfig(window.__routerConfig);
  if (window.__routerState) window.RouterState(window.__routerState);
  if (window.__routerMappings) window.RouterMappings(window.__routerMappings);
}());
