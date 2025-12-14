/*
 * web_ui.h - Web interface for Leora
 * Contains HTML page and server setup
 */

#ifndef WEB_UI_H
#define WEB_UI_H

// Web page HTML
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Leora Control</title>
  <style>
    body { font-family: Arial; background: #1a1a2e; color: #fff; margin: 0; padding: 20px; }
    h1 { text-align: center; color: #00ff88; }
    .section { background: #16213e; border-radius: 10px; padding: 15px; margin: 10px 0; }
    .section h2 { margin-top: 0; color: #00d4ff; font-size: 16px; border-bottom: 1px solid #00d4ff; padding-bottom: 5px; }
    .btn-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
    .btn { background: #0f3460; border: none; color: #fff; padding: 12px 8px; border-radius: 8px; cursor: pointer; font-size: 14px; transition: all 0.2s; }
    .btn:hover { background: #00ff88; color: #000; transform: scale(1.05); }
    .btn:active { transform: scale(0.95); }
    .btn.expression { background: #e94560; }
    .btn.mouth { background: #533483; }
    .btn.action { background: #f39c12; color: #000; }
    .btn.position { background: #16a085; }
    .pos-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 5px; max-width: 200px; margin: 0 auto; }
    .status { text-align: center; color: #888; font-size: 12px; margin-top: 20px; }
    .slider-row { display: flex; align-items: center; margin: 8px 0; }
    .slider-row label { flex: 1; font-size: 13px; }
    .slider-row input[type=range] { flex: 2; margin: 0 10px; }
    .slider-row span { width: 40px; text-align: right; font-size: 13px; color: #00ff88; }
    input[type=range] { -webkit-appearance: none; background: #0f3460; height: 8px; border-radius: 4px; }
    input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 18px; height: 18px; background: #00ff88; border-radius: 50%; cursor: pointer; }
    .collapse-btn { background: none; border: none; color: #00d4ff; cursor: pointer; float: right; font-size: 14px; }
    .collapsed { display: none; }
    .input-row { display: flex; align-items: center; margin: 8px 0; }
    .input-row label { flex: 1; font-size: 13px; }
    .input-row input[type=text], .input-row input[type=password] { flex: 2; background: #0f3460; color: #fff; border: 1px solid #00d4ff; padding: 8px; border-radius: 4px; font-size: 13px; }
    .wifi-mode { display: flex; gap: 10px; margin-bottom: 10px; }
    .wifi-mode button { flex: 1; }
    .wifi-mode button.active { background: #00ff88; color: #000; }
    .gesture-item { display: flex; align-items: center; background: #0f3460; padding: 8px; border-radius: 6px; margin: 5px 0; }
    .gesture-item span { flex: 1; }
    .gesture-item .action-tag { color: #00d4ff; font-size: 11px; margin-left: 5px; }
    .gesture-item button { padding: 5px 10px; font-size: 12px; }
    .train-status { text-align: center; padding: 15px; background: #e94560; border-radius: 8px; margin: 10px 0; font-size: 18px; font-weight: bold; }
    .train-status.recording { background: #c0392b; animation: pulse 1s infinite; }
    .train-status.done { background: #27ae60; }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.7; } }
    .progress-bar { height: 10px; background: #0f3460; border-radius: 5px; overflow: hidden; margin: 10px 0; }
    .progress-bar .fill { height: 100%; background: #00ff88; transition: width 0.2s; }
    select.action-select { background: #0f3460; color: #fff; border: 1px solid #00d4ff; padding: 8px; border-radius: 4px; font-size: 13px; flex: 2; }
  </style>
</head>
<body>
  <h1>✨ Leora</h1>
  
  <div class="section">
    <h2>😊 Expressions</h2>
    <div class="btn-grid">
      <button class="btn expression" onclick="send('happy')">Happy</button>
      <button class="btn expression" onclick="send('sad')">Sad</button>
      <button class="btn expression" onclick="send('angry')">Angry</button>
      <button class="btn expression" onclick="send('love')">Love</button>
      <button class="btn expression" onclick="send('surprised')">Surprised</button>
      <button class="btn expression" onclick="send('confused')">Confused</button>
      <button class="btn expression" onclick="send('sleepy')">Sleepy</button>
      <button class="btn expression" onclick="send('curious')">Curious</button>
      <button class="btn expression" onclick="send('nervous')">Nervous</button>
      <button class="btn expression" onclick="send('knocked')">Knocked</button>
      <button class="btn expression" onclick="send('neutral')">Neutral</button>
      <button class="btn expression" onclick="send('idle')">Idle</button>
    </div>
  </div>

  <div class="section">
    <h2>👄 Mouth</h2>
    <div class="btn-grid">
      <button class="btn mouth" onclick="send('smile')">Smile</button>
      <button class="btn mouth" onclick="send('frown')">Frown</button>
      <button class="btn mouth" onclick="send('open')">Open</button>
      <button class="btn mouth" onclick="send('ooo')">Ooo</button>
      <button class="btn mouth" onclick="send('flat')">Flat</button>
      <button class="btn mouth" onclick="send('talk')">Talk</button>
      <button class="btn mouth" onclick="send('chew')">Chew</button>
      <button class="btn mouth" onclick="send('wobble')">Wobble</button>
    </div>
  </div>

  <div class="section">
    <h2>⚡ Actions</h2>
    <div class="btn-grid">
      <button class="btn action" onclick="send('blink')">Blink</button>
      <button class="btn action" onclick="send('wink')">Wink L</button>
      <button class="btn action" onclick="send('winkr')">Wink R</button>
      <button class="btn action" onclick="send('laugh')">Laugh</button>
      <button class="btn action" onclick="send('cry')">Cry</button>
    </div>
  </div>

  <div class="section">
    <h2>👁️ Look Direction</h2>
    <div class="pos-grid">
      <button class="btn position" onclick="send('nw')">↖</button>
      <button class="btn position" onclick="send('n')">↑</button>
      <button class="btn position" onclick="send('ne')">↗</button>
      <button class="btn position" onclick="send('w')">←</button>
      <button class="btn position" onclick="send('center')">●</button>
      <button class="btn position" onclick="send('e')">→</button>
      <button class="btn position" onclick="send('sw')">↙</button>
      <button class="btn position" onclick="send('s')">↓</button>
      <button class="btn position" onclick="send('se')">↘</button>
    </div>
  </div>

  <div class="section">
    <h2>⚙️ Settings <button class="collapse-btn" onclick="toggleSettings()">▼</button></h2>
    <div id="settingsPanel">
      <div class="slider-row">
        <label>Eye Width</label>
        <input type="range" id="eyeW" min="20" max="50" value="36" oninput="updateSlider('eyeW')">
        <span id="eyeWVal">36</span>
      </div>
      <div class="slider-row">
        <label>Eye Height</label>
        <input type="range" id="eyeH" min="20" max="50" value="36" oninput="updateSlider('eyeH')">
        <span id="eyeHVal">36</span>
      </div>
      <div class="slider-row">
        <label>Eye Spacing</label>
        <input type="range" id="eyeS" min="2" max="30" value="10" oninput="updateSlider('eyeS')">
        <span id="eyeSVal">10</span>
      </div>
      <div class="slider-row">
        <label>Border Radius</label>
        <input type="range" id="eyeR" min="1" max="20" value="8" oninput="updateSlider('eyeR')">
        <span id="eyeRVal">8</span>
      </div>
      <div class="slider-row">
        <label>Mouth Width</label>
        <input type="range" id="mouthW" min="10" max="40" value="20" oninput="updateSlider('mouthW')">
        <span id="mouthWVal">20</span>
      </div>
      <div class="slider-row">
        <label>Laugh Time</label>
        <input type="range" id="laughT" min="500" max="3000" step="100" value="1000" oninput="updateSlider('laughT')">
        <span id="laughTVal">1.0s</span>
      </div>
      <div class="slider-row">
        <label>Love Time</label>
        <input type="range" id="loveT" min="1000" max="5000" step="100" value="2000" oninput="updateSlider('loveT')">
        <span id="loveTVal">2.0s</span>
      </div>
      <div class="slider-row">
        <label>Blink Interval</label>
        <input type="range" id="blinkI" min="1" max="10" value="3" oninput="updateSlider('blinkI')">
        <span id="blinkIVal">3s</span>
      </div>
      <div class="btn-grid" style="margin-top:10px;">
        <button class="btn" onclick="applySettings()">Apply</button>
        <button class="btn" onclick="resetSettings()">Reset</button>
      </div>
    </div>
  </div>

  <div class="section">
    <h2>🔧 Toggles</h2>
    <div class="btn-grid">
      <button class="btn" onclick="send('sweat')">Sweat</button>
      <button class="btn" onclick="send('cyclops')">Cyclops</button>
      <button class="btn" onclick="send('mouth')">Mouth</button>
    </div>
  </div>

  <div class="section">
    <h2>⏱️ Auto Reset</h2>
    <div class="btn-grid">
      <button class="btn" id="resetBtn" onclick="toggleAutoReset()">Off</button>
      <select id="resetTime" style="background:#0f3460;color:#fff;border:none;padding:12px;border-radius:8px;font-size:14px;">
        <option value="3">3 sec</option>
        <option value="5" selected>5 sec</option>
        <option value="10">10 sec</option>
        <option value="15">15 sec</option>
        <option value="30">30 sec</option>
      </select>
      <button class="btn" onclick="send('neutral')">Reset Now</button>
    </div>
  </div>

  <div class="section">
    <h2>🎲 Shuffle Expressions</h2>
    <div class="btn-grid">
      <button class="btn" id="shuffleBtn" onclick="toggleShuffle()">Off</button>
      <select id="shuffleTime" onchange="updateShuffleTime()" style="background:#0f3460;color:#fff;border:none;padding:12px;border-radius:8px;font-size:14px;">
        <option value="2">2 sec</option>
        <option value="5">5 sec</option>
        <option value="10" selected>10 sec</option>
        <option value="15">15 sec</option>
        <option value="30">30 sec</option>
        <option value="60">60 sec</option>
      </select>
      <button class="btn" onclick="send('shuffle:')">Status</button>
    </div>
  </div>

  <div class="section">
    <h2>📶 WiFi <button class="collapse-btn" onclick="toggleWifi()">▼</button></h2>
    <div id="wifiPanel" class="collapsed">
      <div id="wifiDefaults" style="font-size:11px;color:#888;margin-bottom:10px;padding:8px;background:#0f3460;border-radius:4px;">
        <b>Defaults:</b> SSID: Charmander | AP: Leora (leora123)
      </div>
      <div class="wifi-mode">
        <button class="btn" id="modeStaBtn" onclick="setWifiMode('sta')">Station (STA)</button>
        <button class="btn" id="modeApBtn" onclick="setWifiMode('ap')">Access Point</button>
      </div>
      <div id="staSettings">
        <div class="input-row">
          <label>WiFi SSID</label>
          <input type="text" id="wifiSsid" placeholder="Network name">
        </div>
        <div class="input-row">
          <label>WiFi Password</label>
          <input type="password" id="wifiPass" placeholder="Password">
        </div>
      </div>
      <div id="apSettings" style="display:none;">
        <div class="input-row">
          <label>AP Name</label>
          <input type="text" id="apSsid" placeholder="Leora">
        </div>
        <div class="input-row">
          <label>AP Password</label>
          <input type="password" id="apPass" placeholder="Min 8 chars">
        </div>
      </div>
      <div class="btn-grid" style="margin-top:10px;">
        <button class="btn action" onclick="saveWifi()">Save & Reboot</button>
        <button class="btn" onclick="switchToAP()">Force AP Mode</button>
        <button class="btn" onclick="resetWifi()">Reset Defaults</button>
      </div>
      <div class="btn-grid" style="margin-top:5px;">
        <button class="btn" onclick="getWifiInfo()" style="grid-column: span 3;">Show Current Info</button>
      </div>
      <div id="wifiInfo" style="margin-top:10px;font-size:12px;color:#888;"></div>
    </div>
  </div>

  <div class="section">
    <h2>🎯 Gesture Training <button class="collapse-btn" onclick="toggleGestures()">▼</button></h2>
    <div id="gesturePanel" class="collapsed">
      <div id="trainStatus" style="display:none;"></div>
      <div id="trainProgress" style="display:none;" class="progress-bar"><div class="fill" id="trainFill"></div></div>
      
      <div class="input-row">
        <label>Gesture Name</label>
        <input type="text" id="gestureName" placeholder="e.g. wave, circle">
      </div>
      <div class="input-row">
        <label>Action</label>
        <select id="gestureAction" class="action-select">
          <option value="happy">Happy</option>
          <option value="sad">Sad</option>
          <option value="angry">Angry</option>
          <option value="love">Love</option>
          <option value="surprised">Surprised</option>
          <option value="confused">Confused</option>
          <option value="sleepy">Sleepy</option>
          <option value="curious">Curious</option>
          <option value="nervous">Nervous</option>
          <option value="knocked">Knocked</option>
          <option value="laugh">Laugh</option>
          <option value="cry">Cry</option>
          <option value="blink">Blink</option>
          <option value="wink">Wink</option>
        </select>
      </div>
      <div class="btn-grid" style="margin-top:10px;">
        <button class="btn action" id="trainBtn" onclick="startTraining()">Start Training</button>
        <button class="btn" onclick="cancelTraining()">Cancel</button>
        <button class="btn" id="matchBtn" onclick="toggleMatching()">Match: Off</button>
      </div>
      
      <h3 style="color:#00d4ff;font-size:14px;margin-top:15px;">Gestures</h3>
      <div id="gestureList"></div>
      <div class="btn-grid" style="margin-top:10px;">
        <button class="btn" onclick="refreshGestures()">Refresh</button>
        <button class="btn" style="background:#c0392b;" onclick="clearGestures()">Clear Custom</button>
      </div>
    </div>
  </div>

  <div class="status" id="status">Ready</div>

  <script>
    let autoResetEnabled = false;
    let resetTimer = null;
    let shuffleEnabled = false;
    
    function toggleSettings() {
      let panel = document.getElementById('settingsPanel');
      panel.classList.toggle('collapsed');
    }
    
    function updateSlider(id) {
      let val = document.getElementById(id).value;
      let display = val;
      if (id === 'laughT' || id === 'loveT') display = (val/1000).toFixed(1) + 's';
      else if (id === 'blinkI') display = val + 's';
      document.getElementById(id + 'Val').innerText = display;
    }
    
    function applySettings() {
      let cmd = 'set:' +
        'ew=' + document.getElementById('eyeW').value + ',' +
        'eh=' + document.getElementById('eyeH').value + ',' +
        'es=' + document.getElementById('eyeS').value + ',' +
        'er=' + document.getElementById('eyeR').value + ',' +
        'mw=' + document.getElementById('mouthW').value + ',' +
        'lt=' + document.getElementById('laughT').value + ',' +
        'vt=' + document.getElementById('loveT').value + ',' +
        'bi=' + document.getElementById('blinkI').value;
      send(cmd);
    }
    
    function resetSettings() {
      document.getElementById('eyeW').value = 36;
      document.getElementById('eyeH').value = 36;
      document.getElementById('eyeS').value = 10;
      document.getElementById('eyeR').value = 8;
      document.getElementById('mouthW').value = 20;
      document.getElementById('laughT').value = 1000;
      document.getElementById('loveT').value = 2000;
      document.getElementById('blinkI').value = 3;
      ['eyeW','eyeH','eyeS','eyeR','mouthW','laughT','loveT','blinkI'].forEach(updateSlider);
      applySettings();
    }
    
    function toggleAutoReset() {
      autoResetEnabled = !autoResetEnabled;
      document.getElementById('resetBtn').innerText = autoResetEnabled ? 'On ✓' : 'Off';
      document.getElementById('resetBtn').style.background = autoResetEnabled ? '#00ff88' : '#0f3460';
      document.getElementById('resetBtn').style.color = autoResetEnabled ? '#000' : '#fff';
    }

    function setShuffleButton() {
      document.getElementById('shuffleBtn').innerText = shuffleEnabled ? 'On ✓' : 'Off';
      document.getElementById('shuffleBtn').style.background = shuffleEnabled ? '#00ff88' : '#0f3460';
      document.getElementById('shuffleBtn').style.color = shuffleEnabled ? '#000' : '#fff';
    }

    function toggleShuffle() {
      shuffleEnabled = !shuffleEnabled;
      setShuffleButton();
      let seconds = parseInt(document.getElementById('shuffleTime').value);
      send('shuffle:' + (shuffleEnabled ? 'on' : 'off') + ',time=' + seconds);
    }

    function updateShuffleTime() {
      let seconds = parseInt(document.getElementById('shuffleTime').value);
      // If shuffle is enabled, apply the new timing immediately
      if (shuffleEnabled) {
        send('shuffle:time=' + seconds);
      }
    }
    
    function send(cmd) {
      document.getElementById('status').innerText = 'Sending: ' + cmd;
      fetch('/cmd?c=' + encodeURIComponent(cmd))
        .then(r => r.text())
        .then(t => {
          document.getElementById('status').innerText = t;
          if (autoResetEnabled && cmd !== 'neutral' && !cmd.startsWith('set:')) {
            clearTimeout(resetTimer);
            let seconds = parseInt(document.getElementById('resetTime').value);
            document.getElementById('status').innerText = t + ' (reset in ' + seconds + 's)';
            resetTimer = setTimeout(() => { send('neutral'); }, seconds * 1000);
          }
        })
        .catch(e => { document.getElementById('status').innerText = 'Error: ' + e; });
    }
    
    let wifiMode = 'sta';
    function toggleWifi() {
      document.getElementById('wifiPanel').classList.toggle('collapsed');
    }
    function setWifiMode(mode) {
      wifiMode = mode;
      document.getElementById('modeStaBtn').classList.toggle('active', mode === 'sta');
      document.getElementById('modeApBtn').classList.toggle('active', mode === 'ap');
      document.getElementById('staSettings').style.display = mode === 'sta' ? 'block' : 'none';
      document.getElementById('apSettings').style.display = mode === 'ap' ? 'block' : 'none';
    }
    function saveWifi() {
      let cmd;
      if (wifiMode === 'sta') {
        let ssid = document.getElementById('wifiSsid').value;
        let pass = document.getElementById('wifiPass').value;
        if (!ssid) { alert('Enter WiFi SSID'); return; }
        cmd = 'wifi:ssid=' + ssid + ',pass=' + pass;
      } else {
        let ssid = document.getElementById('apSsid').value || 'Leora';
        let pass = document.getElementById('apPass').value;
        if (pass && pass.length < 8) { alert('AP password must be at least 8 characters'); return; }
        cmd = 'wifi:ap_ssid=' + ssid + ',ap_pass=' + pass;
      }
      if (confirm('Save WiFi settings and reboot?')) {
        send(cmd);
        setTimeout(() => { alert('Device will reboot. You may need to reconnect.'); }, 500);
      }
    }
    function switchToAP() {
      if (confirm('Switch to Access Point mode? Device will reboot.')) {
        send('wifi:force_ap');
        setTimeout(() => { alert('Device rebooting to AP mode. Connect to Leora network.'); }, 500);
      }
    }
    function getWifiInfo() {
      fetch('/cmd?c=wifi:info')
        .then(r => r.text())
        .then(t => { document.getElementById('wifiInfo').innerHTML = t.replace(/\n/g, '<br>'); });
    }
    
    // Gesture Training
    let trainPollTimer = null;
    let matchingOn = false;
    
    function toggleGestures() {
      document.getElementById('gesturePanel').classList.toggle('collapsed');
      if (!document.getElementById('gesturePanel').classList.contains('collapsed')) {
        refreshGestures();
      }
    }
    
    function startTraining() {
      let name = document.getElementById('gestureName').value.trim();
      let action = document.getElementById('gestureAction').value;
      if (!name) { alert('Enter a gesture name'); return; }
      
      send('gesture:train=' + name + ',' + action);
      document.getElementById('trainBtn').disabled = true;
      
      // Start polling for training status
      trainPollTimer = setInterval(pollTrainStatus, 200);
    }
    
    function pollTrainStatus() {
      fetch('/cmd?c=gesture:status')
        .then(r => r.text())
        .then(t => {
          let parts = t.split(',');
          let state = parts[0];
          let progress = parseInt(parts[1]) || 0;
          let statusDiv = document.getElementById('trainStatus');
          let progressDiv = document.getElementById('trainProgress');
          let fillDiv = document.getElementById('trainFill');
          
          if (state === 'idle') {
            statusDiv.style.display = 'none';
            progressDiv.style.display = 'none';
            document.getElementById('trainBtn').disabled = false;
            clearInterval(trainPollTimer);
            refreshGestures();
          } else if (state === 'waiting') {
            statusDiv.style.display = 'block';
            statusDiv.className = 'train-status';
            statusDiv.innerText = '👋 Place hand in range...';
            progressDiv.style.display = 'none';
          } else if (state === 'countdown') {
            statusDiv.style.display = 'block';
            statusDiv.className = 'train-status';
            statusDiv.innerText = '⏱️ ' + parts[1];
            progressDiv.style.display = 'none';
          } else if (state === 'recording') {
            statusDiv.style.display = 'block';
            statusDiv.className = 'train-status recording';
            statusDiv.innerText = '🔴 RECORDING...';
            progressDiv.style.display = 'block';
            fillDiv.style.width = progress + '%';
          } else if (state === 'done') {
            statusDiv.style.display = 'block';
            statusDiv.className = 'train-status done';
            statusDiv.innerText = '✅ Gesture Saved!';
            progressDiv.style.display = 'none';
          }
        });
    }
    
    function cancelTraining() {
      send('gesture:cancel');
      clearInterval(trainPollTimer);
      document.getElementById('trainStatus').style.display = 'none';
      document.getElementById('trainProgress').style.display = 'none';
      document.getElementById('trainBtn').disabled = false;
    }
    
    function toggleMatching() {
      matchingOn = !matchingOn;
      send('gesture:match=' + (matchingOn ? '1' : '0'));
      event.target.innerText = 'Match: ' + (matchingOn ? 'On ✓' : 'Off');
      event.target.style.background = matchingOn ? '#00ff88' : '#0f3460';
      event.target.style.color = matchingOn ? '#000' : '#fff';
    }
    
    function refreshGestures() {
      fetch('/cmd?c=gesture:list')
        .then(r => r.text())
        .then(t => {
          let list = document.getElementById('gestureList');
          try {
            let gestures = JSON.parse(t);
            if (gestures.length === 0) {
              list.innerHTML = '<div style="color:#888;font-size:12px;">No gestures</div>';
            } else {
              list.innerHTML = gestures.map(g => {
                let isDefault = g.default === true;
                let style = isDefault ? 'opacity:0.7;' : '';
                let tag = isDefault ? '<span style="color:#f39c12;font-size:10px;margin-left:5px;">[built-in]</span>' : '';
                let deleteBtn = isDefault ? '' : '<button class="btn" style="padding:5px 10px;font-size:12px;" onclick="deleteGesture(\'' + g.name + '\')">🗑️</button>';
                return '<div class="gesture-item" style="' + style + '">' +
                  '<span>' + g.name + tag + '<span class="action-tag">→ ' + g.action + '</span></span>' +
                  deleteBtn + '</div>';
              }).join('');
            }
          } catch(e) {
            list.innerHTML = '<div style="color:#888;font-size:12px;">' + t + '</div>';
          }
        });
    }
    
    function deleteGesture(name) {
      if (confirm('Delete gesture "' + name + '"?')) {
        send('gesture:delete=' + name);
        setTimeout(refreshGestures, 500);
      }
    }
    
    function clearGestures() {
      if (confirm('Delete all CUSTOM gestures? (Built-in pat/rub will remain)')) {
        send('gesture:clear');
        setTimeout(refreshGestures, 500);
      }
    }
    
    function resetWifi() {
      if (confirm('Reset WiFi to defaults and reboot?')) {
        send('wifi:reset');
        setTimeout(() => { alert('Device rebooting with default WiFi settings.'); }, 500);
      }
    }
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
