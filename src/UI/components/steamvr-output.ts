import { html } from "lit";
import { RouterElement } from "./base";

export class SteamVROutput extends RouterElement {
  render() {
    return html`<section id="steamvrPanel" class="keys steamvr-panel">
      <label class="switch"
        ><span>Enable SteamVR controller output</span
        ><input
          id="steamvrEnabled"
          type="checkbox"
          onchange="RouterAction('steamvr',this.checked?'1':'0')"
      /></label>
      <p class="hint" id="steamvrStatus">Disabled</p>

      <div class="field">
        <label>Axis strength</label>
        <div class="range">
          <input
            id="steamvrStrength"
            type="range"
            min="1"
            max="200"
            oninput="steamvrStrengthValue.textContent=this.value;RouterAction('steamvr-strength',this.value)"
          /><span class="value" id="steamvrStrengthValue"></span>
        </div>
      </div>

      <div class="field">
        <label>Map output to</label>
        <select id="steamvrAxis" onchange="RouterAction('steamvr-axis',this.value)">
          <option value="0">Joystick X</option>
          <option value="1">Joystick Y</option>
          <option value="2">Trigger (forward only)</option>
        </select>
      </div>

      <p class="hint">
        Needs the MultiTron SteamVR driver registered with SteamVR
        (<code>vrpathreg adddriver</code> on the <code>steamvr-driver\\multitron</code>
        folder next to MultiTron2.exe). This adds a separate virtual controller.
      </p>
    </section>`;
  }
}
customElements.define("router-steamvr-output", SteamVROutput);
