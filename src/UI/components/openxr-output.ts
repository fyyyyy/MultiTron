import { html } from "lit";
import { RouterElement } from "./base";

export class OpenXROutput extends RouterElement {
  render() {
    return html`<section id="openxrPanel" class="keys openxr-panel">
      <label class="switch"
        ><span>Add to left thumbstick (OpenXR layer)</span
        ><input
          id="openxrEnabled"
          type="checkbox"
          onchange="RouterAction('openxr',this.checked?'1':'0')"
      /></label>
      <p class="hint" id="openxrStatus">Disabled</p>

      <div class="field">
        <label>Axis strength</label>
        <div class="range">
          <input
            id="openxrStrength"
            type="range"
            min="1"
            max="200"
            oninput="openxrStrengthValue.textContent=this.value;RouterAction('openxr-strength',this.value)"
          /><span class="value" id="openxrStrengthValue"></span>
        </div>
      </div>

      <div class="field">
        <label>Add to stick axis</label>
        <select id="openxrAxis" onchange="RouterAction('openxr-axis',this.value)">
          <option value="0">Left stick X</option>
          <option value="1">Left stick Y (forward)</option>
        </select>
      </div>

      <p class="hint">
        Adds this value to the real left thumbstick (for example Quest 3) in OpenXR
        games, including OpenVR games through OpenComposite. Needs the OpenXR layer
        registered: run <code>install-openxr-layer.ps1</code> in the
        <code>openxr-layer</code> folder next to MultiTron2.exe, then restart the game.
      </p>
    </section>`;
  }
}
customElements.define("router-openxr-output", OpenXROutput);
