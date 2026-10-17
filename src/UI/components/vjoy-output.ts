import { html } from "lit";
import { RouterElement } from "./base";

export class VJoyOutput extends RouterElement {
  render() {
    return html`<div id="visual" style="display:none"></div><section id="vjoyPanel" class="keys vjoy-panel">
        <label class="switch"
          ><span>Enable virtual joystick output</span
          ><input
            id="vjoyEnabled"
            type="checkbox"
            onchange="RouterAction('vjoy',this.checked?'1':'0')"
        /></label>
        <div class="field vjoy-axis-field">
          <label>Map to vJoy axis</label
          ><select id="targetAxis" onchange="RouterAction('target',this.value)">
            <option value="0">X</option>
            <option value="1">Y</option>
          </select>
        </div>
      </section>`;
  }
}
customElements.define("router-vjoy-output", VJoyOutput);
