import { LitElement } from "lit";

// The dashboard intentionally uses light DOM. The native bridge and legacy
// canvas code address stable element IDs, while Lit owns each UI section.
export abstract class RouterElement extends LitElement {
  createRenderRoot() {
    return this;
  }
}
