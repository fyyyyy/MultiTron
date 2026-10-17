import { html } from "lit";
import { RouterElement } from "./base";

export class MouseInput extends RouterElement {
  render() {
    return html`<div class="page active" id="mouse">
      <h1>Mouse belt sensor</h1>
      <p class="hint">
        Choose the mouse mounted on the bike. Its normal Windows movement is
        suppressed.
      </p>
      <div class="mouse-source-settings">
        <section class="mouse-source-panel">
          <label class="switch"
            ><span>Use bike mouse input</span
            ><input id="mouseEnabled" type="checkbox" onchange="RouterAction('mouse',this.checked?'1':'0')" />
          </label>
          <div class="field">
            <label>Bike mouse</label>
            <div class="inline">
              <select id="mouseDevice" onchange="RouterAction('mouse-device',this.value)">
                <option>Choose bike mouse in router</option></select
              ><button class="button secondary" onclick="RouterAction('refresh','')">Refresh</button>
            </div>
          </div>
        </section>
        <section class="mouse-source-panel">
          <div class="field mouse-rotation-calibration">
            <label title="Used to convert mouse movement into distance and speed for Stats. It has no gameplay effect.">STATS</label>
            <div class="inline"><button class="button secondary" type="button" onclick="RouterAction('mouse-rotation-start','');document.getElementById('mouseRotationModal').style.display='grid'">Measure full rotation</button><span id="mouseRotationDelta">Not measured</span></div>
            <label>Travel distance per rotation</label><div class="inline mouse-distance"><input id="mouseDistancePerRotation" type="number" min="0.001" max="10000" step="0.001" onchange="RouterAction('mouse-distance',this.value)" /><select id="mouseDistanceUnit" onchange="window.changeMouseDistanceUnit(this.value)"><option value="0">m</option><option value="1">ft</option></select></div>
          </div>
        </section>
      </div>
      <div class="mouse-rotation-modal" id="mouseRotationModal" style="display:none">
        <div class="mouse-rotation-dialog" role="dialog" aria-modal="true" aria-label="Measure a full rotation"><h2>Measure a full rotation</h2><p>Turn the belt through one complete rotation, then select Done.</p><strong class="mouse-rotation-live" id="mouseRotationLivePixels">0 pixels</strong><div><button class="button secondary" type="button" onclick="RouterAction('mouse-rotation-cancel','');document.getElementById('mouseRotationModal').style.display='none'">Cancel</button><button class="button" type="button" onclick="RouterAction('mouse-rotation-finish','');document.getElementById('mouseRotationModal').style.display='none'">Done</button></div></div>
      </div>
      <div class="form mouse-tuning">
     
        <div class="field">
          <label>Read mouse axis</label
          ><select id="sourceAxis" onchange="RouterAction('source',this.value)">
            <option value="0">X — left / right</option>
            <option value="1">Y — up / down</option>
          </select>
        </div>
        <div class="field">
          <label>Axis strength</label>
          <div class="range">
            <input
              id="vjoyStrength"
              type="range"
              min="1"
              max="200"
              oninput="vjoyStrengthValue.textContent=this.value;RouterAction('vjoy-strength',this.value)"
            /><span class="value" id="vjoyStrengthValue"></span>
          </div>
        </div>
        <div class="field">
          <label title="How often input is refreshed. Lower values feel more responsive but use more CPU">Input refresh</label>
          <div class="range">
            <input
              id="inputUpdateInterval"
              type="range"
              min="5"
              max="30"
              oninput="inputUpdateIntervalValue.textContent=this.value+' ms';RouterAction('input-update-interval',this.value)"
            /><span class="value" id="inputUpdateIntervalValue"></span>
          </div>
        </div>
        <label class="switch"
          ><span>Reverse direction</span
          ><input
            id="reverse"
            type="checkbox"
            onchange="RouterAction('reverse',this.checked?'1':'0')"
        /></label>
        <div class="field">
          <label title="Higher values make output less noisy but increase latency">Output smoothing</label>
          <div class="range">
            <input
              id="smoothing"
              type="range"
              min="1"
              max="200"
              oninput="smoothingValue.textContent=this.value;RouterAction('smoothing',this.value)"
            /><span class="value" id="smoothingValue"></span>
          </div>
        </div>
      </div>
      <div class="mouse-graphs">
        <div class="mouse-graph">
          <div class="mouse-graph-header">
            <span>Smoothed mouse</span><strong id="mouseSmoothed">0</strong>
          </div>
          <canvas
            id="mouseSmoothedGraph"
            aria-label="Smoothed mouse input graph"
          ></canvas>
        </div>
        <div class="mouse-graph">
          <div class="mouse-graph-header">
            <span class="mouse-graph-values"
              ><span class="mouse-graph-range"
                >MIN <b id="mouseRawMin">0</b> · MAX
                <b id="mouseRawMax">0</b></span
              ><strong id="mouseRaw">0</strong></span
            >
          </div>
          <canvas
            id="mouseRawGraph"
            aria-label="Raw mouse input graph"
          ></canvas>
        </div>
      </div>
    </div>`;
  }
}
customElements.define("router-mouse-input", MouseInput);
