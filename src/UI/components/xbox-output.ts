import { html } from "lit";
import { RouterElement } from "./base";

export class XboxOutput extends RouterElement {
  render() {
    return html`<section id="xboxPanel" class="keys xbox-panel">

      <label class="switch"
        ><span>Enable Xbox controller output</span
        ><input
          id="xboxEnabled"
          type="checkbox"
          onchange="RouterAction('xbox',this.checked?'1':'0')"
      /></label>
      <p class="hint" id="xboxStatus">Disabled</p>

      <div class="field">
        <label>Axis strength</label>
        <div class="range">
          <input
            id="xboxStrength"
            type="range"
            min="1"
            max="200"
            oninput="xboxStrengthValue.textContent=this.value;RouterAction('xbox-strength',this.value)"
          /><span class="value" id="xboxStrengthValue"></span>
        </div>
      </div>

      <div class="field">
        <label>Map output to</label>
        <select id="xboxAxis" onchange="RouterAction('xbox-axis',this.value);window.updateXboxTriggerFlipVisibility()">
          <option value="0">Left stick X</option>
          <option value="1">Left stick Y</option>
          <option value="2">Right stick X</option>
          <option value="3">Right stick Y</option>
          <option value="4">Triggers: left / right</option>
        </select>
        <label class="switch" id="xboxTriggerReverse">
          <span>Reverse triggers (LT positive, RT negative)</span>
          <input id="xboxTriggersFlipped" type="checkbox" onchange="RouterAction('xbox-triggers-flipped',this.checked?'1':'0')" />
        </label>
      </div>

      <div class="field">
        <label>Emulate Xbox controller X-axis via keyboard</label>
        <div class="xbox-key-bindings">
          <label
            >Left key
            <input
              id="xboxXLeftKey"
              type="text"
              maxlength="1"
              oninput="this.value=this.value.toUpperCase();RouterAction('xbox-x-left-key',this.value)" /></label
          ><label
            >Right key
            <input
              id="xboxXRightKey"
              type="text"
              maxlength="1"
              oninput="this.value=this.value.toUpperCase();RouterAction('xbox-x-right-key',this.value)"
          /></label>
        </div>
      </div>

      <a
        class="driver-link"
        href="https://github.com/nefarius/ViGEmBus/releases"
        >Install ViGEmBus driver</a
      >
    </section>`;
  }
}
customElements.define("router-xbox-output", XboxOutput);
