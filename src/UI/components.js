(() => {
  // node_modules/@lit/reactive-element/css-tag.js
  var t = globalThis;
  var e = t.ShadowRoot && (void 0 === t.ShadyCSS || t.ShadyCSS.nativeShadow) && "adoptedStyleSheets" in Document.prototype && "replace" in CSSStyleSheet.prototype;
  var s = Symbol();
  var o = /* @__PURE__ */ new WeakMap();
  var n = class {
    constructor(t3, e4, o5) {
      if (this._$cssResult$ = true, o5 !== s) throw Error("CSSResult is not constructable. Use `unsafeCSS` or `css` instead.");
      this.cssText = t3, this.t = e4;
    }
    get styleSheet() {
      let t3 = this.o;
      const s4 = this.t;
      if (e && void 0 === t3) {
        const e4 = void 0 !== s4 && 1 === s4.length;
        e4 && (t3 = o.get(s4)), void 0 === t3 && ((this.o = t3 = new CSSStyleSheet()).replaceSync(this.cssText), e4 && o.set(s4, t3));
      }
      return t3;
    }
    toString() {
      return this.cssText;
    }
  };
  var r = (t3) => new n("string" == typeof t3 ? t3 : t3 + "", void 0, s);
  var S = (s4, o5) => {
    if (e) s4.adoptedStyleSheets = o5.map((t3) => t3 instanceof CSSStyleSheet ? t3 : t3.styleSheet);
    else for (const e4 of o5) {
      const o6 = document.createElement("style"), n4 = t.litNonce;
      void 0 !== n4 && o6.setAttribute("nonce", n4), o6.textContent = e4.cssText, s4.appendChild(o6);
    }
  };
  var c = e ? (t3) => t3 : (t3) => t3 instanceof CSSStyleSheet ? ((t4) => {
    let e4 = "";
    for (const s4 of t4.cssRules) e4 += s4.cssText;
    return r(e4);
  })(t3) : t3;

  // node_modules/@lit/reactive-element/reactive-element.js
  var { is: i2, defineProperty: e2, getOwnPropertyDescriptor: h, getOwnPropertyNames: r2, getOwnPropertySymbols: o2, getPrototypeOf: n2 } = Object;
  var a = globalThis;
  var c2 = a.trustedTypes;
  var l = c2 ? c2.emptyScript : "";
  var p = a.reactiveElementPolyfillSupport;
  var d = (t3, s4) => t3;
  var u = { toAttribute(t3, s4) {
    switch (s4) {
      case Boolean:
        t3 = t3 ? l : null;
        break;
      case Object:
      case Array:
        t3 = null == t3 ? t3 : JSON.stringify(t3);
    }
    return t3;
  }, fromAttribute(t3, s4) {
    let i5 = t3;
    switch (s4) {
      case Boolean:
        i5 = null !== t3;
        break;
      case Number:
        i5 = null === t3 ? null : Number(t3);
        break;
      case Object:
      case Array:
        try {
          i5 = JSON.parse(t3);
        } catch (t4) {
          i5 = null;
        }
    }
    return i5;
  } };
  var f = (t3, s4) => !i2(t3, s4);
  var b = { attribute: true, type: String, converter: u, reflect: false, useDefault: false, hasChanged: f };
  var _a, _b;
  (_a = Symbol.metadata) != null ? _a : Symbol.metadata = Symbol("metadata"), (_b = a.litPropertyMetadata) != null ? _b : a.litPropertyMetadata = /* @__PURE__ */ new WeakMap();
  var y = class extends HTMLElement {
    static addInitializer(t3) {
      var _a6;
      this._$Ei(), ((_a6 = this.l) != null ? _a6 : this.l = []).push(t3);
    }
    static get observedAttributes() {
      return this.finalize(), this._$Eh && [...this._$Eh.keys()];
    }
    static createProperty(t3, s4 = b) {
      if (s4.state && (s4.attribute = false), this._$Ei(), this.prototype.hasOwnProperty(t3) && ((s4 = Object.create(s4)).wrapped = true), this.elementProperties.set(t3, s4), !s4.noAccessor) {
        const i5 = Symbol(), h3 = this.getPropertyDescriptor(t3, i5, s4);
        void 0 !== h3 && e2(this.prototype, t3, h3);
      }
    }
    static getPropertyDescriptor(t3, s4, i5) {
      var _a6;
      const { get: e4, set: r4 } = (_a6 = h(this.prototype, t3)) != null ? _a6 : { get() {
        return this[s4];
      }, set(t4) {
        this[s4] = t4;
      } };
      return { get: e4, set(s5) {
        const h3 = e4 == null ? void 0 : e4.call(this);
        r4 == null ? void 0 : r4.call(this, s5), this.requestUpdate(t3, h3, i5);
      }, configurable: true, enumerable: true };
    }
    static getPropertyOptions(t3) {
      var _a6;
      return (_a6 = this.elementProperties.get(t3)) != null ? _a6 : b;
    }
    static _$Ei() {
      if (this.hasOwnProperty(d("elementProperties"))) return;
      const t3 = n2(this);
      t3.finalize(), void 0 !== t3.l && (this.l = [...t3.l]), this.elementProperties = new Map(t3.elementProperties);
    }
    static finalize() {
      if (this.hasOwnProperty(d("finalized"))) return;
      if (this.finalized = true, this._$Ei(), this.hasOwnProperty(d("properties"))) {
        const t4 = this.properties, s4 = [...r2(t4), ...o2(t4)];
        for (const i5 of s4) this.createProperty(i5, t4[i5]);
      }
      const t3 = this[Symbol.metadata];
      if (null !== t3) {
        const s4 = litPropertyMetadata.get(t3);
        if (void 0 !== s4) for (const [t4, i5] of s4) this.elementProperties.set(t4, i5);
      }
      this._$Eh = /* @__PURE__ */ new Map();
      for (const [t4, s4] of this.elementProperties) {
        const i5 = this._$Eu(t4, s4);
        void 0 !== i5 && this._$Eh.set(i5, t4);
      }
      this.elementStyles = this.finalizeStyles(this.styles);
    }
    static finalizeStyles(s4) {
      const i5 = [];
      if (Array.isArray(s4)) {
        const e4 = new Set(s4.flat(1 / 0).reverse());
        for (const s5 of e4) i5.unshift(c(s5));
      } else void 0 !== s4 && i5.push(c(s4));
      return i5;
    }
    static _$Eu(t3, s4) {
      const i5 = s4.attribute;
      return false === i5 ? void 0 : "string" == typeof i5 ? i5 : "string" == typeof t3 ? t3.toLowerCase() : void 0;
    }
    constructor() {
      super(), this._$Ep = void 0, this.isUpdatePending = false, this.hasUpdated = false, this._$Em = null, this._$Ev();
    }
    _$Ev() {
      var _a6;
      this._$ES = new Promise((t3) => this.enableUpdating = t3), this._$AL = /* @__PURE__ */ new Map(), this._$E_(), this.requestUpdate(), (_a6 = this.constructor.l) == null ? void 0 : _a6.forEach((t3) => t3(this));
    }
    addController(t3) {
      var _a6, _b2;
      ((_a6 = this._$EO) != null ? _a6 : this._$EO = /* @__PURE__ */ new Set()).add(t3), void 0 !== this.renderRoot && this.isConnected && ((_b2 = t3.hostConnected) == null ? void 0 : _b2.call(t3));
    }
    removeController(t3) {
      var _a6;
      (_a6 = this._$EO) == null ? void 0 : _a6.delete(t3);
    }
    _$E_() {
      const t3 = /* @__PURE__ */ new Map(), s4 = this.constructor.elementProperties;
      for (const i5 of s4.keys()) this.hasOwnProperty(i5) && (t3.set(i5, this[i5]), delete this[i5]);
      t3.size > 0 && (this._$Ep = t3);
    }
    createRenderRoot() {
      var _a6;
      const t3 = (_a6 = this.shadowRoot) != null ? _a6 : this.attachShadow(this.constructor.shadowRootOptions);
      return S(t3, this.constructor.elementStyles), t3;
    }
    connectedCallback() {
      var _a6, _b2;
      (_a6 = this.renderRoot) != null ? _a6 : this.renderRoot = this.createRenderRoot(), this.enableUpdating(true), (_b2 = this._$EO) == null ? void 0 : _b2.forEach((t3) => {
        var _a7;
        return (_a7 = t3.hostConnected) == null ? void 0 : _a7.call(t3);
      });
    }
    enableUpdating(t3) {
    }
    disconnectedCallback() {
      var _a6;
      (_a6 = this._$EO) == null ? void 0 : _a6.forEach((t3) => {
        var _a7;
        return (_a7 = t3.hostDisconnected) == null ? void 0 : _a7.call(t3);
      });
    }
    attributeChangedCallback(t3, s4, i5) {
      this._$AK(t3, i5);
    }
    _$ET(t3, s4) {
      var _a6;
      const i5 = this.constructor.elementProperties.get(t3), e4 = this.constructor._$Eu(t3, i5);
      if (void 0 !== e4 && true === i5.reflect) {
        const h3 = (void 0 !== ((_a6 = i5.converter) == null ? void 0 : _a6.toAttribute) ? i5.converter : u).toAttribute(s4, i5.type);
        this._$Em = t3, null == h3 ? this.removeAttribute(e4) : this.setAttribute(e4, h3), this._$Em = null;
      }
    }
    _$AK(t3, s4) {
      var _a6, _b2, _c;
      const i5 = this.constructor, e4 = i5._$Eh.get(t3);
      if (void 0 !== e4 && this._$Em !== e4) {
        const t4 = i5.getPropertyOptions(e4), h3 = "function" == typeof t4.converter ? { fromAttribute: t4.converter } : void 0 !== ((_a6 = t4.converter) == null ? void 0 : _a6.fromAttribute) ? t4.converter : u;
        this._$Em = e4;
        const r4 = h3.fromAttribute(s4, t4.type);
        this[e4] = (_c = r4 != null ? r4 : (_b2 = this._$Ej) == null ? void 0 : _b2.get(e4)) != null ? _c : r4, this._$Em = null;
      }
    }
    requestUpdate(t3, s4, i5, e4 = false, h3) {
      var _a6, _b2;
      if (void 0 !== t3) {
        const r4 = this.constructor;
        if (false === e4 && (h3 = this[t3]), i5 != null ? i5 : i5 = r4.getPropertyOptions(t3), !(((_a6 = i5.hasChanged) != null ? _a6 : f)(h3, s4) || i5.useDefault && i5.reflect && h3 === ((_b2 = this._$Ej) == null ? void 0 : _b2.get(t3)) && !this.hasAttribute(r4._$Eu(t3, i5)))) return;
        this.C(t3, s4, i5);
      }
      false === this.isUpdatePending && (this._$ES = this._$EP());
    }
    C(t3, s4, { useDefault: i5, reflect: e4, wrapped: h3 }, r4) {
      var _a6, _b2, _c;
      i5 && !((_a6 = this._$Ej) != null ? _a6 : this._$Ej = /* @__PURE__ */ new Map()).has(t3) && (this._$Ej.set(t3, (_b2 = r4 != null ? r4 : s4) != null ? _b2 : this[t3]), true !== h3 || void 0 !== r4) || (this._$AL.has(t3) || (this.hasUpdated || i5 || (s4 = void 0), this._$AL.set(t3, s4)), true === e4 && this._$Em !== t3 && ((_c = this._$Eq) != null ? _c : this._$Eq = /* @__PURE__ */ new Set()).add(t3));
    }
    async _$EP() {
      this.isUpdatePending = true;
      try {
        await this._$ES;
      } catch (t4) {
        Promise.reject(t4);
      }
      const t3 = this.scheduleUpdate();
      return null != t3 && await t3, !this.isUpdatePending;
    }
    scheduleUpdate() {
      return this.performUpdate();
    }
    performUpdate() {
      var _a6, _b2;
      if (!this.isUpdatePending) return;
      if (!this.hasUpdated) {
        if ((_a6 = this.renderRoot) != null ? _a6 : this.renderRoot = this.createRenderRoot(), this._$Ep) {
          for (const [t5, s5] of this._$Ep) this[t5] = s5;
          this._$Ep = void 0;
        }
        const t4 = this.constructor.elementProperties;
        if (t4.size > 0) for (const [s5, i5] of t4) {
          const { wrapped: t5 } = i5, e4 = this[s5];
          true !== t5 || this._$AL.has(s5) || void 0 === e4 || this.C(s5, void 0, i5, e4);
        }
      }
      let t3 = false;
      const s4 = this._$AL;
      try {
        t3 = this.shouldUpdate(s4), t3 ? (this.willUpdate(s4), (_b2 = this._$EO) == null ? void 0 : _b2.forEach((t4) => {
          var _a7;
          return (_a7 = t4.hostUpdate) == null ? void 0 : _a7.call(t4);
        }), this.update(s4)) : this._$EM();
      } catch (s5) {
        throw t3 = false, this._$EM(), s5;
      }
      t3 && this._$AE(s4);
    }
    willUpdate(t3) {
    }
    _$AE(t3) {
      var _a6;
      (_a6 = this._$EO) == null ? void 0 : _a6.forEach((t4) => {
        var _a7;
        return (_a7 = t4.hostUpdated) == null ? void 0 : _a7.call(t4);
      }), this.hasUpdated || (this.hasUpdated = true, this.firstUpdated(t3)), this.updated(t3);
    }
    _$EM() {
      this._$AL = /* @__PURE__ */ new Map(), this.isUpdatePending = false;
    }
    get updateComplete() {
      return this.getUpdateComplete();
    }
    getUpdateComplete() {
      return this._$ES;
    }
    shouldUpdate(t3) {
      return true;
    }
    update(t3) {
      this._$Eq && (this._$Eq = this._$Eq.forEach((t4) => this._$ET(t4, this[t4]))), this._$EM();
    }
    updated(t3) {
    }
    firstUpdated(t3) {
    }
  };
  var _a2;
  y.elementStyles = [], y.shadowRootOptions = { mode: "open" }, y[d("elementProperties")] = /* @__PURE__ */ new Map(), y[d("finalized")] = /* @__PURE__ */ new Map(), p == null ? void 0 : p({ ReactiveElement: y }), ((_a2 = a.reactiveElementVersions) != null ? _a2 : a.reactiveElementVersions = []).push("2.1.2");

  // node_modules/lit-html/lit-html.js
  var t2 = globalThis;
  var i3 = (t3) => t3;
  var s2 = t2.trustedTypes;
  var e3 = s2 ? s2.createPolicy("lit-html", { createHTML: (t3) => t3 }) : void 0;
  var h2 = "$lit$";
  var o3 = `lit$${Math.random().toFixed(9).slice(2)}$`;
  var n3 = "?" + o3;
  var r3 = `<${n3}>`;
  var l2 = document;
  var c3 = () => l2.createComment("");
  var a2 = (t3) => null === t3 || "object" != typeof t3 && "function" != typeof t3;
  var u2 = Array.isArray;
  var d2 = (t3) => u2(t3) || "function" == typeof (t3 == null ? void 0 : t3[Symbol.iterator]);
  var f2 = "[ 	\n\f\r]";
  var v = /<(?:(!--|\/[^a-zA-Z])|(\/?[a-zA-Z][^>\s]*)|(\/?$))/g;
  var _ = /-->/g;
  var m = />/g;
  var p2 = RegExp(`>|${f2}(?:([^\\s"'>=/]+)(${f2}*=${f2}*(?:[^ 	
\f\r"'\`<>=]|("|')|))|$)`, "g");
  var g = /'/g;
  var $ = /"/g;
  var y2 = /^(?:script|style|textarea|title)$/i;
  var x = (t3) => (i5, ...s4) => ({ _$litType$: t3, strings: i5, values: s4 });
  var b2 = x(1);
  var w = x(2);
  var T = x(3);
  var E = Symbol.for("lit-noChange");
  var A = Symbol.for("lit-nothing");
  var C = /* @__PURE__ */ new WeakMap();
  var P = l2.createTreeWalker(l2, 129);
  function V(t3, i5) {
    if (!u2(t3) || !t3.hasOwnProperty("raw")) throw Error("invalid template strings array");
    return void 0 !== e3 ? e3.createHTML(i5) : i5;
  }
  var N = (t3, i5) => {
    const s4 = t3.length - 1, e4 = [];
    let n4, l3 = 2 === i5 ? "<svg>" : 3 === i5 ? "<math>" : "", c4 = v;
    for (let i6 = 0; i6 < s4; i6++) {
      const s5 = t3[i6];
      let a3, u3, d3 = -1, f3 = 0;
      for (; f3 < s5.length && (c4.lastIndex = f3, u3 = c4.exec(s5), null !== u3); ) f3 = c4.lastIndex, c4 === v ? "!--" === u3[1] ? c4 = _ : void 0 !== u3[1] ? c4 = m : void 0 !== u3[2] ? (y2.test(u3[2]) && (n4 = RegExp("</" + u3[2], "g")), c4 = p2) : void 0 !== u3[3] && (c4 = p2) : c4 === p2 ? ">" === u3[0] ? (c4 = n4 != null ? n4 : v, d3 = -1) : void 0 === u3[1] ? d3 = -2 : (d3 = c4.lastIndex - u3[2].length, a3 = u3[1], c4 = void 0 === u3[3] ? p2 : '"' === u3[3] ? $ : g) : c4 === $ || c4 === g ? c4 = p2 : c4 === _ || c4 === m ? c4 = v : (c4 = p2, n4 = void 0);
      const x2 = c4 === p2 && t3[i6 + 1].startsWith("/>") ? " " : "";
      l3 += c4 === v ? s5 + r3 : d3 >= 0 ? (e4.push(a3), s5.slice(0, d3) + h2 + s5.slice(d3) + o3 + x2) : s5 + o3 + (-2 === d3 ? i6 : x2);
    }
    return [V(t3, l3 + (t3[s4] || "<?>") + (2 === i5 ? "</svg>" : 3 === i5 ? "</math>" : "")), e4];
  };
  var S2 = class _S {
    constructor({ strings: t3, _$litType$: i5 }, e4) {
      let r4;
      this.parts = [];
      let l3 = 0, a3 = 0;
      const u3 = t3.length - 1, d3 = this.parts, [f3, v2] = N(t3, i5);
      if (this.el = _S.createElement(f3, e4), P.currentNode = this.el.content, 2 === i5 || 3 === i5) {
        const t4 = this.el.content.firstChild;
        t4.replaceWith(...t4.childNodes);
      }
      for (; null !== (r4 = P.nextNode()) && d3.length < u3; ) {
        if (1 === r4.nodeType) {
          if (r4.hasAttributes()) for (const t4 of r4.getAttributeNames()) if (t4.endsWith(h2)) {
            const i6 = v2[a3++], s4 = r4.getAttribute(t4).split(o3), e5 = /([.?@])?(.*)/.exec(i6);
            d3.push({ type: 1, index: l3, name: e5[2], strings: s4, ctor: "." === e5[1] ? I : "?" === e5[1] ? L : "@" === e5[1] ? z : H }), r4.removeAttribute(t4);
          } else t4.startsWith(o3) && (d3.push({ type: 6, index: l3 }), r4.removeAttribute(t4));
          if (y2.test(r4.tagName)) {
            const t4 = r4.textContent.split(o3), i6 = t4.length - 1;
            if (i6 > 0) {
              r4.textContent = s2 ? s2.emptyScript : "";
              for (let s4 = 0; s4 < i6; s4++) r4.append(t4[s4], c3()), P.nextNode(), d3.push({ type: 2, index: ++l3 });
              r4.append(t4[i6], c3());
            }
          }
        } else if (8 === r4.nodeType) if (r4.data === n3) d3.push({ type: 2, index: l3 });
        else {
          let t4 = -1;
          for (; -1 !== (t4 = r4.data.indexOf(o3, t4 + 1)); ) d3.push({ type: 7, index: l3 }), t4 += o3.length - 1;
        }
        l3++;
      }
    }
    static createElement(t3, i5) {
      const s4 = l2.createElement("template");
      return s4.innerHTML = t3, s4;
    }
  };
  function M(t3, i5, s4 = t3, e4) {
    var _a6, _b2, _c;
    if (i5 === E) return i5;
    let h3 = void 0 !== e4 ? (_a6 = s4._$Co) == null ? void 0 : _a6[e4] : s4._$Cl;
    const o5 = a2(i5) ? void 0 : i5._$litDirective$;
    return (h3 == null ? void 0 : h3.constructor) !== o5 && ((_b2 = h3 == null ? void 0 : h3._$AO) == null ? void 0 : _b2.call(h3, false), void 0 === o5 ? h3 = void 0 : (h3 = new o5(t3), h3._$AT(t3, s4, e4)), void 0 !== e4 ? ((_c = s4._$Co) != null ? _c : s4._$Co = [])[e4] = h3 : s4._$Cl = h3), void 0 !== h3 && (i5 = M(t3, h3._$AS(t3, i5.values), h3, e4)), i5;
  }
  var R = class {
    constructor(t3, i5) {
      this._$AV = [], this._$AN = void 0, this._$AD = t3, this._$AM = i5;
    }
    get parentNode() {
      return this._$AM.parentNode;
    }
    get _$AU() {
      return this._$AM._$AU;
    }
    u(t3) {
      var _a6;
      const { el: { content: i5 }, parts: s4 } = this._$AD, e4 = ((_a6 = t3 == null ? void 0 : t3.creationScope) != null ? _a6 : l2).importNode(i5, true);
      P.currentNode = e4;
      let h3 = P.nextNode(), o5 = 0, n4 = 0, r4 = s4[0];
      for (; void 0 !== r4; ) {
        if (o5 === r4.index) {
          let i6;
          2 === r4.type ? i6 = new k(h3, h3.nextSibling, this, t3) : 1 === r4.type ? i6 = new r4.ctor(h3, r4.name, r4.strings, this, t3) : 6 === r4.type && (i6 = new Z(h3, this, t3)), this._$AV.push(i6), r4 = s4[++n4];
        }
        o5 !== (r4 == null ? void 0 : r4.index) && (h3 = P.nextNode(), o5++);
      }
      return P.currentNode = l2, e4;
    }
    p(t3) {
      let i5 = 0;
      for (const s4 of this._$AV) void 0 !== s4 && (void 0 !== s4.strings ? (s4._$AI(t3, s4, i5), i5 += s4.strings.length - 2) : s4._$AI(t3[i5])), i5++;
    }
  };
  var k = class _k {
    get _$AU() {
      var _a6, _b2;
      return (_b2 = (_a6 = this._$AM) == null ? void 0 : _a6._$AU) != null ? _b2 : this._$Cv;
    }
    constructor(t3, i5, s4, e4) {
      var _a6;
      this.type = 2, this._$AH = A, this._$AN = void 0, this._$AA = t3, this._$AB = i5, this._$AM = s4, this.options = e4, this._$Cv = (_a6 = e4 == null ? void 0 : e4.isConnected) != null ? _a6 : true;
    }
    get parentNode() {
      let t3 = this._$AA.parentNode;
      const i5 = this._$AM;
      return void 0 !== i5 && 11 === (t3 == null ? void 0 : t3.nodeType) && (t3 = i5.parentNode), t3;
    }
    get startNode() {
      return this._$AA;
    }
    get endNode() {
      return this._$AB;
    }
    _$AI(t3, i5 = this) {
      t3 = M(this, t3, i5), a2(t3) ? t3 === A || null == t3 || "" === t3 ? (this._$AH !== A && this._$AR(), this._$AH = A) : t3 !== this._$AH && t3 !== E && this._(t3) : void 0 !== t3._$litType$ ? this.$(t3) : void 0 !== t3.nodeType ? this.T(t3) : d2(t3) ? this.k(t3) : this._(t3);
    }
    O(t3) {
      return this._$AA.parentNode.insertBefore(t3, this._$AB);
    }
    T(t3) {
      this._$AH !== t3 && (this._$AR(), this._$AH = this.O(t3));
    }
    _(t3) {
      this._$AH !== A && a2(this._$AH) ? this._$AA.nextSibling.data = t3 : this.T(l2.createTextNode(t3)), this._$AH = t3;
    }
    $(t3) {
      var _a6;
      const { values: i5, _$litType$: s4 } = t3, e4 = "number" == typeof s4 ? this._$AC(t3) : (void 0 === s4.el && (s4.el = S2.createElement(V(s4.h, s4.h[0]), this.options)), s4);
      if (((_a6 = this._$AH) == null ? void 0 : _a6._$AD) === e4) this._$AH.p(i5);
      else {
        const t4 = new R(e4, this), s5 = t4.u(this.options);
        t4.p(i5), this.T(s5), this._$AH = t4;
      }
    }
    _$AC(t3) {
      let i5 = C.get(t3.strings);
      return void 0 === i5 && C.set(t3.strings, i5 = new S2(t3)), i5;
    }
    k(t3) {
      u2(this._$AH) || (this._$AH = [], this._$AR());
      const i5 = this._$AH;
      let s4, e4 = 0;
      for (const h3 of t3) e4 === i5.length ? i5.push(s4 = new _k(this.O(c3()), this.O(c3()), this, this.options)) : s4 = i5[e4], s4._$AI(h3), e4++;
      e4 < i5.length && (this._$AR(s4 && s4._$AB.nextSibling, e4), i5.length = e4);
    }
    _$AR(t3 = this._$AA.nextSibling, s4) {
      var _a6;
      for ((_a6 = this._$AP) == null ? void 0 : _a6.call(this, false, true, s4); t3 !== this._$AB; ) {
        const s5 = i3(t3).nextSibling;
        i3(t3).remove(), t3 = s5;
      }
    }
    setConnected(t3) {
      var _a6;
      void 0 === this._$AM && (this._$Cv = t3, (_a6 = this._$AP) == null ? void 0 : _a6.call(this, t3));
    }
  };
  var H = class {
    get tagName() {
      return this.element.tagName;
    }
    get _$AU() {
      return this._$AM._$AU;
    }
    constructor(t3, i5, s4, e4, h3) {
      this.type = 1, this._$AH = A, this._$AN = void 0, this.element = t3, this.name = i5, this._$AM = e4, this.options = h3, s4.length > 2 || "" !== s4[0] || "" !== s4[1] ? (this._$AH = Array(s4.length - 1).fill(new String()), this.strings = s4) : this._$AH = A;
    }
    _$AI(t3, i5 = this, s4, e4) {
      const h3 = this.strings;
      let o5 = false;
      if (void 0 === h3) t3 = M(this, t3, i5, 0), o5 = !a2(t3) || t3 !== this._$AH && t3 !== E, o5 && (this._$AH = t3);
      else {
        const e5 = t3;
        let n4, r4;
        for (t3 = h3[0], n4 = 0; n4 < h3.length - 1; n4++) r4 = M(this, e5[s4 + n4], i5, n4), r4 === E && (r4 = this._$AH[n4]), o5 || (o5 = !a2(r4) || r4 !== this._$AH[n4]), r4 === A ? t3 = A : t3 !== A && (t3 += (r4 != null ? r4 : "") + h3[n4 + 1]), this._$AH[n4] = r4;
      }
      o5 && !e4 && this.j(t3);
    }
    j(t3) {
      t3 === A ? this.element.removeAttribute(this.name) : this.element.setAttribute(this.name, t3 != null ? t3 : "");
    }
  };
  var I = class extends H {
    constructor() {
      super(...arguments), this.type = 3;
    }
    j(t3) {
      this.element[this.name] = t3 === A ? void 0 : t3;
    }
  };
  var L = class extends H {
    constructor() {
      super(...arguments), this.type = 4;
    }
    j(t3) {
      this.element.toggleAttribute(this.name, !!t3 && t3 !== A);
    }
  };
  var z = class extends H {
    constructor(t3, i5, s4, e4, h3) {
      super(t3, i5, s4, e4, h3), this.type = 5;
    }
    _$AI(t3, i5 = this) {
      var _a6;
      if ((t3 = (_a6 = M(this, t3, i5, 0)) != null ? _a6 : A) === E) return;
      const s4 = this._$AH, e4 = t3 === A && s4 !== A || t3.capture !== s4.capture || t3.once !== s4.once || t3.passive !== s4.passive, h3 = t3 !== A && (s4 === A || e4);
      e4 && this.element.removeEventListener(this.name, this, s4), h3 && this.element.addEventListener(this.name, this, t3), this._$AH = t3;
    }
    handleEvent(t3) {
      var _a6, _b2;
      "function" == typeof this._$AH ? this._$AH.call((_b2 = (_a6 = this.options) == null ? void 0 : _a6.host) != null ? _b2 : this.element, t3) : this._$AH.handleEvent(t3);
    }
  };
  var Z = class {
    constructor(t3, i5, s4) {
      this.element = t3, this.type = 6, this._$AN = void 0, this._$AM = i5, this.options = s4;
    }
    get _$AU() {
      return this._$AM._$AU;
    }
    _$AI(t3) {
      M(this, t3);
    }
  };
  var B = t2.litHtmlPolyfillSupport;
  var _a3;
  B == null ? void 0 : B(S2, k), ((_a3 = t2.litHtmlVersions) != null ? _a3 : t2.litHtmlVersions = []).push("3.3.3");
  var D = (t3, i5, s4) => {
    var _a6, _b2;
    const e4 = (_a6 = s4 == null ? void 0 : s4.renderBefore) != null ? _a6 : i5;
    let h3 = e4._$litPart$;
    if (void 0 === h3) {
      const t4 = (_b2 = s4 == null ? void 0 : s4.renderBefore) != null ? _b2 : null;
      e4._$litPart$ = h3 = new k(i5.insertBefore(c3(), t4), t4, void 0, s4 != null ? s4 : {});
    }
    return h3._$AI(t3), h3;
  };

  // node_modules/lit-element/lit-element.js
  var s3 = globalThis;
  var i4 = class extends y {
    constructor() {
      super(...arguments), this.renderOptions = { host: this }, this._$Do = void 0;
    }
    createRenderRoot() {
      var _a6, _b2;
      const t3 = super.createRenderRoot();
      return (_b2 = (_a6 = this.renderOptions).renderBefore) != null ? _b2 : _a6.renderBefore = t3.firstChild, t3;
    }
    update(t3) {
      const r4 = this.render();
      this.hasUpdated || (this.renderOptions.isConnected = this.isConnected), super.update(t3), this._$Do = D(r4, this.renderRoot, this.renderOptions);
    }
    connectedCallback() {
      var _a6;
      super.connectedCallback(), (_a6 = this._$Do) == null ? void 0 : _a6.setConnected(true);
    }
    disconnectedCallback() {
      var _a6;
      super.disconnectedCallback(), (_a6 = this._$Do) == null ? void 0 : _a6.setConnected(false);
    }
    render() {
      return E;
    }
  };
  var _a4;
  i4._$litElement$ = true, i4["finalized"] = true, (_a4 = s3.litElementHydrateSupport) == null ? void 0 : _a4.call(s3, { LitElement: i4 });
  var o4 = s3.litElementPolyfillSupport;
  o4 == null ? void 0 : o4({ LitElement: i4 });
  var _a5;
  ((_a5 = s3.litElementVersions) != null ? _a5 : s3.litElementVersions = []).push("4.2.2");

  // src/UI/components/base.ts
  var RouterElement = class extends i4 {
    createRenderRoot() {
      return this;
    }
  };

  // src/UI/components/router-header.ts
  var RouterHeader = class extends RouterElement {
    render() {
      return b2`<header>
      <div class="brand">
        <span class="logo"></span><span class="brand-copy"><span class="brand-name"><span>MultiTron2</span><span class="beta">1.0.1 beta</span></span><span class="sub">&nbsp;io</span></span>
      </div>
      <div class="settings-actions">
        <div><button type="button" onclick="RouterAction('settings-save','')">Save</button><button type="button" onclick="RouterAction('settings-load','')">Load</button></div>
        <span id="loadedSettingsFile"></span>
      </div>
      <div class="session-stats" aria-label="Session stats">
        <span class="stats-title">S T A T S</span>
        <div class="stats-units" aria-label="Distance unit">
          <button type="button" class="active" data-stats-unit="km" aria-pressed="true" onclick="RouterStatsUnit('km')">KM</button>
          <button type="button" data-stats-unit="mi" aria-pressed="false" onclick="RouterStatsUnit('mi')">MI</button>
        </div>
        <div id="sessionMetrics"></div>
      </div>
    </header>`;
    }
  };
  customElements.define("router-header", RouterHeader);

  // src/UI/components/input-tabs.ts
  var InputTabs = class extends RouterElement {
    tab(page, label, active = false) {
      return b2`<button class="tab ${active ? "active" : ""}" data-page="${page}" @click=${() => this.select(page)}>
      <span class="tab-output ${page}"><i id="${page}OutputBar"></i><b id="${page}OutputValue">0%</b></span>${label}
    </button>`;
    }
    select(page) {
      this.querySelectorAll(".tab").forEach(
        (tab) => tab.classList.toggle("active", tab.dataset.page === page)
      );
      document.querySelectorAll(".page").forEach((panel) => panel.classList.toggle("active", panel.id === page));
    }
    render() {
      return b2`<div class="tabs" style="align-items:center">
      <span class="section-label">I N P U T</span
      >${this.tab("mouse", "Mouse", true)}${this.tab("audio", "Audio")}${this.tab("loudness", "Loudness")}${this.tab("combined", "Combined")}
      <button class="tab input-settings-cog" type="button" data-page="input-settings" aria-label="Input settings" @click=${() => this.select("input-settings")}>
        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M19.43 12.98c.04-.32.07-.65.07-.98s-.02-.66-.07-.98l2.11-1.65a.5.5 0 0 0 .12-.64l-2-3.46a.5.5 0 0 0-.61-.22l-2.49 1a7.3 7.3 0 0 0-1.69-.98L14.5 2.42A.5.5 0 0 0 14 2h-4a.5.5 0 0 0-.49.42l-.38 2.65c-.61.25-1.18.59-1.69.98l-2.49-1a.5.5 0 0 0-.61.22l-2 3.46a.5.5 0 0 0 .12.64L4.57 11.02c-.04.32-.07.65-.07.98s.02.66.07.98l-2.11 1.65a.5.5 0 0 0-.12.64l2 3.46a.5.5 0 0 0 .61.22l2.49-1c.51.4 1.08.73 1.69.98l.38 2.65a.5.5 0 0 0 .49.42h4a.5.5 0 0 0 .49-.42l.38-2.65c.61-.25 1.18-.59 1.69-.98l2.49 1a.5.5 0 0 0 .61-.22l2-3.46a.5.5 0 0 0-.12-.64l-2.11-1.65ZM12 15.5A3.5 3.5 0 1 1 12 8a3.5 3.5 0 0 1 0 7.5Z" /></svg>
      </button>
    </div>
    <section class="page input-settings-page" id="input-settings">
      <h1>Settings</h1>
      <p class="hint">Configure the stats overlay shown above other apps.</p>
      <div class="form">
        <label class="switch"><span>Show stats overlay</span><input id="overlayEnabled" type="checkbox" onchange="RouterAction('overlay',this.checked?'1':'0')" /></label>
        <div class="field"><label>Overlay position</label><select id="overlayPosition" onchange="RouterAction('overlay-position',this.value)"><option value="0">Top left</option><option value="1">Top right</option><option value="2">Bottom left</option><option value="3">Bottom right</option></select></div>
        <div class="field"><label>Log</label><a class="driver-link" href="#" onclick="RouterAction('open-log','');return false;">Show Log</a></div>
        <div class="field"><label>Help</label><a class="driver-link" href="https://discord.gg/XkfF9mu6kx" onclick="RouterAction('open-external','https://discord.gg/XkfF9mu6kx');return false;">Join the Discord</a></div>
      </div>
    </section>`;
    }
  };
  customElements.define("router-input-tabs", InputTabs);

  // src/UI/components/mouse-input.ts
  var MouseInput = class extends RouterElement {
    render() {
      return b2`<div class="page active" id="mouse">
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
  };
  customElements.define("router-mouse-input", MouseInput);

  // src/UI/components/audio-input.ts
  var AudioInput = class extends RouterElement {
    render() {
      return b2`<div class="page" id="audio">
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
  };
  customElements.define("router-audio-input", AudioInput);

  // src/UI/components/loudness-input.ts
  var LoudnessInput = class extends RouterElement {
    render() {
      return b2`<div class="page" id="loudness">

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
  };
  customElements.define("router-loudness-input", LoudnessInput);

  // src/UI/components/combined-input.ts
  var sourceRow = (source, label) => b2`<div class="mix-row">
  <strong>${label}</strong>
  <label>Range <input id="${source}MixMinimum" type="number" min="0" max="100" onchange="RouterAction('${source}-mix-min',this.value)" /> to <input id="${source}MixMaximum" type="number" min="0" max="100" onchange="RouterAction('${source}-mix-max',this.value)" />%</label>
  <label>Priority &nbsp;<select id="${source}MixPriority" onchange="RouterAction('${source}-mix-priority',this.value)"><option value="0">Off</option><option value="1">Low</option><option value="2">Medium</option><option value="3">High</option></select></label>
</div>`;
  var CombinedInput = class extends RouterElement {
    render() {
      return b2`<div class="page" id="combined">
      <h1>Combined inputs</h1>
      <p class="hint">Each source maps to its range. Off sources do not affect the combined output; otherwise, the highest-priority active source wins overlap.</p>
      <div class="visual combined-visual" id="combinedVisual">
        <div class="cross"></div>
        <span class="axis-label x">X</span><span class="axis-label y">Y</span>
        <div class="dot mouse" id="mouseDot"></div>
        <div class="dot audio" id="audioDot"></div>
        <div class="dot loudness" id="loudnessDot"></div>
        <div class="dot output" id="outputDot"></div>
        <div class="legend"><span><i class="mouse"></i>Mouse</span><span><i class="audio"></i>Audio</span><span><i class="loudness"></i>Loudness</span><span><i class="combined"></i>Combined</span></div>
      </div>
      <div class="mix-controls">
        ${sourceRow("mouse", "Mouse")}
        ${sourceRow("audio", "Audio")}
        ${sourceRow("loudness", "Loudness")}
      </div>
    </div>`;
    }
  };
  customElements.define("router-combined-input", CombinedInput);

  // src/UI/components/output-tabs.ts
  var OutputTabs = class extends RouterElement {
    select(output) {
      this.querySelectorAll(".outtab").forEach(
        (tab) => tab.classList.toggle("active", tab.dataset.output === output)
      );
    }
    render() {
      return b2`<div class="outtabs" style="align-items:center">
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
  };
  customElements.define("router-output-tabs", OutputTabs);

  // src/UI/components/vjoy-output.ts
  var VJoyOutput = class extends RouterElement {
    render() {
      return b2`<div id="visual" style="display:none"></div><section id="vjoyPanel" class="keys vjoy-panel">
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
  };
  customElements.define("router-vjoy-output", VJoyOutput);

  // src/UI/components/keyboard-output.ts
  var KeyboardOutput = class extends RouterElement {
    render() {
      return b2`<section id="keyboardPanel" class="keys" style="display:block">
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
  };
  customElements.define("router-keyboard-output", KeyboardOutput);

  // src/UI/components/xbox-output.ts
  var XboxOutput = class extends RouterElement {
    render() {
      return b2`<section id="xboxPanel" class="keys xbox-panel">

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
  };
  customElements.define("router-xbox-output", XboxOutput);
})();
/*! Bundled license information:

@lit/reactive-element/css-tag.js:
  (**
   * @license
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: BSD-3-Clause
   *)

@lit/reactive-element/reactive-element.js:
lit-html/lit-html.js:
lit-element/lit-element.js:
  (**
   * @license
   * Copyright 2017 Google LLC
   * SPDX-License-Identifier: BSD-3-Clause
   *)

lit-html/is-server.js:
  (**
   * @license
   * Copyright 2022 Google LLC
   * SPDX-License-Identifier: BSD-3-Clause
   *)
*/
