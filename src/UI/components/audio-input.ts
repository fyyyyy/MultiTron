import { html } from "lit";
import { RouterElement } from "./base";

export class AudioInput extends RouterElement {
  render() {
    return html`<div class="page" id="audio">
      <h1>3.5 mm cadence sensor</h1>
      <p class="hint">
        Connect the cadence output to the audio input of your soundcard. Not as precise control as the mouse, but works for general RPM tracking.
      </p>
      <div class="form">
        <label class="switch full"
          ><span>Use audio cadence RPM</span
          ><input
            id="audioEnabled"
            type="checkbox"
            onchange="RouterAction('audio',this.checked?'1':'0')" /></label
        ><label class="switch full"
          ><span>Beep on cadence pulse</span
          ><input
            id="audioBeep"
            type="checkbox"
            onchange="RouterAction('beep',this.checked?'1':'0')"
        /></label>
        <div class="audio-rpm full" aria-live="polite">
          <div class="audio-rpm-value">
            <div class="audio-signal-header">
              <span>Current cadence</span
              ><strong><b id="audioRpm">0</b> RPM</strong>
            </div>
            <canvas id="rpmGraph" aria-label="Live cadence RPM graph"></canvas>
          </div>
          <div class="audio-signal">
            <div class="audio-signal-header">
              <span>Raw audio</span
              ><span class="mouse-graph-values"
                ><span class="mouse-graph-range"
                  >MIN <b id="audioLevelMin">0</b> · MAX
                  <b id="audioLevelMax">0</b></span
                ><strong id="audioLevel">0</strong></span
              >
            </div>
            <div class="audio-chart">
              <canvas
                id="audioGraph"
                aria-label="Live raw audio signal graph"
              ></canvas
              ><label class="threshold-control" title="Pulse threshold"
                ><input
                  id="audioThreshold"
                  type="range"
                  min="1"
                  max="255"
                  oninput="audioThresholdValue.textContent=this.value;RouterAction('threshold',this.value)"
                /><span id="audioThresholdValue"></span><b>THRESHOLD</b></label
              >
            </div>
          </div>
        </div>
        <div class="field">
          <label>Travel distance per rotation (stats)</label
          ><input
            id="wheelRadius"
            type="number"
            value="1000"
            onchange="RouterAction('radius',this.value)"
          />
        </div>
        <div class="field">
          <label>Unit</label
          ><select id="wheelUnit" onchange="RouterAction('unit',this.value)">
            <option value="0">mm</option>
            <option value="1">inch</option>
          </select>
        </div>
        <div class="field full">
          <label>Map RPM to full output</label
          ><input
            id="mapRpm"
            type="number"
            onchange="RouterAction('rpm',this.value)"
          />
        </div>
      </div>
    </div>`;
  }
}
customElements.define("router-audio-input", AudioInput);
