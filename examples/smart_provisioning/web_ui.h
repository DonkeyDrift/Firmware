#pragma once

const char SMART_PROVISIONING_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-S3 智能配网</title>
  <style>
    body{margin:0;min-height:100vh;display:grid;place-items:center;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:#f3f4f6;color:#111827}
    .card{width:min(92vw,420px);padding:24px;border-radius:16px;background:#fff;box-shadow:0 12px 32px rgba(15,23,42,.12)}
    h1{margin:0 0 8px;font-size:22px}p{color:#6b7280;line-height:1.5}label{display:block;margin:16px 0 6px;font-weight:600}
    input{width:100%;box-sizing:border-box;padding:11px 12px;border:1px solid #d1d5db;border-radius:10px;font-size:16px}
    button{width:100%;margin-top:18px;padding:12px;border:0;border-radius:10px;background:#2563eb;color:#fff;font-size:16px;font-weight:700;cursor:pointer}
    button:disabled{cursor:not-allowed;opacity:.65}#status{margin-top:16px;padding:12px;border-radius:10px;background:#eff6ff;color:#1d4ed8;white-space:pre-wrap;line-height:1.5;display:none}
    #status.ok{background:#ecfdf5;color:#047857}#status.err{background:#fef2f2;color:#b91c1c}a{color:inherit;font-weight:700}
  </style>
</head>
<body>
  <main class="card">
    <h1>ESP32-S3 智能配网</h1>
    <p>请输入 2.4GHz Wi-Fi 信息。连接成功后，热点会自动关闭，页面会尝试跳转到设备的新 IP。</p>
    <form id="wifiForm">
      <label for="ssid">Wi-Fi 名称 SSID</label>
      <input id="ssid" name="ssid" autocomplete="off" required placeholder="例如 MyHome_2.4G">
      <label for="password">Wi-Fi 密码</label>
      <input id="password" name="password" type="password" autocomplete="current-password" placeholder="开放网络可留空">
      <button id="submitBtn" type="submit">连接并跳转</button>
    </form>
    <div id="status"></div>
  </main>
  <script>
    const form=document.getElementById('wifiForm');
    const button=document.getElementById('submitBtn');
    const statusBox=document.getElementById('status');
    function setStatus(message,type=''){
      statusBox.style.display='block';
      statusBox.className=type;
      statusBox.innerHTML=message;
    }
    function sleep(ms){return new Promise(resolve=>setTimeout(resolve,ms));}
    async function probeUrl(url,timeoutMs=2500){
      const controller=new AbortController();
      const timer=setTimeout(()=>controller.abort(),timeoutMs);
      try{
        await fetch(url,{mode:'no-cors',cache:'no-store',signal:controller.signal});
        return true;
      }catch(error){
        return false;
      }finally{
        clearTimeout(timer);
      }
    }
    async function pollAndRedirect(ip){
      const directUrl=`http://${ip}/`;
      const mdnsUrl='http://esp32.local/';
      setStatus(`设备已连接路由器，新 IP：<a href="${directUrl}">${directUrl}</a>\n热点即将关闭，等待电脑恢复到家庭网络后自动跳转...`,'ok');
      for(let attempt=1;attempt<=30;attempt++){
        await sleep(2000);
        setStatus(`正在检测设备是否可达：${directUrl}\n第 ${attempt}/30 次尝试...`,'ok');
        if(await probeUrl(directUrl)){
          const url=directUrl;
          location.href=url;
          return;
        }
      }
      setStatus(`直接访问 IP 暂未成功，尝试 mDNS：<a href="${mdnsUrl}">${mdnsUrl}</a>`,'ok');
      for(let attempt=1;attempt<=5;attempt++){
        await sleep(2000);
        if(await probeUrl(mdnsUrl)){
          const url=mdnsUrl;
          location.href=url;
          return;
        }
      }
      setStatus(`未能自动跳转。\n\n请确认电脑已经切回家庭 Wi-Fi，然后手动打开：\n<a href="${directUrl}">${directUrl}</a>\n\n如果网络支持 mDNS，也可以尝试：\n<a href="${mdnsUrl}">${mdnsUrl}</a>`,'err');
      button.disabled=false;
    }
    form.addEventListener('submit',async event=>{
      event.preventDefault();
      const ssid=document.getElementById('ssid').value.trim();
      const password=document.getElementById('password').value;
      if(!ssid){
        setStatus('SSID 不能为空。','err');
        return;
      }
      button.disabled=true;
      setStatus('正在提交配置，ESP32-S3 正在连接路由器...');
      try{
        const response=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password})});
        const data=await response.json();
        if(!response.ok||!data.ok){
          setStatus(`配网失败：${data.message||data.error||'未知错误'}`,'err');
          button.disabled=false;
          return;
        }
        await pollAndRedirect(data.ip);
      }catch(error){
        setStatus(`请求失败：${error.message}`,'err');
        button.disabled=false;
      }
    });
  </script>
</body>
</html>
)rawliteral";
