import { html } from "lit";
import { RouterElement } from "./base";

export class InputTabs extends RouterElement {
  private tab(page: string, label: string, active = false) {
    return html`<button class="tab ${active ? "active" : ""}" data-page="${page}" @click=${() => this.select(page)}>
      <span class="tab-output ${page}"><i id="${page}OutputBar"></i><b id="${page}OutputValue">0%</b></span>${label}
    </button>`;
  }

  private select(page: string) {
    this.querySelectorAll<HTMLButtonElement>(".tab").forEach((tab) =>
      tab.classList.toggle("active", tab.dataset.page === page),
    );
    document
      .querySelectorAll<HTMLElement>(".page")
      .forEach((panel) => panel.classList.toggle("active", panel.id === page));
  }

  render() {
    return html`<div class="tabs" style="align-items:center">
      <span class="section-label">I N P U T</span
      >${this.tab("mouse", "Mouse", true)}${this.tab("audio", "Audio")}${this.tab("loudness", "Loudness")}${this.tab("combined", "Combined")}
      <button class="tab input-settings-cog" type="button" data-page="input-settings" aria-label="Input settings" @click=${() => this.select("input-settings")}>
        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19.43 12.98c.04-.32.07-.65.07-.98s-.02-.66-.07-.98l2.11-1.65a.5.5 0 0 0 .12-.64l-2-3.46a.5.5 0 0 0-.61-.22l-2.49 1a7.3 7.3 0 0 0-1.69-.98L14.5 2.42A.5.5 0 0 0 14 2h-4a.5.5 0 0 0-.49.42l-.38 2.65c-.61.25-1.18.59-1.69.98l-2.49-1a.5.5 0 0 0-.61.22l-2 3.46a.5.5 0 0 0 .12.64L4.57 11.02c-.04.32-.07.65-.07.98s.02.66.07.98l-2.11 1.65a.5.5 0 0 0-.12.64l2 3.46a.5.5 0 0 0 .61.22l2.49-1c.51.4 1.08.73 1.69.98l.38 2.65a.5.5 0 0 0 .49.42h4a.5.5 0 0 0 .49-.42l.38-2.65c.61-.25 1.18-.59 1.69-.98l2.49 1a.5.5 0 0 0 .61-.22l2-3.46a.5.5 0 0 0-.12-.64l-2.11-1.65ZM12 15.5A3.5 3.5 0 1 1 12 8a3.5 3.5 0 0 1 0 7.5Z" /></svg>
      </button>
    </div>
    <section class="page input-settings-page" id="input-settings">
      <h1>Settings</h1>
      <p class="hint">Configure the stats overlay shown above other apps.</p>
      <div class="form">
        <label class="switch"><span>Show stats overlay</span><input id="overlayEnabled" type="checkbox" onchange="RouterAction('overlay',this.checked?'1':'0')" /></label>
        <div class="field"><label>Overlay position</label><select id="overlayPosition" onchange="RouterAction('overlay-position',this.value)"><option value="0">Top left</option><option value="1">Top right</option><option value="2">Bottom left</option><option value="3">Bottom right</option></select></div>
        <div class="field"><label>Log</label><a class="driver-link" href="#" onclick="RouterAction('open-log','');return false;">Show Log</a></div>
        <div class="field"><label>Help</label><a class="driver-link" href="https://discord.gg/XkfF9mu6kx" onclick="RouterAction('open-external','https://discord.gg/XkfF9mu6kx');return false;">Join the Discord</a></div>
      </div>
    </section>`;
  }
}
customElements.define("router-input-tabs", InputTabs);
