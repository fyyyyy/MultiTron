import { html } from "lit";
import { RouterElement } from "./base";

export class RouterHeader extends RouterElement {
  render() {
    return html`<header>
      <div class="brand">
        <span class="logo"></span><span class="brand-copy"><span class="brand-name"><span>MultiTron2</span><span class="beta">1.0.1 beta</span></span><span class="sub">&nbsp;io</span></span>
      </div>
      <div class="settings-actions">
        <div><button type="button" onclick="RouterAction('settings-save','')">Save</button><button type="button" onclick="RouterAction('settings-load','')">Load</button></div>
        <span id="loadedSettingsFile"></span>
      </div>
      <div class="session-stats" aria-label="Session stats">
        <span class="stats-title">S T A T S</span>
        <div class="stats-units" aria-label="Distance unit">
          <button type="button" class="active" data-stats-unit="km" aria-pressed="true" onclick="RouterStatsUnit('km')">KM</button>
          <button type="button" data-stats-unit="mi" aria-pressed="false" onclick="RouterStatsUnit('mi')">MI</button>
        </div>
        <div id="sessionMetrics"></div>
      </div>
    </header>`;
  }
}
customElements.define("router-header", RouterHeader);
