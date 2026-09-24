# Prompt 3 — Claude Code (assemble HTML)

You are building a single self-contained HTML file: `system_overview.html`.
You will receive `synthesis.md` containing prose + Mermaid diagram blocks.
Your job is purely mechanical: convert it into a polished, navigable HTML page.

## INPUT
Paste the FULL content of `synthesis.md` below:

<PASTE synthesis.md HERE>

## TECHNICAL REQUIREMENTS
- Single file, zero build step, opens directly in a browser.
- Mermaid: <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
  Initialize with: mermaid.initialize({ startOnLoad: true, theme: 'dark' });
- Every Mermaid block from input -> wrap in <div class="mermaid">...</div>.
- All prose -> wrap in <p> tags inside section containers.
- Do NOT modify any diagram content or prose — assemble only.

## LAYOUT REQUIREMENTS
- Dark theme: background #0d1117, text #e6edf3, accent #58a6ff.
- Sticky left sidebar with a table of contents linking to all 11 sections.
- Each section wrapped in <details open> (collapsible).
- Section headers <h2> with matching <a id="..."> for TOC anchors.
- Code/constant values in <code> tags (monospace).
- Section 10 Key Constants Table rendered as a real <table> with thead/tbody,
  borders, and alternating row colors.
- Responsive: sidebar collapses below 768px, sections go full-width.

## SIDEBAR TOC (exactly 11 entries, in this order)
```html
<nav id="toc">
  <h3>Contents</h3>
  <a href="#executive-summary">1. Executive Summary</a>
  <a href="#nfc-provisioning">2. NFC Provisioning</a>
  <a href="#ble-auth">3. BLE Authentication</a>
  <a href="#uwb-pipeline">4. UWB Localization</a>
  <a href="#access-control">5. Access Control</a>
  <a href="#tinyml">6. Intent Recognition</a>
  <a href="#mobile-app">7. Mobile App</a>
  <a href="#cloud-anomaly">8. Cloud &amp; Anomaly</a>
  <a href="#freertos">9. FreeRTOS Runtime</a>
  <a href="#constants">10. Key Constants</a>
  <a href="#e2e-sequence">11. End-to-End Sequence</a>
</nav>
```

## FILE PATH REFERENCES
Whenever synthesis.md mentions a source file, render it as:
<code class="filepath">iot/src/uwb/ekf_stub.cpp</code>
with style: color #79c0ff; font-size 0.85em.

## RESEARCH NOTE CALLOUT
In Section 4 (UWB Pipeline), add this styled callout box after the prose:
```html
<div class="callout-research">
  &#9888;&#65039; Research transport: The PC bridge (run_fira_bridge.py) is a
  development-grade serial relay, not a production topology. In a production
  system this would be replaced by direct SPI/UART between the UWB module and ESP32.
</div>
```
Style: border-left 3px solid #d29922; background #272115; padding 12px; margin 16px 0;

## DELIVERABLE
Output only the complete HTML file. No explanation, no markdown wrapper.
Start with <!DOCTYPE html> and end with </html>.
