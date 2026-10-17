import { html } from "lit";
import { RouterElement } from "./base";

export class OutputTabs extends RouterElement {
  private select(output: string) {
    this.querySelectorAll<HTMLButtonElement>(".outtab").forEach((tab) =>
      tab.classList.toggle("active", tab.dataset.output === output),
    );
  }

  render() {
    return html`<div class="outtabs" style="align-items:center">
      <span class="section-label output-label">O U T P U T</span
      ><button
        class="outtab active"
        data-output="keyboard"
        @click=${() => this.select("keyboard")}
      >
        Keyboard</button
      ><button
        class="outtab"
        data-output="vjoy"
        @click=${() => this.select("vjoy")}
      >
        vJoy</button
      ><button
        class="outtab"
        data-output="xbox"
        @click=${() => this.select("xbox")}
      >
        Xbox
      </button>
    </div>`;
  }
}
customElements.define("router-output-tabs", OutputTabs);
