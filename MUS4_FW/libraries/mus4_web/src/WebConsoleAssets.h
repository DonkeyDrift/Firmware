static const char WIFI_WEB_CONSOLE_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<link rel="icon" type="image/png" href="/favicon.png">
<title>Drifter Console</title>
<script>try{let t=window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark';document.documentElement.dataset.theme=t}catch(e){}</script>
<style>
body.preinit{visibility:hidden}
:root{--bg:#000;--ink:#f5f5f7;--ink2:rgba(235,235,245,.85);--ink3:rgba(235,235,245,.72);--ink4:rgba(235,235,245,.62);--inkHi:#f5f5f7;--inkPill:rgba(235,235,245,.72);--inkT:rgba(235,235,245,.72);--panel:#1c1c1e;--card:#1c1c1e;--card2:#1c1c1e;--card3:#2c2c2e;--foldHover:#2c2c2e;--line:rgba(255,255,255,.10);--line2:rgba(255,255,255,.10);--line3:rgba(255,255,255,.10);--control:#2c2c2e;--controlHover:#3a3a3c;--controlInk:#f5f5f7;--segHover:transparent;--accent:#2997ff;--accent2:#2997ff;--accentHi:#2997ff;--accentFill:#0a84ff;--onAccent:#fff;--toggleOn:#30d158;--accentSoft:rgba(10,132,255,.15);--accentSoft2:rgba(10,132,255,.28);--ok:#30d158;--warn:#ff9f0a;--bad:#ff453a;--bad2:#ff453a;--badSoft:rgba(255,69,58,.15);--badGlow:rgba(255,69,58,.35);--off:#636366;--dotOff:#636366;--drift:#bf5af2;--driftBarBg:#2c2c2e;--driftScan:linear-gradient(90deg,transparent,rgba(191,90,242,.16),transparent);--logBg:#000;--logInk:#30d158;--canvasBg:#000;--termBg:#000;--scrim:rgba(0,0,0,.44);--scrim2:rgba(0,0,0,.44);--overlay:rgba(0,0,0,.6);--cardGrad:#1c1c1e;--stateCardGrad:linear-gradient(135deg,#1c2430,#121821);--stateCardLine:#344154;--stateCardShadow:0 0 0 rgba(0,0,0,0);--cardShadow:0 1px 3px rgba(0,0,0,.55);--popShadow:0 18px 50px rgba(0,0,0,.5);--popInk:#f5f5f7;--popBorder:rgba(255,255,255,.10);--toastShadow:0 12px 40px rgba(0,0,0,.4);--scanShadow:0 12px 32px rgba(0,0,0,.4);--foldIcon:#2997ff;--closeInk:rgba(235,235,245,.62);--closeHoverBg:#2c2c2e;--closeHoverInk:#f5f5f7;--fabBg:#0a84ff;--fabBgHover:#2997ff;--fabGlow:none;--fabGlowHover:none;--fabActionBg:#0a84ff;--fabActionBorder:#0a84ff;--fabShadow:none;--fabShadowHover:none;--navHover:#2997ff;--tubRec:#ff453a;--tabActiveInk:#2997ff;--tabActiveBg:#1c1c1e;--termTabLine:rgba(255,255,255,.10);--termTabBg:#1c1c1e;--histDelLine:rgba(255,255,255,.10);--setRowBg:#1c1c1e;--frameBg:#000;--knob:#fff;--c6:#ff375f;--c7:#32d74b;--c8:#ff9f0a;--ease-apple:cubic-bezier(.32,.72,0,1);--ok-text:#30d158;--warn-text:#ff9f0a;--bad-text:#ff453a;--drift-text:#bf5af2;--sep:rgba(255,255,255,.16);--cardLine:rgba(255,255,255,.10);--mat:rgba(28,28,30,.72);--matSolid:rgba(28,28,30,.96);--elev:0 8px 30px rgba(0,0,0,.5);--appleFont:-apple-system,BlinkMacSystemFont,system-ui,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;--appleMono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}html[data-theme="light"]{--bg:#f5f5f7;--ink:#1d1d1f;--ink2:rgba(60,60,67,.85);--ink3:rgba(60,60,67,.72);--ink4:rgba(60,60,67,.62);--inkHi:#1d1d1f;--inkPill:rgba(60,60,67,.72);--inkT:rgba(60,60,67,.72);--panel:rgba(120,120,128,.12);--card:#fff;--card2:#fff;--card3:#f5f5f7;--foldHover:#f5f5f7;--line:rgba(0,0,0,.08);--line2:rgba(0,0,0,.08);--line3:rgba(0,0,0,.08);--control:#fff;--controlHover:#f5f5f7;--controlInk:#1d1d1f;--segHover:transparent;--accent:#0066cc;--accent2:#0066cc;--accentHi:#0066cc;--accentFill:#0071e3;--onAccent:#fff;--toggleOn:#34c759;--ok:#34c759;--warn:#ff9500;--bad:#ff3b30;--bad2:#ff3b30;--badSoft:rgba(255,59,48,.12);--badGlow:rgba(255,59,48,.3);--off:#8e8e93;--dotOff:#8e8e93;--drift:#af52de;--driftBarBg:rgba(120,120,128,.16);--driftScan:linear-gradient(90deg,transparent,rgba(175,82,222,.12),transparent);--logBg:#f5f5f7;--logInk:#1a7f37;--canvasBg:#fff;--scrim:rgba(0,0,0,.30);--scrim2:rgba(0,0,0,.30);--overlay:rgba(245,245,247,.85);--cardGrad:#fff;--stateCardGrad:linear-gradient(135deg,#fff,#edf1f6);--stateCardLine:#ccd5df;--stateCardShadow:0 1px 3px rgba(15,23,42,.08);--cardShadow:0 1px 3px rgba(0,0,0,.06);--popShadow:0 18px 50px rgba(0,0,0,.18);--popInk:#1d1d1f;--popBorder:rgba(0,0,0,.08);--toastShadow:0 12px 40px rgba(0,0,0,.14);--scanShadow:0 12px 32px rgba(0,0,0,.12);--foldIcon:#0066cc;--closeInk:rgba(60,60,67,.62);--closeHoverBg:#f5f5f7;--closeHoverInk:#1d1d1f;--fabBg:#0071e3;--fabBgHover:#0066cc;--fabShadow:none;--fabShadowHover:none;--navHover:#0066cc;--tubRec:#ff3b30;--tabActiveInk:#0066cc;--tabActiveBg:#fff;--termTabLine:rgba(0,0,0,.08);--termTabBg:#fff;--histDelLine:rgba(0,0,0,.08);--setRowBg:#fff;--c6:#ff2d55;--c7:#34c759;--c8:#ff9500;--accentSoft:rgba(0,113,227,.10);--accentSoft2:rgba(0,113,227,.18);--termBg:#f5f5f7;--frameBg:#f5f5f7;--ok-text:#1a7f37;--warn-text:#c93400;--bad-text:#d70015;--drift-text:#8944ab;--sep:rgba(60,60,67,.29);--cardLine:rgba(0,0,0,.08);--mat:rgba(245,245,247,.72);--matSolid:rgba(245,245,247,.96);--elev:0 8px 30px rgba(0,0,0,.12)}html:root h1{font-weight:600;letter-spacing:-0.02em}html:root button{font-weight:600!important;border-radius:8px}html:root input,html:root select{border-radius:8px}html:root .themeButton,html:root .langButton,html:root .muteButton,html:root .otaLink,html:root #devModeToggle{border-radius:9999px}html:root .fabToggle,html:root .helpFab,html:root .gear,html:root .helpClose{border-radius:50%}html:root .rcCell{border-radius:12px}html:root .dialog,html:root .helpModal{border-radius:18px}html:root .toast{border-radius:14px;transition:opacity .28s cubic-bezier(.32,.72,0,1),transform .28s cubic-bezier(.32,.72,0,1)}html:root .modal.show .dialog{animation:uiPop .32s cubic-bezier(.32,.72,0,1)}@keyframes uiPop{from{opacity:0;transform:scale(.96)}}html:root .slider{width:46px;height:28px;border-radius:9999px}html:root .slider:before{width:24px;height:24px;left:2px;bottom:2px}html:root .toggleSwitch input:checked+.slider:before{transform:translateX(18px)}html:root button:active{transform:scale(.97)}html:root .fabToggle:active{transform:scale(.94)}html:root .stateValue,html:root .rcNum,html:root .recMeta b{font-variant-numeric:tabular-nums}body{font-family:system-ui,sans-serif;margin:12px;background:var(--bg);color:var(--ink)}h1{margin:0;font-size:1.25rem;font-weight:700;line-height:1.75rem}.headerRow{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin:0 0 10px;font-family:system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale}body.embedded .headerRow{display:none}.headerRow h1{color:var(--ink);margin:0 20px 0 0}.headerLogo{width:32px;height:32px;border-radius:8px;border:1px solid var(--line);align-self:center}.logoLink{display:inline-flex}.titleLink{color:inherit;text-decoration:none}.rowBreak{display:none}.version{color:var(--ink3);font-size:12px;text-transform:uppercase;letter-spacing:.08em;display:inline-block}.toggleSwitch{position:relative;display:inline-flex;align-items:center;gap:8px;cursor:pointer}.toggleSwitch input{opacity:0;width:0;height:0;position:absolute}.slider{position:relative;width:44px;height:24px;background:var(--off);border-radius:999px;transition:.25s}.slider:before{content:"";position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:var(--knob);border-radius:50%;transition:.25s}.toggleSwitch input:checked+.slider{background:var(--toggleOn)}.toggleSwitch input:checked+.slider:before{transform:translateX(20px)}.toggleLabel{color:var(--ink3);font-size:12px;text-transform:uppercase;letter-spacing:.08em}.ghLink{display:inline-flex;align-items:center;color:var(--ink3);margin-left:6px}.ghLink:hover{color:var(--accent)}.langTabs{display:inline-flex;align-items:center;gap:2px;background:var(--panel);border:1px solid var(--line2);border-radius:999px;padding:0 2px;height:24px;box-sizing:border-box;box-shadow:inset 0 0 0 1px var(--line)}.langTabs button{padding:0 10px;height:24px;min-width:0;border:none;border-radius:999px;background:transparent;color:var(--ink3);font-size:11px;font-weight:800;line-height:1;cursor:pointer}.langTabs button:hover{background:var(--segHover);color:var(--inkHi)}.langTabs button.active{background:var(--accentFill);color:var(--onAccent)}.langTabs button.active:hover{background:var(--accentHi);color:var(--onAccent)}.themeButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:var(--card);border:1px solid var(--line2);box-shadow:inset 0 0 0 1px var(--line);color:var(--inkPill);cursor:pointer}.themeButton:hover{color:var(--ink)}.themeButton .icoSun{display:none}html[data-theme="light"] .themeButton .icoSun{display:block}html[data-theme="light"] .themeButton .icoMoon{display:none}html[data-theme="light"] .themeButton{background:#f4f6f9;border-color:#ccd5df;box-shadow:inset 0 0 0 1px #d5dce4}html[data-theme="light"] .themeButton{color:#3f4f63}html[data-theme="light"] .themeButton:hover{color:#1a2330}.langButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border:1px solid var(--line2);border-radius:9999px;background:var(--card);box-shadow:inset 0 0 0 1px var(--line);color:var(--inkPill);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Noto Sans",Helvetica,Arial,sans-serif,"Apple Color Emoji","Segoe UI Emoji";font-synthesis:none;text-rendering:optimizeLegibility;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale;font-size:12px;font-weight:600;line-height:1;cursor:pointer;transition:color .15s cubic-bezier(.4,0,.2,1),background-color .15s cubic-bezier(.4,0,.2,1),border-color .15s cubic-bezier(.4,0,.2,1)}.langButton:hover,.langButton:focus-visible{color:var(--ink);background:var(--card)}.muteButton{display:inline-flex;align-items:center;justify-content:center;margin-left:auto;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:var(--card);border:1px solid var(--line2);box-shadow:inset 0 0 0 1px var(--line);color:var(--inkPill);cursor:pointer}.muteButton:hover{color:var(--ink)}.muteButton.muted{background:var(--accentSoft);border-color:var(--accentFill);box-shadow:inset 0 0 0 1px var(--accentFill);color:var(--accentFill)}.muteButton .icoMute{display:none}.muteButton.muted .icoMute{display:block}.muteButton.muted .icoSound{display:none}.otaLink{display:inline-flex;align-items:center;justify-content:center;height:32px;padding:0 12px;border-radius:9999px;background:var(--card);border:1px solid var(--line2);box-shadow:inset 0 0 0 1px var(--line);color:var(--inkPill);font-size:12px;font-weight:600;text-decoration:none;cursor:pointer;transition:color .15s,border-color .15s,box-shadow .15s}.otaLink:hover{color:var(--accent);border-color:var(--accent);box-shadow:inset 0 0 0 1px var(--accent)}#devModeToggle{display:inline-flex;align-items:center;justify-content:center;height:32px;padding:0 12px;border-radius:9999px;background:var(--card);border:1px solid var(--line2);box-shadow:inset 0 0 0 1px var(--line);color:var(--inkPill);font-size:12px;font-weight:600;cursor:pointer;transition:color .15s,border-color .15s,box-shadow .15s,background-color .15s}#devModeToggle:hover{color:var(--accent);border-color:var(--accent);box-shadow:inset 0 0 0 1px var(--accent)}#devModeToggle.devOn{background:var(--accentSoft2);border-color:var(--accentFill);box-shadow:inset 0 0 0 1px var(--accentFill);color:var(--accentFill)}.navTab{font-family:inherit;color:var(--ink3);font-size:0.875rem;font-weight:500;text-decoration:none;background:transparent;border:none;padding:0;line-height:1.25rem;white-space:nowrap;display:inline-flex;align-items:center;cursor:pointer;margin-right:12px}.navTab:hover{color:var(--navHover);background:transparent}.navLinks{display:inline-flex;align-items:center;gap:12px}
.navTabWeak{font-family:inherit;color:var(--ink4);font-size:0.75rem;font-weight:500;text-decoration:none;background:transparent;border:none;padding:0;line-height:1rem;white-space:nowrap;display:inline-flex;align-items:center;gap:4px;cursor:pointer;margin-right:12px}.navTabWeak:hover{color:var(--inkPill);background:transparent}.grid{display:grid;grid-template-columns:minmax(0,1fr);gap:10px}.panel{background:transparent;border:none;border-radius:0;padding:10px}#status{white-space:pre-wrap;color:var(--ink2);font-size:13px;margin-top:10px}.fold{margin-top:10px}.foldHead{width:100%;display:flex;align-items:center;gap:6px;text-align:left;background:var(--card);border:1px solid var(--line);color:var(--popInk)}.foldHead:hover{background:var(--foldHover)}.foldIcon{display:inline-block;width:16px;color:var(--foldIcon)}.fold:not(.open) .foldBody{display:none}.statusTable{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:4px 12px}.statusRow{display:grid;grid-template-columns:minmax(100px,auto) 1fr;gap:10px;align-items:start}.statusRow b{color:var(--ink3);font-weight:700}.statusRow span{font:13px Consolas,monospace;color:var(--ink);overflow-wrap:anywhere}.stateGrid{display:grid;gap:10px;align-items:stretch;grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr) minmax(112px,.30fr) minmax(220px,.80fr);grid-template-areas:"mode park drift voltage network"}#modeCard{grid-area:mode}#parkCard{grid-area:park}#driftCard{grid-area:drift}#voltageCard{grid-area:voltage}#networkCard{grid-area:network}.stateCard{position:relative;overflow:hidden;border:1px solid var(--stateCardLine);border-radius:10px;padding:12px;background:var(--stateCardGrad);box-shadow:var(--stateCardShadow);transition:.25s}.stateHead{color:var(--ink3);font-size:12px;text-transform:uppercase;letter-spacing:.08em}.stateValue{font-size:24px;font-weight:800;margin-top:4px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.08}.copyValue{cursor:pointer;position:relative}.copyValue:hover{color:var(--accent)}.copyValue:hover:after{content:'点击复制 IP';position:absolute;left:72px;top:-26px;background:var(--card);border:1px solid var(--popBorder);border-radius:8px;padding:4px 8px;color:var(--popInk);font-size:12px;font-weight:600;white-space:nowrap;pointer-events:none;z-index:4}html[lang=zh] .copyValue:hover:after{content:'点击复制 IP'}html[lang=en] .copyValue:hover:after{content:'Click to copy IP'}.stateSub{color:var(--ink2);font-size:12px;margin-top:3px;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.25}.stateDot{position:absolute;right:12px;top:12px;width:10px;height:10px;border-radius:50%;background:var(--dotOff)}.tunePair{position:absolute;right:12px;top:11px;font-size:11px;line-height:10px;white-space:nowrap;z-index:2}.tunePair a{color:var(--accent);text-decoration:none;vertical-align:-1px}.tunePair .stateDot{position:static;display:inline-block;vertical-align:middle;margin-left:5px;width:10px;height:10px}.gear{position:absolute;right:10px;top:32px;width:30px;height:30px;min-width:0;padding:0;border-radius:50%;font-size:16px;line-height:1;z-index:6}.netTabs{position:absolute;right:28px;top:8px;display:flex;gap:4px}.netTabs button{min-width:0;padding:2px 8px;border-radius:999px;font-size:11px;line-height:1.2}.netTabs button.active{background:var(--accentFill);color:var(--onAccent);font-weight:800}#networkCard .netTabs button.active{background:#5cc8ff;color:#061019}.stateMeta{display:grid;grid-template-columns:1fr;gap:3px;margin-top:8px;font-size:12px}.stateMeta b{color:var(--ink3);font-size:11px;letter-spacing:.08em;text-transform:uppercase}.stateMeta span{font-size:15px;font-weight:700;white-space:normal;overflow:visible;text-overflow:clip;word-break:normal;overflow-wrap:normal;line-height:1.2}#networkCard .stateValue,#networkCard .stateMeta span,#driftCard .stateSub{overflow-wrap:anywhere}.mode0{border-color:var(--ok)}.mode1{border-color:var(--warn)}.mode2{border-color:var(--accent)}.mode0 .stateDot{background:var(--ok)}.mode1 .stateDot{background:var(--warn)}.mode2 .stateDot{background:var(--accent)}.netDown{border-color:var(--bad)}.netDown .stateDot{background:var(--bad)}.parkLocked{border-color:var(--bad);animation:pulse 1.2s infinite}.parkUnlocked{border-color:var(--ok)}.parkLocked .stateDot{background:var(--bad)}.parkUnlocked .stateDot{background:var(--ok)}.driftOff{border-color:var(--off)}.driftArmed{border-color:var(--warn)}.driftActive{border-color:var(--drift);animation:pulse 1s infinite}.driftBar{height:6px;background:var(--driftBarBg);border-radius:999px;margin-top:10px;position:relative}.driftBar i{position:absolute;top:-3px;width:4px;height:12px;background:var(--drift);border-radius:2px;left:50%;transition:left .2s}.driftActive:before{content:"";position:absolute;inset:0;background:var(--driftScan);animation:scan 1.4s infinite}.rcGrid{display:grid;grid-template-columns:repeat(6,minmax(72px,1fr));gap:6px;margin-top:10px}.rcCell{background:var(--card2);border:1px solid var(--line);border-radius:8px;padding:8px;text-align:center}.rcCell b{display:block;color:var(--ink3);font-size:11px}.rcCell span{font:700 18px Consolas,monospace}.rcCell.modeCh{border-color:var(--warn)}.rcNum{flex:0 0 auto;min-width:0;max-width:none;width:4.5ch;font:700 14px Consolas,monospace;text-align:center;background:transparent;border:none;border-radius:4px;color:var(--ink);padding:0;margin:0;-moz-appearance:textfield;appearance:textfield}.rcNum::-webkit-outer-spin-button,.rcNum::-webkit-inner-spin-button{-webkit-appearance:none;margin:0}.rcNum:hover{background:var(--card3)}.rcNum:focus{outline:none;background:var(--card3)}.rcSetBtn{background:var(--accentFill);color:var(--onAccent);border-color:var(--accentFill);font-weight:800}.rcSetBtn:hover{background:var(--accent2);color:var(--onAccent)}.row{display:flex;gap:6px;flex-wrap:nowrap;align-items:center}#cmd{flex:1;min-width:0}#cmdTarget{font-size:15px;border-radius:6px;border:1px solid var(--line3);background:var(--control);color:var(--controlInk);padding:8px;flex:0 0 auto}button,input,select{font-size:15px;border-radius:6px;border:1px solid var(--line3);background:var(--control);color:var(--controlInk);padding:8px}button{cursor:pointer}button:hover{background:var(--controlHover)}input{flex:0 1 180px;min-width:120px;max-width:220px}.formRow{display:flex;gap:8px;align-items:center;margin-top:8px}.formRow label{width:42px;color:var(--ink3);font-size:13px;font-weight:700;text-align:right}.inputWithAction{position:relative;display:flex;gap:6px;align-items:center;flex:1}.inputWithAction input{flex:1;min-width:0;max-width:none}.iconButton{min-width:0;width:26px;height:26px;padding:0;border-radius:6px;display:inline-flex;align-items:center;justify-content:center}.scanPopover{display:none;position:absolute;left:0;right:0;top:42px;max-height:210px;overflow:auto;background:var(--card);border:1px solid var(--popBorder);border-radius:10px;padding:8px;z-index:12}.scanPopover.show{display:block}.scanRow{display:flex;justify-content:space-between;gap:8px;width:100%;text-align:left;margin-top:4px}.scanMeta{color:var(--ink3);font-size:12px;white-space:nowrap}.modal{position:fixed;inset:0;display:none;align-items:center;justify-content:center;background:var(--scrim);z-index:10}.modal.show{display:flex}.toast{position:fixed;right:18px;bottom:18px;background:var(--card);border:1px solid var(--ok);border-radius:12px;padding:12px 14px;box-shadow:var(--toastShadow);color:var(--ink);opacity:0;transform:translateY(12px);transition:.25s;pointer-events:none;z-index:20}.toast.show{opacity:1;transform:translateY(0)}.helpFab{position:fixed;right:18px;bottom:18px;width:46px;height:46px;min-width:0;padding:0;border-radius:50%;background:var(--fabActionBg);color:var(--onAccent);border-color:var(--fabActionBorder);font-size:24px;font-weight:900;line-height:1;z-index:17;box-shadow:var(--fabShadow);backdrop-filter:blur(4px)}.helpFab:hover,.helpFab:focus-visible{background:var(--accentHi);border-color:var(--accentHi);box-shadow:var(--fabShadowHover)}.helpOverlay{position:fixed;inset:0;display:none;background:var(--scrim2);z-index:18}.helpOverlay.show{display:block}.helpModal{position:fixed;right:18px;bottom:74px;width:min(340px,calc(100vw - 36px));max-height:calc(100vh - 100px);overflow-y:auto;display:none;background:var(--cardGrad);border:1px solid var(--accent);border-radius:14px;padding:14px;box-shadow:var(--popShadow);color:var(--popInk);z-index:19}.helpModal.show{display:block}.helpHead{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:8px}.helpHead h2{margin:0;font-size:16px;font-weight:700;color:var(--ink)}.helpClose{min-width:0;width:28px;height:28px;padding:0;border:none;border-radius:50%;background:transparent;color:var(--closeInk);font-size:20px;line-height:1}.helpClose:hover{background:var(--closeHoverBg);color:var(--closeHoverInk)}.helpSection{margin-bottom:16px}.helpSection:last-child{margin-bottom:0}.helpSection h3{margin:0 0 8px;font-size:12px;font-weight:500;text-transform:uppercase;letter-spacing:.05em;color:var(--ink3)}.helpList{margin:0;padding-left:18px;color:var(--popInk);font-size:13px;line-height:1.55}.helpList li{margin:0 0 8px}.dialog{width:min(420px,calc(100vw - 28px));background:var(--cardGrad);border:1px solid var(--warn);border-radius:14px;padding:18px;box-shadow:var(--popShadow)}.dialog h2{margin:0 0 8px;font-size:20px}.dialog p{color:var(--ink2);font-size:14px;line-height:1.5}.dialogActions{display:flex;gap:8px;justify-content:flex-end;margin-top:14px}.chartFooter{display:flex;gap:10px;align-items:center;justify-content:space-between;flex-wrap:wrap;margin-top:8px}.chartToolbar{display:flex;gap:6px;align-items:center}.chartTools{display:flex;gap:6px;flex-wrap:wrap;justify-content:flex-end;margin-top:8px}#tubRecordBtn.recording{color:var(--tubRec)}#serialPanel{display:flex;flex-direction:column;gap:8px;padding-bottom:6px}.log{height:calc(5 * 1.35em + 16px);overflow:auto;background:var(--logBg);color:var(--logInk);font:13px/1.35 Consolas,monospace;padding:8px;border-radius:6px;white-space:pre-wrap}#serialPanel .log{flex:1 1 auto;min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px)}.muted{color:var(--ink3);font-size:12px}canvas{width:100%;height:auto;aspect-ratio:38/13;background:var(--canvasBg);border-radius:6px;border:1px solid var(--line)}.legend{display:flex;gap:18px;align-items:flex-start;flex-wrap:wrap}.legend span{display:inline-block;font-size:12px}.legend b{display:block;color:inherit;font:700 13px Consolas,monospace;margin-top:2px}.recMeta{display:inline-flex;align-items:center;gap:6px;font-size:12px;color:var(--ink3);margin-left:4px;line-height:1.1}.recMeta b{color:var(--ink);font:700 13px Consolas,monospace}#chartPanel:fullscreen{background:var(--bg);padding:12px;display:flex;flex-direction:column}#chartPanel:fullscreen .chartCanvasWrap{width:min(100%,calc((100vh - 118px) * 38 / 13))}#chartPanel:fullscreen canvas{width:100%;height:auto;max-height:calc(100vh - 118px);aspect-ratio:38/13}.c1{color:var(--ok)}.c2{color:var(--accent)}.c3{color:var(--warn)}.c4{color:var(--bad)}.c5{color:var(--drift)}.c6{color:var(--c6)}.c7{color:var(--c7)}.c8{color:var(--c8)}@keyframes pulse{50%{box-shadow:0 0 18px var(--badGlow);transform:translateY(-1px)}}@keyframes scan{from{transform:translateX(-100%)}to{transform:translateX(100%)}}@media(max-width:900px){.statusTable{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:860px){.stateGrid{grid-template-columns:minmax(96px,.30fr) minmax(160px,.56fr) minmax(260px,1.30fr);grid-template-areas:"mode park drift" "voltage network network"}}@media(max-width:620px){.stateGrid{grid-template-columns:84px 154px 100px;grid-template-areas:"mode park voltage" "drift drift drift" "network network network"}#modeCard,#parkCard,#voltageCard{padding:10px 8px}#modeCard .stateValue,#parkCard .stateValue,#voltageCard .stateValue,#driftCard .stateValue,#networkCard .stateValue{font-size:18px}#modeCard .stateSub,#parkCard .stateSub,#driftCard .stateSub{font-size:11px}#voltageCard .stateMeta span,#networkCard .stateMeta span{font-size:13px}#modeCard .stateHead,#parkCard .stateHead,#voltageCard .stateHead,#driftCard .stateHead,#networkCard .stateHead{font-size:11px}.rcGrid{grid-template-columns:repeat(3,minmax(72px,1fr))}}@media(max-width:560px){.statusTable{grid-template-columns:1fr}}@media(min-width:900px){.grid{grid-template-columns:minmax(0,2fr) minmax(0,1fr)}.wide{grid-column:1/-1}#diagnosticsPanel{grid-column:1/-1}}.chartCanvasWrap{position:relative}#chartFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}#termFullscreenBtn{position:absolute;right:8px;bottom:8px;z-index:2}#terminalWrap{display:none;flex:1 1 auto;flex-direction:column;height:calc(5 * 1.35em + 16px);min-height:calc(5 * 1.35em + 16px);max-height:calc(20 * 1.35em + 16px);padding:8px;font-size:13px;overflow:hidden;background:var(--termBg);border:1px solid var(--line);border-radius:6px;position:relative}#terminalWrap:fullscreen{background:var(--termBg);height:auto;min-height:0;max-height:none;padding:0;border-radius:0}.termFrame{display:block;flex:1 1 auto;width:100%;min-height:0;border:0;border-radius:6px;background:var(--termBg)}#termTabs{display:none;align-items:center;gap:4px;overflow-x:auto;flex:1 1 auto;min-width:0;scrollbar-width:none}#termTabs::-webkit-scrollbar{display:none}.termTab{flex:0 0 auto;display:inline-flex;align-items:center;gap:6px;padding:2px 8px;height:22px;border:1px solid var(--termTabLine);border-radius:6px;background:var(--termTabBg);color:var(--inkT);font-size:12px;line-height:1;cursor:pointer;white-space:nowrap}.termTab.active{color:var(--tabActiveInk);border-color:var(--tabActiveInk);background:var(--tabActiveBg)}.termTabClose{display:inline-flex;align-items:center;justify-content:center;width:14px;height:14px;border-radius:4px;font-size:13px;font-weight:700;line-height:1;color:inherit;opacity:.55;cursor:pointer}.termTabClose:hover{opacity:1;color:var(--bad2);background:var(--badSoft)}#newTermBtn{width:22px;height:22px;flex:0 0 auto}.terminalHint{color:var(--inkT);font-size:12px;padding:6px 2px;word-break:break-all}.terminalHint:empty{display:none}@media (max-width:820px){.headerRow{align-items:center;gap:8px}.rowBreak{display:block;flex-basis:100%;height:0}.headerLogo{order:1}.headerRow h1{order:2}.ghLink{order:3}#versionLabel{order:4}.br1{order:5}.navLinks{order:6;flex-basis:100%;overflow-x:auto;scrollbar-width:none}.navLinks::-webkit-scrollbar{display:none}.navLinks>*{flex:0 0 auto}.br2{order:11}.headerRow .otaLink{order:13}#muteToggle{order:14;margin-left:0}#devModeToggle{order:15;margin-left:auto}.br3{order:16}#themeToggle{order:17}#langToggle{order:18;margin-left:auto}}
</style>
<style id="dd-embed-native">/* DonkeyDrifter 原生风格——仅 embedded 作用域（CC 设置视图），独立页不动 */body.embedded [data-i18n="panel.rcChannels"],body.embedded [data-i18n="drift.steering.label"],body.embedded [data-i18n="drift.throttle.label"],body.embedded [data-i18n="judge.section.thresholds"],body.embedded [data-i18n="judge.section.scoring"],body.embedded [data-i18n="judge.dimLabel"]{font-weight:600!important;letter-spacing:-0.02em!important;font-size:15px!important;color:#e4e7eb!important}html[data-theme="light"] body.embedded [data-i18n="panel.rcChannels"],html[data-theme="light"] body.embedded [data-i18n="drift.steering.label"],html[data-theme="light"] body.embedded [data-i18n="drift.throttle.label"],html[data-theme="light"] body.embedded [data-i18n="judge.section.thresholds"],html[data-theme="light"] body.embedded [data-i18n="judge.section.scoring"],html[data-theme="light"] body.embedded [data-i18n="judge.dimLabel"]{color:#1a2330!important}body.embedded [data-i18n="panel.rcChannels"]::before,body.embedded [data-i18n="drift.steering.label"]::before,body.embedded [data-i18n="drift.throttle.label"]::before,body.embedded [data-i18n="judge.section.thresholds"]::before,body.embedded [data-i18n="judge.section.scoring"]::before,body.embedded [data-i18n="judge.dimLabel"]::before{content:"";display:inline-block;width:18px;height:18px;margin-right:8px;vertical-align:-3px;flex:0 0 auto;background:currentColor;-webkit-mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat}body.embedded .panel,body.embedded .rcCell,body.embedded .summaryItem,body.embedded .field,body.embedded .card{border-radius:8px}html[data-theme="light"] body.embedded .rcCell,html[data-theme="light"] body.embedded .summaryItem,html[data-theme="light"] body.embedded .field,html[data-theme="light"] body.embedded .card{background:#fff!important;border-color:#ccd5df!important}html[data-theme="light"] body.embedded input[type=number],html[data-theme="light"] body.embedded input[type=text],html[data-theme="light"] body.embedded select{background:#fff;border-color:#ccd5df}body.embedded input[type=number],body.embedded input[type=text],body.embedded select{border-radius:6px}</style>
<style>
.fabToggle{position:fixed;right:24px;bottom:24px;width:18px;height:18px;min-width:0;padding:0;border-radius:50%;background:var(--fabBg);border-color:var(--fabBg);z-index:17;box-shadow:var(--fabGlow)}.fabToggle:hover,.fabToggle:focus-visible,.fabToggle:active{background:var(--fabBgHover);border-color:var(--fabBgHover);transform:scale(1.18);box-shadow:var(--fabGlowHover)}.fabActions{position:fixed;right:18px;bottom:18px;z-index:17;pointer-events:none}.fabActions .helpFab{position:absolute;right:0;bottom:0;opacity:0;transform:scale(.55);pointer-events:none;transition:opacity .18s,transform .18s}.fabActions.show .helpFab{opacity:1;transform:translateX(-56px) scale(1);pointer-events:auto}.fabActions .helpFab{width:46px;height:46px;min-width:0;padding:0;border-radius:50%;background:var(--fabActionBg);color:var(--onAccent);border-color:var(--fabActionBorder);font-size:24px;font-weight:900;line-height:1;box-shadow:var(--fabShadow);backdrop-filter:blur(4px)}.fabActions .helpFab:hover,.fabActions .helpFab:focus-visible{background:var(--accentHi);border-color:var(--accentHi);box-shadow:var(--fabShadowHover)}.reconnectOverlay{position:fixed;inset:0;background:var(--overlay);display:none;align-items:center;justify-content:center;z-index:100}.reconnectOverlay.show{display:flex}.reconnectBox{text-align:center;color:var(--ink)}.reconnectSpinner{width:48px;height:48px;border:4px solid var(--line);border-top-color:var(--accent);border-radius:50%;animation:spin 1s linear infinite;margin:0 auto 16px}.reconnectText{font-size:16px;line-height:1.6}@keyframes spin{to{transform:rotate(360deg)}}.staCols{display:grid;grid-template-columns:1.2fr 1fr;gap:14px;align-items:start}#wifiStaModal .dialog{width:min(640px,calc(100vw - 28px))}@media(max-width:640px){.staCols{grid-template-columns:1fr}}.histRow{display:flex;align-items:center;gap:8px;width:100%;text-align:left;margin-top:4px;padding:6px 8px;background:var(--card);border:1px solid var(--line);border-radius:6px;color:var(--ink);cursor:pointer}.histRank{flex:none;display:inline-flex;align-items:center;justify-content:center;min-width:24px;height:18px;padding:0 5px;border-radius:999px;background:var(--accentFill);color:var(--onAccent);font-size:11px;font-weight:800}.histSsid{flex:1;min-width:0;overflow:hidden;white-space:nowrap;font-size:13px}.histDel{flex:none;min-width:0;width:26px;height:26px;padding:0;border-radius:6px;display:inline-flex;align-items:center;justify-content:center;background:transparent;border-color:var(--histDelLine);color:var(--ink3);font-size:13px}.histDel:hover{color:var(--bad);border-color:var(--bad)}
</style>
<style>
html[data-theme="light"] .scanPopover{color:var(--popInk);box-shadow:var(--scanShadow)}
</style>
<style>
.settingsView{display:none}
body.settings .settingsView{display:block}
body.settings .grid,body.wifi .grid{display:block}
body.settings .grid>section.panel,body.wifi .grid>section.panel{display:none}
body.settings #serialPanel,body.wifi #serialPanel{display:none}
body.settings #diagnosticsPanel,body.wifi #diagnosticsPanel{display:block}
body.settings #diagnosticsPanel>div,body.wifi #diagnosticsPanel>div{display:none}
body.settings #diagnosticsPanel #rcFold,body.wifi #diagnosticsPanel #rcFold{display:block}body.settings #rcFold .foldHead,body.wifi #rcFold .foldHead{background:transparent;border:none;cursor:default;pointer-events:none}body.settings #rcFold .foldHead:hover,body.wifi #rcFold .foldHead:hover{background:transparent}body.settings #rcFold .foldIcon,body.wifi #rcFold .foldIcon{display:none}
body.settings .headerRow{display:none}
body.settings .fabToggle{display:none}
body.settings .fabActions{display:none}
.settingsView .setTitle{margin:0 0 10px;font-size:1.1rem;font-weight:700;color:var(--ink)}
.settingsView .setRow{display:flex;align-items:center;gap:12px;flex-wrap:wrap;padding:12px;background:var(--setRowBg);border:1px solid var(--line);border-radius:10px;margin-bottom:10px}
.settingsView .setRow h3{margin:0;font-size:12px;font-weight:700;color:var(--ink3);min-width:72px;text-transform:uppercase;letter-spacing:.05em}
.settingsView .setActions{display:flex;align-items:center;gap:8px;flex-wrap:wrap}



/* DD 内嵌主视图（?embedded=1 且无 settings/wifi 参数）：设置类板块/入口已移至 DD Car Connector 的车辆设置，
   此视图只留显示与驾驶——RC Channels 校准面板、手柄校准/漂移设置按钮行、Network 卡 ⚙ 配网入口、Drift 卡 Tune 链接全部隐藏；
   车端独立 DC 页面（无参数）与 DD Car Connector 的 settings/wifi 内嵌视图均不受影响 */
body.embedded:not(.settings):not(.wifi) #rcFold{display:none}
body.embedded:not(.settings):not(.wifi) #diagSettingsRow{display:none}
body.embedded:not(.settings):not(.wifi) #networkGear{display:none}
body.embedded:not(.settings):not(.wifi) #driftTuneLink{display:none}
/* DD 内嵌设置视图（embedded+settings 同时成立，即 CC 车辆设置）：漂移/Judge 设置以子 iframe 默认展开、
   不设分类标题、前后相接融合为一页（逻辑顺序：先车辆动态参数=漂移，后评判规则=Judge；Judge 页内嵌时
   隐藏得分 hero/gyroZ 曲线等显示类区域并撑满宽度，见其页内 body.embedded 规则）；
   「车辆设置」标题与「调校」行（跳转按钮 + 手柄校准按钮）整行隐藏——手柄校准已移至 DD CC 页顶栏，
   经 postMessage(dd-open-joystick-cal) 让本页打开校准弹窗；车端独立 DC 的设置视图（无 embedded）保持原样。
   子 iframe 的 src 经 data-src 懒加载（仅本作用域由 initEmbedTuneFrames() 赋值，其余视图不加载/不跑子页轮询），
   加载后按内容高度自动撑高（ResizeObserver 跟踪），不出现内部滚动条；下方 820px/1000px 仅为加载前占位高度 */
.embedTuneSections{display:none}
body.embedded.settings .embedTuneSections{display:block}
.embedTuneFrame{display:block;width:100%;border:0;border-radius:10px;background:var(--frameBg);overflow:hidden}
.embedTuneFrame.driftFrame{height:820px}
.embedTuneFrame.judgeFrame{height:1000px}
body.embedded.settings #settingsView .setTitle{display:none}
body.embedded.settings #settingsView .setRow{display:none}
/* ?wifi=1：把 STA/AP 配网弹窗作为静态板块直接呈现（1:1 复用车端表单），供 DD Car Connector 内嵌 */
body.wifi .headerRow{display:none}
body.wifi .fabToggle{display:none}
body.wifi .fabActions{display:none}
body.wifi #wifiApModal,body.wifi #wifiStaModal{position:static;display:block;background:transparent;backdrop-filter:none;align-items:stretch;justify-content:flex-start}
body.wifi #wifiApModal .dialog,body.wifi #wifiStaModal .dialog{position:relative;width:100%;max-width:640px;margin:0 0 14px;transform:none;opacity:1}
body.wifi #wifiApModal .dialogActions button:first-child,body.wifi #wifiStaModal .dialogActions button:first-child{display:none}
/* AP/STA 两个配网板块在 wifi 内嵌视图融合为单卡片：上卡（AP）去底边/底圆角/底间距/阴影，下卡（STA）去顶圆角接上 */
body.wifi #wifiApModal .dialog{margin:0;border-bottom:none;border-radius:14px 14px 0 0;padding-bottom:8px;box-shadow:none}
body.wifi #wifiStaModal .dialog{margin:0 0 14px;border-top:none;border-radius:0 0 14px 14px;padding-top:4px}
/* 标题悬停灰字提示（参考 DD group-hover 与 /drift 页样式）：手柄校准弹窗与 RC Channels 校准面板标题通用 */




</style>
<style id="apple-deep">
/* ===== Apple 深化（自 v1.10.0 起为唯一界面风格；原座舱象限覆写已并入基值） ===== */


html:root,html:root *{-webkit-tap-highlight-color:transparent}
html:root body{font-family:var(--appleFont);-webkit-font-smoothing:antialiased;padding-bottom:env(safe-area-inset-bottom)}
html:root button,html:root input,html:root select,html:root textarea{font-family:inherit}
html:root .headerRow,html:root .langButton{font-family:var(--appleFont)}
html:root .log,html:root .statusRow span,html:root .rcCell span,html:root .rcNum,html:root .legend b,html:root .recMeta b,html:root .termTab{font-family:var(--appleMono)}
/* 聚焦环：不透明 accent 3px，覆盖 UA 默认与 .rcNum:focus{outline:none} */
html:root :focus-visible{outline:3px solid var(--accent);outline-offset:2px}
html:root .rcNum:focus{outline:3px solid var(--accent);outline-offset:2px;background:var(--card3)}
/* 排版 */
html:root h1{font-size:20px;font-weight:600;letter-spacing:-.02em;line-height:1.4}
html:root .stateHead,html:root .version,html:root .helpSection h3,html:root .toggleLabel,html:root .settingsView .setRow h3{text-transform:none;letter-spacing:0;font-weight:600;font-size:13px;line-height:1.35}

html:root .stateSub,
html:root .navTabWeak{color:var(--ink3)}
html:root .stateSub,html:root .muted,html:root .recMeta,html:root .legend span,html:root .rcCell b,html:root .netTabs button{font-size:13px}
html:root .log{font-size:13px;line-height:1.45}
/* 语义色双轨：文字走 --*-text，填充（点/条/描边/canvas）保持原 token */
html:root .c1{color:var(--ok-text)}
html:root .c3{color:var(--warn-text)}
html:root .c4{color:var(--bad-text)}
html:root .c5{color:var(--drift-text)}
/* 材质与层级 */
html:root .panel{background:var(--card);border:1px solid var(--cardLine);border-radius:16px;padding:14px}

html:root .mode0,html:root .mode1,html:root .mode2,html:root .parkUnlocked,html:root .driftOff{border-color:var(--cardLine)}
html:root .driftArmed{border-color:var(--warn)}
html:root .driftActive{border-color:var(--drift);animation-duration:2s}
html:root .driftActive:before{animation-duration:2.8s}
html:root .parkLocked{border-color:var(--bad);animation-duration:2.4s}
html:root .netDown{border-color:var(--bad)}
html:root .modal{background:var(--scrim);backdrop-filter:saturate(180%) blur(20px);-webkit-backdrop-filter:saturate(180%) blur(20px)}
html:root .dialog{background:var(--matSolid);backdrop-filter:saturate(180%) blur(20px);-webkit-backdrop-filter:saturate(180%) blur(20px);box-shadow:var(--elev)}
html:root .helpModal{background:var(--mat);backdrop-filter:saturate(180%) blur(20px);-webkit-backdrop-filter:saturate(180%) blur(20px);box-shadow:var(--elev)}
html:root .reconnectOverlay{background:var(--matSolid);backdrop-filter:saturate(180%) blur(20px);-webkit-backdrop-filter:saturate(180%) blur(20px)}
html:root .reconnectActions{margin-top:16px}
html:root .reconnectActions button{min-height:44px;border-radius:22px;padding:0 22px}
/* 发丝线（列表分隔走 separator 档）/ 圆角收敛 8·12·16·22·9999 */
html:root .settingsView .setRow{border-color:var(--sep);border-radius:16px}
html:root .rcCell{border-color:var(--cardLine);border-radius:12px}
html:root .rcCell.modeCh{border-color:var(--warn)}
html:root .foldHead{border-color:var(--cardLine)}
html:root .termTab{border-color:var(--cardLine);border-radius:8px}
html:root .termTabClose{border-radius:8px}
html:root canvas{border-color:var(--cardLine);border-radius:12px}
html:root .log{border-radius:12px}
html:root .rcNum,html:root .iconButton,html:root #cmdTarget{border-radius:8px}
html:root .dialog,html:root .helpModal,html:root .toast{border-radius:22px}
html:root .embedTuneFrame{border-radius:16px}
/* toast：底部居中毛玻璃胶囊，上移避开 FAB（原与 .helpFab 完全重叠） */
html:root .toast{left:0;right:0;inset-inline:18px;margin-inline:auto;width:fit-content;max-width:none;bottom:calc(90px + env(safe-area-inset-bottom));min-height:44px;display:inline-flex;align-items:center;justify-content:center;padding:10px 20px;background:var(--mat);backdrop-filter:saturate(180%) blur(20px);-webkit-backdrop-filter:saturate(180%) blur(20px);box-shadow:var(--elev);color:var(--ink);transform:translateY(12px);transition:opacity .28s var(--ease-apple),transform .28s var(--ease-apple)}
html:root .toast.show{transform:translateY(0)}
/* 禁用态（console 原缺） */
html:root button:disabled,html:root input:disabled,html:root select:disabled{opacity:1;background:var(--control);color:var(--ink3);border-color:var(--cardLine);cursor:not-allowed;box-shadow:none}
html:root button.alt:disabled{background:transparent}
/* #fabToggle：原 18×18 空白 → 44×44 + 可见图标（mask 图标，非 emoji） */
html:root .fabToggle{width:44px;height:44px;bottom:calc(24px + env(safe-area-inset-bottom))}
html:root .fabToggle::before{content:"";position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:20px;height:20px;background:currentColor;-webkit-mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cpath%20d%3D%22M4%2014a1%201%200%200%201-.78-1.63l9.9-10.2a.5.5%200%200%201%20.86.46l-1.92%206.02A1%201%200%200%200%2013%2010h7a1%201%200%200%201%20.78%201.63l-9.9%2010.2a.5.5%200%200%201-.86-.46l1.92-6.02A1%201%200%200%200%2011%2014z%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cpath%20d%3D%22M4%2014a1%201%200%200%201-.78-1.63l9.9-10.2a.5.5%200%200%201%20.86.46l-1.92%206.02A1%201%200%200%200%2013%2010h7a1%201%200%200%201%20.78%201.63l-9.9%2010.2a.5.5%200%200%201-.86-.46l1.92-6.02A1%201%200%200%200%2011%2014z%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat;pointer-events:none}
html:root .fabActions{bottom:calc(18px + env(safe-area-inset-bottom));right:18px}
html:root .fabActions .helpFab{bottom:0;right:0}
/* 命中区 ≥44×44：::after 不可见命中区，视觉尺寸不变 */
html:root .themeButton,html:root .langButton,html:root .muteButton,html:root .navTabWeak,html:root .navTab,html:root .ghLink,html:root .logoLink,html:root .titleLink,html:root .otaLink,html:root #devModeToggle,html:root .iconButton,html:root .rcSetBtn,html:root .netTabs button,html:root #driftTuneLink,html:root .foldHead,html:root .copyValue,html:root .termTab,html:root .termTabClose,html:root .helpFab,html:root .histRow{position:relative}
html:root .themeButton::after,html:root .langButton::after,html:root .muteButton::after,html:root .navTabWeak::after,html:root .navTab::after,html:root .ghLink::after,html:root .logoLink::after,html:root .titleLink::after,html:root .otaLink::after,html:root #devModeToggle::after,html:root .iconButton::after,html:root .rcSetBtn::after,html:root .netTabs button::after,html:root #driftTuneLink::after,html:root .foldHead::after,html:root .copyValue::after,html:root .gear::after{content:"";position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:max(100%,44px);height:max(100%,44px);border-radius:inherit}
/* 卡片 overflow:hidden 裁掉命中区顶部：整体下移补回 44px */
html:root .netTabs button::after{top:calc(50% + 3px)}
html:root #driftTuneLink::after{top:calc(50% + 7px)}
/* IP 复制区：改为左对齐半宽命中区（44 高），避开右上角 AP/STA/HOST 与 ⚙ 的命中带 */
html:root .copyValue::after{top:calc(50% + 3px)}
/* 相邻目标不重叠：行内 pitch ≥44（可见间距 ≥8px） */
html:root .chartToolbar{gap:18px}
html:root .row{gap:18px}
html:root .netTabs{gap:12px;z-index:3}
html:root .copyValue{z-index:1}
/* 表单控件 44px */
html:root input,html:root select,html:root textarea{min-height:44px;box-sizing:border-box}
html:root .rcNum{min-height:44px;min-width:44px}
html:root input[type=range]{height:44px}
html:root .toggleSwitch{min-height:44px;min-width:44px}
html:root .dialogActions button{min-height:44px}
html:root .histRow{min-height:44px}
/* 移动端页头：56px 标题行 + 44px 横向可滚弱入口行，两行合计 100px */
@media (max-width:820px){
html:root .headerRow{align-items:center;gap:12px;row-gap:0;margin-bottom:8px}
html:root .headerRow .rowBreak{display:none}
html:root .headerRow .br3{display:block;flex-basis:100%;height:0;order:8;margin:0}
html:root .headerLogo{order:1}
html:root .headerRow h1{order:2;display:flex;align-items:center;min-height:56px;min-width:0;flex:1 1 auto;white-space:nowrap;overflow:hidden;text-overflow:clip;margin:0}
html:root #muteToggle{order:3;margin-left:0}
html:root #themeToggle{order:4}
html:root #langToggle{order:5;margin-left:0}
html:root #versionLabel{order:6}
html:root .ghLink{order:7;margin-left:0}
html:root .headerRow .otaLink{order:10}
html:root #devModeToggle{order:11;margin-left:0}
html:root .navLinks{order:12;flex:1 1 0;min-width:96px;height:44px;align-items:center;flex-wrap:nowrap;overflow-x:auto;overflow-y:hidden;scrollbar-width:none;-ms-overflow-style:none;-webkit-mask-image:linear-gradient(90deg,#000 88%,transparent);mask-image:linear-gradient(90deg,#000 88%,transparent)}
html:root .navLinks::-webkit-scrollbar{display:none}
html:root .navLinks>*{flex:0 0 auto}
html:root .navTab,html:root .navTabWeak{min-height:44px;display:inline-flex;align-items:center;margin-right:16px}
}
@media (max-width:560px){
html:root .headerLogo,html:root .logoLink{display:none}
}
/* 降级 */
@media (prefers-reduced-motion: reduce){
html:root *,html:root *::before,html:root *::after{animation-duration:.001ms !important;animation-iteration-count:1 !important;transition-duration:.001ms !important;scroll-behavior:auto !important}
html:root .reconnectSpinner{animation-duration:1.2s !important;animation-iteration-count:infinite !important}
}
@media (prefers-reduced-transparency: reduce){
html:root .toast,html:root .helpModal,html:root .dialog,html:root .modal,html:root .reconnectOverlay{background:var(--matSolid);backdrop-filter:none;-webkit-backdrop-filter:none}
}
@media (prefers-contrast: more){
:root{--ink2:#f5f5f7;--ink3:#f5f5f7;--ink4:#f5f5f7;--inkPill:#f5f5f7;--inkT:#f5f5f7;--segInk:#f5f5f7;--cardLine:rgba(255,255,255,.34);--sep:rgba(255,255,255,.34)}
html:root[data-theme="light"]{--ink2:#1d1d1f;--ink3:#1d1d1f;--ink4:#1d1d1f;--inkPill:#1d1d1f;--inkT:#1d1d1f;--segInk:#1d1d1f;--cardLine:rgba(60,60,67,.55);--sep:rgba(60,60,67,.55)}
html:root .toast,html:root .helpModal,html:root .dialog,html:root .modal,html:root .reconnectOverlay{background:var(--matSolid);backdrop-filter:none;-webkit-backdrop-filter:none}
}
@media (forced-colors: active){
html:root .stateCard,html:root .rcCell,html:root .panel,html:root .foldHead,html:root .log,html:root canvas{border:1px solid CanvasText}
html:root .stateDot{forced-color-adjust:none;background:CanvasText;border:1px solid Canvas}
html:root .driftBar{forced-color-adjust:none;background:Canvas;border:1px solid CanvasText}
}
/* 设置行的动作按钮（漂移设置 / Judge 设置 / 手柄校准）：原高 39px（<44），
   触屏上偏小；apple 象限提到 44（iOS 表单动作按钮的最小高度） */
html:root .setActions button {
  min-height: 44px;
}
</style>
</head>
<body>
<script>try{var _q=location.search,_b=document.body;_b.classList.add('preinit');if(_q.indexOf('embedded=1')>=0)_b.classList.add('embedded');if(_q.indexOf('settings=1')>=0)_b.classList.add('settings');if(_q.indexOf('wifi=1')>=0)_b.classList.add('wifi');setTimeout(function(){_b.classList.remove('preinit')},2000)}catch(e){try{document.body.classList.remove('preinit')}catch(_e){}}</script>
<div class="headerRow"><a class="logoLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener"><img class="headerLogo" src="/favicon.png" alt="Drifter Console"></a><h1><a class="titleLink" href="https://www.donkeydrift.com" target="_blank" rel="noopener" data-i18n="app.title">Drifter Console</a></h1><span class="navLinks"><a class="navTab" data-i18n="button.enterDonkey" id="enterDonkeyBtn" href="http://192.168.3.41:8090/" target="_blank" rel="noopener">Donkey</a><a class="navTab" data-i18n="button.enterDonkeyDrifter" id="enterDonkeyDrifterBtn" href="http://192.168.3.41:8090/launch/drive" target="_blank" rel="noopener">DonkeyDrifter</a><button type="button" class="navTabWeak" id="openKimiCodeWebBtn" onclick="openKimiCodeWeb()"><svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M9.937 15.5A2 2 0 0 0 8.5 14.063l-6.135-1.582a.5.5 0 0 1 0-.962L8.5 9.936A2 2 0 0 0 9.937 8.5l1.582-6.135a.5.5 0 0 1 .963 0L14.063 8.5A2 2 0 0 0 15.5 9.937l6.135 1.581a.5.5 0 0 1 0 .964L15.5 14.063a2 2 0 0 0-1.437 1.437l-1.582 6.135a.5.5 0 0 1-.963 0z"></path><path d="M20 3v4"></path><path d="M22 5h-4"></path><path d="M4 17v2"></path><path d="M5 18H3"></path></svg><span data-i18n="button.openKimiCodeWeb">Kimi Code Web</span></button><button type="button" class="navTabWeak" id="openZCodeBtn" onclick="openZCode()" ondblclick="editZCodeUrl()" title="单击打开 ZCode 远程控制（自动复制链接），双击更新链接" data-i18n-title="zcode.remoteHint"><svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m18 16 4-4-4-4"></path><path d="m6 8-4 4 4 4"></path><path d="m14.5 4-5 16"></path></svg><span data-i18n="button.openZCode">ZCode</span></button><button type="button" class="navTabWeak" id="openDshBtn" onclick="openDsh()"><svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M14 2v6a2 2 0 0 0 .245.96l5.51 10.08A2 2 0 0 1 18 22H6a2 2 0 0 1-1.755-2.96l5.51-10.08A2 2 0 0 0 10 8V2"></path><path d="M6.453 15h11.094"></path><path d="M8.5 2h7"></path></svg><span data-i18n="button.openDsh">DeepSeek Harness</span></button></span><a class="ghLink" href="https://github.com/DonkeyDrift/Firmware" target="_blank" rel="noopener" title="DonkeyDrift/Firmware on GitHub" aria-label="GitHub: DonkeyDrift/Firmware"><svg viewBox="0 0 16 16" width="20" height="20" fill="currentColor" aria-hidden="true"><path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27s1.36.09 2 .27c1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0 0 16 8c0-4.42-3.58-8-8-8Z"/></svg></a><span class="version" id="versionLabel">--</span><span class="rowBreak br1"></span><button type="button" id="muteToggle" class="muteButton" onclick="toggleMute()" aria-label="静音" data-i18n-aria="mute.title"><svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M11 5 6 9H2v6h4l5 4V5z"/><g class="icoSound"><path d="M15.5 8.5a5 5 0 0 1 0 7"/><path d="M18.6 5.4a9 9 0 0 1 0 13.2"/></g><g class="icoMute"><line x1="16" y1="9" x2="22" y2="15"/><line x1="22" y1="9" x2="16" y2="15"/></g></svg></button><button type="button" id="themeToggle" class="themeButton" onclick="toggleTheme()" aria-label="主题" data-i18n-aria="theme.title"><svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><g class="icoMoon"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></g><g class="icoSun"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></g></svg></button><button type="button" id="langToggle" class="langButton" onclick="toggleLanguage()" aria-label="语言" data-i18n-aria="language.title">中</button><span class="rowBreak br2"></span><a href="/update" class="otaLink" data-i18n="button.ota">OTA</a><button type="button" id="devModeToggle" onclick="toggleDevModeFromSwitch()" role="switch" aria-checked="false">DEV</button><span class="rowBreak br3"></span></div>
<div id="settingsView" class="settingsView">
<h2 class="setTitle" data-i18n="settings.title">车辆设置</h2>
<div class="setRow"><h3 data-i18n="settings.tuning">调校</h3><div class="setActions"><button type="button" id="driftSettingsBtn" onclick="location.href='/drift?theme='+resolvedTheme()" data-i18n="button.driftSettings">漂移设置</button><button type="button" id="judgeSettingsBtn" onclick="location.href='/judge?theme='+resolvedTheme()" data-i18n="settings.judge">Judge 设置</button><button type="button" onclick="openJoystickCalModal()" data-i18n="button.joystickCal">手柄校准</button></div></div>
<div class="embedTuneSections"><iframe class="embedTuneFrame driftFrame" data-src="/drift?embedded=1" title="漂移设置" data-i18n-title="embed.driftTune"></iframe><iframe class="embedTuneFrame judgeFrame" data-src="/judge?embedded=1" title="Judge 设置" data-i18n-title="embed.judgeTune"></iframe></div>
</div>
<div class="grid">
<section class="panel wide">
<div class="stateGrid">
<div id="modeCard" class="stateCard"><div class="stateHead" data-i18n="state.mode">Mode</div><div class="stateValue" id="modeValue">--</div><div class="stateSub" id="modeSub" data-i18n="state.waiting">waiting</div><span class="stateDot"></span></div>
<div id="parkCard" class="stateCard"><div class="stateHead" data-i18n="state.park">Park</div><div class="stateValue" id="parkValue">--</div><div class="stateSub" id="parkSub" data-i18n="state.waiting">waiting</div><span class="stateDot"></span></div>
<div id="driftCard" class="stateCard"><span class="tunePair"><a href="/drift" id="driftTuneLink" data-i18n="state.tune">Tune</a><span class="stateDot"></span></span><div class="stateHead" data-i18n="state.drift">Drift</div><div class="stateValue" id="driftValue">--</div><div class="stateSub" id="driftSub" data-i18n="state.waiting">waiting</div><div class="driftBar"><i id="driftNeedle"></i></div></div>
<div id="voltageCard" class="stateCard"><div class="stateHead" data-i18n="state.voltage">Voltage</div><div class="stateValue" id="voltageValue">--</div><div class="stateMeta"><b data-i18n="state.remain">REMAIN</b><span id="voltageSub">--</span></div><span class="stateDot"></span></div>
<div id="networkCard" class="stateCard"><div class="netTabs"><button id="networkApTab" type="button" onclick="setNetworkTab('ap')">AP</button><button id="networkStaTab" type="button" onclick="setNetworkTab('sta')">STA</button><button id="networkHostTab" type="button" onclick="setNetworkTab('host')">HOST</button></div><button id="networkGear" class="gear" onclick="event.stopPropagation();openNetworkSettings()">⚙</button><div class="stateHead" data-i18n="state.network">Network</div><div class="stateValue copyValue" id="networkValue" onclick="copyNetworkIp()" data-i18n-title="button.copyIp">--</div><div class="stateMeta"><b data-i18n="state.ssid">SSID</b><span id="networkSsidValue">--</span></div><span class="stateDot"></span></div>
</div>
</section>
<section class="panel" id="chartPanel">
<div class="chartCanvasWrap">
<canvas id="chart" width="760" height="260"></canvas>
<button class="iconButton" onclick="toggleChartFullscreen()" id="chartFullscreenBtn" title="全屏" data-i18n-title="button.fullscreen"><svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M0 0h5L0 5z"/><path d="M16 0h-5L16 5z"/><path d="M0 16h5L0 11z"/><path d="M16 16h-5L16 11z"/></svg></button>
</div>
<div class="chartFooter"><div class="chartToolbar"><button class="iconButton" onclick="toggleChart()" id="chartBtn" title="暂停" data-i18n-title="button.pause"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg></button><button class="iconButton" onclick="clearChart()" title="清空" data-i18n-title="button.clear"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg></button><button class="iconButton" onclick="toggleTub()" id="tubRecordBtn" title="开始录制" data-i18n-title="button.tubRecord"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="8"/></svg></button><button class="iconButton" onclick="td()" id="tubDownloadBtn" title="下载 Tub JSON" data-i18n-title="button.tubDownload"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg></button><span class="recMeta"><span data-i18n="tub.recorded">录制量</span><b id="tubMeta">0</b></span></div><div class="legend"><span class="c1"><span data-i18n="chart.throttle">Throttle</span><b id="thrMeta">--</b></span><span class="c2"><span data-i18n="chart.steering">Steering</span><b id="strMeta">--</b></span><span class="c4"><span data-i18n="chart.gyroz">GyroZ</span><b id="gzMeta">--</b></span></div></div>
</section>
<section class="panel" id="serialPanel">
<div class="row"><select id="cmdTarget"><option value="serial" data-i18n="cmd.serial">Serial</option><option value="web" data-i18n="cmd.web">Web</option></select><div id="termTabs"></div><button class="iconButton" onclick="addTerminalTab()" id="newTermBtn" title="新建终端" data-i18n-title="terminal.new"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 5v14M5 12h14"/></svg></button><button class="iconButton" onclick="togglePause()" id="pauseBtn" title="暂停" data-i18n-title="button.pause"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg></button><button class="iconButton" onclick="sendCmd()" id="sendBtn" title="发送" data-i18n-title="button.send"><svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M2 21l21-9L2 3v7l15 2-15 2v7z"/></svg></button><input id="cmd"></div>
<div id="terminalWrap"><div id="terminalHint" class="terminalHint"></div><button class="iconButton" onclick="toggleTerminalFullscreen()" id="termFullscreenBtn" title="全屏" data-i18n-title="button.fullscreen"><svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M0 0h5L0 5z"/><path d="M16 0h-5L16 5z"/><path d="M0 16h5L0 11z"/><path d="M16 16h-5L16 11z"/></svg></button></div>
<div id="log" class="log"></div>
</section>
<section class="panel wide" id="diagnosticsPanel">
<div id="rcFold" class="fold"><button class="foldHead" onclick="toggleFold('rcFold')" aria-expanded="false"><span class="foldIcon">▸</span><span data-i18n="panel.rcChannels">RC Channels</span></button><div class="foldBody"><div class="rcGrid"><div class="rcCell"><b data-i18n="rc.ch1">CH1 Steering</b><span id="ch1Value">----</span></div><div class="rcCell"><b data-i18n="rc.ch2">CH2 Throttle</b><span id="ch2Value">----</span></div><div class="rcCell"><b data-i18n="rc.ch3">CH3 Park</b><span id="ch3Value">----</span></div><div class="rcCell modeCh"><b data-i18n="rc.ch4">CH4 Mode</b><span id="ch4Value">----</span></div><div class="rcCell"><b data-i18n="rc.ch5">CH5 Drift</b><span id="ch5Value">----</span></div><div class="rcCell"><b data-i18n="rc.ch6">CH6 Scale</b><span id="ch6Value">----</span></div></div><div style="display:flex;gap:6px;margin-top:6px"><div class="rcCell" style="flex:1"><b data-i18n="rc.outSteering">OUT Steering</b><span id="servoDutyValue">----</span></div><div class="rcCell" style="flex:1"><b data-i18n="rc.outThrottle">OUT Throttle</b><span id="escDutyValue">----</span></div></div><div style="display:flex;gap:6px;margin-top:6px"><div class="rcCell" style="flex:1"><b data-i18n="rc.midS">Mid S</b><div style="display:flex;align-items:center;justify-content:center"><span id="servoMidValue">----</span><button class="rcSetBtn" onclick="setServoMid()" style="font-size:11px;padding:1px 6px;margin-left:4px;cursor:pointer" title="设为转向中点" data-i18n="rc.set" data-i18n-title="rc.setSteeringMid">Set</button></div></div><div class="rcCell" style="flex:1"><b data-i18n="rc.midT">Mid T</b><div style="display:flex;align-items:center;justify-content:center"><span id="motorMidValue">----</span><button class="rcSetBtn" onclick="setMotorMid()" style="font-size:11px;padding:1px 6px;margin-left:4px;cursor:pointer" title="设为油门中点" data-i18n="rc.set" data-i18n-title="rc.setThrottleMid">Set</button></div></div></div><div style="display:flex;gap:6px;margin-top:6px"><div class="rcCell" style="flex:1"><b data-i18n="rc.minT">Min T</b><div style="display:flex;align-items:center;justify-content:center;gap:10px"><input type="range" id="throttleMinSlider" min="4915" max="7372" value="4915" step="1" oninput="setThrottleMin(this.value)" style="width:70%;min-width:0;height:18px;margin:2px 0"><input type="number" class="rcNum" id="throttleMinValue" min="4915" max="7372" step="1" placeholder="----" title="点击输入数值" data-i18n-title="rc.numInput" onchange="commitThrottleMin(this)"></div></div><div class="rcCell" style="flex:1"><b data-i18n="rc.maxT">Max T</b><div style="display:flex;align-items:center;justify-content:center;gap:10px"><input type="range" id="throttleMaxSlider" min="7372" max="9830" value="9830" step="1" oninput="setThrottleMax(this.value)" style="width:70%;min-width:0;height:18px;margin:2px 0"><input type="number" class="rcNum" id="throttleMaxValue" min="7372" max="9830" step="1" placeholder="----" title="点击输入数值" data-i18n-title="rc.numInput" onchange="commitThrottleMax(this)"></div></div></div></div></div>
<div id="diagSettingsRow" style="margin:10px 0"><button onclick="openJoystickCalModal()" data-i18n="button.joystickCal">Calibrate Joystick</button><button onclick="window.open('/drift?theme='+resolvedTheme(),'_blank')" data-i18n="button.driftSettings" style="margin-left:8px">漂移设置</button></div>
<div id="joystickCalStatus" style="font-size:12px;color:var(--ink3);margin-bottom:8px">方向: -- / -- / -- | 油门: -- / -- / --</div>
<div id="statusFold" class="fold"><button class="foldHead" onclick="toggleFold('statusFold')" aria-expanded="false"><span class="foldIcon">▸</span><span data-i18n="panel.statusDetails">STATUS Details</span></button><div class="foldBody"><div id="status">loading...</div></div></div>
</section>
</div>
<div id="devModeModal" class="modal"><div class="dialog"><h2 data-i18n="dev.title">开启开发模式？</h2><p data-i18n="dev.body">开发模式会持久化，并允许 Web Console 免认证保持 OTA 监听。不会放宽控制命令；实际 OTA 传输期间固件会默认 Park Locked。</p><div class="dialogActions"><button onclick="closeDevModeModal(false)" data-i18n="button.cancel">取消</button><button onclick="closeDevModeModal(true)" data-i18n="button.confirmDev">确认开启</button></div></div></div>
<div id="wifiApModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.apTitle">AP 名称配置</h2><div class="formRow"><label for="apSsid" data-i18n="wifi.apPrefixLabel">前缀</label><div class="inputWithAction"><input id="apSsid" placeholder="如 MUS4" maxlength="6" data-i18n-placeholder="wifi.apPlaceholder" oninput="updateApPreview()"></div></div><p id="apPreview" style="margin:4px 0 0;font-size:12px;color:var(--ink3)"></p><p id="apNotice" data-i18n="wifi.apNotice" style="display:none">前缀仅限大小写字母和数字，不超过6位；后缀固定为“-ESP”。保存后会重启 AP，当前浏览器连接会短暂断开。</p><div class="dialogActions"><button onclick="closeWifiApModal()" data-i18n="button.cancel">取消</button><button id="apSaveBtn" onclick="saveWifiAp()" data-i18n="wifi.saveRestartAp">保存并重启 AP</button></div></div></div>
<div id="wifiStaModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.staTitle">STA Wi-Fi 配置</h2><div class="staCols"><div class="staFormCol"><div class="formRow"><label for="staSsid">SSID</label><div class="inputWithAction"><input id="staSsid" placeholder="STA SSID" data-i18n-placeholder="wifi.staPlaceholder" autocomplete="off" autocapitalize="none" spellcheck="false"><button id="staSsidSearchBtn" class="iconButton" type="button" onclick="openWifiScanPopover(event)">⌕</button><div id="wifiScanPopover" class="scanPopover"><div id="wifiScanStatus" class="muted" data-i18n="wifi.scanning">扫描中...</div><div id="wifiScanList"></div></div></div></div><div class="formRow"><label for="staPassword" data-i18n="wifi.passwordLabel">密码</label><div class="inputWithAction"><input id="staPassword" type="password" placeholder="Wi-Fi 密码，留空表示开放网络" data-i18n-placeholder="wifi.passwordPlaceholder" autocomplete="new-password" data-1p-ignore="true" data-lpignore="true" data-form-type="other"><button id="staPasswordEye" class="iconButton" type="button" onclick="toggleStaPasswordVisibility()">👁</button></div></div><label class="toggleSwitch" style="margin:8px 0;gap:6px"><span class="toggleLabel" data-i18n="wifi.hostToggle">上位机配网</span><input type="checkbox" id="hostWifiToggle" onchange="onHostWifiToggle()"><span class="slider"></span></label><div id="hostWifiStatusBar" style="display:none;margin:6px 0;padding:8px;background:var(--card2);border:1px solid var(--line);border-radius:6px;font-size:12px;line-height:1.5"><b id="hostWifiStatusLabel" style="color:var(--ink3)" data-i18n="wifi.hostStatus.waitingHost">等待上位机上报</b><br><span id="hostWifiStatusIp" style="color:var(--accent);display:none"></span><span id="hostWifiStatusError" style="color:var(--bad);display:none"></span></div><p id="staNotice" data-i18n="wifi.staNotice">注意只能连接2.4G WiFi</p><div class="dialogActions"><button onclick="closeWifiStaModal()" data-i18n="button.cancel">取消</button><button id="staClearBtn" onclick="clearWifiSta()" data-i18n="button.clear">清除</button><button id="staConnectBtn" onclick="saveWifiSta()" data-i18n="button.connect">连接</button></div></div><div class="staHistoryCol"><div class="stateHead" data-i18n="wifi.historyTitle">已保存的 WiFi</div><div id="wifiHistoryList"></div><div id="wifiHistoryEmpty" data-i18n="wifi.historyEmpty" style="display:none">暂无记录</div></div></div></div></div>
<div id="wifiStaFailureModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.failureTitle">STA 连接失败</h2><p id="wifiStaFailureText" data-i18n="wifi.failureGeneric">连接失败。</p><div class="dialogActions"><button onclick="closeWifiStaFailureModal()" data-i18n="button.ok">知道了</button><button onclick="openWifiStaModal();closeWifiStaFailureModal()" data-i18n="button.reconfigure">重新配置</button></div></div></div>
<div id="wifiStaHandoffModal" class="modal"><div class="dialog"><h2 data-i18n="wifi.handoffTitle">STA 切换提示</h2><p id="wifiStaHandoffText" data-i18n="wifi.handoffGeneric">设备正在切换 Wi-Fi。</p><div class="dialogActions"><button onclick="copyHandoffIp()" data-i18n="button.copyUrl">复制地址</button><button onclick="closeWifiStaHandoffModal(true)" data-i18n="button.noMoreStaHint">不再提示</button><button onclick="closeWifiStaHandoffModal()" data-i18n="button.ok">知道了</button></div></div></div>
<button id="fabToggle" class="fabToggle" onclick="toggleFabActions(event)" aria-label="快捷入口" data-i18n-aria="fab.quick"></button>
<div id="fabActions" class="fabActions"><button id="helpFab" class="helpFab" onclick="openHelpModal()" aria-label="功能说明" data-i18n-aria="help.title">?</button></div>

<div id="helpOverlay" class="helpOverlay" onclick="closeHelpModal()"></div>
<div id="helpModal" class="helpModal" role="dialog" aria-modal="true" aria-labelledby="helpTitle"><div class="helpHead"><h2 id="helpTitle" data-i18n="help.title">功能说明</h2><button class="helpClose" onclick="closeHelpModal()" aria-label="关闭功能说明" data-i18n-aria="help.close">×</button></div><section class="helpSection"><h3 data-i18n="help.groupStatus">状态与日志</h3><ul class="helpList"><li data-i18n="help.statusCards">状态卡片：查看模式、Park、OTA、连接状态</li><li data-i18n="help.serialLog">Serial Log：查看设备日志和命令反馈</li></ul></section><section class="helpSection"><h3 data-i18n="help.groupNetwork">网络与诊断</h3><ul class="helpList"><li data-i18n="help.network">Network：查看 AP/STA IP，配置 Wi-Fi</li><li data-i18n="help.diagnostics">Diagnostics：运行测试、回归、维护命令</li></ul></section><section class="helpSection"><h3 data-i18n="help.groupData">数据与维护</h3><ul class="helpList"><li data-i18n="help.tubJson">Tub JSON：记录并下载遥测样本</li><li data-i18n="help.otaDev">OTA / DEV：固件更新与开发模式开关</li></ul></section></div>
<div id="joystickCalModal" class="modal"><div class="dialog" style="max-width:480px"><h3 data-i18n="cal.title">手柄校准</h3><div id="joystickCalStepText" style="margin:10px 0;line-height:1.5"></div><div id="joystickCalLive" style="font-family:monospace;font-size:13px;color:var(--ink3);margin:8px 0"></div><div class="dialogActions"><button id="joystickCalActionBtn" onclick="joystickCalAction()" data-i18n="cal.action.start">开始</button><button id="joystickCalRetryBtn" onclick="joystickCalRetry()" style="display:none" data-i18n="cal.action.retry">重试</button><button id="joystickCalSaveBtn" onclick="joystickCalSave()" style="display:none" data-i18n="cal.action.save">保存</button><button onclick="closeJoystickCalModal()" data-i18n="button.cancel">取消</button></div></div></div>
<div id="toast" class="toast"></div>
<div id="reconnectOverlay" class="reconnectOverlay"><div class="reconnectBox"><div class="reconnectSpinner"></div><div class="reconnectText"><span data-i18n="reconnect.title">连接已断开</span><br><span data-i18n="reconnect.body">正在尝试重新连接...</span></div><div class="reconnectActions"><button type="button" id="reconnectRetryBtn" onclick="manualReconnect()" data-i18n="reconnect.retry">重试</button></div></div></div>
<script>
var _launcherIp='192.168.3.41',_launcherIpAge=-1;function _applyLauncherStatus(txt){const m=txt.match(/host_ip=(\S+)/);if(m&&m[1]){_launcherIp=m[1];const a=txt.match(/host_ip_age_s=(\d+)/);_launcherIpAge=a?+a[1]:-1;document.getElementById('enterDonkeyBtn').href='http://'+_launcherIp+':8090/';document.getElementById('enterDonkeyDrifterBtn').href='http://'+_launcherIp+':8090/launch/drive';}}async function _fetchLauncherIp(){try{const r=await fetch('/api/status');_applyLauncherStatus(await r.text())}catch(e){}}_fetchLauncherIp();
let kimiCodeWebLaunching=false;async function openKimiCodeWeb(){if(kimiCodeWebLaunching)return;const btn=document.getElementById('openKimiCodeWebBtn'),newTab=window.open('about:blank','_blank');kimiCodeWebLaunching=true;btn.disabled=true;btn.querySelector('span[data-i18n]').textContent=t('button.openKimiCodeWebLaunching');const ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),120000);try{const r=await fetch('http://'+_launcherIp+':8090/api/launch/kimi-code-web',{method:'POST',signal:ctrl.signal});const j=await r.json().catch(()=>({}));if(!r.ok||j.status!=='ok'||!j.url)throw new Error(j.error||('HTTP '+r.status));if(newTab)newTab.location.href=j.url;else window.open(j.url,'_blank')}catch(e){if(newTab)newTab.close();line('kimi code web launch error: '+e);showToast(t(e&&e.name==='AbortError'?'toast.kimiCodeWebTimeout':'toast.kimiCodeWebFailed')+(e&&e.message&&e.name!=='AbortError'?': '+e.message:''),false)}finally{clearTimeout(timer);kimiCodeWebLaunching=false;btn.disabled=false;btn.querySelector('span[data-i18n]').textContent=t('button.openKimiCodeWeb')}}
let dshLaunching=false;async function openDsh(){if(dshLaunching)return;const btn=document.getElementById('openDshBtn'),newTab=window.open('about:blank','_blank');dshLaunching=true;btn.disabled=true;btn.querySelector('span[data-i18n]').textContent=t('button.openDshLaunching');const ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),120000);try{const r=await fetch('http://'+_launcherIp+':8090/api/launch/dsh',{method:'POST',signal:ctrl.signal});const j=await r.json().catch(()=>({}));if(!r.ok||j.status!=='ok'||!j.url)throw new Error(j.error||('HTTP '+r.status));if(newTab)newTab.location.href=j.url;else window.open(j.url,'_blank')}catch(e){if(newTab)newTab.close();line('dsh launch error: '+e);showToast(t(e&&e.name==='AbortError'?'toast.dshTimeout':'toast.dshFailed')+(e&&e.message&&e.name!=='AbortError'?': '+e.message:''),false)}finally{clearTimeout(timer);dshLaunching=false;btn.disabled=false;btn.querySelector('span[data-i18n]').textContent=t('button.openDsh')}}
let zcodeClickTimer=0;function zcodeRemoteGet(){try{return localStorage.getItem('zcodeRemoteUrl')||''}catch(e){return''}}function zcodeRemoteNormalize(raw){let u;try{u=new URL(String(raw||'').trim().replace(/^["'“”‘’「」]+|["'“”‘’「」]+$/g,''))}catch(e){return''}if(u.protocol!=='https:')return'';if(u.hash.length>1){let h=u.hash.slice(1);const q=h.indexOf('?');if(q>=0)h=h.slice(q+1);if(h.indexOf('=')>0)new URLSearchParams(h).forEach((v,k)=>{if(!u.searchParams.get(k))u.searchParams.set(k,v)});u.hash=''}if(u.searchParams.get('remoteControlToken'))return u.toString();if(!u.searchParams.get('sid')||!u.searchParams.get('hash'))return'';u.searchParams.set('t',String(Date.now()));return u.toString()}function zcodeRemoteFreshUrl(){return zcodeRemoteNormalize(zcodeRemoteGet())}function zcodeRemoteWake(){if(!_launcherIp)return;fetch('http://'+_launcherIp+':8090/api/launch/zcode-remote',{method:'POST'}).catch(()=>{})}function zcodeRemoteFetchLive(cb){if(!_launcherIp){cb(null);return}const ctl=new AbortController();const to=setTimeout(()=>ctl.abort(),30000);fetch('http://'+_launcherIp+':8000/api/zcode-remote/link',{method:'POST',signal:ctl.signal}).then(r=>r.ok?r.json():null).then(j=>{clearTimeout(to);cb(j&&j.status==='ok'&&j.url?j.url:null)}).catch(()=>{clearTimeout(to);cb(null)})}function zcodeRemotePrompt(){const cur=zcodeRemoteFreshUrl();const v=window.prompt(t('zcode.remotePrompt'),cur);if(v===null)return'';const url=zcodeRemoteNormalize(v);if(!url){alert(t('zcode.remoteInvalid'));return''}try{localStorage.setItem('zcodeRemoteUrl',url)}catch(e){}return url}function openZCode(){if(zcodeClickTimer)return;zcodeClickTimer=setTimeout(()=>{zcodeClickTimer=0;const win=window.open('about:blank','_blank');if(win)win.opener=null;const nav=u=>{if(win)win.location.href=u;else window.open(u,'_blank','noopener')};zcodeRemoteFetchLive(live=>{if(live){try{localStorage.setItem('zcodeRemoteUrl',live)}catch(e){}nav(live);return}const url=zcodeRemoteFreshUrl()||zcodeRemotePrompt();if(url){nav(url);zcodeRemoteWake()}else if(win)win.close()})},260)}function editZCodeUrl(){if(zcodeClickTimer){clearTimeout(zcodeClickTimer);zcodeClickTimer=0}zcodeRemotePrompt()}
const log=document.getElementById('log'),cmd=document.getElementById('cmd'),cmdTarget=document.getElementById('cmdTarget'),newTermBtn=document.getElementById('newTermBtn'),statusBox=document.getElementById('status'),devModeModal=document.getElementById('devModeModal'),versionLabel=document.getElementById('versionLabel'),apSsid=document.getElementById('apSsid'),apPreview=document.getElementById('apPreview'),apNotice=document.getElementById('apNotice'),apSaveBtn=document.getElementById('apSaveBtn'),wifiApModal=document.getElementById('wifiApModal'),staSsid=document.getElementById('staSsid'),staPassword=document.getElementById('staPassword'),staPasswordEye=document.getElementById('staPasswordEye'),staNotice=document.getElementById('staNotice'),wifiScanPopover=document.getElementById('wifiScanPopover'),wifiScanStatus=document.getElementById('wifiScanStatus'),wifiScanList=document.getElementById('wifiScanList'),wifiStaModal=document.getElementById('wifiStaModal'),wifiStaFailureModal=document.getElementById('wifiStaFailureModal'),wifiStaFailureText=document.getElementById('wifiStaFailureText'),wifiStaHandoffModal=document.getElementById('wifiStaHandoffModal'),wifiStaHandoffText=document.getElementById('wifiStaHandoffText'),toast=document.getElementById('toast'),networkCard=document.getElementById('networkCard'),networkApTab=document.getElementById('networkApTab'),networkStaTab=document.getElementById('networkStaTab'),networkValue=document.getElementById('networkValue'),networkSsidValue=document.getElementById('networkSsidValue'),voltageCard=document.getElementById('voltageCard'),voltageValue=document.getElementById('voltageValue'),voltageSub=document.getElementById('voltageSub'),chartPanel=document.getElementById('chartPanel'),canvas=document.getElementById('chart'),ctx=canvas.getContext('2d'),thrMeta=document.getElementById('thrMeta'),strMeta=document.getElementById('strMeta'),gzMeta=document.getElementById('gzMeta'),modeCard=document.getElementById('modeCard'),modeValue=document.getElementById('modeValue'),modeSub=document.getElementById('modeSub'),parkCard=document.getElementById('parkCard'),parkValue=document.getElementById('parkValue'),parkSub=document.getElementById('parkSub'),driftCard=document.getElementById('driftCard'),driftValue=document.getElementById('driftValue'),driftSub=document.getElementById('driftSub'),driftNeedle=document.getElementById('driftNeedle'),chValues=[1,2,3,4,5,6].map(n=>document.getElementById('ch'+n+'Value'));
const fabActions=document.getElementById('fabActions'),reconnectOverlay=document.getElementById('reconnectOverlay');
const joystickCalModal=document.getElementById('joystickCalModal'),joystickCalStepText=document.getElementById('joystickCalStepText'),joystickCalLive=document.getElementById('joystickCalLive'),joystickCalActionBtn=document.getElementById('joystickCalActionBtn'),joystickCalRetryBtn=document.getElementById('joystickCalRetryBtn'),joystickCalSaveBtn=document.getElementById('joystickCalSaveBtn'),joystickCalStatus=document.getElementById('joystickCalStatus');
const LANG_STORAGE_KEY='mus4.ui.lang';
let connectionLost=false,reconnectTimer=0;
const ICON_PAUSE='<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="4" width="4" height="16" rx="1"/><rect x="14" y="4" width="4" height="16" rx="1"/></svg>';
const ICON_PLAY='<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>';
const ICON_CLEAR='<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>';
const ICON_FULLSCREEN='<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M0 0h5L0 5z"/><path d="M16 0h-5L16 5z"/><path d="M0 16h5L0 11z"/><path d="M16 16h-5L16 11z"/></svg>';
const ICON_FULLSCREEN_EXIT='<svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor"><path d="M5 0L0 5h5V0z"/><path d="M11 0l5 5h-5V0z"/><path d="M5 16L0 11h5v5z"/><path d="M11 16l5-5h-5v5z"/></svg>';
const ICON_SEND='<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M2 21l21-9L2 3v7l15 2-15 2v7z"/></svg>';
const ICON_RECORD='<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="8"/></svg>';
const ICON_RECORDING='<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><rect x="7" y="7" width="10" height="10" rx="2"/></svg>';
const ICON_DOWNLOAD='<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>';
const I18N={zh:{'app.title':'Drifter Console','language.title':'语言','mute.title':'静音','button.ota':'OTA','button.enterDonkey':'Donkey','button.enterDonkeyDrifter':'DonkeyDrifter','button.openKimiCodeWeb':'Kimi Code Web','button.openKimiCodeWebLaunching':'正在启动 Kimi Code Web...','toast.kimiCodeWebFailed':'Kimi Code Web 启动失败','toast.kimiCodeWebTimeout':'Kimi Code Web 启动超时','button.openDsh':'DeepSeek Harness','button.openDshLaunching':'正在启动 DeepSeek Harness...','toast.dshFailed':'DeepSeek Harness 启动失败','toast.dshTimeout':'DeepSeek Harness 启动超时','button.openZCode':'ZCode','zcode.remoteHint':'单击打开 ZCode 远程控制，双击更新链接','zcode.remotePrompt':'请粘贴 ZCode 桌面端「复制链接」给出的完整远控链接（形如 https://zcode.z.ai/remote/v4?sid=…&hash=…）','zcode.remoteInvalid':'链接无效：必须是桌面端复制的完整链接（含 sid 与 hash 参数），未保存','button.pause':'暂停','button.resume':'继续','button.draw':'绘制','button.clear':'清空','button.fullscreen':'全屏','button.split':'分屏','button.send':'发送','button.tubRecord':'开始录制','button.tubStopRecord':'停止录制','button.tubDownload':'下载 Tub JSON','button.cancel':'取消','button.confirmDev':'确认开启','button.connect':'连接','button.ok':'知道了','button.reconfigure':'重新配置','button.copyIp':'复制 IP','button.copyUrl':'复制地址','button.openNewUrl':'打开新地址','wifi.saveRestartAp':'保存并重启 AP','state.mode':'模式','state.park':'驻车','state.drift':'漂移','state.voltage':'电压','state.network':'网络','state.remain':'剩余','state.ssid':'SSID','panel.rcChannels':'RC 通道','panel.statusDetails':'状态详情','dev.title':'开启开发模式？','dev.body':'开发模式会持久化，并允许 Web Console 免认证保持 OTA 监听。不会放宽控制命令；实际 OTA 传输期间固件会默认 Park Locked。','wifi.apTitle':'AP 名称配置','wifi.apPrefixLabel':'前缀','wifi.apPlaceholder':'如 MUS4','wifi.apNotice':'前缀仅限大小写字母和数字，不超过6位；后缀固定为“-ESP”。保存后会重启 AP，当前浏览器连接会短暂断开。','wifi.staTitle':'STA Wi-Fi 配置','wifi.staPlaceholder':'STA SSID','wifi.passwordLabel':'密码','wifi.passwordPlaceholder':'Wi-Fi 密码，留空表示开放网络','wifi.scanning':'扫描中...','wifi.staNotice':'注意只能连接2.4G WiFi','wifi.hidePassword':'隐藏密码','wifi.showPassword':'显示密码','wifi.failureTitle':'STA 连接失败','wifi.handoffTitle':'STA 切换提示','help.title':'功能说明','help.close':'关闭功能说明','help.statusCards':'状态卡片：查看模式、Park、OTA、连接状态','help.network':'Network：查看 AP/STA IP，配置 Wi-Fi','help.diagnostics':'Diagnostics：运行测试、回归、维护命令','help.serialLog':'Serial Log：查看设备日志','help.tubJson':'Tub JSON：记录并下载遥测样本','help.otaDev':'OTA / DEV：固件更新与开发模式开关','help.groupStatus':'状态与日志','help.groupNetwork':'网络与诊断','help.groupData':'数据与维护','mode.manual':'手动输入','mode.assist':'辅助转向','mode.auto':'自动驾驶','mode.unknown':'未知','park.guarded':'输出已保护','park.enabled':'可驾驶','battery':'电池','voltage.disconnected':'未连接','toast.copyFailed':'复制失败，请手动选择 IP','toast.copiedIp':'已复制 IP：','toast.copiedUrl':'已复制地址：','toast.newIpUnavailable':'新 IP 暂不可用，请先连接设备 AP 查看','toast.newUrlUnavailable':'新地址暂不可用，请连接设备 AP 查看','error.parkRequired':'当前操作需要 Park Locked。请将 CH3/Park 切到锁定状态后重试。','error.authRequired':'当前操作需要授权。请先 AUTH，或开启 DEV MODE 后重试。','theme.title':'主题','reconnect.retry':'重试','embed.driftTune':'漂移设置','embed.judgeTune':'Judge 设置'},en:{'app.title':'Drifter Console','language.title':'Language','mute.title':'Mute','button.ota':'OTA','button.enterDonkey':'Donkey','button.enterDonkeyDrifter':'DonkeyDrifter','button.openKimiCodeWeb':'Kimi Code Web','button.openKimiCodeWebLaunching':'Launching Kimi Code Web...','toast.kimiCodeWebFailed':'Failed to launch Kimi Code Web','toast.kimiCodeWebTimeout':'Kimi Code Web launch timed out','button.openDsh':'DeepSeek Harness','button.openDshLaunching':'Launching DeepSeek Harness...','toast.dshFailed':'Failed to launch DeepSeek Harness','toast.dshTimeout':'DeepSeek Harness launch timed out','button.openZCode':'ZCode','zcode.remoteHint':'Click to open ZCode Remote Control, double-click to update the link','zcode.remotePrompt':'Paste the full Remote Control link from the ZCode desktop "Copy link" button (e.g. https://zcode.z.ai/remote/v4?sid=…&hash=…)','zcode.remoteInvalid':'Invalid link: must be the full link copied from the desktop app (with sid and hash params), not saved','button.pause':'Pause','button.resume':'Resume','button.draw':'Draw','button.clear':'Clear','button.fullscreen':'Fullscreen','button.split':'Split','button.send':'Send','button.tubRecord':'Start Recording','button.tubStopRecord':'Stop Recording','button.tubDownload':'Download Tub JSON','button.cancel':'Cancel','button.confirmDev':'Enable','button.connect':'Connect','button.ok':'OK','button.reconfigure':'Reconfigure','button.copyIp':'Copy IP','button.copyUrl':'Copy URL','button.openNewUrl':'Open new URL','wifi.saveRestartAp':'Save and restart AP','state.mode':'Mode','state.park':'Park','state.drift':'Drift','state.voltage':'Voltage','state.network':'Network','state.remain':'REMAIN','state.ssid':'SSID','panel.rcChannels':'RC Channels','panel.statusDetails':'STATUS Details','dev.title':'Enable dev mode?','dev.body':'Dev mode is persistent and lets Web Console keep OTA listening without AUTH. It does not loosen control commands; firmware still defaults to Park Locked during OTA transfer.','wifi.apTitle':'AP Name Configuration','wifi.apPrefixLabel':'Prefix','wifi.apPlaceholder':'e.g. MUS4','wifi.apNotice':'Prefix is limited to letters and digits (max 6 chars). Suffix "-ESP" is fixed. Saving restarts AP and briefly disconnects this browser.','wifi.staTitle':'STA Wi-Fi settings','wifi.staPlaceholder':'STA SSID','wifi.passwordLabel':'Password','wifi.passwordPlaceholder':'Wi-Fi password, leave blank for open network','wifi.scanning':'Scanning...','wifi.staNotice':'Only 2.4 GHz Wi-Fi is supported','wifi.hidePassword':'Hide password','wifi.showPassword':'Show password','wifi.failureTitle':'STA connection failed','wifi.handoffTitle':'STA handoff notice','help.title':'Help','help.close':'Close help','help.statusCards':'Status Cards: view mode, Park, OTA, and connection status','help.network':'Network: view AP/STA IP and configure Wi-Fi','help.diagnostics':'Diagnostics: run tests, regression, and maintenance commands','help.serialLog':'Serial Log: view device logs','help.tubJson':'Tub JSON: record and download telemetry samples','help.otaDev':'OTA / DEV: firmware update and development mode switches','help.groupStatus':'Status & Logs','help.groupNetwork':'Network & Diagnostics','help.groupData':'Data & Maintenance','mode.manual':'Manual input','mode.assist':'Pilot steering','mode.auto':'Pilot control','mode.unknown':'unknown','park.guarded':'output guarded','park.enabled':'drive enabled','battery':'battery','voltage.disconnected':'Not connected','toast.copyFailed':'Copy failed, select the IP manually','toast.copiedIp':'Copied IP: ','toast.copiedUrl':'Copied URL: ','toast.newIpUnavailable':'New IP is not available yet; connect to device AP first','toast.newUrlUnavailable':'New URL is not available yet; connect to device AP first','error.parkRequired':'This action requires Park Locked. Switch CH3/Park to locked and retry.','error.authRequired':'This action requires authorization. Send AUTH first, or enable DEV MODE and retry.','theme.title':'Theme','reconnect.retry':'Retry','embed.driftTune':'Drift settings','embed.judgeTune':'Judge settings'}};
I18N.zh['button.joystickCal']='手柄校准';
I18N.zh['button.driftSettings']='漂移设置';
I18N.zh['settings.title']='车辆设置';
I18N.zh['settings.wifi']='Wi-Fi 配网';
I18N.zh['settings.system']='系统';
I18N.zh['settings.tuning']='调校';
I18N.zh['settings.judge']='Judge 设置';
I18N.zh['settings.dev']='开发模式';
I18N.en['settings.title']='Vehicle Settings';
I18N.en['settings.wifi']='Wi-Fi Network';
I18N.en['settings.system']='System';
I18N.en['settings.tuning']='Tuning';
I18N.en['settings.judge']='Judge Settings';
I18N.en['settings.dev']='Dev Mode';
I18N.zh['cal.title']='手柄校准';
I18N.zh['cal.label.steering']='方向';
I18N.zh['cal.label.throttle']='油门';
I18N.zh['cal.step.center']='第 1 步：请将手柄（方向和油门）完全回中，保持不动，然后点击“开始”。';
I18N.zh['cal.step.minmax']='第 2 步：请在 5 秒内将手柄依次推到最大位置：方向左、方向右、油门前、油门后。';
I18N.zh['cal.step.done']='校准完成。请检查下方数值，确认后点击“保存”。';
I18N.zh['cal.action.start']='开始校准';
I18N.zh['cal.action.save']='保存到设备';
I18N.zh['cal.action.retry']='重试';
I18N.zh['cal.prompt.auth']='手柄校准需要认证。请输入 Web Console AP 密码：';
I18N.zh['error.authRequired']='认证失败或未被授权：请检查 AP 密码，并在 Serial Log 中发送 AUTH:<密码> 后重试。';
I18N.zh['error.parkRequired']='需要 Park 锁定：请确保车辆已切换到 Park Locked 状态。';
I18N.zh['data.seqReset']='检测到设备重启，数据与日志序号已重置，继续接收';
I18N.zh['error.joystickInvalidRange']='校准未完成：摇杆未打满行程。请重新校准，并在时限内把方向与油门都打满到极限位置。';
I18N.zh['error.joystickSaveFailed']='校准保存失败，请重试。';
I18N.zh['tub.clearedWhileRecording']='图表已清空，正在进行的录制已停止';


I18N.en['button.joystickCal']='Calibrate Joystick';
I18N.en['button.driftSettings']='Drift Settings';
I18N.en['cal.title']='Joystick Calibration';
I18N.en['cal.label.steering']='Steering';
I18N.en['cal.label.throttle']='Throttle';
I18N.en['cal.step.center']='Step 1: Center both sticks (steering and throttle), hold steady, then click Start.';
I18N.en['cal.step.minmax']='Step 2: Within 5 seconds, move both sticks to their extremes: left, right, throttle forward, throttle backward.';
I18N.en['cal.step.done']='Calibration complete. Review the values below, then click Save.';
I18N.en['cal.action.start']='Start Calibration';
I18N.en['cal.action.save']='Save to Device';
I18N.en['cal.action.retry']='Retry';
I18N.en['cal.prompt.auth']='Joystick calibration requires authentication. Please enter the Web Console AP password:';
I18N.en['error.authRequired']='Authentication failed or not authorized: please check the AP password and try again after sending AUTH:<password> in Serial Log.';
I18N.en['error.parkRequired']='Park lock required: make sure the vehicle is in Park Locked state.';
I18N.en['data.seqReset']='Device reboot detected; data/log sequence reset, resuming';
I18N.en['error.joystickInvalidRange']='Calibration incomplete: sticks did not reach full travel. Recalibrate and push both sticks to their extremes in time.';
I18N.en['error.joystickSaveFailed']='Failed to save calibration; please try again.';
I18N.en['tub.clearedWhileRecording']='Chart cleared; ongoing recording stopped';


I18N.zh['wifi.historyTitle']='已保存的 WiFi';
I18N.en['wifi.historyTitle']='Saved WiFi';
I18N.zh['wifi.historyEmpty']='暂无记录';
I18N.en['wifi.historyEmpty']='No saved networks';
I18N.zh['wifi.historyDeleteConfirm']='删除这条 WiFi 记录？';
I18N.en['wifi.historyDeleteConfirm']='Delete this saved WiFi?';
I18N.zh['wifi.historyKeepCurrentNote']='仅移除记录，不影响本次连接';
I18N.en['wifi.historyKeepCurrentNote']='Record removed; current connection kept';
I18N.zh['wifi.historyDelete']='删除';
I18N.en['wifi.historyDelete']='Delete';
I18N.zh['wifi.historyFill']='点击填充 SSID 与密码';
I18N.en['wifi.historyFill']='Click to fill SSID and password';
I18N.zh['tub.recorded']='录制量';
I18N.en['tub.recorded']='Recorded';
I18N.zh['mode.value.rc']='遥控';I18N.en['mode.value.rc']='RC';
I18N.zh['mode.value.assist']='辅助';I18N.en['mode.value.assist']='ASSIST';
I18N.zh['mode.value.auto']='自动';I18N.en['mode.value.auto']='AUTO';
I18N.zh['park.locked']='已锁定';I18N.en['park.locked']='LOCKED';
I18N.zh['park.unlocked']='已解锁';I18N.en['park.unlocked']='UNLOCKED';
I18N.zh['drift.off']='关闭';I18N.en['drift.off']='OFF';
I18N.zh['drift.armed']='待命';I18N.en['drift.armed']='ARMED';
I18N.zh['drift.active']='生效中';I18N.en['drift.active']='ACTIVE';
I18N.zh['state.tune']='调参';I18N.en['state.tune']='Tune';
I18N.zh['state.waiting']='等待中';I18N.en['state.waiting']='waiting';
I18N.zh['chart.throttle']='油门';I18N.en['chart.throttle']='Throttle';
I18N.zh['chart.steering']='转向';I18N.en['chart.steering']='Steering';
I18N.zh['chart.gyroz']='GyroZ';I18N.en['chart.gyroz']='GyroZ';
I18N.zh['cmd.serial']='串口';I18N.en['cmd.serial']='Serial';
I18N.zh['cmd.web']='网络';I18N.en['cmd.web']='Web';
I18N.zh['rc.ch1']='CH1 转向';I18N.en['rc.ch1']='CH1 Steering';
I18N.zh['rc.ch2']='CH2 油门';I18N.en['rc.ch2']='CH2 Throttle';
I18N.zh['rc.ch3']='CH3 驻车';I18N.en['rc.ch3']='CH3 Park';
I18N.zh['rc.ch4']='CH4 模式';I18N.en['rc.ch4']='CH4 Mode';
I18N.zh['rc.ch5']='CH5 漂移';I18N.en['rc.ch5']='CH5 Drift';
I18N.zh['rc.ch6']='CH6 比例';I18N.en['rc.ch6']='CH6 Scale';
I18N.zh['rc.outSteering']='输出 转向';I18N.en['rc.outSteering']='OUT Steering';
I18N.zh['rc.outThrottle']='输出 油门';I18N.en['rc.outThrottle']='OUT Throttle';
I18N.zh['rc.midS']='中点 方向';I18N.en['rc.midS']='Mid S';
I18N.zh['rc.midT']='中点 油门';I18N.en['rc.midT']='Mid T';
I18N.zh['rc.minT']='油门下限';I18N.en['rc.minT']='Min T';
I18N.zh['rc.maxT']='油门上限';I18N.en['rc.maxT']='Max T';
I18N.zh['rc.set']='设定';I18N.en['rc.set']='Set';
I18N.zh['button.noMoreStaHint']='不再提示';I18N.en['button.noMoreStaHint']="Don't show again";
I18N.zh['rc.setSteeringMid']='设为转向中点';
I18N.en['rc.setSteeringMid']='Set as steering center';
I18N.zh['rc.setThrottleMid']='设为油门中点';
I18N.en['rc.setThrottleMid']='Set as throttle center';
I18N.zh['rc.numInput']='点击输入数值';
I18N.en['rc.numInput']='Click to type a value';
I18N.zh['fab.quick']='快捷入口';
I18N.en['fab.quick']='Quick actions';
I18N.zh['reconnect.title']='连接已断开';
I18N.en['reconnect.title']='Connection lost';
I18N.zh['reconnect.body']='正在尝试重新连接...';
I18N.en['reconnect.body']='Reconnecting...';
I18N.zh['log.empty']='[当前来源暂无日志]';
I18N.en['log.empty']='[No logs for current source]';
I18N.zh['terminal.loading']='正在连接上位机终端…';
I18N.en['terminal.loading']='Connecting to host terminal…';
I18N.zh['terminal.unreachable']='无法连接上位机终端服务，请确认上位机在线。可尝试直接打开：';
I18N.en['terminal.unreachable']='Host terminal unreachable. Make sure the host is online, or open directly: ';
I18N.zh['terminal.staleIp']='上位机 IP 已 {n} 秒未上报，可能已过期';
I18N.en['terminal.staleIp']='Host IP not reported for {n}s, may be stale';
I18N.zh['terminal.unknownIp']='尚未收到上位机 IP 上报（_launcherIp 仍为默认回退值），请确认上位机已连接设备并通过串口上报 HOSTIP。';
I18N.en['terminal.unknownIp']='Host IP not reported yet (using fallback). Make sure the host is connected and reporting HOSTIP over serial.';
I18N.zh['terminal.new']='新建终端标签页';
I18N.en['terminal.new']='New terminal tab';
I18N.zh['terminal.tab']='终端';
I18N.en['terminal.tab']='Term';
I18N.zh['terminal.closeTab']='关闭该终端';
I18N.en['terminal.closeTab']='Close this terminal';
I18N.zh['terminal.empty']='终端已关闭，点 ➕ 新建';
I18N.en['terminal.empty']='Terminal closed — click ➕ for a new one';
I18N.zh['wifi.apPreview']='完整 SSID: ';
I18N.en['wifi.apPreview']='Full SSID: ';
I18N.zh['wifi.apInvalid']='前缀只能使用大小写字母和数字，长度为 1-6 位。';
I18N.en['wifi.apInvalid']='Prefix must be 1-6 letters or digits.';
I18N.zh['wifi.apSaving']='正在保存...';
I18N.en['wifi.apSaving']='Saving...';
I18N.zh['wifi.apSavingNotice']='正在保存';
I18N.en['wifi.apSavingNotice']='Saving';
I18N.zh['wifi.apSaveFailed']='保存失败';
I18N.en['wifi.apSaveFailed']='Save failed';
I18N.zh['wifi.apSaved']='AP 名称已保存，正在重启 AP';
I18N.en['wifi.apSaved']='AP name saved, restarting AP';
I18N.zh['wifi.scanNone']='未扫描到 2.4G WiFi';
I18N.en['wifi.scanNone']='No 2.4 GHz Wi-Fi found';
I18N.zh['wifi.scanSelect']='选择 2.4G WiFi';
I18N.en['wifi.scanSelect']='Select a 2.4 GHz Wi-Fi';
I18N.zh['wifi.scanFailed']='扫描失败';
I18N.en['wifi.scanFailed']='Scan failed';
I18N.zh['wifi.hostToggle']='上位机配网';
I18N.en['wifi.hostToggle']='Provision via host';
I18N.zh['wifi.hostSendBtn']='发送到上位机';
I18N.en['wifi.hostSendBtn']='Send to host';
I18N.zh['wifi.hostDoneBtn']='完成';
I18N.en['wifi.hostDoneBtn']='Done';
I18N.zh['wifi.hostNotice']='通过串口将 WiFi 凭据发送给 Linux 上位机，由上位机执行 nmcli 连接';
I18N.en['wifi.hostNotice']='Send Wi-Fi credentials over serial to the Linux host, which connects via nmcli';
I18N.zh['wifi.hostStatus.hostOnline']='上位机在线';
I18N.en['wifi.hostStatus.hostOnline']='Host online';
I18N.zh['wifi.hostStatus.waitingHost']='等待上位机上报';
I18N.en['wifi.hostStatus.waitingHost']='Waiting for host report';
I18N.zh['wifi.hostStatus.connecting']='正在连接上位机 WiFi...';
I18N.en['wifi.hostStatus.connecting']='Connecting to host Wi-Fi...';
I18N.zh['wifi.hostStatus.connected']='上位机配网成功!';
I18N.en['wifi.hostStatus.connected']='Host Wi-Fi configured!';
I18N.zh['wifi.hostStatus.failed']='上位机配网失败';
I18N.en['wifi.hostStatus.failed']='Host Wi-Fi provisioning failed';
I18N.zh['wifi.hostOkToast']='上位机配网成功: ';
I18N.en['wifi.hostOkToast']='Host Wi-Fi configured: ';
I18N.zh['wifi.hostFailPrefix']='失败: ';
I18N.en['wifi.hostFailPrefix']='Failed: ';
I18N.zh['wifi.ssidEmpty']='SSID 不能为空';
I18N.en['wifi.ssidEmpty']='SSID cannot be empty';
I18N.zh['wifi.hostSending']='正在发送配网指令...';
I18N.en['wifi.hostSending']='Sending provisioning command...';
I18N.zh['wifi.hostWaitResponse']='等待上位机响应...';
I18N.en['wifi.hostWaitResponse']='Waiting for host response...';
I18N.zh['wifi.hostSendFailed']='发送失败';
I18N.en['wifi.hostSendFailed']='Send failed';
I18N.zh['wifi.networkError']='网络错误';
I18N.en['wifi.networkError']='Network error';
I18N.zh['wifi.staConnecting']='设备正在连接 Wi-Fi。连上后请在电脑/手机的 Wi-Fi 列表中查看名为 MUS4-<设备IP> 的网络（约 60 秒后该 AP 自动关闭）';
I18N.en['wifi.staConnecting']='Device is connecting to Wi-Fi. Once connected, look for a network named MUS4-<device IP> in the Wi-Fi list on your computer/phone (the AP turns off after about 60 seconds)';
I18N.zh['wifi.connectFailed']='连接失败';
I18N.en['wifi.connectFailed']='Connection failed';
I18N.zh['wifi.clearConfirm']='确认清除并禁用 STA 配置？';
I18N.en['wifi.clearConfirm']='Clear and disable the STA configuration?';
I18N.zh['wifi.failureGeneric']='连接失败。';
I18N.en['wifi.failureGeneric']='Connection failed.';
I18N.zh['wifi.handoffGeneric']='设备正在切换 Wi-Fi。';
I18N.en['wifi.handoffGeneric']='Device is switching Wi-Fi.';
I18N.zh['wifi.handoffConnecting']='设备正在连接 Wi-Fi：';
I18N.en['wifi.handoffConnecting']='Connecting to Wi-Fi: ';
I18N.zh['wifi.handoffSwitch']='请将电脑/手机切换到 Wi-Fi：';
I18N.en['wifi.handoffSwitch']='Switch your computer/phone to Wi-Fi: ';
I18N.zh['wifi.handoffLanIp']='局域网 IP：';
I18N.en['wifi.handoffLanIp']='LAN IP: ';
I18N.zh['wifi.handoffWaitingIp']='等待设备获取新 IP';
I18N.en['wifi.handoffWaitingIp']='Waiting for device to obtain new IP';
I18N.zh['wifi.handoffUrl']='访问地址：';
I18N.en['wifi.handoffUrl']='URL: ';
I18N.zh['wifi.handoffHint']='连上后设备会把 Wi-Fi 名临时改成 MUS4-<设备IP> 并广播约 60 秒，请在电脑/手机的 Wi-Fi 列表中找到该网络读出设备 IP；若设备未连上会自动恢复 AP（';
I18N.en['wifi.handoffHint']='Once connected, the device temporarily renames its Wi-Fi to MUS4-<device IP> and broadcasts for about 60 seconds. Find that network in the Wi-Fi list on your computer/phone to read the device IP; if the device does not connect, it restores the AP (';
I18N.zh['wifi.handoffHintTail']='）用于重配。';
I18N.en['wifi.handoffHintTail']=') for reconfiguration.';
I18N.zh['wifi.failureSsidLabel']='SSID：';
I18N.en['wifi.failureSsidLabel']='SSID: ';
I18N.zh['wifi.failureReasonLabel']='原因：';
I18N.en['wifi.failureReasonLabel']='Reason: ';
I18N.zh['wifi.failureAdvice']='建议：检查 SSID、密码、路由器距离后重新保存。';
I18N.en['wifi.failureAdvice']='Advice: check the SSID, password, and router distance, then save again.';
I18N.zh['wifi.failureReasonDefault']='STA 连接失败，请检查 SSID、密码与路由器状态。';
I18N.en['wifi.failureReasonDefault']='STA connection failed; check the SSID, password, and router status.';
I18N.zh['wifi.staConnected']='STA 已连接';
I18N.en['wifi.staConnected']='STA connected';
I18N.zh['wifi.staConnectedIp']='STA 已连接，IP：';
I18N.en['wifi.staConnectedIp']='STA connected, IP: ';
I18N.zh['wifi.staSwitchAndOpen']='，请切换到该 Wi-Fi 后打开 ';
I18N.en['wifi.staSwitchAndOpen']=', switch to that Wi-Fi and open ';
I18N.zh['wifi.staConnectedToast']='STA 已连接：';
I18N.en['wifi.staConnectedToast']='STA connected: ';
I18N.zh['wifi.staGettingIp']='STA 已连接，正在获取 IP...';
I18N.en['wifi.staGettingIp']='STA connected, obtaining IP...';
I18N.zh['wifi.staTimeout']='STA 连接超时，请检查 SSID、密码与路由器信号。';
I18N.en['wifi.staTimeout']='STA connection timed out; check the SSID, password, and router signal.';
let uiLang=readStoredLanguage();
let uiMuted=false;
let uiDevMode=false;
let uiTheme='auto';
const CHART_THEMES={dark:{grid:'#233041',axis:'#8fa1b5',thr:'#39d98a',str:'#5cc8ff',gz:'#ff6b6b',saver:'#5cc8ff',toastOk:'#39d98a',toastErr:'#ff6b6b'},light:{grid:'#dbe2ea',axis:'#5b6b7d',thr:'#1fae6b',str:'#0c9bd6',gz:'#e5484d',saver:'#0c9bd6',toastOk:'#1fae6b',toastErr:'#e5484d'}};
function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}
function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}
function applyTheme(){document.documentElement.dataset.theme=resolvedTheme();gridReady=false;draw();const dl=document.getElementById('driftTuneLink');if(dl)dl.href='/drift?theme='+resolvedTheme()}
const LOG_SOURCE_MAX_BYTES=1024*1024;
const sourceBuffers={web:'',serial:'',serial1:''};
let currentLogSource='web';
let lastLogSeq=0,lastDataSeq=0,pointHead=0,pointCount=0,logPaused=false,chartPaused=false,wifiScanTimer=0,wifiScanBusy=false,apSaving=false,staPasswordPlaceholder=false,staPasswordDirty=false,staPasswordVisible=false,staSavedPassword='',staSavedPasswordKnown=false,dataPolling=false,points=new Array(256),scrollOffset=0,lastFrameTime=performance.now(),lastDrawTime=0,smoothedDt=16,dataWs=null,dataWsConnected=false,dataWsReconnectDelay=500,dataWsReconnectTimer=0,dataTransport='poll',screenSaverActive=false,screenSaverStartTime=0,parkLockedAt=0,ch1Samples=[],networkTab='auto',networkTabPinned=false,networkCopyIp='',toastTimer=0,tubRecording=false,tubSamples=[],tubStartedMs=0,tubStoppedMs=0,tubLastSeq=0,tubRecordBtn=document.getElementById('tubRecordBtn'),tubMeta=document.getElementById('tubMeta'),gridCanvas=document.createElement('canvas'),gridCtx=gridCanvas.getContext('2d'),gridReady=false,saverTime=0,dpr=1,cw=760,ch=260;const TUB_MAX_SAMPLES=12000;const TUB_SCHEMA='mus4.web_data_point.tub.v2';
let calPollTimer=0;
function initCanvasDpr(){dpr=window.devicePixelRatio||1;cw=Math.round((canvas.clientWidth||canvas.width||760));ch=Math.round((canvas.clientHeight||canvas.height||260));canvas.width=cw*dpr;canvas.height=ch*dpr;ctx.setTransform(dpr,0,0,dpr,0,0);gridCanvas.width=cw*dpr;gridCanvas.height=ch*dpr;gridCtx.setTransform(dpr,0,0,dpr,0,0);gridReady=false;}function normalizeLanguage(lang){return lang==='en'?'en':'zh'}
function readUrlLanguage(){try{const m=/[?&]lang=(zh|en)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}function readStoredLanguage(){try{const v=localStorage.getItem(LANG_STORAGE_KEY);return v==='zh'||v==='en'?v:detectBrowserLanguage()}catch(e){return detectBrowserLanguage()}}
function writeStoredLanguage(lang){try{localStorage.setItem(LANG_STORAGE_KEY,lang)}catch(e){}}
function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}
function t(key){return (I18N[uiLang]&&I18N[uiLang][key])||I18N.zh[key]||key}
let hostWifiLabelKey='wifi.hostStatus.waitingHost';function setHostWifiLabel(k){hostWifiLabelKey=k;const l=document.getElementById('hostWifiStatusLabel');if(l)l.textContent=t(k)}
function refreshDynamicLabels(){setHostWifiLabel(hostWifiLabelKey);const p=document.getElementById('pauseBtn'),c=document.getElementById('chartBtn'),f=document.getElementById('chartFullscreenBtn'),tf=document.getElementById('termFullscreenBtn'),s=document.getElementById('sendBtn');if(s)s.title=t('button.send');p.innerHTML=logPaused?ICON_PLAY:ICON_PAUSE;p.title=logPaused?t('button.resume'):t('button.pause');c.innerHTML=chartPaused?ICON_PLAY:ICON_PAUSE;c.title=chartPaused?t('button.draw'):t('button.pause');f.innerHTML=document.fullscreenElement===chartPanel?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN;f.title=document.fullscreenElement===chartPanel?t('button.split'):t('button.fullscreen');if(tf){tf.innerHTML=document.fullscreenElement===terminalWrap?ICON_FULLSCREEN_EXIT:ICON_FULLSCREEN;tf.title=document.fullscreenElement===terminalWrap?t('button.split'):t('button.fullscreen');}if(tubRecordBtn){tubRecordBtn.innerHTML=tubRecording?ICON_RECORDING:ICON_RECORD;tubRecordBtn.title=tubRecording?t('button.tubStopRecord'):t('button.tubRecord');tubRecordBtn.classList.toggle('recording',tubRecording)}updateStaPasswordEye();fitTermTabLabels()}
function applyLanguage(lang){uiLang=normalizeLanguage(lang);document.documentElement.lang=uiLang;document.querySelectorAll('[data-i18n]').forEach(e=>{const v=t(e.dataset.i18n);if(v)e.textContent=v});document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>{const v=t(e.dataset.i18nPlaceholder);if(v)e.placeholder=v});document.querySelectorAll('[data-i18n-aria]').forEach(e=>{const v=t(e.dataset.i18nAria);if(v)e.setAttribute('aria-label',v)});document.querySelectorAll('[data-i18n-title]').forEach(e=>{const v=t(e.dataset.i18nTitle);if(v)e.title=v});renderLangButton();refreshDynamicLabels();refreshJoystickCalStatus()}
function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);fetch('/api/language?lang='+uiLang,{method:'POST'}).catch(()=>{})}
function toggleLanguage(){setLanguage(uiLang==='zh'?'en':'zh')}
function renderLangButton(){const b=document.getElementById('langToggle');if(b)b.textContent=uiLang==='zh'?'中':'EN'}
async function initLanguage(){const urlLang=readUrlLanguage();let lang=urlLang;if(!lang)lang=readStoredLanguage();if(!lang)lang=detectBrowserLanguage();applyLanguage(lang);if(document.body)document.body.classList.remove('preinit');if(!urlLang){fetch('/api/language',{cache:'no-store'}).then(r=>{if(!r.ok)return null;return r.json()}).then(j=>{if(!j)return;let srv=null;if(j.lang==='zh'||j.lang==='en')srv=normalizeLanguage(j.lang);else if(j.lang==='auto')srv=detectBrowserLanguage();if(srv&&srv!==uiLang){writeStoredLanguage(srv);applyLanguage(srv)}}).catch(()=>{})}}
function renderMuteButton(){const b=document.getElementById('muteToggle');if(b)b.classList.toggle('muted',uiMuted)}
async function initMute(){try{const r=await fetch('/api/mute',{cache:'no-store'});if(!r.ok)return;const j=await r.json();uiMuted=j.muted===1||j.muted===true;renderMuteButton()}catch(e){}}
async function toggleMute(){const next=uiMuted?0:1;try{const r=await fetch('/api/mute',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body:'muted='+next});if(!r.ok)return;const j=await r.json();uiMuted=j.muted===1||j.muted===true;renderMuteButton()}catch(e){}}
window.addEventListener('message',function(e){const d=e.data;if(!d)return;if(d.type==='dd-console-mute-changed'){uiMuted=!!d.muted;renderMuteButton()}else if(d.type==='dd-open-wifi-sta'){openWifiStaModal()}else if(d.type==='dd-open-wifi-ap'){openWifiApModal()}else if(d.type==='dd-open-joystick-cal'){openJoystickCalModal()}});
function toggleTheme(){setTheme(resolvedTheme()==='light'?'dark':'light')}
function setTheme(theme){uiTheme=theme;applyTheme()}
function readUrlTheme(){try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}function initTheme(){uiTheme=readUrlTheme()||'auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}

function toggleFabActions(e){if(e)e.stopPropagation();fabActions.classList.toggle('show')}
function collapseFabActions(){fabActions.classList.remove('show')}
function openHelpModal(){fabActions.classList.add('show');helpOverlay.classList.add('show');helpModal.classList.add('show')}
function closeHelpModal(){helpOverlay.classList.remove('show');helpModal.classList.remove('show')}
function parseJoystickCalStatus(text){const m=text.match(/JOYSTICK_CAL steer_en=(\d+) steer=\{(-?\d+),(-?\d+),(-?\d+)\} throt_en=(\d+) throt=\{(-?\d+),(-?\d+),(-?\d+)\} state=(\d+)/);if(!m)return null;return{steer_en:+m[1],steer_min:+m[2],steer_mid:+m[3],steer_max:+m[4],throt_en:+m[5],throt_min:+m[6],throt_mid:+m[7],throt_max:+m[8],state:+m[9]};}
function formatCalAxis(en,min,mid,max){return en?`${min} / ${mid} / ${max}`:'-- / -- / --';}
// 弹窗内 live 行与弹窗外状态行写同一份数值——此前 joystickCalLive 从未被写入，
// DONE 步骤"请检查下方数值"下方永远空白
async function refreshJoystickCalStatus(){try{const r=await fetch('/api/joystick-cal');const text=await r.text();const s=parseJoystickCalStatus(text);if(s){const txt=`${t('cal.label.steering')}: ${formatCalAxis(s.steer_en,s.steer_min,s.steer_mid,s.steer_max)} | ${t('cal.label.throttle')}: ${formatCalAxis(s.throt_en,s.throt_min,s.throt_mid,s.throt_max)}`;joystickCalStatus.textContent=txt;joystickCalLive.textContent=txt;renderCalStep(s.state);}else{joystickCalStatus.textContent=`${t('cal.label.steering')}: -- / -- / -- | ${t('cal.label.throttle')}: -- / -- / --`;joystickCalLive.textContent='';}}catch(e){joystickCalStatus.textContent=`${t('cal.label.steering')}: -- / -- / -- | ${t('cal.label.throttle')}: -- / -- / --`;joystickCalLive.textContent='';}}
function renderCalStep(state){joystickCalActionBtn.style.display=state===0?'inline-block':'none';joystickCalRetryBtn.style.display=(state===2||state===3)?'inline-block':'none';joystickCalSaveBtn.style.display=state===3?'inline-block':'none';if(state===0){joystickCalStepText.textContent=t('cal.step.center');joystickCalActionBtn.textContent=t('cal.action.start');}else if(state===1){joystickCalStepText.textContent=t('cal.step.center');}else if(state===2){joystickCalStepText.textContent=t('cal.step.minmax');}else if(state===3){joystickCalStepText.textContent=t('cal.step.done');joystickCalRetryBtn.textContent=t('cal.action.retry');joystickCalSaveBtn.textContent=t('cal.action.save');}}
function startCalPoll(){if(calPollTimer)return;calPollTimer=setInterval(refreshJoystickCalStatus,500);}
function stopCalPoll(){if(calPollTimer){clearInterval(calPollTimer);calPollTimer=0;}}
function openJoystickCalModal(){joystickCalModal.classList.add('show');refreshJoystickCalStatus();startCalPoll();}
async function closeJoystickCalModal(){const r=await fetch('/api/joystick-cal');const text=await r.text();const s=parseJoystickCalStatus(text);if(s&&s.state!==0&&s.state!==3){await fetch('/api/joystick-cal',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({action:'abort'})});}stopCalPoll();joystickCalModal.classList.remove('show');refreshJoystickCalStatus();}
async function sendAuthCommand(password){const r=await fetch('/api/cmd?target=web',{method:'POST',headers:{'Content-Type':'text/plain'},body:'AUTH:'+password});return (await r.text()).trim()==='AUTH_OK';}
async function postJoystickCalAction(action,allowRetry=true){const r=await fetch('/api/joystick-cal',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({action})});const text=await r.text();if(text.startsWith('NACK:UNAUTHORIZED')&&allowRetry){const pwd=prompt(t('cal.prompt.auth'));if(pwd&&await sendAuthCommand(pwd)){return postJoystickCalAction(action,false)}}if(text.startsWith('NACK')){showCommandError(text);return false}return true}
async function joystickCalAction(){if(await postJoystickCalAction('start'))refreshJoystickCalStatus();}
async function joystickCalRetry(){if(await postJoystickCalAction('retry'))refreshJoystickCalStatus();}
async function joystickCalSave(){if(await postJoystickCalAction('save')){stopCalPoll();joystickCalModal.classList.remove('show');refreshJoystickCalStatus();}}
const LOG_DISPLAY_MAX_BYTES=16000;
function canonicalLogSource(src){if(src==='serial'||src==='serial1')return src;return 'web';}
function appendLogLine(t,src){const s=canonicalLogSource(src||'web');let buf=sourceBuffers[s];buf+=t+'\n';while(buf.length>LOG_SOURCE_MAX_BYTES){const idx=buf.indexOf('\n');if(idx<0){buf=buf.slice(-LOG_SOURCE_MAX_BYTES);break;}buf=buf.substring(idx+1);}sourceBuffers[s]=buf;if(s!==currentLogSource||logPaused)return;log.textContent=buf.slice(-LOG_DISPLAY_MAX_BYTES);log.scrollTop=log.scrollHeight;}
function line(t){appendLogLine(t,'web');}
function switchLogSource(src){currentLogSource=canonicalLogSource(src||'web');cmdTarget.value=currentLogSource;const buf=sourceBuffers[currentLogSource];log.textContent=buf.length>0?buf.slice(-LOG_DISPLAY_MAX_BYTES):t('log.empty');log.scrollTop=log.scrollHeight;}
const CMD_TARGET_KEY='donkeydrifter.ui.cmdTarget';
let termInited=false,termSeq=0,termActive=0;const termList=[];
function terminalUrl(){var de=document.documentElement;return 'http://'+_launcherIp+':8090/terminal?theme='+(de.getAttribute('data-theme')||'dark');}
// #89：失败提示拼上 host_ip 上报年龄（>90s 视为过期，上位机正常 30s 上报一次）；
// 从未收到上报（age=-1，_launcherIp 还是默认回退值）时明确提示 IP 未知，不显示误导性的回退地址
function termFailHint(){if(_launcherIpAge===-1)return t('terminal.unknownIp');return t('terminal.unreachable')+terminalUrl()+(_launcherIpAge>90?' ('+t('terminal.staleIp').replace('{n}',_launcherIpAge)+')':'')}
// #89：探测改为可重入；失败后 scheduleTermRetry 每 4s 先刷新 _launcherIp（含年龄）再重探，
// 上位机恢复在线或 IP 更新后自动加载终端并清掉提示
// #101：loading 态加超时兜底——上位机 IP 不可达时 no-cors fetch 可能长时间挂起（TCP 无响应，
// reject 也不来），此前会无限期停在「正在连接上位机终端…」；未落定一律按 fail 处理，
// 复用失败提示与 4s 自动重试；探测序号防止旧探测的迟到结果覆盖新探测的状态。
// 首探前 addTerminalTab 先 await _fetchLauncherIp() 拿到真实上报 IP，避免用默认回退值
// 探测必然失败的窗口；超时由 10s 缩短为 5s，配合重试更快收敛
function probeTerminal(term){term._probe=(term._probe||0)+1;const seq=term._probe,ctrl=new AbortController(),timer=setTimeout(()=>ctrl.abort(),5000);const done=st=>{clearTimeout(timer);if(seq!==term._probe)return;if(st==='ok'){if(!term.f.src)term.f.src=terminalUrl();term.state='ok';if(termActive===term.id)terminalHint.textContent='';}else{term.state='fail';if(termActive===term.id)terminalHint.textContent=termFailHint();scheduleTermRetry();}};fetch('http://'+_launcherIp+':8090/api/status',{mode:'no-cors',cache:'no-store',signal:ctrl.signal}).then(()=>done('ok')).catch(()=>done('fail'));}
var _termRetryTimer=0;function scheduleTermRetry(){if(_termRetryTimer)return;_termRetryTimer=setInterval(async()=>{if(!termList.some(x=>x.state==='fail')){clearInterval(_termRetryTimer);_termRetryTimer=0;return}await _fetchLauncherIp();termList.forEach(x=>{if(x.state==='fail')probeTerminal(x)})},4000)}
// #149: default 'Terminal N' picks smallest free N; rename (num=null) or close frees it
function freeTermNumber(){let n=1;const used=new Set();termList.forEach(x=>{if(x.num)used.add(x.num)});while(used.has(n))n++;return n;}
function addTerminalTab(){termInited=true;const id=++termSeq,f=document.createElement('iframe');f.className='termFrame';f.title='host terminal';f.setAttribute('scrolling','no');terminalWrap.insertBefore(f,terminalHint);const b=document.createElement('button');b.className='termTab';const c=document.createElement('span');c.className='termTabClose';c.textContent='×';c.title=t('terminal.closeTab');c.onclick=e=>{e.stopPropagation();killTerminalTab(id)};b.appendChild(c);const num=freeTermNumber();const l=document.createElement('span');l.className='termTabLabel';l.textContent=t('terminal.tab')+' '+num;b.appendChild(l);b.onclick=()=>selectTerminalTab(id);termTabs.appendChild(b);const term={id:id,f:f,b:b,l:l,c:c,name:null,num:num,state:'loading'};termList.push(term);selectTerminalTab(id);fitTermTabLabels();updateTermTabClose();_fetchLauncherIp().then(()=>probeTerminal(term));}
function selectTerminalTab(id){termActive=id;termList.forEach(x=>{x.f.style.display=x.id===id?'':'none';x.b.classList.toggle('active',x.id===id)});const cur=termList.find(x=>x.id===id);terminalHint.textContent=!cur?t('terminal.empty'):cur.state==='loading'?t('terminal.loading'):cur.state==='fail'?termFailHint():'';}
function updateTermTabClose(){const hide=termList.length<=1;termList.forEach(x=>{x.c.style.display=hide?'none':''});}
function killTerminalTab(id){if(termList.length<=1)return;const i=termList.findIndex(x=>x.id===id);if(i<0)return;const cur=termList[i];cur.f.remove();cur.b.remove();termList.splice(i,1);fitTermTabLabels();updateTermTabClose();if(termList.length===0){termActive=0;terminalHint.textContent=t('terminal.empty')}else if(id===termActive)selectTerminalTab(termList[Math.min(i,termList.length-1)].id);}
// #90：先统一按长名（终端 N）测量一次，放不下才缩写为 N。
// 原实现按改名前的布局判 packed，临界宽度下长名↔短名来回振荡（改名后不复查，
// 用户看到长名+溢出，"功能没生效"）；现在每次触发都从长名状态重新测量，结果确定、可收敛
function fitTermTabLabels(){termList.forEach(x=>{if(!x.name&&x.num)x.l.textContent=t('terminal.tab')+' '+x.num});if(termTabs.scrollWidth>termTabs.clientWidth)termList.forEach(x=>{if(!x.name&&x.num)x.l.textContent=''+x.num});}
window.addEventListener('resize',fitTermTabLabels);
window.addEventListener('message',e=>{const d=e.data;if(!d||d.type!=='donkeydrifter.term.name'||typeof d.name!=='string')return;const cur=termList.find(x=>x.f.contentWindow===e.source);if(!cur||cur.name)return;cur.name=d.name;cur.num=null;cur.l.textContent=d.name;cur.b.title=d.name;});
function applyCmdTarget(src,save){src=src==='serial'?'serial':'web';cmdTarget.value=src;if(save!==false){try{localStorage.setItem(CMD_TARGET_KEY,src)}catch(e){}}const term=src==='serial';terminalWrap.style.display=term?'flex':'none';log.style.display=term?'none':'';newTermBtn.style.display=term?'':'none';termTabs.style.display=term?'flex':'none';pauseBtn.style.display=term?'none':'';sendBtn.style.display=term?'none':'';cmd.style.display=term?'none':'';if(term){if(!termInited)addTerminalTab();fitTermTabLabels()}else switchLogSource('web');}
function restoreCmdTarget(){let saved=null;try{saved=localStorage.getItem(CMD_TARGET_KEY)}catch(e){}applyCmdTarget(saved||'serial',false);}
function clearLog(){sourceBuffers[currentLogSource]='';log.textContent=t('log.empty')}
function togglePause(){logPaused=!logPaused;refreshDynamicLabels()}
function toggleChart(){chartPaused=!chartPaused;refreshDynamicLabels()}
function toggleChartFullscreen(){if(document.fullscreenElement===chartPanel)document.exitFullscreen();else chartPanel.requestFullscreen()}
function toggleTerminalFullscreen(){if(document.fullscreenElement===terminalWrap)document.exitFullscreen();else terminalWrap.requestFullscreen()}
document.addEventListener('fullscreenchange',()=>{refreshDynamicLabels();gridReady=false;draw()});
document.addEventListener('click',collapseFabActions);
window.addEventListener('scroll',collapseFabActions,{passive:true});
window.addEventListener('touchmove',collapseFabActions,{passive:true});
// 清空会把 tubSamples 一并清掉并停录：补 refreshDynamicLabels 让录制按钮从"停止录制"回弹，
// 正在录制时被清空额外提示一行，避免用户以为还在录
function clearChart(){const wasRecording=tubRecording;pointHead=0;pointCount=0;points.fill(null);scrollOffset=0;smoothedDt=16;gridReady=false;tubSamples=[];tubStartedMs=0;tubStoppedMs=0;tubLastSeq=0;tubRecording=false;updateTubMeta();refreshDynamicLabels();if(wasRecording)line(t('tub.clearedWhileRecording'));draw()}function enterScreenSaver(){screenSaverActive=true;screenSaverStartTime=performance.now();saverTime=0}function exitScreenSaver(){screenSaverActive=false;screenSaverStartTime=0;parkLockedAt=0;ch1Samples=[];clearChart()}
function toggleFold(id){const f=document.getElementById(id);if(!f)return;const open=!f.classList.contains('open');f.classList.toggle('open',open);const i=f.querySelector('.foldIcon'),b=f.querySelector('.foldHead');if(i)i.textContent=open?'▾':'▸';if(b)b.setAttribute('aria-expanded',open?'true':'false')}
function parseStatusPairs(t){const pairs=[],n=t.length;let i=0;while(i<n){while(i<n&&/\s/.test(t[i]))i++;let k='';while(i<n&&!/\s|=/.test(t[i]))k+=t[i++];if(!k||t[i]!=='='){while(i<n&&!/\s/.test(t[i]))i++;continue}i++;let v='';if(t[i]==='\"'){const q=t[i++];while(i<n&&t[i]!==q)v+=t[i++];if(i<n&&t[i]===q)i++}else{while(i<n&&!/\s/.test(t[i]))v+=t[i++]}pairs.push([k,v])}return pairs}
function parseStatusText(t){const m={};parseStatusPairs(t).forEach(p=>m[p[0]]=p[1]);return m}
function renderStatus(t){const pairs=parseStatusPairs(t);statusBox.textContent='';if(!pairs.length){statusBox.textContent=t;return}const table=document.createElement('div');table.className='statusTable';pairs.forEach(p=>{const r=document.createElement('div'),k=document.createElement('b'),v=document.createElement('span');r.className='statusRow';k.textContent=p[0];v.textContent=p[1];r.appendChild(k);r.appendChild(v);table.appendChild(r)});statusBox.appendChild(table)}
function toggleTub(){if(tubRecording)te();else ts()}
function updateTubMeta(){if(tubMeta)tubMeta.textContent=tubSamples.length}
function ts(){tubSamples=[];tubStartedMs=0;tubStoppedMs=0;tubLastSeq=0;tubRecording=true;updateTubMeta();refreshDynamicLabels()}
function te(){if(!tubRecording)return;tubRecording=false;tubStoppedMs=tubSamples.length?tubSamples[tubSamples.length-1].t:tubStartedMs;updateTubMeta();refreshDynamicLabels()}
// 录制只要求点本身存在：帧内历史点是紧凑形（seq/t/dt/thr/str/gz，无 ch1~ch6），
// 若仍以 ch6 存在为门槛则批量帧录不进；下游转换脚本对样本缺失字段填 0 容错
function tp(p){if(!tubRecording||!p)return;const s=Number(p.seq||0);if(s&&s===tubLastSeq)return;if(!tubSamples.length)tubStartedMs=Number(p.t||0);tubSamples.push(p);tubLastSeq=s;tubStoppedMs=Number(p.t||tubStoppedMs);updateTubMeta();if(tubSamples.length>=TUB_MAX_SAMPLES)te()}
function td(){if(!tubSamples.length)return;const x=tubRecording?(tubSamples[tubSamples.length-1].t||tubStartedMs):tubStoppedMs,p={schema:TUB_SCHEMA,source:'mus4-web-console',started_ms:tubStartedMs,stopped_ms:x,count:tubSamples.length,samples:tubSamples},b=new Blob([JSON.stringify(p)],{type:'application/json'}),a=document.createElement('a');a.href=URL.createObjectURL(b);a.download='mus4-tub.json';document.body.appendChild(a);a.click();setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove()},0)}
function showToast(t,ok=true){toast.textContent=t;toast.style.borderColor=ok?CHART_THEMES[resolvedTheme()].toastOk:CHART_THEMES[resolvedTheme()].toastErr;toast.classList.add('show');clearTimeout(toastTimer);toastTimer=setTimeout(()=>toast.classList.remove('show'),1600)}
function fallbackCopyText(t){const a=document.createElement('textarea');a.value=t;a.style.position='fixed';a.style.left='-9999px';document.body.appendChild(a);a.focus();a.select();let ok=false;try{ok=document.execCommand('copy')}catch(e){ok=false}a.remove();return ok}
async function copyNetworkIp(){const ip=networkCopyIp;if(!ip||ip==='--'||ip==='0.0.0.0'||ip==='disabled'){showToast(t('toast.copyFailed'),false);return}try{if(navigator.clipboard&&navigator.clipboard.writeText)await navigator.clipboard.writeText(ip);else if(!fallbackCopyText(ip))throw new Error('copy failed');showToast(t('toast.copiedIp')+ip,true)}catch(e){if(fallbackCopyText(ip))showToast(t('toast.copiedIp')+ip,true);else showToast(t('toast.copyFailed'),false)}}
function setNetworkTab(t){networkTab=t;networkTabPinned=true;updateNetworkCard.last&&updateNetworkCard(updateNetworkCard.last)}
function selectedNetworkTab(){const s=updateNetworkCard.last||{};return networkTabPinned?networkTab:(s.sta_connected==='1'?'sta':'ap')}
function wifiStaSaveSource(){const s=updateNetworkCard.last||{},h=location.hostname,ap=s.ap_ip||'192.168.4.1';return h===ap||h==='192.168.4.1'?'ap':'sta'}
function openNetworkSettings(){const selected=selectedNetworkTab();selected==='ap'?openWifiApModal():openWifiStaModal()}
function netIpValid(v){return !!v&&v!=='--'&&v!=='0.0.0.0'&&v.toLowerCase()!=='disabled'}function updateNetworkCard(s){updateNetworkCard.last=s;const ap=s.ap_ip||'--',sta=s.sta_ip||'0.0.0.0',apSsid=s.ap_ssid||'MUS4-ESP',staSsid=s.sta_ssid||'--',hostIp=s.host_ip||'',configured=s.sta_configured==='1',staConnected=s.sta_connected==='1',selected=networkTabPinned?networkTab:(staConnected?'sta':'ap');networkApTab.classList.toggle('active',selected==='ap');networkStaTab.classList.toggle('active',selected==='sta');networkHostTab.classList.toggle('active',selected==='host');networkGear.style.display=selected==='host'?'none':'';if(selected==='host'){networkCopyIp=hostIp;networkValue.textContent=hostIp||'--';networkSsidValue.textContent='Serial2';networkCard.className='stateCard '+(hostIp?'mode0':'netDown')}else if(selected==='ap'){networkCopyIp=ap;networkValue.textContent=ap;networkSsidValue.textContent=apSsid;networkCard.className='stateCard '+(netIpValid(ap)?'mode0':'netDown')}else{networkCopyIp=configured?sta:'';networkValue.textContent=configured?sta:'disabled';networkSsidValue.textContent=configured?staSsid:'--';networkCard.className='stateCard '+(staConnected?'mode0':'driftOff')}networkValue.classList.toggle('copyValue',netIpValid(networkCopyIp));if(staConnected&&sta&&sta!=='0.0.0.0'&&sta!=='--'&&configured&&window.handoffShownForStaIp!==sta){line('[sta-debug] updateNetworkCard sees STA IP, mark handoff shown only');/* 只标记不弹窗：页面加载/5s 状态刷新时 STA 已连接是常态（用户多半已在新地址浏览），弹窗只出现在配网等待流程（refreshWifiSta / waitWifiStaConnectionResult 路径） */window.handoffShownForStaIp=sta}if(s.version){versionLabel.textContent=s.version.replace(/^V/,'v')}}
function showReconnectOverlay(){if(reconnectOverlay)reconnectOverlay.classList.add('show')}
function hideReconnectOverlay(){if(reconnectOverlay)reconnectOverlay.classList.remove('show')}
function manualReconnect(){if(reconnectOverlay)reconnectOverlay.classList.add('show');tryReconnect()}
function semText(n,f){try{const v=getComputedStyle(document.documentElement).getPropertyValue(n);return (v&&v.trim())||f}catch(e){return f}}
async function tryReconnect(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw new Error('status not ok');const t=await r.text();renderStatus(t);updateNetworkCard(parseStatusText(t));connectionLost=false;hideReconnectOverlay();line('[sta-debug] reconnected');return true}catch(e){return false}}
async function refreshStatus(){try{const r=await fetch('/api/status');const t=await r.text();renderStatus(t);updateNetworkCard(parseStatusText(t));if(connectionLost){connectionLost=false;hideReconnectOverlay();line('[sta-debug] connection restored')}}catch(e){if(!connectionLost){connectionLost=true;showReconnectOverlay();line('[sta-debug] connection lost, start reconnect')}if(!reconnectTimer){reconnectTimer=setInterval(async ()=>{if(await tryReconnect()){clearInterval(reconnectTimer);reconnectTimer=0}},3000)}}}
async function pollLog(){try{const r=await fetch('/api/log?since='+lastLogSeq);const j=await r.json();for(const e of j.entries){lastLogSeq=Math.max(lastLogSeq,e.seq);appendLogLine('['+e.t+']['+e.src+'] '+e.line,e.src)}}catch(e){line('log error: '+e)}}
let lastRcDomUpdate=0;function updateState(p){const modes={0:[t('mode.value.rc'),t('mode.manual')],1:[t('mode.value.assist'),t('mode.assist')],2:[t('mode.value.auto'),t('mode.auto')]},m=modes[p.mode]||['MODE '+p.mode,t('mode.unknown')];modeCard.className='stateCard mode'+p.mode;modeValue.textContent=m[0];modeSub.textContent=m[1];parkCard.className='stateCard '+(p.park?'parkLocked':'parkUnlocked');parkValue.textContent=p.park?t('park.locked'):t('park.unlocked');parkSub.textContent=p.park?t('park.guarded'):t('park.enabled');const de=!!p.de,da=!!p.da,dc=Number(p.dc||0),gzf=Number(p.gzf||0);driftCard.className='stateCard '+(!de?'driftOff':da?'driftActive':'driftArmed');driftValue.textContent=!de?t('drift.off'):da?t('drift.active'):t('drift.armed');driftSub.textContent='comp='+dc.toFixed(1)+' gzf='+gzf.toFixed(2);{const _np=Math.max(0,Math.min(100,(Math.max(-70,Math.min(70,dc))+70)*100/140));driftNeedle.style.left='0';driftNeedle.style.transform='translateX('+(_np/100*(driftNeedle.parentElement?driftNeedle.parentElement.clientWidth:0))+'px)'}if(performance.now()-lastRcDomUpdate>200){[p.ch1,p.ch2,p.ch3,p.ch4,p.ch5,p.ch6].forEach((v,i)=>chValues[i].textContent=v??'----');lastRcDomUpdate=performance.now()}const sd=Number(p.sd),ed=Number(p.ed),sdEl=document.getElementById('servoDutyValue'),edEl=document.getElementById('escDutyValue');if(sdEl)sdEl.textContent=!isNaN(sd)?sd:'----';if(edEl)edEl.textContent=!isNaN(ed)?ed:'----';const sm=Number(p.sm),mm=Number(p.mm),smEl=document.getElementById('servoMidValue'),mmEl=document.getElementById('motorMidValue');if(smEl)smEl.textContent=!isNaN(sm)?sm:'----';if(mmEl)mmEl.textContent=!isNaN(mm)?mm:'----';const tl=Number(p.tl),tu=Number(p.tu),mmv=Number(p.mm),tlEl=document.getElementById('throttleMinValue'),tuEl=document.getElementById('throttleMaxValue'),tls=document.getElementById('throttleMinSlider'),tus=document.getElementById('throttleMaxSlider');if(tlEl&&document.activeElement!==tlEl)tlEl.value=!isNaN(tl)?tl:'';if(tuEl&&document.activeElement!==tuEl)tuEl.value=!isNaN(tu)?tu:'';if(tls&&!isNaN(mmv)){tls.max=mmv;if(tlEl)tlEl.max=mmv;if(!isNaN(tl))tls.value=tl;}if(tus&&!isNaN(mmv)){tus.min=mmv;if(tuEl)tuEl.min=mmv;if(!isNaN(tu))tus.value=tu;}const v=Number(p.vol);if(!isNaN(v)&&v>=5){voltageValue.textContent=v.toFixed(1)+'V';const pct=Math.max(0,Math.min(100,Math.round((v-10.5)/(12.6-10.5)*100)));voltageSub.textContent=pct+'%';voltageCard.className='stateCard '+(pct>30?'mode0':pct>15?'driftArmed':'driftOff')}else{voltageValue.textContent='--';voltageSub.textContent='--';voltageCard.className='stateCard driftOff'}const parkLocked=!!p.park;if(parkLocked){if(parkLockedAt===0)parkLockedAt=performance.now()}else{parkLockedAt=0;if(screenSaverActive)exitScreenSaver()}const now=performance.now();const ch1Val=Number(p.ch1);if(!isNaN(ch1Val)){ch1Samples.push({t:now,v:ch1Val});while(ch1Samples.length>0&&now-ch1Samples[0].t>60000)ch1Samples.shift();if(ch1Samples.length>=2){let minCh1=Infinity,maxCh1=-Infinity;for(const s of ch1Samples){if(s.v<minCh1)minCh1=s.v;if(s.v>maxCh1)maxCh1=s.v}const range=maxCh1-minCh1;if(!screenSaverActive&&parkLockedAt>0&&now-parkLockedAt>=60000&&range<10){enterScreenSaver()}else if(screenSaverActive&&range>=10){exitScreenSaver()}}else if(screenSaverActive&&ch1Samples.length===1){const last=ch1Samples[0].v;if(Math.abs(ch1Val-last)>=10){exitScreenSaver()}}}}
let drawPending=false;function scheduleDraw(){if(drawPending)return;drawPending=true;requestAnimationFrame(()=>{drawPending=false;draw()})}// 服务端遥测/日志 seq 是 RAM 计数器，设备重启归零；lastDataSeq/lastLogSeq 只增不减会把重启后的
// 新数据判成"已读"永久静默——/api/data 的 latest 不受 since 过滤，seq 回退即可靠检出并重置续收
function resetDataSeqOnRollback(seq){if(typeof seq!=='number'||seq>=lastDataSeq)return false;lastDataSeq=0;lastLogSeq=0;line(t('data.seqReset'));return true}
// 帧内历史点也逐个进 tub 录制（此前只录 latest，一帧多点全丢）；points 末点与 latest 同 seq 时
// 由 tp 内的 tubLastSeq 去重吸收，TUB_MAX_SAMPLES 自动停语义不变
function handleDataPayload(j,transport,elapsed){if(j.latest)resetDataSeqOnRollback(j.latest.seq);const arr=j.points||[];let latest=j.latest||null;let added=0;for(const p of arr){p.req=transport==='ws'?0:elapsed;lastDataSeq=Math.max(lastDataSeq,p.seq||0);tp(p);if(!chartPaused&&!screenSaverActive){addPoint(p);added++}}if(latest){lastDataSeq=Math.max(lastDataSeq,latest.seq||0);updateState(latest);tp(latest)}const p=latest||latestPoint();dataTransport=transport;if(p){thrMeta.textContent=p.thr;strMeta.textContent=p.str;gzMeta.textContent=Number(p.gz||0).toFixed(3)}if(added>0)scheduleDraw()}
function decodeBinaryDataPayload(buffer){const v=new DataView(buffer);let o=0;const u8=()=>v.getUint8(o++),u16=()=>{const x=v.getUint16(o,true);o+=2;return x},u32=()=>{const x=v.getUint32(o,true);o+=4;return x},i16=()=>{const x=v.getInt16(o,true);o+=2;return x},f32=()=>{const x=v.getFloat32(o,true);o+=4;return x};if(u8()!==77||u8()!==52)throw new Error('bad magic');const version=u8();u8();if(version!==2)throw new Error('bad version');const dropped=u32(),seq=u32(),t=u32(),dt=u16(),thr=i16(),str=i16(),gz=f32(),gx=f32(),gy=f32(),ax=f32(),ay=f32(),az=f32(),mode=u8(),park=u8();const ch=[u16(),u16(),u16(),u16(),u16(),u16()];const latest={seq,t,dt,thr,str,gz,gx,gy,ax,ay,az,mode,park,ch1:ch[0],ch2:ch[1],ch3:ch[2],ch4:ch[3],ch5:ch[4],ch6:ch[5],rct:i16(),rcs:i16(),pt:i16(),ps:i16(),gzf:f32(),dc:f32(),de:u8(),da:u8(),vol:f32(),pseudoSpeed:f32(),sd:u16(),ed:u16(),sm:u16(),mm:u16(),tl:i16(),tu:i16()};const count=u8(),points=[];for(let i=0;i<count;i++)points.push({seq:u32(),t:u32(),dt:u16(),thr:i16(),str:i16(),gz:f32()});return{type:'data',dropped,latest,points}}
function dataWsUrl(){return (location.protocol==='https:'?'wss:':'ws:')+'//'+location.hostname+':81/'}
function scheduleDataWsReconnect(){if(dataWsReconnectTimer)return;dataWsReconnectTimer=setTimeout(()=>{dataWsReconnectTimer=0;connectDataSocket();dataWsReconnectDelay=Math.min(8000,dataWsReconnectDelay*2)},dataWsReconnectDelay)}
function connectDataSocket(){try{if(dataWs&&dataWs.readyState!==WebSocket.CLOSED)return;if(dataWs){dataWs.onclose=null;dataWs.onerror=null;try{dataWs.close()}catch(e){}}const ws=new WebSocket(dataWsUrl());dataWs=ws;ws.binaryType='arraybuffer';ws.onopen=()=>{if(dataWs!==ws){ws.close();return}dataWsConnected=true;dataWsReconnectDelay=1000;dataTransport='ws';ws.send('since:'+lastDataSeq)};ws.onmessage=e=>{if(dataWs!==ws)return;try{if(typeof e.data==='string'){const j=JSON.parse(e.data);if(j&&j.type==='log'&&j.line!==undefined){lastLogSeq=Math.max(lastLogSeq,j.seq||0);appendLogLine('['+j.t+']['+j.src+'] '+j.line,j.src);return}if(j&&j.type==='hello'){resetDataSeqOnRollback(j.seq);return}return}if(e.data instanceof ArrayBuffer){handleDataPayload(decodeBinaryDataPayload(e.data),'ws',0);return}if(e.data instanceof Blob){e.data.arrayBuffer().then(b=>{if(dataWs===ws)handleDataPayload(decodeBinaryDataPayload(b),'ws',0)}).catch(err=>line('ws parse error: '+err));return}}catch(err){line('ws parse error: '+err)}};ws.onclose=()=>{if(dataWs!==ws)return;dataWsConnected=false;dataWs=null;scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollData,2000)};ws.onerror=()=>{if(dataWs!==ws)return;dataWsConnected=false;try{ws.close()}catch(e){}}}catch(e){dataWsConnected=false;dataWs=null;line('ws error: '+e);scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollData,2000)}}
async function pollData(){if(dataWsConnected)return;if(dataPolling)return;dataPolling=true;let delay=60;const start=performance.now();try{const r=await fetch('/api/data?since='+lastDataSeq);const j=await r.json();const elapsed=performance.now()-start;handleDataPayload(j,'poll',elapsed);delay=(j.points||[]).length?Math.max(30,Math.min(80,Math.round(elapsed*1.2))):100}catch(e){delay=160;line('data error: '+e)}finally{dataPolling=false;if(!dataWsConnected)setTimeout(pollData,delay)}}
// 新契约：受保护端点鉴权失败统一 403 JSON {"error":"auth_required"}，与大写 NACK 同路；
// JOYSTICK_* 两个 NACK 给具体指引；其余 NACK/JSON error 串兜底原文展示，不再静默吞掉
function explainCommandError(text){if(!text)return '';const s=String(text).trim();if(!s)return '';if(s.includes('PARK_REQUIRED'))return t('error.parkRequired');if(s.includes('AUTH_REQUIRED')||s.includes('UNAUTHORIZED')||s.includes('auth_required'))return t('error.authRequired');if(s.includes('JOYSTICK_INVALID_RANGE'))return t('error.joystickInvalidRange');if(s.includes('JOYSTICK_SAVE_FAILED'))return t('error.joystickSaveFailed');if(s.startsWith('NACK')||s.includes('"error"'))return s;return ''}
function showCommandError(text){const msg=explainCommandError(text);if(msg)alert(msg)}
async function setServoMid(){const sd=document.getElementById('servoDutyValue').textContent;if(sd&&sd!=='----'){cmd.value='SERVO_MID '+sd;await sendCmd()}}
async function setMotorMid(){const ed=document.getElementById('escDutyValue').textContent;if(ed&&ed!=='----'){cmd.value='MOTOR_MID '+ed;await sendCmd()}}
function setThrottleMin(v){cmd.value='THROTTLE_MIN '+v;sendCmd()}
function setThrottleMax(v){cmd.value='THROTTLE_MAX '+v;sendCmd()}
function commitThrottleLimit(el,slider,setter){const s=document.getElementById(slider);const v=parseInt(el.value,10);if(isNaN(v)){el.value=s.value;return}const c=Math.max(Number(s.min),Math.min(Number(s.max),v));el.value=c;s.value=c;window[setter](c)}
function commitThrottleMin(el){commitThrottleLimit(el,'throttleMinSlider','setThrottleMin')}
function commitThrottleMax(el){commitThrottleLimit(el,'throttleMaxSlider','setThrottleMax')}
async function sendCmd(){const v=cmd.value.trim();if(!v)return;const target=(cmdTarget?cmdTarget.value:'web')||'web';const r=await fetch('/api/cmd?target='+encodeURIComponent(target),{method:'POST',headers:{'Content-Type':'text/plain'},body:v});const t=await r.text();showCommandError(t);cmd.value='';refreshStatus();setTimeout(pollLog,200)}
function renderDevMode(v){uiDevMode=!!v;const b=document.getElementById('devModeToggle');if(b){b.classList.toggle('devOn',uiDevMode);b.setAttribute('aria-checked',uiDevMode?'true':'false')}}function toggleDevModeFromSwitch(){if(uiDevMode){setDevMode(false)}else{devModeModal.classList.add('show')}}
async function refreshDevMode(){try{const r=await fetch('/api/devmode');const j=await r.json();renderDevMode(!!j.enabled)}catch(e){}}
function closeDevModeModal(ok){devModeModal.classList.remove('show');if(ok)setDevMode(true)}
async function setDevMode(v){try{const r=await fetch('/api/devmode',{method:'POST',headers:{'Content-Type':'text/plain'},body:v?'1':'0'});if(!r.ok)throw new Error(await r.text());const j=await r.json();renderDevMode(!!j.enabled);refreshStatus()}catch(e){line('dev mode error: '+e);refreshDevMode()}}





async function refreshWifiAp(){try{const r=await fetch('/api/wifi-ap');const j=await r.json();apSsid.value=j.ssid||'';return j}catch(e){line('ap config error: '+e);return null}}
function updateApPreview(){const raw=apSsid.value;const clean=raw.replace(/[^A-Za-z0-9]/g,'');apSsid.value=clean;const p=clean.trim();apPreview.textContent=p?t('wifi.apPreview')+p+'-ESP':'';if(!apSaving){if(raw!==clean){apNotice.textContent=t('wifi.apNotice');apNotice.style.display=''}else apNotice.style.display='none'}}async function openWifiApModal(){apNotice.textContent=t('wifi.apNotice');apNotice.style.display='none';apSaveBtn.disabled=false;apSaveBtn.textContent=t('wifi.saveRestartAp');await refreshWifiAp();const s=apSsid.value;if(s&&s.endsWith('-ESP'))apSsid.value=s.slice(0,-4);updateApPreview();wifiApModal.classList.add('show')}
function closeWifiApModal(){if(apSaving)return;wifiApModal.classList.remove('show')}
async function saveWifiAp(){if(apSaving)return;const v=apSsid.value.trim();if(!v||!/^([A-Za-z0-9]{1,6})$/.test(v)){apNotice.textContent=t('wifi.apInvalid');apNotice.style.display='';return}try{apSaving=true;apSaveBtn.disabled=true;apSaveBtn.textContent=t('wifi.apSaving');apNotice.textContent=t('wifi.apSavingNotice');apNotice.style.display='';const body=new URLSearchParams();body.set('ssid',v);const r=await fetch('/api/wifi-ap',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!r.ok){const txt=await r.text();showCommandError(txt);apNotice.style.display='';if(txt.includes('invalid_ssid'))apNotice.textContent=t('wifi.apInvalid');else apNotice.textContent=t('wifi.apSaveFailed');throw new Error(txt)}showToast(t('wifi.apSaved'),true);wifiApModal.classList.remove('show');refreshStatus()}catch(e){line('wifi ap save error: '+e)}finally{apSaving=false;apSaveBtn.disabled=false;apSaveBtn.textContent=t('wifi.saveRestartAp')}}
function isWifiStaModalOpen(){return wifiStaModal.classList.contains('show')}
function updateStaPasswordEye(){staPasswordEye.textContent=staPasswordVisible?'🙈':'👁';staPasswordEye.title=staPasswordVisible?t('wifi.hidePassword'):t('wifi.showPassword')}
// SSID 输入框已被改动（≠ 服务端当前配置）时跳过密码掩码回填：否则 5s 轮询会把旧配置的
// 掩码填回来，staSsid 监听刚清掉的占位被复活，saveWifiSta 又会走 keep_password=1 拿旧密码连新网
function renderStaPasswordState(j,force=false){if(!j)return;if(!force&&(document.activeElement===staPassword||staPasswordDirty||staSsid.value.trim()!==(j.ssid||'')))return;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;if(j.password_set&&Number(j.password_len||0)>0){staPassword.value='*'.repeat(Number(j.password_len||0));staPassword.type='password';staPasswordPlaceholder=true}else{staPassword.value='';staPassword.type='password';staPasswordPlaceholder=false}staPasswordDirty=false;updateStaPasswordEye()}
async function refreshWifiSta(forceFill=false){try{const r=await fetch('/api/wifi-sta');const j=await r.json();if(forceFill||(!isWifiStaModalOpen()&&document.activeElement!==staSsid))staSsid.value=j.ssid||'';renderStaPasswordState(j,forceFill);if(!j.connected){window.handoffShownForStaIp=''}return j}catch(e){line('sta config error: '+e);return null}}
function openWifiScanPopover(e){if(e)e.stopPropagation();wifiScanPopover.classList.add('show');refreshWifiScan();if(!wifiScanTimer)wifiScanTimer=setInterval(refreshWifiScan,1000)}
function closeWifiScanPopover(){wifiScanPopover.classList.remove('show');if(wifiScanTimer){clearInterval(wifiScanTimer);wifiScanTimer=0}wifiScanBusy=false}
async function refreshWifiScan(){if(wifiScanBusy)return;wifiScanBusy=true;try{const r=await fetch('/api/wifi-sta/scan');const j=await r.json();const nets=(j.networks||[]).sort((a,b)=>(b.rssi||-999)-(a.rssi||-999));wifiScanList.textContent='';if(!nets.length){wifiScanStatus.textContent=j.scanning?t('wifi.scanning'):t('wifi.scanNone')}else{wifiScanStatus.textContent=j.scanning?t('wifi.scanning'):t('wifi.scanSelect');nets.forEach(n=>{const b=document.createElement('button'),m=document.createElement('span');b.className='scanRow';b.type='button';b.onclick=()=>selectWifiSsid(n.ssid,n.channel);b.textContent=n.ssid;m.className='scanMeta';m.textContent=(n.rssi||0)+' dBm CH'+(n.channel||'?')+(n.secure?' 🔒':' OPEN');b.appendChild(m);wifiScanList.appendChild(b)})}}catch(e){wifiScanStatus.textContent=t('wifi.scanFailed');line('wifi scan error: '+e)}finally{wifiScanBusy=false}}
function selectWifiSsid(ssid,channel){staSelectedChannel=Number(channel)||0;staSsid.value=ssid;staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();closeWifiScanPopover();resetStaConnectBtn();staPassword.focus()}
async function fetchSavedStaPassword(){const r=await fetch('/api/wifi-sta/password');if(!r.ok){showCommandError(await r.text());return null}const j=await r.json();staSavedPassword=j.password||'';staSavedPasswordKnown=true;return staSavedPassword}
function resetStaConnectBtn(){const b=document.getElementById('staConnectBtn');if(b&&document.getElementById('hostWifiToggle').checked){b.textContent=t('wifi.hostSendBtn');b.onclick=saveHostWifi}}
function maskStaPassword(){if(staSavedPasswordKnown&&!staPasswordDirty){staPassword.value='*'.repeat(staSavedPassword.length);staPasswordPlaceholder=staSavedPassword.length>0}else if(staPasswordPlaceholder){staPassword.value='*'.repeat(staPassword.value.length)}staPassword.type='password';staPasswordVisible=false;updateStaPasswordEye()}
async function toggleStaPasswordVisibility(){if(staPasswordVisible){maskStaPassword();return}if(staPasswordPlaceholder&&!staPasswordDirty){const p=await fetchSavedStaPassword();if(p===null)return;staPassword.value=p;staPasswordPlaceholder=false}else{staSavedPassword='';staSavedPasswordKnown=false}staPassword.type='text';staPasswordVisible=true;updateStaPasswordEye()}
async function openWifiStaModal(){closeWifiScanPopover();staNotice.textContent=t('wifi.staNotice');document.getElementById('hostWifiToggle').checked=true;onHostWifiToggle();await refreshWifiSta(true);wifiStaModal.classList.add('show');refreshWifiHistory()}
function closeWifiStaModal(){closeWifiScanPopover();maskStaPassword();stopHostWifiPoll();wifiStaModal.classList.remove('show')}
function closeWifiStaFailureModal(){wifiStaFailureModal.classList.remove('show')}
async function refreshWifiHistory(){if(!isWifiStaModalOpen())return;const list=document.getElementById('wifiHistoryList'),empty=document.getElementById('wifiHistoryEmpty');if(!list||!empty)return;try{const hr=await fetch('/api/wifi-sta/history');const hj=await hr.json();const sr=await fetch('/api/wifi-sta');const sj=await sr.json().catch(()=>({}));const currentSsid=sj&&sj.connected?(sj.ssid||''):'';const entries=hj.entries||[];list.textContent='';if(!entries.length){empty.style.display='block';return}empty.style.display='none';entries.forEach(e=>{const row=document.createElement('div'),rank=document.createElement('span'),name=document.createElement('span'),del=document.createElement('button');row.className='histRow';row.title=t('wifi.historyFill');row.onclick=()=>selectWifiHistory(e.ssid||'',!!e.password_set);rank.className='histRank';rank.textContent='#'+(e.rank||0);name.className='histSsid';name.textContent=e.ssid||'';del.className='histDel';del.type='button';del.title=t('wifi.historyDelete');del.setAttribute('aria-label',t('wifi.historyDelete'));del.textContent='🗑';del.onclick=ev=>{ev.stopPropagation();deleteWifiHistoryEntry(e.ssid,currentSsid!==''&&e.ssid===currentSsid)};row.appendChild(rank);row.appendChild(name);row.appendChild(del);list.appendChild(row)})}catch(e){line('wifi history error: '+e)}}
async function deleteWifiHistoryEntry(ssid,isCurrent){if(!confirm(t('wifi.historyDeleteConfirm')))return;try{const body=new URLSearchParams();body.set('ssid',ssid);const r=await fetch('/api/wifi-sta/history/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!r.ok){const msg=await r.text();showCommandError(msg);throw new Error(msg)}if(isCurrent)showToast(t('wifi.historyKeepCurrentNote'),true);await refreshWifiHistory()}catch(e){line('wifi history delete error: '+e)}}
async function selectWifiHistory(ssid,passwordSet){if(!ssid)return;staSelectedChannel=0;staSsid.value=ssid;staPasswordVisible=false;staPasswordPlaceholder=false;staSavedPassword='';staSavedPasswordKnown=false;staPassword.type='password';if(passwordSet){try{const r=await fetch('/api/wifi-sta/password?ssid='+encodeURIComponent(ssid));if(!r.ok){showCommandError(await r.text());return}const j=await r.json();staPassword.value=j.password||''}catch(e){line('wifi history password error: '+e);return}}else{staPassword.value=''}staPasswordDirty=true;updateStaPasswordEye();resetStaConnectBtn();staConnectBtn.focus()}
function closeWifiStaHandoffModal(skip){wifiStaHandoffModal.classList.remove('show');wifiStaModal.classList.remove('show');if(skip)try{localStorage.setItem('mus4.wifi.staHintSkip',window.handoffShownForStaIp||'')}catch(e){}}
function handoffStaUrl(j){const ip=(j&&j.sta_ip)||(j&&j.handoff_sta_ip)||'';return ip&&ip!=='0.0.0.0'?'http://'+ip+'/':''}
function showWifiStaHandoffModal(j){const target=(j&&j.ssid)||(j&&j.handoff_target_ssid)||staSsid.value.trim()||'--',ap=(j&&j.handoff_ap_ssid)||'--',url=handoffStaUrl(j),ip=url.replace(/^http:\/\//,'').replace(/\/$/,''),connecting=!!(j&&j.connecting);if(!connecting&&ip){if(window.handoffShownForStaIp===ip)return;try{if(localStorage.getItem('mus4.wifi.staHintSkip')===ip)return}catch(e){}}wifiStaHandoffText.textContent=(connecting?t('wifi.handoffConnecting'):t('wifi.handoffSwitch'))+target+'\n'+t('wifi.handoffLanIp')+(ip||t('wifi.handoffWaitingIp'))+'\n'+t('wifi.handoffUrl')+(url||t('wifi.handoffWaitingIp'))+'\n'+t('wifi.handoffHint')+ap+t('wifi.handoffHintTail');wifiStaHandoffModal.classList.add('show');if(!connecting&&ip)window.handoffShownForStaIp=ip}
async function copyHandoffIp(){const j=await refreshWifiSta(false),url=handoffStaUrl(j);if(!url){showToast(t('toast.newUrlUnavailable'),false);return}try{if(navigator.clipboard&&navigator.clipboard.writeText)await navigator.clipboard.writeText(url);else if(!fallbackCopyText(url))throw new Error('copy failed');showToast(t('toast.copiedUrl')+url,true)}catch(e){if(fallbackCopyText(url))showToast(t('toast.copiedUrl')+url,true);else showToast(t('toast.copyFailed'),false)}}
async function openHandoffUrl(){const j=await refreshWifiSta(false),url=handoffStaUrl(j);if(!url){showToast(t('toast.newUrlUnavailable'),false);return}location.href=url}
function showWifiStaFailureModal(j){const ssid=(j&&j.ssid)||staSsid.value.trim()||'--',reason=(j&&j.last_error_message)||t('wifi.failureReasonDefault');wifiStaFailureText.textContent=t('wifi.failureSsidLabel')+ssid+'\n'+t('wifi.failureReasonLabel')+reason+'\n'+t('wifi.failureAdvice');wifiStaFailureModal.classList.add('show')}
async function probeStaConsoleUrl(url){try{await fetch(url,{mode:'no-cors',cache:'no-store'});return true}catch(e){return false}}
async function redirectToStaConsole(ip){const url='http://'+ip+'/';staNotice.textContent=t('wifi.staConnectedIp')+ip+t('wifi.staSwitchAndOpen')+url;showToast(t('wifi.staConnectedToast')+url,true);return true}
async function waitWifiStaConnectionResult(){const deadline=Date.now()+60000;let ipDeadline=0;line('[sta-debug] waitWifiStaConnectionResult start');while(Date.now()<deadline){let j=null;try{j=await refreshWifiSta(false)}catch(e){j=null;line('[sta-debug] refreshWifiSta exception: '+e)}if(!j){line('[sta-debug] poll null, retry');await new Promise(resolve=>setTimeout(resolve,800));continue}/* apply_pending=true 期间 connected/last_error 是 deferred apply 前的陈旧状态，一律继续等，false 后才评估，消除 800ms 轮询窗口内的假失败/假成功（旧固件无此字段恒走原逻辑） */if(j.apply_pending===true){line('[sta-debug] apply pending, keep waiting');showWifiStaHandoffModal({...j,connecting:true});await new Promise(resolve=>setTimeout(resolve,800));continue}line('[sta-debug] poll connected='+j.connected+' ip='+j.sta_ip+' handoff='+j.handoff_active+' error='+j.last_error+' timed_out='+j.timed_out);if(j&&j.connected){if(!j.sta_ip||j.sta_ip==='0.0.0.0'){if(ipDeadline===0)ipDeadline=Date.now()+5000;if(Date.now()<ipDeadline){staNotice.textContent=t('wifi.staGettingIp');showWifiStaHandoffModal({...j,connecting:true});await new Promise(resolve=>setTimeout(resolve,800));continue}}staNotice.textContent=t('wifi.staConnected')+'\n'+t('wifi.handoffLanIp')+j.sta_ip+'\n'+t('wifi.handoffUrl')+'http://'+j.sta_ip+'/';await refreshStatus();cmd.value='';if(j.sta_ip&&j.sta_ip!=='0.0.0.0'){line('[sta-debug] showing handoff modal');showWifiStaHandoffModal(j);showToast(t('wifi.staConnectedToast')+handoffStaUrl(j),true)}else if(j.handoff_active){line('[sta-debug] showing handoff modal (handoff_active)');showWifiStaHandoffModal(j)}else{line('[sta-debug] connected but no handoff condition met')}return true}if(j&&(j.last_error||j.timed_out)){window.handoffShownForStaIp='';staNotice.textContent=t('wifi.connectFailed');line('[sta-debug] failure: '+j.last_error+' / '+j.last_error_message);showWifiStaFailureModal(j);return false}showWifiStaHandoffModal({...j,connecting:true});await new Promise(resolve=>setTimeout(resolve,800))}window.handoffShownForStaIp='';staNotice.textContent=t('wifi.connectFailed');line('[sta-debug] timeout');showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:t('wifi.staTimeout')});return false}
function onHostWifiToggle(){const hw=document.getElementById('hostWifiToggle').checked,bar=document.getElementById('hostWifiStatusBar'),clearBtn=document.getElementById('staClearBtn'),connectBtn=document.getElementById('staConnectBtn'),notice=document.getElementById('staNotice');if(hw){clearBtn.style.display='none';connectBtn.textContent=t('wifi.hostSendBtn');connectBtn.onclick=saveHostWifi;notice.textContent=t('wifi.hostNotice');bar.style.display='block';pollHostWifiStatus();stopHostWifiPoll();hostWifiPollTimer=setInterval(pollHostWifiStatus,5000)}else{clearBtn.style.display='';connectBtn.textContent=t('button.connect');connectBtn.onclick=saveWifiSta;notice.textContent=t('wifi.staNotice');bar.style.display='none';stopHostWifiPoll()}}
let hostWifiPollTimer=0;function stopHostWifiPoll(){if(hostWifiPollTimer){clearInterval(hostWifiPollTimer);hostWifiPollTimer=0}}
async function pollHostWifiStatus(){try{const r=await fetch('/api/host-wifi-status');const j=await r.json();const lbl=document.getElementById('hostWifiStatusLabel'),ipEl=document.getElementById('hostWifiStatusIp'),errEl=document.getElementById('hostWifiStatusError');if(j.status==='IDLE'){if(j.host_ip&&j.host_ip_age_s<=60){setHostWifiLabel('wifi.hostStatus.hostOnline');ipEl.style.display='';ipEl.textContent='IP: '+j.host_ip}else{setHostWifiLabel('wifi.hostStatus.waitingHost');ipEl.style.display='none'}errEl.style.display='none'}else{setHostWifiLabel({connecting:'wifi.hostStatus.connecting',connected:'wifi.hostStatus.connected',failed:'wifi.hostStatus.failed'}[j.status]||'wifi.hostStatus.connecting');if(j.status==='connected'){ipEl.style.display='';ipEl.textContent='IP: '+j.ip;errEl.style.display='none';stopHostWifiPoll();const doneBtn=document.getElementById('staConnectBtn');if(doneBtn&&document.getElementById('hostWifiToggle').checked){doneBtn.textContent=t('wifi.hostDoneBtn');doneBtn.onclick=closeWifiStaModal}showToast(t('wifi.hostOkToast')+j.ip,true)}else if(j.status==='failed'){errEl.style.display='';errEl.textContent=t('wifi.hostFailPrefix')+j.error;ipEl.style.display='none';stopHostWifiPoll()}else{ipEl.style.display='none';errEl.style.display='none'}}}catch(e){}};
async function saveHostWifi(){const ssid=staSsid.value.trim();let pwd=staPassword.value;if(!ssid){alert(t('wifi.ssidEmpty'));return}closeWifiScanPopover();if(!staPasswordDirty&&staPasswordPlaceholder){let p=null;try{const pr=await fetch('/api/wifi-sta/password?ssid='+encodeURIComponent(ssid));if(pr.ok){const pj=await pr.json();p=pj.password||''}}catch(e){}if(p===null){p=await fetchSavedStaPassword();if(p===null)return}pwd=p}document.getElementById('hostWifiStatusBar').style.display='block';document.getElementById('hostWifiStatusLabel').textContent=t('wifi.hostSending');const cmd='WIFI|'+ssid+'|'+pwd;try{const r=await fetch('/api/cmd?target=serial',{method:'POST',headers:{'Content-Type':'text/plain'},body:cmd});if(r.ok){document.getElementById('hostWifiStatusLabel').textContent=t('wifi.hostWaitResponse');stopHostWifiPoll();hostWifiPollTimer=setInterval(pollHostWifiStatus,2000)}else{const txt=await r.text();document.getElementById('hostWifiStatusLabel').textContent=t('wifi.hostSendFailed');document.getElementById('hostWifiStatusError').textContent=txt;document.getElementById('hostWifiStatusError').style.display=''}}catch(e){document.getElementById('hostWifiStatusLabel').textContent=t('wifi.hostSendFailed');document.getElementById('hostWifiStatusError').textContent=e.message||t('wifi.networkError');document.getElementById('hostWifiStatusError').style.display=''}}
async function saveWifiSta(){try{closeWifiScanPopover();staNotice.textContent=t('wifi.staConnecting');line('[sta-debug] saveWifiSta start ssid='+staSsid.value.trim());const body=new URLSearchParams();body.set('ssid',staSsid.value.trim());body.set('source',wifiStaSaveSource());if(staSelectedChannel>=1&&staSelectedChannel<=14)body.set('channel',String(staSelectedChannel));if(!staPasswordDirty&&(staPasswordPlaceholder||staSavedPasswordKnown))body.set('keep_password','1');else body.set('password',staPassword.value);const r=await fetch('/api/wifi-sta',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!r.ok){const txt=await r.text();staNotice.textContent=t('wifi.connectFailed');showWifiStaFailureModal({ssid:staSsid.value.trim(),last_error_message:txt});throw new Error(txt)}const saved=await r.json().catch(()=>null);line('[sta-debug] saved response handoff_active='+(saved&&saved.state&&saved.state.handoff_active));showWifiStaHandoffModal({...(saved&&saved.state||{}),connecting:true});staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();await refreshWifiSta(true);refreshStatus();await waitWifiStaConnectionResult()}catch(e){line('wifi sta save error: '+e)}}
async function clearWifiSta(){if(!confirm(t('wifi.clearConfirm')))return;try{closeWifiScanPopover();const r=await fetch('/api/wifi-sta/clear',{method:'POST'});if(!r.ok){const t=await r.text();showCommandError(t);throw new Error(t)}staPassword.value='';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye();await refreshWifiSta(true);refreshStatus();closeWifiStaModal()}catch(e){line('wifi sta clear error: '+e)}}
async function quick(v){cmd.value=v;await sendCmd()}
staPassword.addEventListener('input',()=>{if(staPasswordPlaceholder){staPassword.value=staPassword.value.replace(/^\*+/,'');staPasswordPlaceholder=false}staPasswordDirty=true;staSavedPasswordKnown=false;resetStaConnectBtn()});
let staSelectedChannel=0;
// 改 SSID 时同步清掉旧密码的掩码占位与 keep_password 依据，
// 否则 saveWifiSta 会带 keep_password=1 拿旧密码去连新网络
staSsid.addEventListener('input',()=>{staSelectedChannel=0;staPassword.value='';staPassword.type='password';staPasswordPlaceholder=false;staPasswordDirty=false;staPasswordVisible=false;staSavedPassword='';staSavedPasswordKnown=false;updateStaPasswordEye()});
cmd.addEventListener('keydown',e=>{if(e.key==='Enter')sendCmd()});
cmdTarget.addEventListener('change',e=>{applyCmdTarget(e.target.value)});
function addPoint(p){const dt=Number(p.dt||16);smoothedDt=smoothedDt*0.85+Math.max(0,Math.min(80,dt))*0.15;p.dts=smoothedDt;points[pointHead]=p;pointHead=(pointHead+1)%points.length;if(pointCount<points.length)pointCount++}
function latestPoint(){return pointCount?points[(pointHead+points.length-1)%points.length]:null}
function pointAt(i){return points[(pointHead-pointCount+i+points.length)%points.length]}
function map(v,min,max,h){if(max===min)return h/2;return h-(v-min)*(h/(max-min))}
function ensureGrid(){const w=cw,h=ch;if(gridReady&&gridCanvas.width===w*dpr&&gridCanvas.height===h*dpr)return;gridCanvas.width=w*dpr;gridCanvas.height=h*dpr;gridCtx.setTransform(dpr,0,0,dpr,0,0);gridCtx.clearRect(0,0,w,h);gridCtx.strokeStyle=CHART_THEMES[resolvedTheme()].grid;gridCtx.lineWidth=1;for(let i=0;i<9;i++){const y=Math.round(20+i*(h-40)/8);gridCtx.beginPath();gridCtx.moveTo(36,y);gridCtx.lineTo(w-16,y);gridCtx.stroke()}gridReady=true}
function drawSeries(key,color,min,max,divisor=1){const w=cw,h=ch,plotX=36,plotW=w-52,plotH=h-40;if(pointCount<2)return;const stepX=plotW/255,rightX=plotX+plotW,buckets=[];for(let i=0;i<pointCount;i++){const p=pointAt(i);if(!p)continue;const x=rightX-(pointCount-1-i)*stepX;const xi=Math.round(x);if(xi<plotX-5||xi>w-16+5)continue;const y=20+map((p[key]||0)/divisor,min,max,plotH);let b=buckets[xi];if(!b)buckets[xi]={min:y,max:y,xSum:x,count:1,gap:(p.dt||16)>80};else{if(y<b.min)b.min=y;if(y>b.max)b.max=y;b.xSum+=x;b.count++;if((p.dt||16)>80)b.gap=true}}ctx.strokeStyle=color;ctx.beginPath();let drawn=false;for(let xi=0;xi<=w;xi++){const b=buckets[xi];if(!b)continue;const x=b.xSum/b.count;const mid=(b.min+b.max)/2;if(!drawn||b.gap){ctx.moveTo(x,mid);drawn=true}else{ctx.lineTo(x,mid)}if(b.max-b.min>1){ctx.moveTo(x,b.min);ctx.lineTo(x,b.max);ctx.moveTo(x,mid)}}if(drawn)ctx.stroke()}
function draw(){const w=cw,h=ch;ensureGrid();ctx.clearRect(36,0,w-52,h);ctx.drawImage(gridCanvas,36*dpr,0,(w-52)*dpr,h*dpr,36,0,w-52,h);const ct=CHART_THEMES[resolvedTheme()];ctx.fillStyle=ct.axis;ctx.font='12px sans-serif';ctx.textAlign='right';ctx.textBaseline='middle';const yLabels=h<150?[1,0,-1]:[1,0.75,0.5,0.25,0,-0.25,-0.5,-0.75,-1];for(let i=0;i<yLabels.length;i++){ctx.fillText(String(yLabels[i]),32,Math.round(20+i*(h-40)/(yLabels.length-1)))}ctx.save();ctx.beginPath();ctx.rect(36,0,w-52,h);ctx.clip();ctx.lineWidth=2;drawSeries('thr',ct.thr,-1,1,100);drawSeries('str',ct.str,-1,1,100);drawSeries('gz',ct.gz,-1,1,5);if(screenSaverActive){ctx.fillStyle=ct.saver;ctx.font='20px sans-serif';ctx.textAlign='center';ctx.fillText('Drifting for Fun~',w/2,h/2)}ctx.restore()}
function renderLoop(now){requestAnimationFrame(renderLoop);let dt=Math.min(100,now-lastFrameTime);lastFrameTime=now;if(document.hidden||chartPaused)return;if(screenSaverActive){const stepX=(cw-52)/255;scrollOffset+=dt/18*stepX;scrollOffset=Math.min(scrollOffset,stepX*1.5);while(scrollOffset>=stepX){addPoint({seq:0,t:saverTime,dt:16,thr:90*Math.sin(saverTime/400),str:90*Math.sin(saverTime/550+1),gz:5*Math.sin(saverTime/300+2)});saverTime+=16;scrollOffset-=stepX}if(now-lastDrawTime>=16){lastDrawTime=now;draw()}}}
function initEmbedTuneFrames(){document.querySelectorAll('.embedTuneFrame').forEach(function(f){var src=f.getAttribute('data-src');if(!src)return;f.addEventListener('load',function(){try{var doc=f.contentDocument;if(!doc)return;var fit=function(){try{var b=doc.body;if(!b)return;var cs=doc.defaultView?doc.defaultView.getComputedStyle(b):null;var mt=cs?parseFloat(cs.marginTop)||0:0;var mb=cs?parseFloat(cs.marginBottom)||0:0;var h=Math.max(b.scrollHeight,b.offsetHeight)+mt+mb;if(h>0&&f.style.height!==h+'px')f.style.height=(h+2)+'px'}catch(e){}};fit();setTimeout(fit,400);setTimeout(fit,1600);if(window.ResizeObserver&&doc.body)new ResizeObserver(fit).observe(doc.body)}catch(e){}});var q='';if(src.indexOf('theme=')<0)q+='theme='+(document.documentElement.getAttribute('data-theme')||'dark');f.src=src+(q?(src.indexOf('?')<0?'?':'&')+q:'');});}
if(location.search.indexOf('embedded=1')>=0)document.body.classList.add('embedded');if(location.search.indexOf('settings=1')>=0)document.body.classList.add('settings');if(location.search.indexOf('wifi=1')>=0){document.body.classList.add('wifi');openWifiApModal();openWifiStaModal();const settingsView=document.getElementById('settingsView');const wifiAp=document.getElementById('wifiApModal');const wifiSta=document.getElementById('wifiStaModal');if(settingsView&&wifiAp&&wifiSta){settingsView.parentNode.insertBefore(wifiAp,settingsView);settingsView.parentNode.insertBefore(wifiSta,settingsView);}}if(location.search.indexOf('settings=1')>=0){const settingsView=document.getElementById('settingsView');const diagPanel=document.getElementById('diagnosticsPanel');if(settingsView&&diagPanel&&settingsView.parentNode){settingsView.parentNode.insertBefore(diagPanel,settingsView);}}if(location.search.indexOf('settings=1')>=0||location.search.indexOf('wifi=1')>=0){const rcFold=document.getElementById('rcFold');if(rcFold){rcFold.classList.add('open');const rcHead=rcFold.querySelector('.foldHead');if(rcHead){rcHead.removeAttribute('onclick');rcHead.setAttribute('aria-expanded','true');}}}initCanvasDpr();initLanguage();initMute();initTheme();if(document.body.classList.contains('embedded')&&document.body.classList.contains('settings'))initEmbedTuneFrames();refreshStatus();refreshDevMode();refreshWifiSta();setInterval(refreshStatus,5000);setInterval(refreshWifiSta,5000);setInterval(initMute,5000);setInterval(refreshDevMode,5000);setInterval(pollLog,1000);connectDataSocket();setTimeout(()=>{if(!dataWsConnected)pollData()},1200);updateTubMeta();draw();requestAnimationFrame(renderLoop);refreshJoystickCalStatus();restoreCmdTarget();
</script>
</body>
</html>
)rawliteral";static const char WIFI_WEB_JUDGE_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<link rel="icon" type="image/png" href="/favicon.png">
<title>Drift Judge</title>
<script>try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);document.documentElement.dataset.theme=m?m[1]:(window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark')}catch(e){}</script>
<style>
body.preinit{visibility:hidden}
h1{font-size:24px;margin:0 0 6px}
:root{--bg:#000;--ink:#f5f5f7;--ink2:rgba(235,235,245,.85);--ink3:#f5f5f7;--ink4:#f5f5f7;--panel:#1c1c1e;--card:#1c1c1e;--input:#1c1c1e;--line:rgba(255,255,255,.10);--line2:rgba(255,255,255,.10);--line3:rgba(255,255,255,.10);--bar:#2c2c2e;--bar2:#2c2c2e;--chartBg:#000;--link:#2997ff;--accentFill:#0a84ff;--accentHi:#2997ff;--onAccent:#fff;--ok:#30d158;--warn:#ff9f0a;--bad:#ff453a;--bad2:#ff6961;--badSoft:rgba(255,69,58,.15);--badInk:#ffc9c4;--dotOff:#636366;--heroGrad:#1c1c1e;--fillGrad:linear-gradient(90deg,#30d158,#2997ff,#bf5af2);--dimGrad:linear-gradient(90deg,#ff453a,#ff9f0a,#30d158);--segHover:transparent;--inkHi:#f5f5f7;--inkF:rgba(235,235,245,.72);--accent:#2997ff;--ease-apple:cubic-bezier(.32,.72,0,1);--ok-text:#30d158;--warn-text:#ff9f0a;--bad-text:#ff453a;--status-info:rgba(235,235,245,.72);--sep:rgba(255,255,255,.16);--cardLine:rgba(255,255,255,.10);--matSolid:rgba(28,28,30,.96);--elev:0 8px 30px rgba(0,0,0,.5);--appleFont:-apple-system,BlinkMacSystemFont,system-ui,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;--appleMono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}html[data-theme="light"]{--bg:#f5f5f7;--ink:#1d1d1f;--ink2:rgba(60,60,67,.85);--ink3:#1d1d1f;--ink4:#1d1d1f;--panel:#fff;--card:#f5f5f7;--input:#f5f5f7;--line:rgba(0,0,0,.08);--line2:rgba(0,0,0,.08);--line3:rgba(0,0,0,.08);--bar:rgba(120,120,128,.16);--bar2:rgba(120,120,128,.16);--chartBg:#f5f5f7;--link:#0066cc;--accentFill:#0071e3;--accentHi:#0066cc;--onAccent:#fff;--heroGrad:#fff;--segHover:transparent;--inkHi:#1d1d1f;--inkF:rgba(60,60,67,.72);--accent:#0066cc;--ok:#34c759;--warn:#ff9500;--bad:#ff3b30;--bad2:#ff3b30;--badSoft:rgba(255,59,48,.12);--badInk:#d70015;--dotOff:#8e8e93;--fillGrad:linear-gradient(90deg,#34c759,#0071e3,#af52de);--dimGrad:linear-gradient(90deg,#ff3b30,#ff9500,#34c759);--ok-text:#1a7f37;--warn-text:#c93400;--bad-text:#d70015;--status-info:rgba(60,60,67,.72);--sep:rgba(60,60,67,.29);--cardLine:rgba(0,0,0,.08);--matSolid:rgba(245,245,247,.96);--elev:0 8px 30px rgba(0,0,0,.12)}html:root h1{font-weight:600;letter-spacing:-0.02em}html:root button{font-weight:600!important;border-radius:9999px}html:root .panel,html:root .heroCard,html:root .chartWrap{border-radius:16px}html:root .card,html:root .summaryItem,html:root .collision,html:root .field{border-radius:14px}html:root .field input{border-radius:8px}html:root .value,html:root .heroValue,html:root .scoreValue,html:root .summaryItem .v,html:root .dimScore{font-weight:600;font-variant-numeric:tabular-nums}html:root .heroValue,html:root .scoreValue{letter-spacing:-0.02em}html:root .fieldTitle,html:root .sectionTitle,html:root .statusPill,html:root .dimTrend{font-weight:600}html:root button:active{transform:scale(.97)}body{font-family:system-ui,sans-serif;margin:16px auto;max-width:760px;background:var(--bg);color:var(--ink);padding:0 12px}
a{color:var(--link);text-decoration:none}
button{background:var(--accentFill);color:var(--onAccent);border:1px solid var(--accentFill);border-radius:10px;padding:10px 14px;font-weight:700;cursor:pointer}
button.alt{background:var(--panel);color:var(--ink);border-color:var(--line2)}
button:disabled{opacity:.5;cursor:not-allowed}
button:hover{background:var(--accentHi);border-color:var(--accentHi)}
button.alt:hover{background:var(--segHover);color:var(--inkHi)}
.themeButton,.langButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:var(--card);border:1px solid var(--line2);color:var(--ink2);cursor:pointer;font-size:12px;font-weight:600;line-height:1}
.themeButton:hover,.langButton:hover{background:var(--segHover);color:var(--ink)}
.themeButton .icoSun{display:none}
html[data-theme="light"] .themeButton .icoSun{display:block}
html[data-theme="light"] .themeButton .icoMoon{display:none}
.muted{color:var(--ink2)}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:12px;padding:14px;margin:12px 0}
.panelHead{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;flex-wrap:wrap}
.row{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px}
.label{font-size:12px;color:var(--ink2);text-transform:uppercase;letter-spacing:.06em}
.value{font-size:28px;font-weight:800;margin-top:6px}
.sub{font-size:13px;color:var(--ink2);margin-top:6px}
.hero{display:grid;grid-template-columns:1.2fr .8fr;gap:12px}
.heroCard{background:var(--heroGrad);border:1px solid var(--line);border-radius:14px;padding:14px}
.heroValue{font-size:56px;font-weight:900;line-height:1}
.scoreValue{font-size:38px;font-weight:900;line-height:1}
.bar{height:12px;background:var(--bar);border-radius:999px;overflow:hidden;margin-top:12px}
.fill{height:100%;width:0;background:var(--fillGrad)}
.metaGrid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
.mini{font-size:12px;color:var(--ink2)}
.statusPill{display:inline-flex;align-items:center;gap:8px;padding:6px 10px;border-radius:999px;background:var(--card);border:1px solid var(--line);font-size:12px;font-weight:700}
.statusDot{width:9px;height:9px;border-radius:50%;background:var(--dotOff)}
.statusOnline .statusDot{background:var(--ok)}
.statusWaiting .statusDot{background:var(--warn)}
.statusOffline .statusDot{background:var(--bad)}
.chartWrap{height:140px;background:var(--chartBg);border:1px solid var(--bar2);border-radius:12px;padding:10px}
.chartWrap canvas{width:100%;height:100%}
.controls{display:flex;gap:10px;flex-wrap:wrap}
.dimensions{display:grid;gap:10px}
.dimRow{display:grid;grid-template-columns:86px 1fr 22px 44px;gap:10px;align-items:center}
.dimName{font-size:12px;color:var(--ink3)}
.dimBar{height:8px;background:var(--bar2);border-radius:999px;overflow:hidden}
.dimFill{height:100%;width:0;background:var(--dimGrad)}
.dimTrend{text-align:center;font-size:13px;font-weight:700;color:var(--ink2)}
.dimTrendUp{color:var(--ok)}
.dimTrendDown{color:var(--bad2)}
.dimTrendFlat{color:var(--ink2)}
.dimScore{text-align:right;font-size:13px;font-weight:700}
.scoreExplain{font-size:12px;color:var(--ink2);margin-top:6px}
.collision{display:inline-flex;align-items:center;padding:5px 12px;border-radius:999px;background:transparent;border:1px solid var(--ok);color:var(--ink);font-size:12px;font-weight:600}
.collision.active{border-color:var(--bad);background:var(--badSoft);color:var(--badInk)}
.tuneGrid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px}
.tuneSection{margin-top:14px;padding-top:14px;border-top:1px solid var(--bar)}
.sectionTitle{font-size:13px;font-weight:700;color:var(--ink4)}

.summaryGrid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px}
.summaryItem{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px 12px}
.summaryItem .k{font-size:11px;color:var(--ink2)}
.summaryItem .v{font-size:18px;font-weight:700;margin-top:4px}
.field{display:flex;flex-direction:column;gap:6px;font-size:12px;color:var(--inkF);background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px 12px}
.fieldTitle{font-size:12px;color:var(--ink4);font-weight:700}
.fieldHint{font-size:11px;color:var(--ink2);line-height:1.35;min-height:28px}
.field input{background:var(--input);border:1px solid var(--line3);border-radius:10px;color:var(--ink);padding:10px 12px;font:inherit}
.tuneActions{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-top:12px}
#judgeConfigStatus{font-size:12px}
body.embedded #judgeHeadPanel{display:none}
/* CC 内嵌视图：judge 页撑满 iframe 宽度（去 760px 居中限宽），并隐藏显示类区域（得分 hero、gyroZ 曲线），只留设置表单 */
body.embedded{max-width:none;margin-top:0}
body.embedded .panel{margin-top:0}
body.embedded #judgeHero{display:none}
body.embedded #gyroChartPanel{display:none}














@media (max-width:640px){body{max-width:560px}.hero,.grid,.metaGrid,.tuneGrid,.summaryGrid{grid-template-columns:1fr}.heroValue{font-size:42px}.scoreValue{font-size:32px}.dimRow{grid-template-columns:74px 1fr 20px 40px}}
</style>
<style id="dd-embed-native">/* DonkeyDrifter 原生风格——仅 embedded 作用域（CC 设置视图），独立页不动 */body.embedded [data-i18n="panel.rcChannels"],body.embedded [data-i18n="drift.steering.label"],body.embedded [data-i18n="drift.throttle.label"],body.embedded [data-i18n="judge.section.thresholds"],body.embedded [data-i18n="judge.section.scoring"],body.embedded [data-i18n="judge.dimLabel"]{font-weight:600!important;letter-spacing:-0.02em!important;font-size:15px!important;color:#e4e7eb!important}html[data-theme="light"] body.embedded [data-i18n="panel.rcChannels"],html[data-theme="light"] body.embedded [data-i18n="drift.steering.label"],html[data-theme="light"] body.embedded [data-i18n="drift.throttle.label"],html[data-theme="light"] body.embedded [data-i18n="judge.section.thresholds"],html[data-theme="light"] body.embedded [data-i18n="judge.section.scoring"],html[data-theme="light"] body.embedded [data-i18n="judge.dimLabel"]{color:#1a2330!important}body.embedded [data-i18n="panel.rcChannels"]::before,body.embedded [data-i18n="drift.steering.label"]::before,body.embedded [data-i18n="drift.throttle.label"]::before,body.embedded [data-i18n="judge.section.thresholds"]::before,body.embedded [data-i18n="judge.section.scoring"]::before,body.embedded [data-i18n="judge.dimLabel"]::before{content:"";display:inline-block;width:18px;height:18px;margin-right:8px;vertical-align:-3px;flex:0 0 auto;background:currentColor;-webkit-mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat}body.embedded .panel,body.embedded .rcCell,body.embedded .summaryItem,body.embedded .field,body.embedded .card{border-radius:8px}html[data-theme="light"] body.embedded .rcCell,html[data-theme="light"] body.embedded .summaryItem,html[data-theme="light"] body.embedded .field,html[data-theme="light"] body.embedded .card{background:#fff!important;border-color:#ccd5df!important}html[data-theme="light"] body.embedded input[type=number],html[data-theme="light"] body.embedded input[type=text],html[data-theme="light"] body.embedded select{background:#fff;border-color:#ccd5df}body.embedded input[type=number],body.embedded input[type=text],body.embedded select{border-radius:6px}</style>
<style id="apple-deep">
/* ===== Apple 深化（自 v1.10.0 起为唯一界面风格；原座舱象限覆写已并入基值） ===== */


html:root,html:root *{-webkit-tap-highlight-color:transparent}
html:root body{font-family:var(--appleFont);-webkit-font-smoothing:antialiased;padding-bottom:env(safe-area-inset-bottom)}
html:root button,html:root input,html:root select,html:root textarea{font-family:inherit}
html:root :focus-visible{outline:3px solid var(--accent);outline-offset:2px}
/* 标题与排版（11px 正文 → 13px，行高 ≥1.3，微标签去 uppercase） */
html:root h1{font-size:20px;font-weight:600;letter-spacing:-.02em;line-height:1.4}
html:root .fieldHint,html:root .summaryItem .k,html:root .dimName,html:root .fieldTitle,html:root .field,html:root .statusPill,html:root .sectionTitle,html:root .dimTrend{font-size:13px;line-height:1.35;text-transform:none;letter-spacing:0}
html:root .fieldHint{min-height:36px}
html:root .heroValue{font-weight:600;letter-spacing:-.02em;line-height:1.05}
html:root .scoreValue{font-weight:600;letter-spacing:-.02em}
/* 语义色双轨：文字用 --*-text，填充（点/条/渐变）保持原 token */
html:root .dimTrendUp{color:var(--ok-text)}
html:root .dimTrendDown{color:var(--bad-text)}
/* 状态双通道：颜色 + 字形 */
html:root .statusPill{gap:6px;border-color:var(--cardLine)}
html:root .statusPill::before{font-weight:700;font-size:13px;line-height:1}
html:root .statusOnline::before{content:"\2713"}
html:root .statusWaiting::before{content:"\2026"}
html:root .statusOffline::before{content:"\00d7"}
/* 材质 / 发丝线 / 圆角 8·12·16·22·9999 */
html:root .panel{background:var(--panel);border:1px solid var(--cardLine);border-radius:16px;padding:14px}
html:root .card,html:root .summaryItem{border-color:var(--cardLine);border-radius:12px}
html:root .heroCard{border-color:var(--cardLine);border-radius:16px}
html:root .field{border-color:var(--cardLine);border-radius:12px}
html:root .tuneSection{border-top:1px solid var(--sep);margin-top:16px;padding-top:16px}
html:root .chartWrap{border-color:var(--cardLine);border-radius:16px}
html:root .field input,html:root .field select{border-color:var(--cardLine);border-radius:12px}
html:root .collision{border-radius:12px}
html:root .dimRow{border-radius:8px}
html:root .row{gap:14px}
html:root .modal .dialog,html:root .dialog{border-radius:22px;background:var(--matSolid);box-shadow:var(--elev)}
/* 控件尺寸 44px + 禁用态 */
html:root button{min-height:44px;border-radius:9999px;padding:10px 18px;font-size:13px}html:root .themeButton,html:root .langButton{min-height:0}
html:root input,html:root select,html:root textarea{min-height:44px;box-sizing:border-box}
html:root button:disabled{opacity:1;background:var(--panel);color:var(--status-info);border-color:var(--cardLine);cursor:not-allowed}
/* 命中区 ≥44×44（::after 不可见命中区，视觉尺寸不变） */
html:root .themeButton,html:root .langButton,html:root #backLink,html:root .dimRow{position:relative}
html:root .themeButton::after,html:root .langButton::after,html:root #backLink::after{content:"";position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:max(100%,44px);height:max(100%,44px);border-radius:inherit}
/* 动效统一 */
html:root button{transition:background-color .15s var(--ease-apple),color .15s var(--ease-apple),border-color .15s var(--ease-apple),transform .1s var(--ease-apple)}
html:root .statusDot{animation-duration:2.4s}
@media (max-width:640px){
html:root h1{font-size:20px}
html:root .heroValue{font-size:42px}
}
/* 降级 */
@media (prefers-reduced-motion: reduce){
html:root *,html:root *::before,html:root *::after{animation-duration:.001ms !important;animation-iteration-count:1 !important;transition-duration:.001ms !important;scroll-behavior:auto !important}
}
@media (prefers-reduced-transparency: reduce){
html:root .dialog,html:root .modal{background:var(--matSolid);backdrop-filter:none;-webkit-backdrop-filter:none}
}
@media (prefers-contrast: more){
:root{--ink2:#f5f5f7;--inkF:#f5f5f7;--cardLine:rgba(255,255,255,.34);--sep:rgba(255,255,255,.34)}
html:root[data-theme="light"]{--ink2:#1d1d1f;--inkF:#1d1d1f;--cardLine:rgba(60,60,67,.55);--sep:rgba(60,60,67,.55)}
html:root .panel,html:root .card,html:root .summaryItem,html:root .field,html:root .heroCard{border-width:1px;border-style:solid}
}
@media (forced-colors: active){
html:root .panel,html:root .card,html:root .summaryItem,html:root .field,html:root .heroCard,html:root .chartWrap{border:1px solid CanvasText}
html:root .statusDot{forced-color-adjust:none;background:CanvasText;border:1px solid Canvas}
}
/* 设置行的动作按钮（漂移设置 / Judge 设置 / 手柄校准）：原高 39px（<44），
   触屏上偏小；apple 象限提到 44（iOS 表单动作按钮的最小高度） */
html:root .setActions button {
  min-height: 44px;
}
</style>
</head>
<body>
<script>try{var _q=location.search,_b=document.body;_b.classList.add('preinit');if(_q.indexOf('embedded=1')>=0)_b.classList.add('embedded');setTimeout(function(){_b.classList.remove('preinit')},2000)}catch(e){try{document.body.classList.remove('preinit')}catch(_e){}}</script>
<div class="panel" id="judgeHeadPanel">
<div class="panelHead">
<div>
<h1>Drift Judge</h1>
</div>
<div class="row">
<div id="statusPill" class="statusPill statusWaiting"><span class="statusDot"></span><span id="status">connecting...</span></div>
<div class="muted"><a id="backLink" href="/" data-i18n="judge.backLink">返回 Drifter Console</a></div><button type="button" id="themeToggle" class="themeButton" onclick="toggleTheme()" aria-label="主题" data-i18n-aria="theme.title"><svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><g class="icoMoon"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></g><g class="icoSun"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></g></svg></button><button type="button" id="langToggle" class="langButton" onclick="toggleLanguage()" aria-label="语言" data-i18n-aria="language.title">中</button>
</div>
</div>
</div>
<div class="hero" id="judgeHero">
<div class="heroCard">
<div class="label">pseudoSpeed</div>
<div id="pseudoSpeed" class="heroValue">--</div>
<div class="sub" data-i18n="judge.pseudoSub">代理速度 / 动态强度</div>
<div class="bar"><div id="pseudoBar" class="fill"></div></div>
<div class="metaGrid" style="margin-top:12px">
<div class="card"><div class="label">gyroZ</div><div id="gyroZ" class="value">--</div></div>
<div class="card"><div class="label">throttle</div><div id="throttle" class="value">--</div></div>
<div class="card"><div class="label">transport</div><div id="transport" class="value">--</div></div>
</div>
</div>
<div class="heroCard">
<div class="label" data-i18n="judge.totalScoreLabel">总分</div>
<div id="totalScore" class="scoreValue">0</div>
<div id="scoreGrade" class="sub">--</div>
<div id="lowestDimension" class="scoreExplain" data-i18n="judge.score.lowestInitial">当前最低项：--</div>
<div id="weakestTrend" class="scoreExplain" data-i18n="judge.score.weakestInitial">最近拖分项：分析中</div>
<div id="weakestTrendReason" class="scoreExplain" data-i18n="judge.score.reasonInitial">拖分原因：拖分分析中，继续保持当前动作。</div>
<div id="collision" class="collision" style="margin-top:12px" data-i18n="judge.collision.ok">状态正常</div>
</div>
</div>
<div class="panel" id="gyroChartPanel">
<span class="label" data-i18n="judge.gyroChartLabel">gyroZ 曲线</span>
<div class="chartWrap"><canvas id="gyroChart" width="700" height="140"></canvas></div>
</div>
<div class="panel">
<div class="panelHead">
<div>
<span class="label" data-i18n="judge.tuneLabel">评分阈值调参</span>
</div>
<div id="judgeConfigCurrent" class="muted" data-i18n="judge.config.loading">读取设备配置中...</div>
</div>
<div id="judgeConfigSummary" class="summaryGrid" style="margin-top:12px">
<div class="summaryItem"><div class="k" data-i18n="judge.summary.collisionThreshold">碰撞阈值</div><div class="v">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="judge.summary.bigTurnThreshold">大弯阈值</div><div class="v">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="judge.summary.windowSize">窗口大小</div><div class="v">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="judge.summary.collisionPenalty">碰撞扣分</div><div class="v">--</div></div>
</div>
<div class="tuneSection">
<span class="sectionTitle" data-i18n="judge.section.thresholds">基础阈值</span>
<div class="tuneGrid" style="margin-top:12px">
<label class="field"><span class="fieldTitle" data-i18n="judge.field.collisionThreshold">碰撞阈值</span><span class="fieldHint" data-i18n="judge.field.collisionThresholdHint">越小越容易触发碰撞。</span><input id="collisionThresholdInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.bigTurnThreshold">大弯阈值</span><span class="fieldHint" data-i18n="judge.field.bigTurnThresholdHint">越小越容易进入大弯区。</span><input id="bigTurnThresholdInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.windowSize">窗口大小</span><span class="fieldHint" data-i18n="judge.field.windowSizeHint">越大越平滑，但响应更慢。</span><input id="windowSizeInput" type="number" step="1"></label>
</div>
<div class="tuneActions">
<button id="saveJudgeConfigBtn" onclick="saveJudgeConfig()" data-i18n="judge.button.save">保存阈值</button>
<button id="resetJudgeConfigBtn" class="alt" onclick="resetJudgeConfigToDefault()" data-i18n="judge.button.resetDefault">恢复默认值</button>
<div id="judgeConfigStatus" class="muted" data-i18n="judge.config.note">保存后仅影响后续样本，不回溯重算。</div>
</div>
<div class="tuneSection">
<span class="sectionTitle" data-i18n="judge.section.scoring">评分参数</span>
<div class="tuneGrid" style="margin-top:12px">
<label class="field"><span class="fieldTitle" data-i18n="judge.field.collisionPenalty">碰撞扣分</span><span class="fieldHint" data-i18n="judge.field.collisionPenaltyHint">每次碰撞扣多少分。</span><input id="collisionPenaltyInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.turnSmoothnessWeight">转弯平滑敏感度</span><span class="fieldHint" data-i18n="judge.field.turnSmoothnessWeightHint">越大越容易因为抖动掉分。</span><input id="turnSmoothnessWeightInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.rangeMatchWeight">区间匹配敏感度</span><span class="fieldHint" data-i18n="judge.field.rangeMatchWeightHint">越大越容易因为动作不协调掉分。</span><input id="rangeMatchWeightInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.gyroStabilityWeight">陀螺稳定敏感度</span><span class="fieldHint" data-i18n="judge.field.gyroStabilityWeightHint">越大越容易因为陀螺波动掉分。</span><input id="gyroStabilityWeightInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.bigTurnStabilityWeight">大弯稳定敏感度</span><span class="fieldHint" data-i18n="judge.field.bigTurnStabilityWeightHint">越大越容易因为大弯阶段不稳掉分。</span><input id="bigTurnStabilityWeightInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.speedStabilityWeight">速度稳定敏感度</span><span class="fieldHint" data-i18n="judge.field.speedStabilityWeightHint">越大越容易因为 pseudoSpeed 波动掉分。</span><input id="speedStabilityWeightInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="judge.field.throttleStabilityWeight">油门稳定敏感度</span><span class="fieldHint" data-i18n="judge.field.throttleStabilityWeightHint">越大越容易因为抽油门掉分。</span><input id="throttleStabilityWeightInput" type="number" step="0.1"></label>
</div>
</div>
</div>
<div class="panel">
<div class="panelHead">
<div>
<span class="label" data-i18n="judge.dimLabel">评分维度</span>
</div>
<div class="controls">
<button id="startBtn" onclick="startRun()" data-i18n="judge.button.start">开始计分</button>
<button class="alt" onclick="resetScore()" data-i18n="judge.button.reset">重置</button>
</div>
</div>
<div id="dimensions" class="dimensions" style="margin-top:12px">
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.turnSmoothness">转弯平滑</div><div class="dimBar"><div id="dim1-fill" class="dimFill"></div></div><div id="dim1-trend" class="dimTrend dimTrendFlat">→</div><div id="dim1-score" class="dimScore">0</div></div>
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.rangeMatch">区间匹配</div><div class="dimBar"><div id="dim2-fill" class="dimFill"></div></div><div id="dim2-trend" class="dimTrend dimTrendFlat">→</div><div id="dim2-score" class="dimScore">0</div></div>
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.gyroStability">陀螺稳定</div><div class="dimBar"><div id="dim3-fill" class="dimFill"></div></div><div id="dim3-trend" class="dimTrend dimTrendFlat">→</div><div id="dim3-score" class="dimScore">0</div></div>
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.bigTurnStability">大弯稳定</div><div class="dimBar"><div id="dim4-fill" class="dimFill"></div></div><div id="dim4-trend" class="dimTrend dimTrendFlat">→</div><div id="dim4-score" class="dimScore">0</div></div>
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.speedStability">速度稳定</div><div class="dimBar"><div id="dim5-fill" class="dimFill"></div></div><div id="dim5-trend" class="dimTrend dimTrendFlat">→</div><div id="dim5-score" class="dimScore">0</div></div>
<div class="dimRow"><div class="dimName" data-i18n="judge.dim.throttleStability">油门稳定</div><div class="dimBar"><div id="dim6-fill" class="dimFill"></div></div><div id="dim6-trend" class="dimTrend dimTrendFlat">→</div><div id="dim6-score" class="dimScore">0</div></div>
</div>
</div>
<script>
const statusEl=document.getElementById('status'),statusPillEl=document.getElementById('statusPill'),pseudoSpeedEl=document.getElementById('pseudoSpeed'),pseudoBarEl=document.getElementById('pseudoBar'),gyroZEl=document.getElementById('gyroZ'),throttleEl=document.getElementById('throttle'),transportEl=document.getElementById('transport'),totalScoreEl=document.getElementById('totalScore'),scoreGradeEl=document.getElementById('scoreGrade'),lowestDimensionEl=document.getElementById('lowestDimension'),weakestTrendEl=document.getElementById('weakestTrend'),weakestTrendReasonEl=document.getElementById('weakestTrendReason'),collisionEl=document.getElementById('collision'),startBtn=document.getElementById('startBtn'),chartCanvas=document.getElementById('gyroChart'),chartCtx=chartCanvas.getContext('2d'),collisionThresholdInput=document.getElementById('collisionThresholdInput'),bigTurnThresholdInput=document.getElementById('bigTurnThresholdInput'),windowSizeInput=document.getElementById('windowSizeInput'),collisionPenaltyInput=document.getElementById('collisionPenaltyInput'),turnSmoothnessWeightInput=document.getElementById('turnSmoothnessWeightInput'),rangeMatchWeightInput=document.getElementById('rangeMatchWeightInput'),gyroStabilityWeightInput=document.getElementById('gyroStabilityWeightInput'),bigTurnStabilityWeightInput=document.getElementById('bigTurnStabilityWeightInput'),speedStabilityWeightInput=document.getElementById('speedStabilityWeightInput'),throttleStabilityWeightInput=document.getElementById('throttleStabilityWeightInput'),judgeConfigCurrentEl=document.getElementById('judgeConfigCurrent'),judgeConfigSummaryEl=document.getElementById('judgeConfigSummary'),judgeConfigStatusEl=document.getElementById('judgeConfigStatus'),saveJudgeConfigBtn=document.getElementById('saveJudgeConfigBtn'),resetJudgeConfigBtn=document.getElementById('resetJudgeConfigBtn');
const LANG_STORAGE_KEY='mus4.ui.lang';
const I18N={zh:{},en:{}};
I18N.zh['judge.backLink']='返回 Drifter Console';I18N.en['judge.backLink']='Back to Drifter Console';I18N.zh['theme.title']='主题';I18N.en['theme.title']='Theme';I18N.zh['language.title']='语言';I18N.en['language.title']='Language';I18N.zh['judge.pseudoSub']='代理速度 / 动态强度';I18N.en['judge.pseudoSub']='Proxy speed / dynamic intensity';
I18N.zh['judge.totalScoreLabel']='总分';I18N.en['judge.totalScoreLabel']='Total Score';
I18N.zh['judge.gyroChartLabel']='gyroZ 曲线';I18N.en['judge.gyroChartLabel']='gyroZ Curve';

I18N.zh['judge.tuneLabel']='评分阈值调参';I18N.en['judge.tuneLabel']='Scoring Threshold Tuning';

I18N.zh['judge.dimLabel']='评分维度';I18N.en['judge.dimLabel']='Scoring Dimensions';

I18N.zh['judge.summary.collisionThreshold']='碰撞阈值';I18N.en['judge.summary.collisionThreshold']='Collision Threshold';
I18N.zh['judge.summary.bigTurnThreshold']='大弯阈值';I18N.en['judge.summary.bigTurnThreshold']='Big-Turn Threshold';
I18N.zh['judge.summary.windowSize']='窗口大小';I18N.en['judge.summary.windowSize']='Window Size';
I18N.zh['judge.summary.collisionPenalty']='碰撞扣分';I18N.en['judge.summary.collisionPenalty']='Collision Penalty';
I18N.zh['judge.section.thresholds']='基础阈值';I18N.en['judge.section.thresholds']='Base Thresholds';

I18N.zh['judge.section.scoring']='评分参数';I18N.en['judge.section.scoring']='Scoring Parameters';

I18N.zh['judge.field.collisionThreshold']='碰撞阈值';I18N.en['judge.field.collisionThreshold']='Collision Threshold';
I18N.zh['judge.field.collisionThresholdHint']='越小越容易触发碰撞。';I18N.en['judge.field.collisionThresholdHint']='Smaller values trigger collisions more easily.';
I18N.zh['judge.field.bigTurnThreshold']='大弯阈值';I18N.en['judge.field.bigTurnThreshold']='Big-Turn Threshold';
I18N.zh['judge.field.bigTurnThresholdHint']='越小越容易进入大弯区。';I18N.en['judge.field.bigTurnThresholdHint']='Smaller values enter the big-turn zone more easily.';
I18N.zh['judge.field.windowSize']='窗口大小';I18N.en['judge.field.windowSize']='Window Size';
I18N.zh['judge.field.windowSizeHint']='越大越平滑，但响应更慢。';I18N.en['judge.field.windowSizeHint']='Larger values are smoother but respond more slowly.';
I18N.zh['judge.field.collisionPenalty']='碰撞扣分';I18N.en['judge.field.collisionPenalty']='Collision Penalty';
I18N.zh['judge.field.collisionPenaltyHint']='每次碰撞扣多少分。';I18N.en['judge.field.collisionPenaltyHint']='Points deducted per collision.';
I18N.zh['judge.field.turnSmoothnessWeight']='转弯平滑敏感度';I18N.en['judge.field.turnSmoothnessWeight']='Turn Smoothness Sensitivity';
I18N.zh['judge.field.turnSmoothnessWeightHint']='越大越容易因为抖动掉分。';I18N.en['judge.field.turnSmoothnessWeightHint']='Larger values lose points more easily from jitter.';
I18N.zh['judge.field.rangeMatchWeight']='区间匹配敏感度';I18N.en['judge.field.rangeMatchWeight']='Range Match Sensitivity';
I18N.zh['judge.field.rangeMatchWeightHint']='越大越容易因为动作不协调掉分。';I18N.en['judge.field.rangeMatchWeightHint']='Larger values lose points more easily from uncoordinated inputs.';
I18N.zh['judge.field.gyroStabilityWeight']='陀螺稳定敏感度';I18N.en['judge.field.gyroStabilityWeight']='Gyro Stability Sensitivity';
I18N.zh['judge.field.gyroStabilityWeightHint']='越大越容易因为陀螺波动掉分。';I18N.en['judge.field.gyroStabilityWeightHint']='Larger values lose points more easily from gyro fluctuation.';
I18N.zh['judge.field.bigTurnStabilityWeight']='大弯稳定敏感度';I18N.en['judge.field.bigTurnStabilityWeight']='Big-Turn Stability Sensitivity';
I18N.zh['judge.field.bigTurnStabilityWeightHint']='越大越容易因为大弯阶段不稳掉分。';I18N.en['judge.field.bigTurnStabilityWeightHint']='Larger values lose points more easily from instability during big turns.';
I18N.zh['judge.field.speedStabilityWeight']='速度稳定敏感度';I18N.en['judge.field.speedStabilityWeight']='Speed Stability Sensitivity';
I18N.zh['judge.field.speedStabilityWeightHint']='越大越容易因为 pseudoSpeed 波动掉分。';I18N.en['judge.field.speedStabilityWeightHint']='Larger values lose points more easily from pseudoSpeed fluctuation.';
I18N.zh['judge.field.throttleStabilityWeight']='油门稳定敏感度';I18N.en['judge.field.throttleStabilityWeight']='Throttle Stability Sensitivity';
I18N.zh['judge.field.throttleStabilityWeightHint']='越大越容易因为抽油门掉分。';I18N.en['judge.field.throttleStabilityWeightHint']='Larger values lose points more easily from throttle surges.';
I18N.zh['judge.config.loading']='读取设备配置中...';I18N.en['judge.config.loading']='Loading device config...';
I18N.zh['judge.config.synced']='基础阈值与评分参数已同步，可直接实车调参';I18N.en['judge.config.synced']='Base thresholds and scoring parameters synced; ready for on-car tuning';
I18N.zh['judge.config.note']='保存后仅影响后续样本，不回溯重算。';I18N.en['judge.config.note']='Saving only affects future samples; past results are not recomputed.';
I18N.zh['judge.config.loaded']='已同步设备配置，可开始调参';I18N.en['judge.config.loaded']='Device config synced; ready to tune';
I18N.zh['judge.config.loadFailed']='读取配置失败，先使用页面默认值';I18N.en['judge.config.loadFailed']='Failed to load config; using page defaults for now';
I18N.zh['judge.config.invalid']='输入值超出允许范围，请先调整后再保存';I18N.en['judge.config.invalid']='Input values are out of the allowed range; adjust them before saving';
I18N.zh['judge.config.saved']='已写入设备，设备重启后仍保留；后续样本立即生效';I18N.en['judge.config.saved']='Written to the device and kept across reboots; takes effect immediately for future samples';
I18N.zh['judge.config.saveFailed']='保存失败: ';I18N.en['judge.config.saveFailed']='Save failed: ';
I18N.zh['judge.config.resetOk']='已恢复默认值并写回设备，设备重启后仍保留';I18N.en['judge.config.resetOk']='Defaults restored and written back to the device; kept across reboots';
I18N.zh['judge.config.resetFailed']='恢复默认值失败: ';I18N.en['judge.config.resetFailed']='Failed to restore defaults: ';
I18N.zh['judge.button.start']='开始计分';I18N.en['judge.button.start']='Start Scoring';
I18N.zh['judge.button.stop']='结束计分';I18N.en['judge.button.stop']='Stop Scoring';
I18N.zh['judge.button.reset']='重置';I18N.en['judge.button.reset']='Reset';
I18N.zh['judge.button.save']='保存阈值';I18N.en['judge.button.save']='Save Thresholds';
I18N.zh['judge.button.resetDefault']='恢复默认值';I18N.en['judge.button.resetDefault']='Restore Defaults';
I18N.zh['judge.dim.turnSmoothness']='转弯平滑';I18N.en['judge.dim.turnSmoothness']='Turn Smooth';
I18N.zh['judge.dim.rangeMatch']='区间匹配';I18N.en['judge.dim.rangeMatch']='Range Match';
I18N.zh['judge.dim.gyroStability']='陀螺稳定';I18N.en['judge.dim.gyroStability']='Gyro Stable';
I18N.zh['judge.dim.bigTurnStability']='大弯稳定';I18N.en['judge.dim.bigTurnStability']='Big Turn';
I18N.zh['judge.dim.speedStability']='速度稳定';I18N.en['judge.dim.speedStability']='Speed Stable';
I18N.zh['judge.dim.throttleStability']='油门稳定';I18N.en['judge.dim.throttleStability']='Throttle Stable';
I18N.zh['judge.grade.standby']='待命';I18N.en['judge.grade.standby']='Standby';
I18N.zh['judge.grade.s']='S 级 - 完美';I18N.en['judge.grade.s']='S - Perfect';
I18N.zh['judge.grade.a']='A 级 - 优秀';I18N.en['judge.grade.a']='A - Excellent';
I18N.zh['judge.grade.b']='B 级 - 良好';I18N.en['judge.grade.b']='B - Good';
I18N.zh['judge.grade.c']='C 级 - 一般';I18N.en['judge.grade.c']='C - Fair';
I18N.zh['judge.grade.d']='D 级 - 及格';I18N.en['judge.grade.d']='D - Pass';
I18N.zh['judge.grade.e']='E 级 - 需练习';I18N.en['judge.grade.e']='E - Needs Practice';
I18N.zh['judge.score.lowestPrefix']='当前最低项：';I18N.en['judge.score.lowestPrefix']='Lowest: ';
I18N.zh['judge.score.weakestPrefix']='最近拖分项：';I18N.en['judge.score.weakestPrefix']='Recent weakest: ';
I18N.zh['judge.score.reasonPrefix']='拖分原因：';I18N.en['judge.score.reasonPrefix']='Reason: ';
I18N.zh['judge.score.analyzing']='分析中';I18N.en['judge.score.analyzing']='Analyzing';
I18N.zh['judge.score.lowestInitial']='当前最低项：--';I18N.en['judge.score.lowestInitial']='Lowest: --';
I18N.zh['judge.score.weakestInitial']='最近拖分项：分析中';I18N.en['judge.score.weakestInitial']='Recent weakest: Analyzing';
I18N.zh['judge.score.reasonInitial']='拖分原因：分析中';I18N.en['judge.score.reasonInitial']='Reason: analyzing';
I18N.zh['judge.reason.analyzing']='分析中';I18N.en['judge.reason.analyzing']='Analyzing';
I18N.zh['judge.reason.turnSmoothness']='转弯平滑：最近 gyroZ 变化偏猛，建议下调转弯平滑敏感度。';I18N.en['judge.reason.turnSmoothness']='Turn Smooth: gyroZ has been changing too sharply lately; try lowering Turn Smoothness Sensitivity.';
I18N.zh['judge.reason.rangeMatch']='区间匹配：最近 pseudoSpeed 与转向强度配合不够协调，建议下调区间匹配敏感度。';I18N.en['judge.reason.rangeMatch']='Range Match: pseudoSpeed and steering intensity have not been well coordinated lately; try lowering Range Match Sensitivity.';
I18N.zh['judge.reason.gyroStability']='陀螺稳定：最近 gyroZ 波动偏大，建议下调陀螺稳定敏感度。';I18N.en['judge.reason.gyroStability']='Gyro Stable: gyroZ fluctuation has been high lately; try lowering Gyro Stability Sensitivity.';
I18N.zh['judge.reason.bigTurnStability']='大弯稳定：最近大弯区输出不够稳定，建议先检查大弯稳定敏感度，再检查大弯阈值。';I18N.en['judge.reason.bigTurnStability']='Big Turn: output in the big-turn zone has been unstable lately; check Big-Turn Stability Sensitivity first, then the Big-Turn Threshold.';
I18N.zh['judge.reason.speedStability']='速度稳定：最近 pseudoSpeed 波动偏大，建议下调速度稳定敏感度。';I18N.en['judge.reason.speedStability']='Speed Stable: pseudoSpeed fluctuation has been high lately; try lowering Speed Stability Sensitivity.';
I18N.zh['judge.reason.throttleStability']='油门稳定：最近油门波动偏大，建议下调油门稳定敏感度。';I18N.en['judge.reason.throttleStability']='Throttle Stable: throttle fluctuation has been high lately; try lowering Throttle Stability Sensitivity.';
I18N.zh['judge.collision.triggerPrefix']='碰撞触发 (-';I18N.en['judge.collision.triggerPrefix']='Collision triggered (-';
I18N.zh['judge.collision.ok']='状态正常';I18N.en['judge.collision.ok']='Status OK';
let uiLang=readStoredLanguage();
let uiTheme='auto';
function readUrlTheme(){try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}
function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}
function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}
function readParentTheme(){try{const t=window.parent&&window.parent.document&&window.parent.document.documentElement.dataset.theme;return t==='light'||t==='dark'?t:null}catch(e){return null}}

function applyTheme(){document.documentElement.dataset.theme=resolvedTheme();drawChart();syncBackLink()}
function toggleTheme(){setTheme(resolvedTheme()==='light'?'dark':'light')}
function setTheme(theme){uiTheme=theme;applyTheme()}
function syncBackLink(){const bl=document.getElementById('backLink');if(bl)bl.href='/?theme='+resolvedTheme()}
function renderLangButton(){const b=document.getElementById('langToggle');if(b)b.textContent=uiLang==='zh'?'中':'EN'}
function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);fetch('/api/language?lang='+uiLang,{method:'POST'}).catch(()=>{})}
function toggleLanguage(){setLanguage(uiLang==='zh'?'en':'zh')}
function initTheme(){uiTheme=readUrlTheme()||readParentTheme()||'auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}
let judgeConfigCurrentKey='judge.config.loading',judgeConfigStatusState={key:'judge.config.note',kind:'',detail:''};
function normalizeLanguage(lang){return lang==='en'?'en':'zh'}
function readUrlLanguage(){try{const m=/[?&]lang=(zh|en)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}function readStoredLanguage(){try{const v=localStorage.getItem(LANG_STORAGE_KEY);return v==='zh'||v==='en'?v:detectBrowserLanguage()}catch(e){return detectBrowserLanguage()}}
function writeStoredLanguage(lang){try{localStorage.setItem(LANG_STORAGE_KEY,lang)}catch(e){}}
function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}
function t(key){return (I18N[uiLang]&&I18N[uiLang][key])||I18N.zh[key]||key}
function applyLanguage(lang){uiLang=normalizeLanguage(lang);document.documentElement.lang=uiLang;document.querySelectorAll('[data-i18n]').forEach(e=>{const v=t(e.dataset.i18n);if(v)e.textContent=v});document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>{const v=t(e.dataset.i18nPlaceholder);if(v)e.placeholder=v});document.querySelectorAll('[data-i18n-aria]').forEach(e=>{const v=t(e.dataset.i18nAria);if(v)e.setAttribute('aria-label',v)});document.querySelectorAll('[data-i18n-title]').forEach(e=>{const v=t(e.dataset.i18nTitle);if(v)e.title=v});renderLangButton();refreshDynamicLabels()}
async function initLanguage(){const urlLang=readUrlLanguage();let lang=urlLang;if(!lang)lang=readStoredLanguage();if(!lang)lang=detectBrowserLanguage();applyLanguage(lang);if(document.body)document.body.classList.remove('preinit');if(!urlLang){fetch('/api/language',{cache:'no-store'}).then(r=>{if(!r.ok)return null;return r.json()}).then(j=>{if(!j)return;let srv=null;if(j.lang==='zh'||j.lang==='en')srv=normalizeLanguage(j.lang);else if(j.lang==='auto')srv=detectBrowserLanguage();if(srv&&srv!==uiLang){writeStoredLanguage(srv);applyLanguage(srv)}}).catch(()=>{})}}
const CHART_MAX_POINTS=120;
const DIMENSION_KEYS=['judge.dim.turnSmoothness','judge.dim.rangeMatch','judge.dim.gyroStability','judge.dim.bigTurnStability','judge.dim.speedStability','judge.dim.throttleStability'];
const SCORE_TREND_WINDOW=8;
const SCORE_TREND_DELTA=2.0;
const judgeConfig={collisionThreshold:2.8,bigTurnThreshold:1.6,windowSize:20,collisionPenalty:10,turnSmoothnessWeight:35,rangeMatchWeight:42,gyroStabilityWeight:40,bigTurnStabilityWeight:34,speedStabilityWeight:220,throttleStabilityWeight:180,defaults:{collisionThreshold:2.8,bigTurnThreshold:1.6,windowSize:20,collisionPenalty:10,turnSmoothnessWeight:35,rangeMatchWeight:42,gyroStabilityWeight:40,bigTurnStabilityWeight:34,speedStabilityWeight:220,throttleStabilityWeight:180},limits:{collisionThresholdMin:0.5,collisionThresholdMax:8,bigTurnThresholdMin:0.3,bigTurnThresholdMax:4,windowSizeMin:5,windowSizeMax:64,collisionPenaltyMin:1,collisionPenaltyMax:30,turnSmoothnessWeightMin:10,turnSmoothnessWeightMax:80,rangeMatchWeightMin:10,rangeMatchWeightMax:100,gyroStabilityWeightMin:10,gyroStabilityWeightMax:100,bigTurnStabilityWeightMin:10,bigTurnStabilityWeightMax:90,speedStabilityWeightMin:40,speedStabilityWeightMax:400,throttleStabilityWeightMin:40,throttleStabilityWeightMax:360}};
let lastSeq=0,dataWs=null,dataWsConnected=false,dataWsReconnectDelay=1000,dataWsReconnectTimer=0,dataPolling=false;
let chartData=[];
let scoreState=createScoreState();
function createScoreState(){
  return{
    running:false,
    samples:0,
    totalScore:0,
    penalty:0,
    dimensionScores:[0,0,0,0,0,0],
    dimensionSums:[0,0,0,0,0,0],
    dimensionTrendWindows:[[],[],[],[],[],[]],
    dimensionTrendStates:[
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false},
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false},
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false},
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false},
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false},
      {symbol:'→',className:'dimTrendFlat',delta:0,ready:false}
    ],
    lowestDimension:'',
    weakestTrend:'',
    weakestTrendReason:'judge.reason.analyzing',
    gyroHistory:[],
    pseudoHistory:[],
    throttleHistory:[],
    lastGyroZ:null,
    collisionCooldown:0,
    inTurn:false
  }
}
function clamp(v,min,max){return Math.max(min,Math.min(max,v))}
function setStatus(text,kind){statusEl.textContent=text;statusPillEl.className='statusPill '+(kind||'statusWaiting')}
function semText(n,f){try{const v=getComputedStyle(document.documentElement).getPropertyValue(n);return (v&&v.trim())||f}catch(e){return f}}
function setJudgeConfigStatus(key,kind,detail){judgeConfigStatusState={key:key,kind:kind||'',detail:detail||''};judgeConfigStatusEl.textContent=t(key)+(detail||'');judgeConfigStatusEl.style.color=kind==='ok'?semText('--ok-text','#39d98a'):kind==='err'?semText('--bad-text','#ff7b7b'):semText('--status-info','#8fa1b5')}
function setJudgeConfigBusy(busy){saveJudgeConfigBtn.disabled=busy;resetJudgeConfigBtn.disabled=busy}
function getGrade(score){if(score>=95)return'judge.grade.s';if(score>=90)return'judge.grade.a';if(score>=80)return'judge.grade.b';if(score>=70)return'judge.grade.c';if(score>=60)return'judge.grade.d';return'judge.grade.e'}
function mean(values){if(!values.length)return 0;let sum=0;for(let i=0;i<values.length;i++)sum+=values[i];return sum/values.length}
function stdDev(values){if(values.length<2)return 0;const m=mean(values);let sum=0;for(let i=0;i<values.length;i++){const d=values[i]-m;sum+=d*d}return Math.sqrt(sum/values.length)}
function getWindowSize(){return clamp(Math.round(Number(judgeConfig.windowSize||20)),judgeConfig.limits.windowSizeMin,judgeConfig.limits.windowSizeMax)}
function pushWindow(list,value){const size=getWindowSize();list.push(value);while(list.length>size)list.shift()}
function pushFixedWindow(list,value,size){list.push(value);while(list.length>size)list.shift()}
function trimScoreWindows(){const size=getWindowSize(),lists=[scoreState.gyroHistory,scoreState.pseudoHistory,scoreState.throttleHistory];for(let i=0;i<lists.length;i++)while(lists[i].length>size)lists[i].shift()}
function computeDimensionTrend(values){
  if(values.length<SCORE_TREND_WINDOW){
    return{symbol:'→',className:'dimTrendFlat',delta:0,ready:false}
  }
  const half=Math.floor(values.length/2);
  const older=values.slice(0,half);
  const newer=values.slice(half);
  if(older.length<2||newer.length<2){
    return{symbol:'→',className:'dimTrendFlat',delta:0,ready:false}
  }
  const delta=mean(newer)-mean(older);
  if(delta>SCORE_TREND_DELTA){
    return{symbol:'↑',className:'dimTrendUp',delta:delta,ready:true}
  }
  if(delta<-SCORE_TREND_DELTA){
    return{symbol:'↓',className:'dimTrendDown',delta:delta,ready:true}
  }
  return{symbol:'→',className:'dimTrendFlat',delta:delta,ready:true}
}
function getWeakestTrendReason(key){return key?key.replace('judge.dim.','judge.reason.'):'judge.reason.analyzing'}
function refreshScoreBreakdown(){
  let lowestIndex=0;
  for(let i=1;i<scoreState.dimensionScores.length;i++){
    if(scoreState.dimensionScores[i]<scoreState.dimensionScores[lowestIndex])lowestIndex=i;
  }
  scoreState.lowestDimension=scoreState.samples?DIMENSION_KEYS[lowestIndex]:'';
  if(!scoreState.running||scoreState.samples<SCORE_TREND_WINDOW){
    scoreState.weakestTrend='';
    scoreState.weakestTrendReason='judge.reason.analyzing';
    return;
  }
  let weakestIndex=-1;
  let weakestDelta=0;
  for(let i=0;i<scoreState.dimensionTrendStates.length;i++){
    const trend=scoreState.dimensionTrendStates[i];
    if(!trend.ready)continue;
    if(weakestIndex===-1||trend.delta<weakestDelta){
      weakestIndex=i;
      weakestDelta=trend.delta;
    }
  }
  scoreState.weakestTrend=weakestIndex===-1?'':DIMENSION_KEYS[weakestIndex];
  scoreState.weakestTrendReason=getWeakestTrendReason(scoreState.weakestTrend);
}
function syncJudgeConfigInputs(){collisionThresholdInput.min=judgeConfig.limits.collisionThresholdMin;collisionThresholdInput.max=judgeConfig.limits.collisionThresholdMax;bigTurnThresholdInput.min=judgeConfig.limits.bigTurnThresholdMin;bigTurnThresholdInput.max=judgeConfig.limits.bigTurnThresholdMax;windowSizeInput.min=judgeConfig.limits.windowSizeMin;windowSizeInput.max=judgeConfig.limits.windowSizeMax;collisionPenaltyInput.min=judgeConfig.limits.collisionPenaltyMin;collisionPenaltyInput.max=judgeConfig.limits.collisionPenaltyMax;turnSmoothnessWeightInput.min=judgeConfig.limits.turnSmoothnessWeightMin;turnSmoothnessWeightInput.max=judgeConfig.limits.turnSmoothnessWeightMax;rangeMatchWeightInput.min=judgeConfig.limits.rangeMatchWeightMin;rangeMatchWeightInput.max=judgeConfig.limits.rangeMatchWeightMax;gyroStabilityWeightInput.min=judgeConfig.limits.gyroStabilityWeightMin;gyroStabilityWeightInput.max=judgeConfig.limits.gyroStabilityWeightMax;bigTurnStabilityWeightInput.min=judgeConfig.limits.bigTurnStabilityWeightMin;bigTurnStabilityWeightInput.max=judgeConfig.limits.bigTurnStabilityWeightMax;speedStabilityWeightInput.min=judgeConfig.limits.speedStabilityWeightMin;speedStabilityWeightInput.max=judgeConfig.limits.speedStabilityWeightMax;throttleStabilityWeightInput.min=judgeConfig.limits.throttleStabilityWeightMin;throttleStabilityWeightInput.max=judgeConfig.limits.throttleStabilityWeightMax;collisionThresholdInput.value=Number(judgeConfig.collisionThreshold).toFixed(2);bigTurnThresholdInput.value=Number(judgeConfig.bigTurnThreshold).toFixed(2);windowSizeInput.value=String(getWindowSize());collisionPenaltyInput.value=Number(judgeConfig.collisionPenalty).toFixed(1);turnSmoothnessWeightInput.value=Number(judgeConfig.turnSmoothnessWeight).toFixed(1);rangeMatchWeightInput.value=Number(judgeConfig.rangeMatchWeight).toFixed(1);gyroStabilityWeightInput.value=Number(judgeConfig.gyroStabilityWeight).toFixed(1);bigTurnStabilityWeightInput.value=Number(judgeConfig.bigTurnStabilityWeight).toFixed(1);speedStabilityWeightInput.value=Number(judgeConfig.speedStabilityWeight).toFixed(1);throttleStabilityWeightInput.value=Number(judgeConfig.throttleStabilityWeight).toFixed(1);judgeConfigCurrentKey='judge.config.synced';judgeConfigCurrentEl.textContent=t(judgeConfigCurrentKey);judgeConfigSummaryEl.innerHTML='<div class=\"summaryItem\"><div class=\"k\">'+t('judge.summary.collisionThreshold')+'</div><div class=\"v\">'+Number(judgeConfig.collisionThreshold).toFixed(2)+'</div></div><div class=\"summaryItem\"><div class=\"k\">'+t('judge.summary.bigTurnThreshold')+'</div><div class=\"v\">'+Number(judgeConfig.bigTurnThreshold).toFixed(2)+'</div></div><div class=\"summaryItem\"><div class=\"k\">'+t('judge.summary.windowSize')+'</div><div class=\"v\">'+String(getWindowSize())+'</div></div><div class=\"summaryItem\"><div class=\"k\">'+t('judge.summary.collisionPenalty')+'</div><div class=\"v\">'+Number(judgeConfig.collisionPenalty).toFixed(1)+'</div></div>'}
function applyJudgeConfigPayload(payload){if(!payload)return;const config=payload.config||payload;const defaults=config.defaults||payload.defaults||{};const limits=config.limits||payload.limits||{};judgeConfig.collisionThreshold=Number(config.collisionThreshold??judgeConfig.collisionThreshold);judgeConfig.bigTurnThreshold=Number(config.bigTurnThreshold??judgeConfig.bigTurnThreshold);judgeConfig.windowSize=Math.round(Number(config.windowSize??judgeConfig.windowSize));judgeConfig.collisionPenalty=Number(config.collisionPenalty??judgeConfig.collisionPenalty);judgeConfig.turnSmoothnessWeight=Number(config.turnSmoothnessWeight??judgeConfig.turnSmoothnessWeight);judgeConfig.rangeMatchWeight=Number(config.rangeMatchWeight??judgeConfig.rangeMatchWeight);judgeConfig.gyroStabilityWeight=Number(config.gyroStabilityWeight??judgeConfig.gyroStabilityWeight);judgeConfig.bigTurnStabilityWeight=Number(config.bigTurnStabilityWeight??judgeConfig.bigTurnStabilityWeight);judgeConfig.speedStabilityWeight=Number(config.speedStabilityWeight??judgeConfig.speedStabilityWeight);judgeConfig.throttleStabilityWeight=Number(config.throttleStabilityWeight??judgeConfig.throttleStabilityWeight);judgeConfig.defaults.collisionThreshold=Number(defaults.collisionThreshold??judgeConfig.defaults.collisionThreshold);judgeConfig.defaults.bigTurnThreshold=Number(defaults.bigTurnThreshold??judgeConfig.defaults.bigTurnThreshold);judgeConfig.defaults.windowSize=Math.round(Number(defaults.windowSize??judgeConfig.defaults.windowSize));judgeConfig.defaults.collisionPenalty=Number(defaults.collisionPenalty??judgeConfig.defaults.collisionPenalty);judgeConfig.defaults.turnSmoothnessWeight=Number(defaults.turnSmoothnessWeight??judgeConfig.defaults.turnSmoothnessWeight);judgeConfig.defaults.rangeMatchWeight=Number(defaults.rangeMatchWeight??judgeConfig.defaults.rangeMatchWeight);judgeConfig.defaults.gyroStabilityWeight=Number(defaults.gyroStabilityWeight??judgeConfig.defaults.gyroStabilityWeight);judgeConfig.defaults.bigTurnStabilityWeight=Number(defaults.bigTurnStabilityWeight??judgeConfig.defaults.bigTurnStabilityWeight);judgeConfig.defaults.speedStabilityWeight=Number(defaults.speedStabilityWeight??judgeConfig.defaults.speedStabilityWeight);judgeConfig.defaults.throttleStabilityWeight=Number(defaults.throttleStabilityWeight??judgeConfig.defaults.throttleStabilityWeight);judgeConfig.limits.collisionThresholdMin=Number(limits.collisionThresholdMin??judgeConfig.limits.collisionThresholdMin);judgeConfig.limits.collisionThresholdMax=Number(limits.collisionThresholdMax??judgeConfig.limits.collisionThresholdMax);judgeConfig.limits.bigTurnThresholdMin=Number(limits.bigTurnThresholdMin??judgeConfig.limits.bigTurnThresholdMin);judgeConfig.limits.bigTurnThresholdMax=Number(limits.bigTurnThresholdMax??judgeConfig.limits.bigTurnThresholdMax);judgeConfig.limits.windowSizeMin=Math.round(Number(limits.windowSizeMin??judgeConfig.limits.windowSizeMin));judgeConfig.limits.windowSizeMax=Math.round(Number(limits.windowSizeMax??judgeConfig.limits.windowSizeMax));judgeConfig.limits.collisionPenaltyMin=Number(limits.collisionPenaltyMin??judgeConfig.limits.collisionPenaltyMin);judgeConfig.limits.collisionPenaltyMax=Number(limits.collisionPenaltyMax??judgeConfig.limits.collisionPenaltyMax);judgeConfig.limits.turnSmoothnessWeightMin=Number(limits.turnSmoothnessWeightMin??judgeConfig.limits.turnSmoothnessWeightMin);judgeConfig.limits.turnSmoothnessWeightMax=Number(limits.turnSmoothnessWeightMax??judgeConfig.limits.turnSmoothnessWeightMax);judgeConfig.limits.rangeMatchWeightMin=Number(limits.rangeMatchWeightMin??judgeConfig.limits.rangeMatchWeightMin);judgeConfig.limits.rangeMatchWeightMax=Number(limits.rangeMatchWeightMax??judgeConfig.limits.rangeMatchWeightMax);judgeConfig.limits.gyroStabilityWeightMin=Number(limits.gyroStabilityWeightMin??judgeConfig.limits.gyroStabilityWeightMin);judgeConfig.limits.gyroStabilityWeightMax=Number(limits.gyroStabilityWeightMax??judgeConfig.limits.gyroStabilityWeightMax);judgeConfig.limits.bigTurnStabilityWeightMin=Number(limits.bigTurnStabilityWeightMin??judgeConfig.limits.bigTurnStabilityWeightMin);judgeConfig.limits.bigTurnStabilityWeightMax=Number(limits.bigTurnStabilityWeightMax??judgeConfig.limits.bigTurnStabilityWeightMax);judgeConfig.limits.speedStabilityWeightMin=Number(limits.speedStabilityWeightMin??judgeConfig.limits.speedStabilityWeightMin);judgeConfig.limits.speedStabilityWeightMax=Number(limits.speedStabilityWeightMax??judgeConfig.limits.speedStabilityWeightMax);judgeConfig.limits.throttleStabilityWeightMin=Number(limits.throttleStabilityWeightMin??judgeConfig.limits.throttleStabilityWeightMin);judgeConfig.limits.throttleStabilityWeightMax=Number(limits.throttleStabilityWeightMax??judgeConfig.limits.throttleStabilityWeightMax);trimScoreWindows();syncJudgeConfigInputs()}
function readJudgeConfigForm(){return{collisionThreshold:Number(collisionThresholdInput.value),bigTurnThreshold:Number(bigTurnThresholdInput.value),windowSize:Math.round(Number(windowSizeInput.value)),collisionPenalty:Number(collisionPenaltyInput.value),turnSmoothnessWeight:Number(turnSmoothnessWeightInput.value),rangeMatchWeight:Number(rangeMatchWeightInput.value),gyroStabilityWeight:Number(gyroStabilityWeightInput.value),bigTurnStabilityWeight:Number(bigTurnStabilityWeightInput.value),speedStabilityWeight:Number(speedStabilityWeightInput.value),throttleStabilityWeight:Number(throttleStabilityWeightInput.value)}}
function judgeConfigFormValid(config){return Number.isFinite(config.collisionThreshold)&&Number.isFinite(config.bigTurnThreshold)&&Number.isFinite(config.windowSize)&&Number.isFinite(config.collisionPenalty)&&Number.isFinite(config.turnSmoothnessWeight)&&Number.isFinite(config.rangeMatchWeight)&&Number.isFinite(config.gyroStabilityWeight)&&Number.isFinite(config.bigTurnStabilityWeight)&&Number.isFinite(config.speedStabilityWeight)&&Number.isFinite(config.throttleStabilityWeight)&&config.collisionThreshold>=judgeConfig.limits.collisionThresholdMin&&config.collisionThreshold<=judgeConfig.limits.collisionThresholdMax&&config.bigTurnThreshold>=judgeConfig.limits.bigTurnThresholdMin&&config.bigTurnThreshold<=judgeConfig.limits.bigTurnThresholdMax&&config.windowSize>=judgeConfig.limits.windowSizeMin&&config.windowSize<=judgeConfig.limits.windowSizeMax&&config.collisionPenalty>=judgeConfig.limits.collisionPenaltyMin&&config.collisionPenalty<=judgeConfig.limits.collisionPenaltyMax&&config.turnSmoothnessWeight>=judgeConfig.limits.turnSmoothnessWeightMin&&config.turnSmoothnessWeight<=judgeConfig.limits.turnSmoothnessWeightMax&&config.rangeMatchWeight>=judgeConfig.limits.rangeMatchWeightMin&&config.rangeMatchWeight<=judgeConfig.limits.rangeMatchWeightMax&&config.gyroStabilityWeight>=judgeConfig.limits.gyroStabilityWeightMin&&config.gyroStabilityWeight<=judgeConfig.limits.gyroStabilityWeightMax&&config.bigTurnStabilityWeight>=judgeConfig.limits.bigTurnStabilityWeightMin&&config.bigTurnStabilityWeight<=judgeConfig.limits.bigTurnStabilityWeightMax&&config.speedStabilityWeight>=judgeConfig.limits.speedStabilityWeightMin&&config.speedStabilityWeight<=judgeConfig.limits.speedStabilityWeightMax&&config.throttleStabilityWeight>=judgeConfig.limits.throttleStabilityWeightMin&&config.throttleStabilityWeight<=judgeConfig.limits.throttleStabilityWeightMax}
async function loadJudgeConfig(){try{const r=await fetch('/api/judge-config',{cache:'no-store'});if(!r.ok)throw new Error('load_failed');applyJudgeConfigPayload(await r.json());setJudgeConfigStatus('judge.config.loaded','ok')}catch(e){syncJudgeConfigInputs();setJudgeConfigStatus('judge.config.loadFailed','err')}}
async function saveJudgeConfig(){const config=readJudgeConfigForm();if(!judgeConfigFormValid(config)){setJudgeConfigStatus('judge.config.invalid','err');return}const body=new URLSearchParams({collisionThreshold:config.collisionThreshold.toFixed(2),bigTurnThreshold:config.bigTurnThreshold.toFixed(2),windowSize:String(config.windowSize),collisionPenalty:config.collisionPenalty.toFixed(1),turnSmoothnessWeight:config.turnSmoothnessWeight.toFixed(1),rangeMatchWeight:config.rangeMatchWeight.toFixed(1),gyroStabilityWeight:config.gyroStabilityWeight.toFixed(1),bigTurnStabilityWeight:config.bigTurnStabilityWeight.toFixed(1),speedStabilityWeight:config.speedStabilityWeight.toFixed(1),throttleStabilityWeight:config.throttleStabilityWeight.toFixed(1)});try{setJudgeConfigBusy(true);const r=await fetch('/api/judge-config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body:body.toString()});const text=await r.text();let payload={};try{payload=JSON.parse(text)}catch(e){}if(!r.ok)throw new Error(payload.error||'save_failed');applyJudgeConfigPayload(payload);setJudgeConfigStatus('judge.config.saved','ok')}catch(e){setJudgeConfigStatus('judge.config.saveFailed','err',e.message||'save_failed')}finally{setJudgeConfigBusy(false)}}
async function resetJudgeConfigToDefault(){try{setJudgeConfigBusy(true);const r=await fetch('/api/judge-config/reset',{method:'POST'});const text=await r.text();let payload={};try{payload=JSON.parse(text)}catch(e){}if(!r.ok)throw new Error(payload.error||'reset_failed');applyJudgeConfigPayload(payload);setJudgeConfigStatus('judge.config.resetOk','ok')}catch(e){setJudgeConfigStatus('judge.config.resetFailed','err',e.message||'reset_failed')}finally{setJudgeConfigBusy(false)}}
function pushChartValue(value){chartData.push(Number(value||0));if(chartData.length>CHART_MAX_POINTS)chartData.shift();drawChart()}
function drawChart(){const w=chartCanvas.width,h=chartCanvas.height;chartCtx.clearRect(0,0,w,h);const light=resolvedTheme()==='light';chartCtx.fillStyle=light?'#f4f6f9':'#0f1720';chartCtx.fillRect(0,0,w,h);chartCtx.strokeStyle=light?'#d5dce4':'#223042';chartCtx.lineWidth=1;for(let i=0;i<5;i++){const y=Math.round(i*(h-1)/4)+.5;chartCtx.beginPath();chartCtx.moveTo(0,y);chartCtx.lineTo(w,y);chartCtx.stroke()}if(chartData.length<2)return;chartCtx.strokeStyle=light?'#0c9bd6':'#5cc8ff';chartCtx.lineWidth=2;chartCtx.beginPath();const maxAbs=3.5;for(let i=0;i<chartData.length;i++){const x=(i*(w-1))/Math.max(1,CHART_MAX_POINTS-1);const y=h/2-clamp(chartData[i],-maxAbs,maxAbs)*(h*0.42/maxAbs);if(i===0)chartCtx.moveTo(x,y);else chartCtx.lineTo(x,y)}chartCtx.stroke()}
function renderScore(){
  const score=Math.round(clamp(scoreState.totalScore,0,100));
  totalScoreEl.textContent=String(score);
  scoreGradeEl.textContent=scoreState.running?t(getGrade(score)):t('judge.grade.standby');
  lowestDimensionEl.textContent=t('judge.score.lowestPrefix')+(scoreState.lowestDimension?t(scoreState.lowestDimension):'--');
  weakestTrendEl.textContent=t('judge.score.weakestPrefix')+((scoreState.samples&&scoreState.weakestTrend)?t(scoreState.weakestTrend):t('judge.score.analyzing'));
  weakestTrendReasonEl.textContent=t('judge.score.reasonPrefix')+t(scoreState.weakestTrendReason);
  for(let i=1;i<=6;i++){
    const dim=Math.round(clamp(scoreState.dimensionScores[i-1]||0,0,100));
    const trend=scoreState.dimensionTrendStates[i-1];
    document.getElementById('dim'+i+'-score').textContent=String(dim);
    document.getElementById('dim'+i+'-fill').style.width=dim+'%';
    const trendEl=document.getElementById('dim'+i+'-trend');
    trendEl.textContent=trend.symbol;
    trendEl.className='dimTrend '+trend.className;
  }
}
function refreshDynamicLabels(){renderScore();startBtn.textContent=scoreState.running?t('judge.button.stop'):t('judge.button.start');if(!collisionEl.classList.contains('active'))collisionEl.textContent=t('judge.collision.ok');judgeConfigCurrentEl.textContent=t(judgeConfigCurrentKey);setJudgeConfigStatus(judgeConfigStatusState.key,judgeConfigStatusState.kind,judgeConfigStatusState.detail);const summaryKeys=['judge.summary.collisionThreshold','judge.summary.bigTurnThreshold','judge.summary.windowSize','judge.summary.collisionPenalty'];judgeConfigSummaryEl.querySelectorAll('.k').forEach((el,i)=>{if(summaryKeys[i])el.textContent=t(summaryKeys[i])})}
function showCollision(){collisionEl.classList.add('active');collisionEl.textContent=t('judge.collision.triggerPrefix')+Number(judgeConfig.collisionPenalty).toFixed(1)+')';clearTimeout(showCollision.timer);showCollision.timer=setTimeout(()=>{collisionEl.classList.remove('active');collisionEl.textContent=t('judge.collision.ok')},700)}
function resetScore(){scoreState=createScoreState();startBtn.textContent=t('judge.button.start');collisionEl.classList.remove('active');collisionEl.textContent=t('judge.collision.ok');renderScore()}
function stopRun(){scoreState.running=false;startBtn.textContent=t('judge.button.start');renderScore()}
function startRun(){if(scoreState.running){stopRun();return}const carryLastGyro=scoreState.lastGyroZ;scoreState=createScoreState();scoreState.running=true;scoreState.lastGyroZ=carryLastGyro;trimScoreWindows();startBtn.textContent=t('judge.button.stop');renderScore()}
function detectCollision(gz){if(scoreState.lastGyroZ===null){scoreState.lastGyroZ=gz;return}if(scoreState.collisionCooldown>0){scoreState.collisionCooldown--;scoreState.lastGyroZ=gz;return}const delta=Math.abs(gz-scoreState.lastGyroZ);if(delta>judgeConfig.collisionThreshold){scoreState.collisionCooldown=12;showCollision();if(scoreState.running)scoreState.penalty+=judgeConfig.collisionPenalty}scoreState.lastGyroZ=gz}
function calcTurnSmoothness(gz){if(scoreState.gyroHistory.length<5)return 80;let total=0;for(let i=1;i<scoreState.gyroHistory.length;i++)total+=Math.abs(scoreState.gyroHistory[i]-scoreState.gyroHistory[i-1]);const avgChange=total/(scoreState.gyroHistory.length-1);if(Math.abs(gz)<0.18)return 100;return clamp(100-avgChange*judgeConfig.turnSmoothnessWeight,0,100)}
function calcRangeMatch(gz,pseudo){const absGyro=Math.abs(gz),absPseudo=Math.abs(pseudo);if(absPseudo<5)return 70;const idealGyro=0.25+(absPseudo/100)*2.5;const diff=Math.abs(absGyro-idealGyro);return clamp(100-(diff/Math.max(.45,idealGyro))*judgeConfig.rangeMatchWeight,0,100)}
function calcGyroStability(){if(scoreState.gyroHistory.length<8)return 75;return clamp(100-stdDev(scoreState.gyroHistory)*judgeConfig.gyroStabilityWeight,0,100)}
function calcBigTurnStability(gz){const absGyro=Math.abs(gz),threshold=judgeConfig.bigTurnThreshold;if(absGyro>threshold)scoreState.inTurn=true;else if(absGyro<threshold*.45)scoreState.inTurn=false;if(!scoreState.inTurn)return 80;const recent=scoreState.gyroHistory.slice(-10);if(recent.length<6)return 70;return clamp(100-stdDev(recent)*judgeConfig.bigTurnStabilityWeight,0,100)}
function calcPseudoSpeedStability(){if(scoreState.pseudoHistory.length<8)return 75;const m=mean(scoreState.pseudoHistory);if(m<5)return 70;return clamp(100-(stdDev(scoreState.pseudoHistory)/Math.max(1,m))*judgeConfig.speedStabilityWeight,0,100)}
function calcThrottleStability(){if(scoreState.throttleHistory.length<8)return 75;const absValues=scoreState.throttleHistory.map(v=>Math.abs(v));const m=mean(absValues);if(m<5)return 70;return clamp(100-(stdDev(absValues)/(m+1))*judgeConfig.throttleStabilityWeight,0,100)}
function updateScore(latest){
  const gz=Number(latest.gz||0),pseudo=clamp(Number(latest.pseudoSpeed||0),0,100),thr=Number(latest.thr||0);
  pushWindow(scoreState.gyroHistory,gz);
  pushWindow(scoreState.pseudoHistory,pseudo);
  pushWindow(scoreState.throttleHistory,thr);
  scoreState.samples++;
  const current=[calcTurnSmoothness(gz),calcRangeMatch(gz,pseudo),calcGyroStability(),calcBigTurnStability(gz),calcPseudoSpeedStability(),calcThrottleStability()];
  for(let i=0;i<current.length;i++){
    scoreState.dimensionSums[i]+=current[i];
    scoreState.dimensionScores[i]=scoreState.dimensionSums[i]/scoreState.samples;
    pushFixedWindow(scoreState.dimensionTrendWindows[i],current[i],SCORE_TREND_WINDOW);
    scoreState.dimensionTrendStates[i]=computeDimensionTrend(scoreState.dimensionTrendWindows[i]);
  }
  scoreState.totalScore=clamp(mean(scoreState.dimensionScores)-scoreState.penalty,0,100);
  refreshScoreBreakdown();
  renderScore();
}
function renderLatest(latest,transport){if(!latest)return;lastSeq=Math.max(lastSeq,Number(latest.seq||0));const pseudo=clamp(Number(latest.pseudoSpeed||0),0,100),gz=Number(latest.gz||0),thr=Number(latest.thr||0);setStatus('online / '+transport,'statusOnline');statusPillEl.title='seq '+(latest.seq??'--');pseudoSpeedEl.textContent=pseudo.toFixed(1);pseudoBarEl.style.width=pseudo.toFixed(1)+'%';gyroZEl.textContent=gz.toFixed(3);throttleEl.textContent=String(Math.round(thr));transportEl.textContent=transport.toUpperCase();pushChartValue(gz);detectCollision(gz);if(scoreState.running)updateScore(latest)}
function handleDataPayload(j,transport){const arr=(j&&j.points)||[];for(let i=0;i<arr.length;i++)pushChartValue(Number(arr[i].gz||0));if(j&&j.latest){renderLatest(j.latest,transport);return}setStatus('waiting data','statusWaiting')}
function decodeBinaryDataPayload(buffer){const v=new DataView(buffer);let o=0;const u8=()=>v.getUint8(o++),u16=()=>{const x=v.getUint16(o,true);o+=2;return x},u32=()=>{const x=v.getUint32(o,true);o+=4;return x},i16=()=>{const x=v.getInt16(o,true);o+=2;return x},f32=()=>{const x=v.getFloat32(o,true);o+=4;return x};if(u8()!==77||u8()!==52)throw new Error('bad magic');const version=u8();u8();if(version!==2)throw new Error('bad version');const dropped=u32(),seq=u32(),ts=u32(),dt=u16(),thr=i16(),str=i16(),gz=f32(),gx=f32(),gy=f32(),ax=f32(),ay=f32(),az=f32(),mode=u8(),park=u8();const ch=[u16(),u16(),u16(),u16(),u16(),u16()];const latest={seq,t:ts,dt,thr,str,gz,gx,gy,ax,ay,az,mode,park,ch1:ch[0],ch2:ch[1],ch3:ch[2],ch4:ch[3],ch5:ch[4],ch6:ch[5],rct:i16(),rcs:i16(),pt:i16(),ps:i16(),gzf:f32(),dc:f32(),de:u8(),da:u8(),vol:f32(),pseudoSpeed:f32(),sd:u16(),ed:u16(),sm:u16(),mm:u16(),tl:i16(),tu:i16()};const count=u8(),points=[];for(let i=0;i<count;i++)points.push({seq:u32(),t:u32(),dt:u16(),thr:i16(),str:i16(),gz:f32()});return{type:'data',dropped,latest,points}}
function dataWsUrl(){return(location.protocol==='https:'?'wss:':'ws:')+'//'+location.hostname+':81/'}
function scheduleDataWsReconnect(){if(dataWsReconnectTimer)return;dataWsReconnectTimer=setTimeout(()=>{dataWsReconnectTimer=0;connectJudgeSocket();dataWsReconnectDelay=Math.min(8000,dataWsReconnectDelay*2)},dataWsReconnectDelay)}
function connectJudgeSocket(){try{if(dataWs&&dataWs.readyState!==WebSocket.CLOSED)return;if(dataWs){dataWs.onclose=null;dataWs.onerror=null;try{dataWs.close()}catch(e){}}const ws=new WebSocket(dataWsUrl());dataWs=ws;ws.binaryType='arraybuffer';ws.onopen=()=>{if(dataWs!==ws){ws.close();return}dataWsConnected=true;dataWsReconnectDelay=1000;setStatus('online / ws','statusOnline');ws.send('since:'+lastSeq)};ws.onmessage=e=>{if(dataWs!==ws)return;try{if(e.data instanceof ArrayBuffer){handleDataPayload(decodeBinaryDataPayload(e.data),'ws');return}if(e.data instanceof Blob){e.data.arrayBuffer().then(b=>{if(dataWs===ws)handleDataPayload(decodeBinaryDataPayload(b),'ws')}).catch(()=>setStatus('offline','statusOffline'));return}if(typeof e.data==='string'){const j=JSON.parse(e.data);if(j&&j.type==='data')handleDataPayload(j,'ws')}}catch(err){setStatus('offline','statusOffline')}};ws.onclose=()=>{if(dataWs!==ws)return;dataWsConnected=false;dataWs=null;setStatus('offline','statusOffline');scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollJudgeData,500)};ws.onerror=()=>{if(dataWs!==ws)return;dataWsConnected=false;try{ws.close()}catch(e){}}}catch(e){dataWsConnected=false;dataWs=null;setStatus('offline','statusOffline');scheduleDataWsReconnect();if(!dataPolling)setTimeout(pollJudgeData,500)}}
async function pollJudgeData(){if(dataWsConnected)return;if(dataPolling)return;dataPolling=true;let delay=160;try{const r=await fetch('/api/data?since='+lastSeq,{cache:'no-store'});const j=await r.json();handleDataPayload(j,'poll');delay=(j&&j.points&&j.points.length)?80:140}catch(e){setStatus('offline','statusOffline');delay=220}finally{dataPolling=false;if(!dataWsConnected)setTimeout(pollJudgeData,delay)}}
if(location.search.indexOf('embedded=1')>=0)document.body.classList.add('embedded');syncJudgeConfigInputs();initTheme();drawChart();renderScore();initLanguage();loadJudgeConfig();connectJudgeSocket();setTimeout(()=>{if(!dataWsConnected)pollJudgeData()},1200);
</script>
</body>
</html>
)rawliteral";static const char WIFI_WEB_DRIFT_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<link rel="icon" type="image/png" href="/favicon.png">
<title>Drift Assist Tuning</title>
<script>try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);document.documentElement.dataset.theme=m?m[1]:(window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark')}catch(e){}</script>
<style>
body.preinit{visibility:hidden}
:root{--bg:#000;--ink:#f5f5f7;--ink2:rgba(235,235,245,.85);--ink4:#f5f5f7;--inkF:rgba(235,235,245,.72);--panel:#1c1c1e;--card:#1c1c1e;--input:#1c1c1e;--line:rgba(255,255,255,.10);--line2:rgba(255,255,255,.10);--line3:rgba(255,255,255,.10);--accent:#2997ff;--accentFill:#0a84ff;--accentHi:#2997ff;--onAccent:#fff;--cardShadow:none;--segHover:transparent;--inkHi:#f5f5f7;--ease-apple:cubic-bezier(.32,.72,0,1);--ok-text:#30d158;--warn-text:#ff9f0a;--bad-text:#ff453a;--status-info:rgba(235,235,245,.72);--sep:rgba(255,255,255,.16);--cardLine:rgba(255,255,255,.10);--matSolid:rgba(28,28,30,.96);--elev:0 8px 30px rgba(0,0,0,.5);--appleFont:-apple-system,BlinkMacSystemFont,system-ui,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;--appleMono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}html[data-theme="light"]{--bg:#f5f5f7;--ink:#1d1d1f;--ink2:rgba(60,60,67,.85);--ink4:#1d1d1f;--inkF:rgba(60,60,67,.72);--panel:#fff;--card:#fff;--input:#f5f5f7;--line:rgba(0,0,0,.08);--line2:rgba(0,0,0,.08);--line3:rgba(0,0,0,.08);--accent:#0066cc;--accentFill:#0071e3;--accentHi:#0066cc;--onAccent:#fff;--cardShadow:none;--segHover:transparent;--inkHi:#1d1d1f;--ok-text:#1a7f37;--warn-text:#c93400;--bad-text:#d70015;--status-info:rgba(60,60,67,.72);--sep:rgba(60,60,67,.29);--cardLine:rgba(0,0,0,.08);--matSolid:rgba(245,245,247,.96);--elev:0 8px 30px rgba(0,0,0,.12)}html:root h1{font-weight:600;letter-spacing:-0.02em}html:root button{font-weight:600!important}html:root .summaryItem,html:root .field{border-radius:14px}html:root .field input{border-radius:8px}html:root .summaryItem .v{font-weight:600;font-variant-numeric:tabular-nums}html:root .fieldTitle,html:root .tuneSection .sectionTitle{font-weight:600}html:root button:active{transform:scale(.97)}body{font-family:system-ui,sans-serif;margin:12px;background:var(--bg);color:var(--ink)}h1{margin:0;font-size:22px}.headerRow{display:flex;align-items:flex-end;gap:12px;flex-wrap:wrap;margin:0 0 10px}.version{color:var(--inkF);font-size:12px;text-transform:uppercase;letter-spacing:.08em}.panel{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:10px;margin-bottom:10px}.panelHead{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;flex-wrap:wrap;margin-bottom:8px}.label{font-size:12px;color:var(--ink2);text-transform:uppercase;letter-spacing:.08em}.muted{color:var(--ink2);font-size:12px}.summaryGrid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:12px}.summaryItem{background:var(--card);border:1px solid var(--line2);border-radius:10px;padding:10px 12px;box-shadow:var(--cardShadow)}.summaryItem .k{font-size:11px;color:var(--inkF)}.summaryItem .v{font-size:18px;font-weight:700;margin-top:4px}.tuneSection{margin-top:16px}.tuneSection .sectionTitle{font-size:13px;font-weight:700;color:var(--ink4)}.tuneGrid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-top:12px}.field{display:flex;flex-direction:column;gap:6px;font-size:12px;color:var(--inkF);background:var(--card);border:1px solid var(--line2);border-radius:10px;padding:10px 12px;box-shadow:var(--cardShadow)}.fieldTitle{font-size:12px;color:var(--ink4);font-weight:700}.fieldHint{font-size:11px;color:var(--ink2);line-height:1.35;min-height:28px}.field input{background:var(--input);border:1px solid var(--line3);border-radius:10px;color:var(--ink);padding:10px 12px;font:inherit}.tuneActions{display:flex;gap:10px;align-items:center;flex-wrap:wrap;margin-top:12px}button{background:var(--accentFill);color:var(--onAccent);border:1px solid var(--accentFill);border-radius:999px;padding:8px 18px;font-weight:700;cursor:pointer;font-size:12px}button.alt{background:transparent;color:var(--accent);border-color:var(--accent)}button:disabled{opacity:.5;cursor:not-allowed}.themeButton,.langButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:var(--card);border:1px solid var(--line2);color:var(--ink2);cursor:pointer;font-size:12px;font-weight:600;line-height:1}.themeButton:hover,.langButton:hover{background:var(--segHover);color:var(--ink)}.themeButton .icoSun{display:none}html[data-theme="light"] .themeButton .icoSun{display:block}html[data-theme="light"] .themeButton .icoMoon{display:none}#driftConfigStatus{font-size:12px}a{color:var(--accent);text-decoration:none}html[data-theme="light"] .panel{background:transparent;border:none}@media(max-width:860px){.tuneGrid{grid-template-columns:repeat(2,1fr)}.summaryGrid{grid-template-columns:repeat(2,1fr)}}@media(max-width:560px){.tuneGrid{grid-template-columns:1fr}.summaryGrid{grid-template-columns:1fr}}
body.embedded .headerRow{display:none}
body.embedded #driftStatusPanel{display:none}
body.embedded{margin-top:0;margin-bottom:10px}
</style>
<style id="dd-embed-native">/* DonkeyDrifter 原生风格——仅 embedded 作用域（CC 设置视图），独立页不动 */body.embedded [data-i18n="panel.rcChannels"],body.embedded [data-i18n="drift.steering.label"],body.embedded [data-i18n="drift.throttle.label"],body.embedded [data-i18n="judge.section.thresholds"],body.embedded [data-i18n="judge.section.scoring"],body.embedded [data-i18n="judge.dimLabel"]{font-weight:600!important;letter-spacing:-0.02em!important;font-size:15px!important;color:#e4e7eb!important}html[data-theme="light"] body.embedded [data-i18n="panel.rcChannels"],html[data-theme="light"] body.embedded [data-i18n="drift.steering.label"],html[data-theme="light"] body.embedded [data-i18n="drift.throttle.label"],html[data-theme="light"] body.embedded [data-i18n="judge.section.thresholds"],html[data-theme="light"] body.embedded [data-i18n="judge.section.scoring"],html[data-theme="light"] body.embedded [data-i18n="judge.dimLabel"]{color:#1a2330!important}body.embedded [data-i18n="panel.rcChannels"]::before,body.embedded [data-i18n="drift.steering.label"]::before,body.embedded [data-i18n="drift.throttle.label"]::before,body.embedded [data-i18n="judge.section.thresholds"]::before,body.embedded [data-i18n="judge.section.scoring"]::before,body.embedded [data-i18n="judge.dimLabel"]::before{content:"";display:inline-block;width:18px;height:18px;margin-right:8px;vertical-align:-3px;flex:0 0 auto;background:currentColor;-webkit-mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat;mask:url("data:image/svg+xml,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%2224%22%20height%3D%2224%22%20viewBox%3D%220%200%2024%2024%22%20fill%3D%22none%22%20stroke%3D%22%23000%22%20stroke-width%3D%222%22%20stroke-linecap%3D%22round%22%20stroke-linejoin%3D%22round%22%3E%3Cline%20x1%3D%2221%22%20x2%3D%2214%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2210%22%20x2%3D%223%22%20y1%3D%224%22%20y2%3D%224%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2212%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%223%22%20y1%3D%2212%22%20y2%3D%2212%22%2F%3E%3Cline%20x1%3D%2221%22%20x2%3D%2216%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2212%22%20x2%3D%223%22%20y1%3D%2220%22%20y2%3D%2220%22%2F%3E%3Cline%20x1%3D%2214%22%20x2%3D%2214%22%20y1%3D%222%22%20y2%3D%226%22%2F%3E%3Cline%20x1%3D%228%22%20x2%3D%228%22%20y1%3D%2210%22%20y2%3D%2214%22%2F%3E%3Cline%20x1%3D%2216%22%20x2%3D%2216%22%20y1%3D%2218%22%20y2%3D%2222%22%2F%3E%3C%2Fsvg%3E") center/contain no-repeat}body.embedded .panel,body.embedded .rcCell,body.embedded .summaryItem,body.embedded .field,body.embedded .card{border-radius:8px}html[data-theme="light"] body.embedded .rcCell,html[data-theme="light"] body.embedded .summaryItem,html[data-theme="light"] body.embedded .field,html[data-theme="light"] body.embedded .card{background:#fff!important;border-color:#ccd5df!important}html[data-theme="light"] body.embedded input[type=number],html[data-theme="light"] body.embedded input[type=text],html[data-theme="light"] body.embedded select{background:#fff;border-color:#ccd5df}body.embedded input[type=number],body.embedded input[type=text],body.embedded select{border-radius:6px}</style>
<style id="apple-deep">
/* ===== Apple 深化（自 v1.10.0 起为唯一界面风格；原座舱象限覆写已并入基值） ===== */


html:root,html:root *{-webkit-tap-highlight-color:transparent}
html:root body{font-family:var(--appleFont);-webkit-font-smoothing:antialiased;padding-bottom:env(safe-area-inset-bottom)}
html:root button,html:root input,html:root select,html:root textarea{font-family:inherit}
html:root :focus-visible{outline:3px solid var(--accent);outline-offset:2px}
/* 标题与排版（drift 36% 文本 ≤11.5px → 13px，行高 ≥1.3，微标签去 uppercase） */
html:root h1{font-size:20px;font-weight:600;letter-spacing:-.02em;line-height:1.4}
html:root .version,html:root .label,html:root .fieldTitle,html:root .fieldHint,html:root .field,html:root .summaryItem .k,html:root .tuneSection .sectionTitle,html:root .muted{font-size:13px;line-height:1.35;text-transform:none;letter-spacing:0}
html:root .fieldHint{min-height:36px}
html:root .summaryItem .v{font-weight:600;font-variant-numeric:tabular-nums;letter-spacing:-.02em}
/* 材质 / 发丝线 / 圆角 8·12·16·22·9999 */
html:root .panel,html:root[data-theme="light"] .panel{background:var(--panel);border:1px solid var(--cardLine);border-radius:16px;padding:14px}
html:root .summaryItem,html:root .field{border-color:var(--cardLine);border-radius:12px}
html:root .field input,html:root .field select{border-color:var(--cardLine);border-radius:12px}
html:root .tuneSection{border-top:1px solid var(--sep)}
html:root .dialog{border-radius:22px;background:var(--matSolid);box-shadow:var(--elev)}
/* 控件尺寸 44px + 禁用态 */
html:root button{min-height:44px;border-radius:9999px;padding:10px 18px;font-size:13px}html:root .themeButton,html:root .langButton{min-height:0}
html:root input,html:root select,html:root textarea{min-height:44px;box-sizing:border-box}
html:root button:disabled{opacity:1;background:transparent;color:var(--status-info);border-color:var(--cardLine);cursor:not-allowed}
/* 命中区 ≥44×44（::after 不可见命中区，视觉尺寸不变） */
html:root .themeButton,html:root .langButton,html:root #backLink{position:relative}
html:root .themeButton::after,html:root .langButton::after,html:root #backLink::after{content:"";position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:max(100%,44px);height:max(100%,44px);border-radius:inherit}
/* 动效统一 */
html:root button{transition:background-color .15s var(--ease-apple),color .15s var(--ease-apple),border-color .15s var(--ease-apple),transform .1s var(--ease-apple)}
html:root .summaryItem,html:root .field{transition:background-color .2s var(--ease-apple),border-color .2s var(--ease-apple)}
@media (prefers-reduced-motion: reduce){
html:root *,html:root *::before,html:root *::after{animation-duration:.001ms !important;animation-iteration-count:1 !important;transition-duration:.001ms !important;scroll-behavior:auto !important}
}
@media (prefers-reduced-transparency: reduce){
html:root .dialog{background:var(--matSolid);backdrop-filter:none;-webkit-backdrop-filter:none}
}
@media (prefers-contrast: more){
:root{--ink2:#f5f5f7;--inkF:#f5f5f7;--cardLine:rgba(255,255,255,.34);--sep:rgba(255,255,255,.34)}
html:root[data-theme="light"]{--ink2:#1d1d1f;--inkF:#1d1d1f;--cardLine:rgba(60,60,67,.55);--sep:rgba(60,60,67,.55)}
html:root .panel,html:root .summaryItem,html:root .field{border-width:1px;border-style:solid}
}
@media (forced-colors: active){
html:root .panel,html:root .summaryItem,html:root .field{border:1px solid CanvasText}
}
/* 设置行的动作按钮（漂移设置 / Judge 设置 / 手柄校准）：原高 39px（<44），
   触屏上偏小；apple 象限提到 44（iOS 表单动作按钮的最小高度） */
html:root .setActions button {
  min-height: 44px;
}
</style>
</head>
<body>
<script>try{var _q=location.search,_b=document.body;_b.classList.add('preinit');if(_q.indexOf('embedded=1')>=0)_b.classList.add('embedded');setTimeout(function(){_b.classList.remove('preinit')},2000)}catch(e){try{document.body.classList.remove('preinit')}catch(_e){}}</script>
<div class="headerRow"><h1 data-i18n="drift.title">Drift Assist Tuning</h1><a id="backLink" class="muted" href="/" data-i18n="drift.backLink" style="margin-left:auto">返回 Drifter Console</a><button type="button" id="themeToggle" class="themeButton" onclick="toggleTheme()" aria-label="主题" data-i18n-aria="theme.title"><svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><g class="icoMoon"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></g><g class="icoSun"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></g></svg></button><button type="button" id="langToggle" class="langButton" onclick="toggleLanguage()" aria-label="语言" data-i18n-aria="language.title">中</button></div>
<div class="panel" id="driftStatusPanel">
<div class="panelHead"><span class="label" data-i18n="drift.status.label">Status</span><div id="driftStateCurrent" class="muted" data-i18n="drift.state.waiting">waiting...</div></div>
<div class="summaryGrid">
<div class="summaryItem"><div class="k" data-i18n="drift.state.enabled">Enabled</div><div class="v" id="stateEnabled">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="drift.state.active">Active</div><div class="v" id="stateActive">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="drift.state.yawError">Yaw Error</div><div class="v" id="stateYawError">--</div></div>
<div class="summaryItem"><div class="k" data-i18n="drift.state.throttleMode">Throttle Mode</div><div class="v" id="stateThrottleMode">--</div></div>
</div>
</div>
<div class="panel">
<div class="panelHead"><span class="label" data-i18n="drift.steering.label">Steering Correction</span><div id="driftConfigCurrent" class="muted" data-i18n="drift.config.reading">reading device config...</div></div>
<div class="tuneGrid">
<label class="field"><span class="fieldTitle" data-i18n="drift.field.steeringGyroSign">Steering Gyro Sign</span><span class="fieldHint" data-i18n="drift.field.steeringGyroSign.hint">-1 or 1: flips the steering↔gyro mapping direction.</span><input id="steeringGyroSignInput" type="number" step="1"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.maxYawRate">Max Yaw Rate</span><span class="fieldHint" data-i18n="drift.field.maxYawRate.hint">Expected peak yaw rate at full steering (rad/s).</span><input id="maxYawRateInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.kp">Kp</span><span class="fieldHint" data-i18n="drift.field.kp.hint">Proportional gain on yaw error.</span><input id="kpInput" type="number" step="0.001"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.kd">Kd</span><span class="fieldHint" data-i18n="drift.field.kd.hint">Damping gain on current yaw rate.</span><input id="kdInput" type="number" step="0.001"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.maxSteeringCorrection">Max Steering Correction</span><span class="fieldHint" data-i18n="drift.field.maxSteeringCorrection.hint">0~1: caps the normalized steering offset.</span><input id="maxSteeringCorrectionInput" type="number" step="0.01"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.gyroFilterAlpha">Gyro Filter Alpha</span><span class="fieldHint" data-i18n="drift.field.gyroFilterAlpha.hint">0~1: higher = more responsive, more noise.</span><input id="gyroFilterAlphaInput" type="number" step="0.01"></label>
</div>
<div class="tuneSection"><span class="sectionTitle" data-i18n="drift.throttle.title">Throttle Strategy</span>
<div class="tuneGrid">
<label class="field"><span class="fieldTitle" data-i18n="drift.field.spinThreshold">Spin Threshold</span><span class="fieldHint" data-i18n="drift.field.spinThreshold.hint">|gyroZ| above this uses continuous throttle.</span><input id="spinThresholdInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.steeringThreshold">Steering Threshold</span><span class="fieldHint" data-i18n="drift.field.steeringThreshold.hint">Min normalized steering before throttle strategy engages.</span><input id="steeringThresholdInput" type="number" step="0.01"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.continuousThrottle">Continuous Throttle</span><span class="fieldHint" data-i18n="drift.field.continuousThrottle.hint">0~1: throttle used during high yaw-rate spin.</span><input id="continuousThrottleInput" type="number" step="0.01"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.pulseThrottle">Pulse Throttle</span><span class="fieldHint" data-i18n="drift.field.pulseThrottle.hint">0~1: peak throttle during pulse-on phase.</span><input id="pulseThrottleInput" type="number" step="0.01"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.pulseFreqHz">Pulse Freq (Hz)</span><span class="fieldHint" data-i18n="drift.field.pulseFreqHz.hint">Pulses per second in drift throttle mode.</span><input id="pulseFreqHzInput" type="number" step="0.1"></label>
<label class="field"><span class="fieldTitle" data-i18n="drift.field.pulseDuty">Pulse Duty</span><span class="fieldHint" data-i18n="drift.field.pulseDuty.hint">0~1: on-time fraction of each pulse.</span><input id="pulseDutyInput" type="number" step="0.01"></label>
</div>
</div>
<div class="tuneActions"><button id="saveDriftConfigBtn" onclick="saveDriftConfig()" data-i18n="drift.action.save">Save Drift Config</button><button id="resetDriftConfigBtn" class="alt" onclick="resetDriftConfigToDefault()" data-i18n="drift.action.reset">Restore Defaults</button><div id="driftConfigStatus" class="muted" data-i18n="drift.status.saveHint">Save to persist in device NVS.</div></div>
</div>
<script>
const steeringGyroSignInput=document.getElementById('steeringGyroSignInput'),maxYawRateInput=document.getElementById('maxYawRateInput'),kpInput=document.getElementById('kpInput'),kdInput=document.getElementById('kdInput'),maxSteeringCorrectionInput=document.getElementById('maxSteeringCorrectionInput'),gyroFilterAlphaInput=document.getElementById('gyroFilterAlphaInput'),spinThresholdInput=document.getElementById('spinThresholdInput'),steeringThresholdInput=document.getElementById('steeringThresholdInput'),continuousThrottleInput=document.getElementById('continuousThrottleInput'),pulseThrottleInput=document.getElementById('pulseThrottleInput'),pulseFreqHzInput=document.getElementById('pulseFreqHzInput'),pulseDutyInput=document.getElementById('pulseDutyInput'),driftConfigCurrentEl=document.getElementById('driftConfigCurrent'),driftConfigStatusEl=document.getElementById('driftConfigStatus'),saveDriftConfigBtn=document.getElementById('saveDriftConfigBtn'),resetDriftConfigBtn=document.getElementById('resetDriftConfigBtn'),stateEnabledEl=document.getElementById('stateEnabled'),stateActiveEl=document.getElementById('stateActive'),stateYawErrorEl=document.getElementById('stateYawError'),stateThrottleModeEl=document.getElementById('stateThrottleMode'),driftStateCurrentEl=document.getElementById('driftStateCurrent');
const driftConfig={steeringGyroSign:-1,maxYawRate:3.0,kp:0.15,kd:0.02,maxSteeringCorrection:0.35,gyroFilterAlpha:0.3,spinThreshold:2.0,steeringThreshold:0.15,continuousThrottle:0.45,pulseThrottle:0.65,pulseFreqHz:5.0,pulseDuty:0.4,defaults:{steeringGyroSign:-1,maxYawRate:3.0,kp:0.15,kd:0.02,maxSteeringCorrection:0.35,gyroFilterAlpha:0.3,spinThreshold:2.0,steeringThreshold:0.15,continuousThrottle:0.45,pulseThrottle:0.65,pulseFreqHz:5.0,pulseDuty:0.4},limits:{steeringGyroSignMin:-1,steeringGyroSignMax:1,maxYawRateMin:0.5,maxYawRateMax:10.0,kpMin:0.0,kpMax:2.0,kdMin:0.0,kdMax:1.0,maxSteeringCorrectionMin:0.0,maxSteeringCorrectionMax:1.0,gyroFilterAlphaMin:0.01,gyroFilterAlphaMax:1.0,spinThresholdMin:0.1,spinThresholdMax:8.0,steeringThresholdMin:0.0,steeringThresholdMax:1.0,continuousThrottleMin:0.0,continuousThrottleMax:1.0,pulseThrottleMin:0.0,pulseThrottleMax:1.0,pulseFreqHzMin:0.5,pulseFreqHzMax:30.0,pulseDutyMin:0.0,pulseDutyMax:1.0}};
let lastSeq=0,lastDriftLatest=null,driftOnline=true,driftConfigSource='',driftStatusKey='drift.status.saveHint',driftStatusKind='',driftStatusSuffix='';
const LANG_STORAGE_KEY='mus4.ui.lang';
let uiTheme='auto';
function readUrlTheme(){try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}
function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}
function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}
function applyTheme(){document.documentElement.dataset.theme=resolvedTheme();syncBackLink()}
function toggleTheme(){setTheme(resolvedTheme()==='light'?'dark':'light')}
function setTheme(theme){uiTheme=theme;applyTheme()}
function syncBackLink(){const bl=document.getElementById('backLink');if(bl)bl.href='/?theme='+resolvedTheme()}
function renderLangButton(){const b=document.getElementById('langToggle');if(b)b.textContent=uiLang==='zh'?'中':'EN'}
function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);fetch('/api/language?lang='+uiLang,{method:'POST'}).catch(()=>{})}
function toggleLanguage(){setLanguage(uiLang==='zh'?'en':'zh')}
function initTheme(){uiTheme=readUrlTheme()||'auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}

const I18N={zh:{'drift.title':'漂移辅助调参','drift.status.label':'状态','drift.state.waiting':'等待中...','drift.state.enabled':'启用','drift.state.active':'激活','drift.state.yawError':'偏航误差','drift.state.throttleMode':'油门模式','drift.steering.label':'转向修正','drift.config.reading':'正在读取设备配置...','drift.config.current':'当前设备配置','drift.config.usingDefaults':'使用页面默认值','drift.throttle.title':'油门策略','drift.action.save':'保存漂移配置','drift.action.reset':'恢复默认值','drift.status.saveHint':'保存后持久化到设备 NVS。','drift.status.synced':'已同步设备配置','drift.status.loadFailed':'读取配置失败，使用默认值','drift.status.invalidInput':'输入超出允许范围','drift.status.saved':'已保存到设备 NVS','drift.status.saveFailed':'保存失败: ','drift.status.reset':'已恢复默认值并保存到设备','drift.status.resetFailed':'重置失败: ','drift.error.saveFailed':'保存失败','drift.error.resetFailed':'重置失败','drift.value.on':'开','drift.value.off':'关','drift.value.active':'生效中','drift.value.armed':'待命','drift.value.spin':'旋转','drift.value.pulse':'脉冲','drift.value.pass':'直通','drift.state.seqPrefix':'序号 ','drift.state.offline':'离线','drift.field.steeringGyroSign':'转向陀螺仪符号','drift.field.steeringGyroSign.hint':'-1 或 1：翻转转向↔陀螺仪映射方向。','drift.field.maxYawRate':'最大偏航角速度','drift.field.maxYawRate.hint':'满舵时预期的偏航角速度峰值（rad/s）。','drift.field.kp':'Kp','drift.field.kp.hint':'偏航误差的比例增益。','drift.field.kd':'Kd','drift.field.kd.hint':'当前偏航角速度的阻尼增益。','drift.field.maxSteeringCorrection':'最大转向修正量','drift.field.maxSteeringCorrection.hint':'0~1：限制归一化转向偏移的上限。','drift.field.gyroFilterAlpha':'陀螺仪滤波系数','drift.field.gyroFilterAlpha.hint':'0~1：越大响应越快，噪声也越大。','drift.field.spinThreshold':'旋转阈值','drift.field.spinThreshold.hint':'|gyroZ| 超过此值时使用连续油门。','drift.field.steeringThreshold':'转向阈值','drift.field.steeringThreshold.hint':'油门策略介入所需的最小归一化转向量。','drift.field.continuousThrottle':'连续油门','drift.field.continuousThrottle.hint':'0~1：高偏航角速度旋转期间使用的油门。','drift.field.pulseThrottle':'脉冲油门','drift.field.pulseThrottle.hint':'0~1：脉冲导通阶段的峰值油门。','drift.field.pulseFreqHz':'脉冲频率 (Hz)','drift.field.pulseFreqHz.hint':'漂移油门模式下每秒的脉冲次数。','drift.field.pulseDuty':'脉冲占空比','drift.field.pulseDuty.hint':'0~1：每个脉冲的导通时间占比。','drift.backLink':'返回 Drifter Console','theme.title':'主题','language.title':'语言','drift.state.online':'在线'},en:{'drift.title':'Drift Assist Tuning','drift.status.label':'Status','drift.state.waiting':'waiting...','drift.state.enabled':'Enabled','drift.state.active':'Active','drift.state.yawError':'Yaw Error','drift.state.throttleMode':'Throttle Mode','drift.steering.label':'Steering Correction','drift.config.reading':'reading device config...','drift.config.current':'Current device config','drift.config.usingDefaults':'Using page defaults','drift.throttle.title':'Throttle Strategy','drift.action.save':'Save Drift Config','drift.action.reset':'Restore Defaults','drift.status.saveHint':'Save to persist in device NVS.','drift.status.synced':'Synced device config','drift.status.loadFailed':'Failed to read config, using defaults','drift.status.invalidInput':'Input out of allowed range','drift.status.saved':'Saved to device NVS','drift.status.saveFailed':'Save failed: ','drift.status.reset':'Restored defaults and saved to device','drift.status.resetFailed':'Reset failed: ','drift.error.saveFailed':'save_failed','drift.error.resetFailed':'reset_failed','drift.value.on':'ON','drift.value.off':'OFF','drift.value.active':'ACTIVE','drift.value.armed':'ARMED','drift.value.spin':'spin','drift.value.pulse':'pulse','drift.value.pass':'pass','drift.state.seqPrefix':'seq ','drift.state.offline':'offline','drift.field.steeringGyroSign':'Steering Gyro Sign','drift.field.steeringGyroSign.hint':'-1 or 1: flips the steering↔gyro mapping direction.','drift.field.maxYawRate':'Max Yaw Rate','drift.field.maxYawRate.hint':'Expected peak yaw rate at full steering (rad/s).','drift.field.kp':'Kp','drift.field.kp.hint':'Proportional gain on yaw error.','drift.field.kd':'Kd','drift.field.kd.hint':'Damping gain on current yaw rate.','drift.field.maxSteeringCorrection':'Max Steering Correction','drift.field.maxSteeringCorrection.hint':'0~1: caps the normalized steering offset.','drift.field.gyroFilterAlpha':'Gyro Filter Alpha','drift.field.gyroFilterAlpha.hint':'0~1: higher = more responsive, more noise.','drift.field.spinThreshold':'Spin Threshold','drift.field.spinThreshold.hint':'|gyroZ| above this uses continuous throttle.','drift.field.steeringThreshold':'Steering Threshold','drift.field.steeringThreshold.hint':'Min normalized steering before throttle strategy engages.','drift.field.continuousThrottle':'Continuous Throttle','drift.field.continuousThrottle.hint':'0~1: throttle used during high yaw-rate spin.','drift.field.pulseThrottle':'Pulse Throttle','drift.field.pulseThrottle.hint':'0~1: peak throttle during pulse-on phase.','drift.field.pulseFreqHz':'Pulse Freq (Hz)','drift.field.pulseFreqHz.hint':'Pulses per second in drift throttle mode.','drift.field.pulseDuty':'Pulse Duty','drift.field.pulseDuty.hint':'0~1: on-time fraction of each pulse.','drift.backLink':'Back to Drifter Console','theme.title':'Theme','language.title':'Language','drift.state.online':'Online'}};
let uiLang=readStoredLanguage();
function normalizeLanguage(lang){return lang==='en'?'en':'zh'}
function readUrlLanguage(){try{const m=/[?&]lang=(zh|en)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}function readStoredLanguage(){try{const v=localStorage.getItem(LANG_STORAGE_KEY);return v==='zh'||v==='en'?v:detectBrowserLanguage()}catch(e){return detectBrowserLanguage()}}
function writeStoredLanguage(lang){try{localStorage.setItem(LANG_STORAGE_KEY,lang)}catch(e){}}
function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}
function t(key){return (I18N[uiLang]&&I18N[uiLang][key])||I18N.zh[key]||key}
function refreshDynamicLabels(){if(lastDriftLatest)renderDriftState(lastDriftLatest);else if(!driftOnline)driftStateCurrentEl.textContent=t('drift.state.offline');if(driftConfigSource)driftConfigCurrentEl.textContent=t(driftConfigSource==='device'?'drift.config.current':'drift.config.usingDefaults');setDriftConfigStatus(driftStatusKey,driftStatusKind,driftStatusSuffix)}
function applyLanguage(lang){uiLang=normalizeLanguage(lang);document.documentElement.lang=uiLang;document.querySelectorAll('[data-i18n]').forEach(e=>{const v=t(e.dataset.i18n);if(v)e.textContent=v});document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>{const v=t(e.dataset.i18nPlaceholder);if(v)e.placeholder=v});document.querySelectorAll('[data-i18n-aria]').forEach(e=>{const v=t(e.dataset.i18nAria);if(v)e.setAttribute('aria-label',v)});document.querySelectorAll('[data-i18n-title]').forEach(e=>{const v=t(e.dataset.i18nTitle);if(v)e.title=v});renderLangButton();refreshDynamicLabels()}
async function initLanguage(){const urlLang=readUrlLanguage();let lang=urlLang;if(!lang)lang=readStoredLanguage();if(!lang)lang=detectBrowserLanguage();applyLanguage(lang);if(document.body)document.body.classList.remove('preinit');if(!urlLang){fetch('/api/language',{cache:'no-store'}).then(r=>{if(!r.ok)return null;return r.json()}).then(j=>{if(!j)return;let srv=null;if(j.lang==='zh'||j.lang==='en')srv=normalizeLanguage(j.lang);else if(j.lang==='auto')srv=detectBrowserLanguage();if(srv&&srv!==uiLang){writeStoredLanguage(srv);applyLanguage(srv)}}).catch(()=>{})}}
function semText(n,f){try{const v=getComputedStyle(document.documentElement).getPropertyValue(n);return (v&&v.trim())||f}catch(e){return f}}
function setDriftConfigStatus(key,kind,suffix){driftStatusKey=key;driftStatusKind=kind||'';driftStatusSuffix=suffix||'';driftConfigStatusEl.textContent=t(driftStatusKey)+driftStatusSuffix;driftConfigStatusEl.style.color=driftStatusKind==='ok'?semText('--ok-text','#39d98a'):driftStatusKind==='err'?semText('--bad-text','#ff7b7b'):semText('--status-info','#8fa1b5')}
function setDriftConfigBusy(busy){saveDriftConfigBtn.disabled=busy;resetDriftConfigBtn.disabled=busy}
function syncDriftConfigInputs(){
steeringGyroSignInput.min=driftConfig.limits.steeringGyroSignMin;steeringGyroSignInput.max=driftConfig.limits.steeringGyroSignMax;
maxYawRateInput.min=driftConfig.limits.maxYawRateMin;maxYawRateInput.max=driftConfig.limits.maxYawRateMax;
kpInput.min=driftConfig.limits.kpMin;kpInput.max=driftConfig.limits.kpMax;
kdInput.min=driftConfig.limits.kdMin;kdInput.max=driftConfig.limits.kdMax;
maxSteeringCorrectionInput.min=driftConfig.limits.maxSteeringCorrectionMin;maxSteeringCorrectionInput.max=driftConfig.limits.maxSteeringCorrectionMax;
gyroFilterAlphaInput.min=driftConfig.limits.gyroFilterAlphaMin;gyroFilterAlphaInput.max=driftConfig.limits.gyroFilterAlphaMax;
spinThresholdInput.min=driftConfig.limits.spinThresholdMin;spinThresholdInput.max=driftConfig.limits.spinThresholdMax;
steeringThresholdInput.min=driftConfig.limits.steeringThresholdMin;steeringThresholdInput.max=driftConfig.limits.steeringThresholdMax;
continuousThrottleInput.min=driftConfig.limits.continuousThrottleMin;continuousThrottleInput.max=driftConfig.limits.continuousThrottleMax;
pulseThrottleInput.min=driftConfig.limits.pulseThrottleMin;pulseThrottleInput.max=driftConfig.limits.pulseThrottleMax;
pulseFreqHzInput.min=driftConfig.limits.pulseFreqHzMin;pulseFreqHzInput.max=driftConfig.limits.pulseFreqHzMax;
pulseDutyInput.min=driftConfig.limits.pulseDutyMin;pulseDutyInput.max=driftConfig.limits.pulseDutyMax;
steeringGyroSignInput.value=String(driftConfig.steeringGyroSign);
maxYawRateInput.value=Number(driftConfig.maxYawRate).toFixed(2);
kpInput.value=Number(driftConfig.kp).toFixed(3);
kdInput.value=Number(driftConfig.kd).toFixed(3);
maxSteeringCorrectionInput.value=Number(driftConfig.maxSteeringCorrection).toFixed(2);
gyroFilterAlphaInput.value=Number(driftConfig.gyroFilterAlpha).toFixed(2);
spinThresholdInput.value=Number(driftConfig.spinThreshold).toFixed(2);
steeringThresholdInput.value=Number(driftConfig.steeringThreshold).toFixed(2);
continuousThrottleInput.value=Number(driftConfig.continuousThrottle).toFixed(2);
pulseThrottleInput.value=Number(driftConfig.pulseThrottle).toFixed(2);
pulseFreqHzInput.value=Number(driftConfig.pulseFreqHz).toFixed(2);
pulseDutyInput.value=Number(driftConfig.pulseDuty).toFixed(2);
}
function applyDriftConfigPayload(payload){if(!payload)return;const config=payload.config||payload;const defaults=config.defaults||payload.defaults||{};const limits=config.limits||payload.limits||{};driftConfig.steeringGyroSign=parseInt(config.steeringGyroSign??driftConfig.steeringGyroSign,10);driftConfig.maxYawRate=Number(config.maxYawRate??driftConfig.maxYawRate);driftConfig.kp=Number(config.kp??driftConfig.kp);driftConfig.kd=Number(config.kd??driftConfig.kd);driftConfig.maxSteeringCorrection=Number(config.maxSteeringCorrection??driftConfig.maxSteeringCorrection);driftConfig.gyroFilterAlpha=Number(config.gyroFilterAlpha??driftConfig.gyroFilterAlpha);driftConfig.spinThreshold=Number(config.spinThreshold??driftConfig.spinThreshold);driftConfig.steeringThreshold=Number(config.steeringThreshold??driftConfig.steeringThreshold);driftConfig.continuousThrottle=Number(config.continuousThrottle??driftConfig.continuousThrottle);driftConfig.pulseThrottle=Number(config.pulseThrottle??driftConfig.pulseThrottle);driftConfig.pulseFreqHz=Number(config.pulseFreqHz??driftConfig.pulseFreqHz);driftConfig.pulseDuty=Number(config.pulseDuty??driftConfig.pulseDuty);Object.keys(defaults).forEach(k=>{if(k in driftConfig.defaults)driftConfig.defaults[k]=Number(defaults[k])});Object.keys(limits).forEach(k=>{if(k in driftConfig.limits)driftConfig.limits[k]=Number(limits[k])});syncDriftConfigInputs();}
function readDriftConfigForm(){return{steeringGyroSign:parseInt(steeringGyroSignInput.value,10),maxYawRate:Number(maxYawRateInput.value),kp:Number(kpInput.value),kd:Number(kdInput.value),maxSteeringCorrection:Number(maxSteeringCorrectionInput.value),gyroFilterAlpha:Number(gyroFilterAlphaInput.value),spinThreshold:Number(spinThresholdInput.value),steeringThreshold:Number(steeringThresholdInput.value),continuousThrottle:Number(continuousThrottleInput.value),pulseThrottle:Number(pulseThrottleInput.value),pulseFreqHz:Number(pulseFreqHzInput.value),pulseDuty:Number(pulseDutyInput.value)};}
function driftConfigFormValid(config){return Number.isFinite(config.steeringGyroSign)&&(config.steeringGyroSign===-1||config.steeringGyroSign===1)&&Number.isFinite(config.maxYawRate)&&Number.isFinite(config.kp)&&Number.isFinite(config.kd)&&Number.isFinite(config.maxSteeringCorrection)&&Number.isFinite(config.gyroFilterAlpha)&&Number.isFinite(config.spinThreshold)&&Number.isFinite(config.steeringThreshold)&&Number.isFinite(config.continuousThrottle)&&Number.isFinite(config.pulseThrottle)&&Number.isFinite(config.pulseFreqHz)&&Number.isFinite(config.pulseDuty)&&config.maxYawRate>=driftConfig.limits.maxYawRateMin&&config.maxYawRate<=driftConfig.limits.maxYawRateMax&&config.kp>=driftConfig.limits.kpMin&&config.kp<=driftConfig.limits.kpMax&&config.kd>=driftConfig.limits.kdMin&&config.kd<=driftConfig.limits.kdMax&&config.maxSteeringCorrection>=driftConfig.limits.maxSteeringCorrectionMin&&config.maxSteeringCorrection<=driftConfig.limits.maxSteeringCorrectionMax&&config.gyroFilterAlpha>=driftConfig.limits.gyroFilterAlphaMin&&config.gyroFilterAlpha<=driftConfig.limits.gyroFilterAlphaMax&&config.spinThreshold>=driftConfig.limits.spinThresholdMin&&config.spinThreshold<=driftConfig.limits.spinThresholdMax&&config.steeringThreshold>=driftConfig.limits.steeringThresholdMin&&config.steeringThreshold<=driftConfig.limits.steeringThresholdMax&&config.continuousThrottle>=driftConfig.limits.continuousThrottleMin&&config.continuousThrottle<=driftConfig.limits.continuousThrottleMax&&config.pulseThrottle>=driftConfig.limits.pulseThrottleMin&&config.pulseThrottle<=driftConfig.limits.pulseThrottleMax&&config.pulseFreqHz>=driftConfig.limits.pulseFreqHzMin&&config.pulseFreqHz<=driftConfig.limits.pulseFreqHzMax&&config.pulseDuty>=driftConfig.limits.pulseDutyMin&&config.pulseDuty<=driftConfig.limits.pulseDutyMax;}
async function loadDriftConfig(){try{const r=await fetch('/api/drift-config',{cache:'no-store'});if(!r.ok)throw new Error('load_failed');applyDriftConfigPayload(await r.json());driftConfigSource='device';driftConfigCurrentEl.textContent=t('drift.config.current');setDriftConfigStatus('drift.status.synced','ok')}catch(e){syncDriftConfigInputs();driftConfigSource='defaults';driftConfigCurrentEl.textContent=t('drift.config.usingDefaults');setDriftConfigStatus('drift.status.loadFailed','err')}}
async function saveDriftConfig(){const config=readDriftConfigForm();if(!driftConfigFormValid(config)){setDriftConfigStatus('drift.status.invalidInput','err');return}const body=new URLSearchParams({steeringGyroSign:String(config.steeringGyroSign),maxYawRate:config.maxYawRate.toFixed(2),kp:config.kp.toFixed(3),kd:config.kd.toFixed(3),maxSteeringCorrection:config.maxSteeringCorrection.toFixed(2),gyroFilterAlpha:config.gyroFilterAlpha.toFixed(2),spinThreshold:config.spinThreshold.toFixed(2),steeringThreshold:config.steeringThreshold.toFixed(2),continuousThrottle:config.continuousThrottle.toFixed(2),pulseThrottle:config.pulseThrottle.toFixed(2),pulseFreqHz:config.pulseFreqHz.toFixed(2),pulseDuty:config.pulseDuty.toFixed(2)});try{setDriftConfigBusy(true);const r=await fetch('/api/drift-config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body:body.toString()});const text=await r.text();let payload={};try{payload=JSON.parse(text)}catch(e){}if(!r.ok)throw new Error(payload.error||t('drift.error.saveFailed'));applyDriftConfigPayload(payload);setDriftConfigStatus('drift.status.saved','ok')}catch(e){setDriftConfigStatus('drift.status.saveFailed','err',e.message||t('drift.error.saveFailed'))}finally{setDriftConfigBusy(false)}}
async function resetDriftConfigToDefault(){try{setDriftConfigBusy(true);const r=await fetch('/api/drift-config/reset',{method:'POST'});const text=await r.text();let payload={};try{payload=JSON.parse(text)}catch(e){}if(!r.ok)throw new Error(payload.error||t('drift.error.resetFailed'));applyDriftConfigPayload(payload);setDriftConfigStatus('drift.status.reset','ok')}catch(e){setDriftConfigStatus('drift.status.resetFailed','err',e.message||t('drift.error.resetFailed'))}finally{setDriftConfigBusy(false)}}
function renderDriftState(latest){if(!latest)return;lastDriftLatest=latest;driftOnline=true;lastSeq=Math.max(lastSeq,Number(latest.seq||0));stateEnabledEl.textContent=latest.de?t('drift.value.on'):t('drift.value.off');stateActiveEl.textContent=latest.da?t('drift.value.active'):t('drift.value.armed');stateYawErrorEl.textContent=Number(latest.dye||0).toFixed(2);const m=Number(latest.dtm||0);stateThrottleModeEl.textContent=m===2?t('drift.value.spin'):m===1?t('drift.value.pulse'):t('drift.value.pass');driftStateCurrentEl.textContent=t('drift.state.online');driftStateCurrentEl.title=t('drift.state.seqPrefix')+latest.seq;}
async function pollDriftData(){try{const r=await fetch('/api/data?since='+lastSeq,{cache:'no-store'});const j=await r.json();if(j&&j.latest)renderDriftState(j.latest)}catch(e){driftOnline=false;driftStateCurrentEl.textContent=t('drift.state.offline');driftStateCurrentEl.title=''}}
if(location.search.indexOf('embedded=1')>=0)document.body.classList.add('embedded');initLanguage();initTheme();loadDriftConfig();setInterval(pollDriftData,250);
</script>
</body>
</html>
)rawliteral";static const char WIFI_WEB_UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<link rel="icon" type="image/png" href="/favicon.png">
<title>MUS4 OTA Update</title>
<script>try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);document.documentElement.dataset.theme=m?m[1]:(window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark')}catch(e){}</script>
<style>
:root{--bg:#000;--ink:#f5f5f7;--panel:#1c1c1e;--panelHi:#2c2c2e;--line:rgba(255,255,255,.10);--line2:rgba(255,255,255,.10);--bar:#2c2c2e;--accent:#2997ff;--accentFill:#0a84ff;--accentHi:#2997ff;--onAccent:#fff;--inkHi:#f5f5f7;--muted:rgba(235,235,245,.85);--muted2:rgba(235,235,245,.72);--ok:#30d158;--bad:#ff453a;--ease-apple:cubic-bezier(.32,.72,0,1);--ok-text:#30d158;--warn-text:#ff9f0a;--bad-text:#ff453a;--status-info:rgba(235,235,245,.72);--sep:rgba(255,255,255,.16);--cardLine:rgba(255,255,255,.10);--matSolid:rgba(28,28,30,.96);--elev:0 8px 30px rgba(0,0,0,.5);--appleFont:-apple-system,BlinkMacSystemFont,system-ui,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;--appleMono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}html[data-theme="light"]{--bg:#f5f5f7;--ink:#1d1d1f;--panel:#fff;--panelHi:#f5f5f7;--line:rgba(0,0,0,.08);--line2:rgba(0,0,0,.08);--bar:rgba(120,120,128,.16);--accent:#0066cc;--accentFill:#0071e3;--accentHi:#0066cc;--onAccent:#fff;--inkHi:#1d1d1f;--muted:rgba(60,60,67,.85);--muted2:rgba(60,60,67,.72);--ok:#34c759;--bad:#ff3b30;--ok-text:#1a7f37;--warn-text:#c93400;--bad-text:#d70015;--status-info:rgba(60,60,67,.72);--sep:rgba(60,60,67,.29);--cardLine:rgba(0,0,0,.08);--matSolid:rgba(245,245,247,.96);--elev:0 8px 30px rgba(0,0,0,.12)}html:root h1{font-weight:600;letter-spacing:-0.02em}html:root button{font-weight:600!important;border-radius:9999px}html:root #drop{border-radius:16px}html:root button:active{transform:scale(.97)}.headerRow{display:flex;align-items:center;gap:6px;margin:0 0 16px;flex-wrap:wrap}.headerRow h1{margin:0}body{font-family:system-ui,sans-serif;margin:24px auto;max-width:480px;background:var(--bg);color:var(--ink);padding:0 12px;box-sizing:border-box}
h1{font-size:17px;margin:0 0 16px}
#drop{border:2px dashed var(--line);border-radius:12px;padding:40px 20px;text-align:center;transition:.2s;background:var(--panel)}
#drop.dragover{border-color:var(--accent);background:var(--panelHi)}
#progress{width:100%;height:8px;background:var(--bar);border-radius:4px;margin-top:16px;overflow:hidden;display:none}
#progressBar{height:100%;width:0%;background:var(--accent);transition:.2s}
#status{margin-top:12px;font-size:14px;color:var(--muted);min-height:20px}
button{background:var(--accentFill);color:var(--onAccent);border:none;padding:10px 20px;border-radius:6px;font-weight:700;cursor:pointer;font-size:14px}
button:disabled{opacity:.5;cursor:not-allowed}
button:hover{background:var(--accentHi)}
a{color:var(--accent);text-decoration:none}
.themeButton,.langButton{display:inline-flex;align-items:center;justify-content:center;width:32px;height:32px;min-width:0;padding:0;border-radius:9999px;background:var(--panel);border:1px solid var(--line2);color:var(--muted);cursor:pointer;font-size:12px;font-weight:600;line-height:1}
.themeButton:hover,.langButton:hover{background:var(--panelHi);color:var(--ink)}
.themeButton .icoSun{display:none}
html[data-theme="light"] .themeButton .icoSun{display:block}
html[data-theme="light"] .themeButton .icoMoon{display:none}
.muted{color:var(--muted2);font-size:12px;margin-top:12px}
</style>
<style id="apple-deep">
/* ===== Apple 深化（自 v1.10.0 起为唯一界面风格；原座舱象限覆写已并入基值） ===== */


html:root,html:root *{-webkit-tap-highlight-color:transparent}
html:root body{font-family:var(--appleFont);-webkit-font-smoothing:antialiased;padding-bottom:env(safe-area-inset-bottom)}
html:root button,html:root input,html:root select,html:root textarea{font-family:inherit}
html:root :focus-visible{outline:3px solid var(--accent);outline-offset:2px}
/* 标题：17px → 20px/600/-.02em */
html:root h1{font-size:20px;font-weight:600;letter-spacing:-.02em;line-height:1.4}
html:root .muted,html:root #status{font-size:13px;line-height:1.35}
/* #drop：2px 虚线 → 实线发丝线 + 材质容器 */
html:root #drop{border:1px solid var(--sep);border-radius:16px;background:var(--panel);padding:44px 20px}
html:root #drop.dragover{border-color:var(--accent);background:var(--panelHi)}
/* 按钮：单一圆角来源（原 9999!important 与 6px 两条冲突规则）22px 胶囊 + ≥44px */
html:root button{min-height:44px;border-radius:22px;padding:10px 22px;font-size:15px;transition:background-color .15s var(--ease-apple),color .15s var(--ease-apple),transform .1s var(--ease-apple)}html:root .themeButton,html:root .langButton{min-height:0;border-radius:9999px;padding:0}
html:root .headerRow{gap:14px}
html:root button:disabled{opacity:1;background:var(--panel);color:var(--muted2);border:1px solid var(--line);cursor:not-allowed}
html:root #progress{height:8px;border-radius:8px;background:var(--bar)}
html:root #progressBar{border-radius:8px;transition:width .2s var(--ease-apple)}
/* 返回链接：iOS 返回样式 */
html:root #backLink{display:inline-flex;align-items:center;gap:6px;color:var(--accent);font-size:15px !important;font-weight:500;text-decoration:none}
html:root #backLink::before{content:"";width:9px;height:9px;border-left:2px solid currentColor;border-bottom:2px solid currentColor;transform:rotate(45deg) translate(1px,-1px);border-radius:1px;flex:0 0 auto}
/* 命中区 ≥44×44（::after 不可见命中区，视觉尺寸不变） */
html:root .themeButton,html:root .langButton,html:root #backLink{position:relative}
html:root .themeButton::after,html:root .langButton::after,html:root #backLink::after{content:"";position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:max(100%,44px);height:max(100%,44px);border-radius:inherit}
@media (prefers-reduced-motion: reduce){
html:root *,html:root *::before,html:root *::after{animation-duration:.001ms !important;animation-iteration-count:1 !important;transition-duration:.001ms !important;scroll-behavior:auto !important}
}
@media (prefers-reduced-transparency: reduce){
html:root #drop{background:var(--panel);backdrop-filter:none;-webkit-backdrop-filter:none}
}
@media (prefers-contrast: more){
:root{--muted:#f5f5f7;--muted2:#f5f5f7;--line:rgba(255,255,255,.34);--sep:rgba(255,255,255,.34)}
html:root[data-theme="light"]{--muted:#1d1d1f;--muted2:#1d1d1f;--line:rgba(60,60,67,.55);--sep:rgba(60,60,67,.55)}
html:root #drop{border-width:1px;border-style:solid}
}
@media (forced-colors: active){
html:root #drop{border:1px solid CanvasText}
}
/* 设置行的动作按钮（漂移设置 / Judge 设置 / 手柄校准）：原高 39px（<44），
   触屏上偏小；apple 象限提到 44（iOS 表单动作按钮的最小高度） */
html:root .setActions button {
  min-height: 44px;
}
</style>
</head>
<body>
<div class="headerRow"><h1 data-i18n="ota.heading">MUS4 HTTP OTA</h1><a id="backLink" href="/" data-i18n="ota.backLink" style="margin-left:auto;font-size:12px">返回 Drifter Console</a><button type="button" id="themeToggle" class="themeButton" onclick="toggleTheme()" aria-label="主题" data-i18n-aria="theme.title"><svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><g class="icoMoon"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></g><g class="icoSun"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></g></svg></button><button type="button" id="langToggle" class="langButton" onclick="toggleLanguage()" aria-label="语言" data-i18n-aria="language.title">中</button></div>
<div id="drop">
<div><span data-i18n="ota.dropHint">拖放固件文件到此处，或</span> <button onclick="document.getElementById('file').click()" data-i18n="ota.chooseFile">选择文件</button></div>
<input type="file" id="file" style="display:none" accept=".bin">
</div>
<div id="progress"><div id="progressBar"></div></div>
<div id="status" data-i18n="ota.status.waiting">等待文件...</div>
<div class="muted" data-i18n="ota.parkNote">OTA 传输期间车辆会自动 Park Locked。需要认证 + Park 锁定（或开发模式）。</div>
<script>
const LANG_STORAGE_KEY='mus4.ui.lang';

let uiTheme='auto';
function readUrlTheme(){try{const m=/[?&]theme=(light|dark)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}
function systemTheme(){try{return window.matchMedia&&window.matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'}catch(e){return 'dark'}}
function resolvedTheme(){return uiTheme==='auto'?systemTheme():(uiTheme==='light'?'light':'dark')}
function applyTheme(){document.documentElement.dataset.theme=resolvedTheme();syncBackLink()}
function initTheme(){uiTheme=readUrlTheme()||'auto';applyTheme();try{const mq=window.matchMedia('(prefers-color-scheme: light)');const onThemeChange=()=>{if(uiTheme==='auto')applyTheme()};if(mq.addEventListener)mq.addEventListener('change',onThemeChange);else if(mq.addListener)mq.addListener(onThemeChange)}catch(e){}}
function toggleTheme(){setTheme(resolvedTheme()==='light'?'dark':'light')}
function setTheme(theme){uiTheme=theme;applyTheme()}
function syncBackLink(){const bl=document.getElementById('backLink');if(bl)bl.href='/?theme='+resolvedTheme()}
function renderLangButton(){const b=document.getElementById('langToggle');if(b)b.textContent=uiLang==='zh'?'中':'EN'}
function setLanguage(lang){uiLang=normalizeLanguage(lang);writeStoredLanguage(uiLang);applyLanguage(uiLang);fetch('/api/language?lang='+uiLang,{method:'POST'}).catch(()=>{})}
function toggleLanguage(){setLanguage(uiLang==='zh'?'en':'zh')}
const I18N={zh:{'ota.pageTitle':'MUS4 OTA Update','ota.heading':'MUS4 HTTP OTA','ota.dropHint':'拖放固件文件到此处，或','ota.chooseFile':'选择文件','ota.parkNote':'OTA 传输期间车辆会自动 Park Locked。需要认证 + Park 锁定（或开发模式）。','ota.status.waiting':'等待文件...','ota.status.uploading':'上传中...','ota.status.success':'成功: ','ota.status.errorPrefix':'错误 ','ota.status.networkError':'网络错误','ota.backLink':'返回 Drifter Console','theme.title':'主题','language.title':'语言'},en:{'ota.pageTitle':'MUS4 OTA Update','ota.heading':'MUS4 HTTP OTA','ota.dropHint':'Drag and drop the firmware file here, or','ota.chooseFile':'Choose File','ota.parkNote':'The vehicle is automatically Park Locked during OTA transfer. Authentication + Park lock (or DEV mode) is required.','ota.status.waiting':'Waiting for file...','ota.status.uploading':'Uploading...','ota.status.success':'Success: ','ota.status.errorPrefix':'Error ','ota.status.networkError':'Network error','ota.backLink':'Back to Drifter Console','theme.title':'Theme','language.title':'Language'}};
let uiLang=readStoredLanguage();
function normalizeLanguage(lang){return lang==='en'?'en':'zh'}
function readUrlLanguage(){try{const m=/[?&]lang=(zh|en)(?:&|$)/.exec(window.location.search);if(m)return m[1]}catch(e){}return null}function readStoredLanguage(){try{const v=localStorage.getItem(LANG_STORAGE_KEY);return v==='zh'||v==='en'?v:detectBrowserLanguage()}catch(e){return detectBrowserLanguage()}}
function writeStoredLanguage(lang){try{localStorage.setItem(LANG_STORAGE_KEY,lang)}catch(e){}}
function detectBrowserLanguage(){try{return String(navigator.language||'').toLowerCase().indexOf('zh')===0?'zh':'en'}catch(e){return 'zh'}}
function t(key){return (I18N[uiLang]&&I18N[uiLang][key])||I18N.zh[key]||key}
function applyLanguage(lang){uiLang=normalizeLanguage(lang);document.documentElement.lang=uiLang;document.title=t('ota.pageTitle');document.querySelectorAll('[data-i18n]').forEach(e=>{const v=t(e.dataset.i18n);if(v)e.textContent=v});document.querySelectorAll('[data-i18n-placeholder]').forEach(e=>{const v=t(e.dataset.i18nPlaceholder);if(v)e.placeholder=v});document.querySelectorAll('[data-i18n-aria]').forEach(e=>{const v=t(e.dataset.i18nAria);if(v)e.setAttribute('aria-label',v)});document.querySelectorAll('[data-i18n-title]').forEach(e=>{const v=t(e.dataset.i18nTitle);if(v)e.title=v});renderLangButton()}
async function initLanguage(){let lang=readUrlLanguage();if(!lang){try{const r=await fetch('/api/language',{cache:'no-store'});if(r.ok){const j=await r.json();if(j&&(j.lang==='zh'||j.lang==='en')){lang=normalizeLanguage(j.lang);writeStoredLanguage(lang)}else if(j&&j.lang==='auto'){lang=detectBrowserLanguage();writeStoredLanguage(lang)}}}catch(e){}}if(!lang)lang=readStoredLanguage();applyLanguage(lang)}
const drop=document.getElementById('drop'),fileInput=document.getElementById('file'),progress=document.getElementById('progress'),bar=document.getElementById('progressBar'),status=document.getElementById('status');
function semText(n,f){try{const v=getComputedStyle(document.documentElement).getPropertyValue(n);return (v&&v.trim())||f}catch(e){return f}}
function setStatus(text,c){status.textContent=text;status.style.color=c||'var(--muted)'}
['dragenter','dragover','dragleave','drop'].forEach(e=>{drop.addEventListener(e,ev=>{ev.preventDefault();ev.stopPropagation()})});
['dragenter','dragover'].forEach(e=>drop.addEventListener(e,()=>drop.classList.add('dragover')));
['dragleave','drop'].forEach(e=>drop.addEventListener(e,()=>drop.classList.remove('dragover')));
drop.addEventListener('drop',e=>upload(e.dataTransfer.files[0]));
fileInput.addEventListener('change',e=>upload(e.target.files[0]));
async function upload(f){
  if(!f)return;
  setStatus(t('ota.status.uploading'));
  progress.style.display='block';bar.style.width='0%';
  const form=new FormData();form.append('firmware',f);
  const xhr=new XMLHttpRequest();
  xhr.upload.addEventListener('progress',e=>{if(e.lengthComputable){bar.style.width=Math.round(e.loaded/e.total*100)+'%'}});
  xhr.addEventListener('load',()=>{
    if(xhr.status===200){setStatus(t('ota.status.success')+xhr.responseText,semText('--ok-text','var(--ok)'));setTimeout(()=>location.href='/',3000)}
    else{setStatus(t('ota.status.errorPrefix')+xhr.status+': '+xhr.responseText,semText('--bad-text','var(--bad)'))}
  });
  xhr.addEventListener('error',()=>setStatus(t('ota.status.networkError'),semText('--bad-text','var(--bad)')));
  xhr.open('POST','/update');xhr.send(form);
}
initLanguage();initTheme();
</script>
</body>
</html>
)rawliteral";
