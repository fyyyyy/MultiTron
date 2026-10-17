import { html } from "lit";
import { RouterElement } from "./base";

const sourceRow = (source: string, label: string) => html`<div class="mix-row">
  <strong>${label}</strong>
  <label>Range <input id="${source}MixMinimum" type="number" min="0" max="100" onchange="RouterAction('${source}-mix-min',this.value)" /> to <input id="${source}MixMaximum" type="number" min="0" max="100" onchange="RouterAction('${source}-mix-max',this.value)" />%</label>
  <label>Priority &nbsp;<select id="${source}MixPriority" onchange="RouterAction('${source}-mix-priority',this.value)"><option value="0">Off</option><option value="1">Low</option><option value="2">Medium</option><option value="3">High</option></select></label>
</div>`;

export class CombinedInput extends RouterElement {
  render() {
    return html`<div class="page" id="combined">
      <h1>Combined inputs</h1>
      <p class="hint">Each source maps to its range. Off sources do not affect the combined output; otherwise, the highest-priority active source wins overlap.</p>
      <div class="visual combined-visual" id="combinedVisual">
        <div class="cross"></div>
        <span class="axis-label x">X</span><span class="axis-label y">Y</span>
        <div class="dot mouse" id="mouseDot"></div>
        <div class="dot audio" id="audioDot"></div>
        <div class="dot loudness" id="loudnessDot"></div>
        <div class="dot output" id="outputDot"></div>
        <div class="legend"><span><i class="mouse"></i>Mouse</span><span><i class="audio"></i>Audio</span><span><i class="loudness"></i>Loudness</span><span><i class="combined"></i>Combined</span></div>
      </div>
      <div class="mix-controls">
        ${sourceRow("mouse", "Mouse")}
        ${sourceRow("audio", "Audio")}
        ${sourceRow("loudness", "Loudness")}
      </div>
    </div>`;
  }
}
customElements.define("router-combined-input", CombinedInput);
