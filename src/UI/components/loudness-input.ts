import { html } from "lit";
import { RouterElement } from "./base";

export class LoudnessInput extends RouterElement {
  render() {
    return html`<div class="page" id="loudness">

      <h1>USB loudness controller</h1>
      <p class="hint">
        Use a USB volume dial as input. While enabled, its
        volume keys are consumed before Windows can change system loudness.
      </p>
      <div class="form">
        <label class="switch"
          ><span>Use loudness controller input</span
          ><input
            id="loudnessEnabled"
            type="checkbox"
            onchange="RouterAction('loudness',this.checked?'1':'0')" />
  </label>
        <div class="full two-column">
          <div class="field">

        <label>Travel distance per rotation (stats)</label
          ><input
            id="loudnessRadius"
            type="number"
            min="1"
            max="10000"
            value="1000"
            onchange="RouterAction('loudness-radius',this.value)"
          />
          <p class="hint compact-hint">
            Millimetres travelled per full rotation.
          </p>
  </div>
          <div class="field">

          <label>Reverse loudness direction</label
            ><input
              id="loudnessReverse"
              type="checkbox"
              onchange="RouterAction('loudness-reverse',this.checked?'1':'0')"
          />
        </div>
        </div>
        <div class="full two-column">
          <div class="field">
            <label>Steps per rotation</label
            ><input
              id="loudnessClicks"
              type="number"
              min="1"
              max="240"
              value="10"
              onchange="RouterAction('loudness-clicks',this.value)"
            />
            <p class="hint compact-hint">How many pulses per full rotation?</p>
          </div>
          <div class="field">
            <label>RPM smoothing</label>
            <div class="range">
              <input
                id="loudnessSmoothing"
                type="range"
                min="0"
                max="97"
                value="55"
                oninput="loudnessSmoothingValue.textContent=this.value;RouterAction('loudness-smoothing',this.value)"
              /><span class="value green-value" id="loudnessSmoothingValue"
                >55</span
              >
            </div>
          </div>
        </div>

        <div class="audio-rpm full" aria-live="polite">
          <div class="audio-rpm-value">
            <div class="audio-signal-header">
              <span>Live turn signal</span
              ><strong id="loudnessDirection">IDLE</strong>
            </div>
            <div class="hint" id="loudnessActivity">
              Waiting for a dial turn
            </div>
          </div>
          <div class="audio-signal">
            <div class="audio-signal-header">
              <span>Controller</span
              ><strong id="loudnessConnection">Disconnected</strong>
            </div>
            <div class="hint">Turns captured: <b id="loudnessTurns">0</b></div>
          </div>
        </div>
        <div class="audio-rpm full">
          <div class="audio-rpm-value">
            <div class="audio-signal-header">
              <span>Dial</span
              ><strong
                ><b id="loudnessRpm">0</b> RPM
                <span class="hint maximum-rpm"
                  >MAX <b id="loudnessMaximumRpm">0</b></span
                ></strong
              >
            </div>
            <canvas
              id="loudnessRpmGraph"
              aria-label="Live signed loudness controller RPM graph"
            ></canvas>
          </div>
          <div class="audio-signal">
            <p class="hint">
              <span class="raw-colour">Purple</span>: raw RPM ·
              <span class="smooth-colour">Green</span>: smoothed RPM
            </p>
          </div>
        </div>
      </div>
    </div>`;
  }
}
customElements.define("router-loudness-input", LoudnessInput);
