import { html } from "lit";
import { RouterElement } from "./base";

export class KeyboardOutput extends RouterElement {
  render() {
    return html`<section id="keyboardPanel" class="keys" style="display:block">
      <label class="switch"
        ><span>Enable keyboard output</span
        ><input
          id="keyboardEnabled"
          type="checkbox"
            onchange="RouterAction('keyboard',this.checked?'1':'0')" /></label
      ><div class="keyboard-repeat-settings">
        <label title="How often held keys are briefly released and pressed again">Repeat interval (ms)<input id="keyboardRepeatInterval" type="number" min="1" max="5000" onchange="RouterAction('keyboard-repeat-interval',this.value)" /></label>
        <label title="How long keys stay released before they are pressed again">Release pause (ms)<input id="keyboardRepeatPause" type="number" min="1" max="1000" onchange="RouterAction('keyboard-repeat-pause',this.value)" /></label>
      </div><canvas
        id="keyboardGraph"
        class="keyboard-graph"
        aria-label="Last 10 seconds of keyboard output"
      ></canvas>
      <h2 class="keymap-title">Keyboard mappings</h2>
      <div class="keymap"></div>
      <div class="keymap-actions"><button class="button secondary keymap-add" onclick="RouterAction('mapping-add','')">+ Add</button><button id="keymapClone" class="button keymap-clone" hidden>Clone</button><button id="keymapDelete" class="button keymap-delete" hidden>Delete</button></div>
    </section>`;
  }
}
customElements.define("router-keyboard-output", KeyboardOutput);
