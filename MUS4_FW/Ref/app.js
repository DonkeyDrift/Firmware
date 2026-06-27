let eventSource = null;
let isConnected = false;
let isRunning = false;

let weights = {
    w1: 1.0,
    w2: 1.0,
    w3: 1.0,
    w4: 1.0,
    w5: 1.0,
    w6: 1.0
};

let dataBuffer = [];
const BUFFER_SIZE = 50;

let scoreState = {
    startTime: 0,
    totalScore: 0,
    samples: 0,
    dimensionScores: [0, 0, 0, 0, 0, 0],
    dimensionSums: [0, 0, 0, 0, 0, 0],
    collisionCount: 0,
    lastGyroZ: 0,
    gyroZHistory: [],
    speedHistory: [],
    throttleHistory: [],
    turnPhase: 0,
    inTurn: false,
    turnStartTime: 0,
    maxGyroZInTurn: 0,
    bigTurnThreshold: 80
};

let history = [];
let collisionCooldown = 0;
let chartData = [];
const CHART_MAX_POINTS = 100;

let gyroChartCtx = null;

window.onload = function() {
    const canvas = document.getElementById('gyroChart');
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * 2;
    canvas.height = rect.height * 2;
    gyroChartCtx = canvas.getContext('2d');
    gyroChartCtx.scale(2, 2);
    drawChart();
};

function toggleConnection() {
    if (isConnected) {
        disconnectSSE();
    } else {
        connectSSE();
    }
}

function connectSSE() {
    const url = document.getElementById('sseUrl').value;
    if (!url) {
        alert('请输入 SSE 地址');
        return;
    }

    try {
        eventSource = new EventSource(url);

        eventSource.onopen = function() {
            isConnected = true;
            updateStatus(true);
            document.getElementById('connectBtn').textContent = '断开';
        };

        eventSource.onmessage = function(event) {
            try {
                const data = JSON.parse(event.data);
                handleData(data);
            } catch (e) {
                console.error('Data parse error:', e);
            }
        };

        eventSource.onerror = function() {
            disconnectSSE();
        };
    } catch (e) {
        alert('连接失败: ' + e.message);
    }
}

function disconnectSSE() {
    if (eventSource) {
        eventSource.close();
        eventSource = null;
    }
    isConnected = false;
    updateStatus(false);
    document.getElementById('connectBtn').textContent = '连接';
}

function updateStatus(connected) {
    const statusEl = document.getElementById('status');
    if (connected) {
        statusEl.textContent = '已连接';
        statusEl.className = 'status connected';
    } else {
        statusEl.textContent = '未连接';
        statusEl.className = 'status disconnected';
    }
}

function handleData(data) {
    dataBuffer.push(data);
    if (dataBuffer.length > BUFFER_SIZE) {
        dataBuffer.shift();
    }

    document.getElementById('gyroZ').textContent = data.gyro_z.toFixed(1);
    document.getElementById('speed').textContent = data.speed.toFixed(1);
    document.getElementById('throttle').textContent = (data.throttle * 100).toFixed(0) + '%';

    chartData.push(data.gyro_z);
    if (chartData.length > CHART_MAX_POINTS) {
        chartData.shift();
    }
    drawChart();

    if (isRunning) {
        updateScore(data);
    }

    detectCollision(data);
}

function detectCollision(data) {
    if (collisionCooldown > 0) {
        collisionCooldown--;
        return;
    }

    const gyroDelta = Math.abs(data.gyro_z - scoreState.lastGyroZ);
    const collisionThreshold = 150;

    if (gyroDelta > collisionThreshold) {
        scoreState.collisionCount++;
        collisionCooldown = 50;
        showCollision();

        if (isRunning) {
            applyCollisionPenalty();
        }
    }

    scoreState.lastGyroZ = data.gyro_z;
}

function showCollision() {
    const indicator = document.getElementById('collisionIndicator');
    indicator.classList.add('active');
    indicator.querySelector('.text').textContent = '碰撞！ (-10分)';

    setTimeout(() => {
        indicator.classList.remove('active');
        indicator.querySelector('.text').textContent = '状态正常';
    }, 800);
}

function applyCollisionPenalty() {
    scoreState.totalScore = Math.max(0, scoreState.totalScore - 10);
}

function updateScore(data) {
    scoreState.samples++;
    scoreState.gyroZHistory.push(data.gyro_z);
    scoreState.speedHistory.push(data.speed);
    scoreState.throttleHistory.push(data.throttle);

    const historySize = 20;
    if (scoreState.gyroZHistory.length > historySize) {
        scoreState.gyroZHistory.shift();
        scoreState.speedHistory.shift();
        scoreState.throttleHistory.shift();
    }

    const s1 = calcTurnSmoothness(data);
    const s2 = calcRangeMatch(data);
    const s3 = calcGyroStability(data);
    const s4 = calcBigTurnStability(data);
    const s5 = calcSpeedStability(data);
    const s6 = calcThrottleStability(data);

    scoreState.dimensionSums[0] += s1;
    scoreState.dimensionSums[1] += s2;
    scoreState.dimensionSums[2] += s3;
    scoreState.dimensionSums[3] += s4;
    scoreState.dimensionSums[4] += s5;
    scoreState.dimensionSums[5] += s6;

    scoreState.dimensionScores[0] = scoreState.dimensionSums[0] / scoreState.samples;
    scoreState.dimensionScores[1] = scoreState.dimensionSums[1] / scoreState.samples;
    scoreState.dimensionScores[2] = scoreState.dimensionSums[2] / scoreState.samples;
    scoreState.dimensionScores[3] = scoreState.dimensionSums[3] / scoreState.samples;
    scoreState.dimensionScores[4] = scoreState.dimensionSums[4] / scoreState.samples;
    scoreState.dimensionScores[5] = scoreState.dimensionSums[5] / scoreState.samples;

    const totalWeight = weights.w1 + weights.w2 + weights.w3 + weights.w4 + weights.w5 + weights.w6;
    const weightedScore = (
        scoreState.dimensionScores[0] * weights.w1 +
        scoreState.dimensionScores[1] * weights.w2 +
        scoreState.dimensionScores[2] * weights.w3 +
        scoreState.dimensionScores[3] * weights.w4 +
        scoreState.dimensionScores[4] * weights.w5 +
        scoreState.dimensionScores[5] * weights.w6
    ) / totalWeight;

    scoreState.totalScore = weightedScore;

    updateUI();
}

function calcTurnSmoothness(data) {
    if (scoreState.gyroZHistory.length < 5) return 80;

    let changes = [];
    for (let i = 1; i < scoreState.gyroZHistory.length; i++) {
        changes.push(Math.abs(scoreState.gyroZHistory[i] - scoreState.gyroZHistory[i-1]));
    }

    const avgChange = changes.reduce((a, b) => a + b, 0) / changes.length;

    if (Math.abs(data.gyro_z) < 10) {
        return 100;
    }

    const smoothness = Math.max(0, 100 - avgChange * 2);
    return Math.min(100, smoothness);
}

function calcRangeMatch(data) {
    const absGyroZ = Math.abs(data.gyro_z);
    const absSpeed = Math.abs(data.speed);

    if (absSpeed < 5) {
        return 70;
    }

    const idealGyro = absSpeed * 2.5;
    const diff = Math.abs(absGyroZ - idealGyro);
    const maxDiff = idealGyro * 0.8;

    const match = Math.max(0, 100 - (diff / maxDiff) * 50);
    return Math.min(100, match);
}

function calcGyroStability(data) {
    if (scoreState.gyroZHistory.length < 10) return 75;

    const values = scoreState.gyroZHistory;
    const mean = values.reduce((a, b) => a + b, 0) / values.length;

    let variance = 0;
    for (let v of values) {
        variance += (v - mean) ** 2;
    }
    variance /= values.length;
    const stdDev = Math.sqrt(variance);

    const absGyroZ = Math.abs(data.gyro_z);
    if (absGyroZ < 10) {
        const stability = Math.max(0, 100 - stdDev * 5);
        return Math.min(100, stability);
    } else {
        const stability = Math.max(0, 100 - stdDev * 1.5);
        return Math.min(100, stability);
    }
}

function calcBigTurnStability(data) {
    const absGyroZ = Math.abs(data.gyro_z);

    if (absGyroZ > scoreState.bigTurnThreshold) {
        if (!scoreState.inTurn) {
            scoreState.inTurn = true;
            scoreState.turnStartTime = Date.now();
            scoreState.maxGyroZInTurn = absGyroZ;
        } else {
            scoreState.maxGyroZInTurn = Math.max(scoreState.maxGyroZInTurn, absGyroZ);
        }
    } else if (scoreState.inTurn && absGyroZ < scoreState.bigTurnThreshold * 0.3) {
        scoreState.inTurn = false;
    }

    if (!scoreState.inTurn) {
        return 80;
    }

    if (scoreState.gyroZHistory.length < 10) return 70;

    const recentValues = scoreState.gyroZHistory.slice(-10);
    const mean = recentValues.reduce((a, b) => a + b, 0) / recentValues.length;

    let variance = 0;
    for (let v of recentValues) {
        variance += (v - mean) ** 2;
    }
    variance /= recentValues.length;
    const stdDev = Math.sqrt(variance);

    const stability = Math.max(0, 100 - stdDev * 2);
    return Math.min(100, stability);
}

function calcSpeedStability(data) {
    if (scoreState.speedHistory.length < 10) return 75;

    const values = scoreState.speedHistory;
    const mean = values.reduce((a, b) => a + b, 0) / values.length;

    if (mean < 5) return 70;

    let variance = 0;
    for (let v of values) {
        variance += (v - mean) ** 2;
    }
    variance /= values.length;
    const stdDev = Math.sqrt(variance);

    const cv = stdDev / mean;
    const stability = Math.max(0, 100 - cv * 200);
    return Math.min(100, stability);
}

function calcThrottleStability(data) {
    if (scoreState.throttleHistory.length < 10) return 75;

    const values = scoreState.throttleHistory;
    const mean = values.reduce((a, b) => a + b, 0) / values.length;

    if (mean < 0.1) return 70;

    let variance = 0;
    for (let v of values) {
        variance += (v - mean) ** 2;
    }
    variance /= values.length;
    const stdDev = Math.sqrt(variance);

    const cv = stdDev / (mean + 0.01);
    const stability = Math.max(0, 100 - cv * 150);
    return Math.min(100, stability);
}

function updateUI() {
    const score = Math.round(scoreState.totalScore);
    document.getElementById('totalScore').textContent = score;

    const circumference = 326.73;
    const offset = circumference - (score / 100) * circumference;
    document.getElementById('scoreProgress').style.strokeDashoffset = offset;

    document.getElementById('scoreGrade').textContent = getGrade(score);

    for (let i = 1; i <= 6; i++) {
        const dimScore = Math.round(scoreState.dimensionScores[i-1]);
        document.getElementById('dim' + i + '-score').textContent = dimScore;
        document.getElementById('dim' + i + '-fill').style.width = dimScore + '%';
    }
}

function getGrade(score) {
    if (score >= 95) return 'S 级 - 完美！';
    if (score >= 90) return 'A 级 - 优秀';
    if (score >= 80) return 'B 级 - 良好';
    if (score >= 70) return 'C 级 - 一般';
    if (score >= 60) return 'D 级 - 及格';
    return 'E 级 - 需练习';
}

function startRun() {
    if (!isRunning) {
        isRunning = true;
        scoreState.startTime = Date.now();
        scoreState.totalScore = 0;
        scoreState.samples = 0;
        scoreState.dimensionScores = [0, 0, 0, 0, 0, 0];
        scoreState.dimensionSums = [0, 0, 0, 0, 0, 0];
        scoreState.collisionCount = 0;
        scoreState.gyroZHistory = [];
        scoreState.speedHistory = [];
        scoreState.throttleHistory = [];
        scoreState.inTurn = false;
        scoreState.maxGyroZInTurn = 0;

        document.getElementById('startBtn').textContent = '结束计分';
        document.getElementById('startBtn').classList.remove('btn-primary');
        document.getElementById('startBtn').classList.add('btn-secondary');
    } else {
        stopRun();
    }
}

function stopRun() {
    isRunning = false;
    document.getElementById('startBtn').textContent = '开始计分';
    document.getElementById('startBtn').classList.add('btn-primary');
    document.getElementById('startBtn').classList.remove('btn-secondary');

    const finalScore = Math.round(scoreState.totalScore);
    if (finalScore > 0 || scoreState.samples > 0) {
        addToHistory(finalScore);
    }
}

function resetScore() {
    isRunning = false;
    scoreState.totalScore = 0;
    scoreState.samples = 0;
    scoreState.dimensionScores = [0, 0, 0, 0, 0, 0];
    scoreState.dimensionSums = [0, 0, 0, 0, 0, 0];
    scoreState.collisionCount = 0;
    scoreState.gyroZHistory = [];
    scoreState.speedHistory = [];
    scoreState.throttleHistory = [];

    document.getElementById('startBtn').textContent = '开始计分';
    document.getElementById('startBtn').classList.add('btn-primary');
    document.getElementById('startBtn').classList.remove('btn-secondary');

    updateUI();
}

function addToHistory(score) {
    const now = new Date();
    const timeStr = now.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });

    history.unshift({
        time: timeStr,
        score: score,
        grade: getGrade(score).split(' ')[0],
        collisions: scoreState.collisionCount
    });

    if (history.length > 20) {
        history.pop();
    }

    renderHistory();
}

function renderHistory() {
    const listEl = document.getElementById('historyList');

    if (history.length === 0) {
        listEl.innerHTML = '<div style="text-align: center; color: #6b7280; font-size: 12px; padding: 20px 0;">暂无记录</div>';
        return;
    }

    let html = '';
    for (let item of history) {
        html += `
            <div class="history-item">
                <div class="time">${item.time}</div>
                <div style="display: flex; align-items: center; gap: 10px;">
                    <span class="grade">${item.grade}</span>
                    <span class="score">${item.score}</span>
                </div>
            </div>
        `;
    }
    listEl.innerHTML = html;
}

function updateWeight(index) {
    const slider = document.getElementById('w' + index);
    const valEl = document.getElementById('w' + index + '-val');
    const value = parseFloat(slider.value);
    weights['w' + index] = value;
    valEl.textContent = value.toFixed(1);
}

function toggleWeights() {
    const sliders = document.getElementById('weightSliders');
    const btn = document.getElementById('toggleWeightsBtn');

    if (sliders.classList.contains('show')) {
        sliders.classList.remove('show');
        btn.textContent = '展开';
    } else {
        sliders.classList.add('show');
        btn.textContent = '收起';
    }
}

function drawChart() {
    if (!gyroChartCtx) return;

    const canvas = gyroChartCtx.canvas;
    const width = canvas.width / 2;
    const height = canvas.height / 2;

    gyroChartCtx.clearRect(0, 0, width, height);

    gyroChartCtx.fillStyle = '#0f172a';
    gyroChartCtx.fillRect(0, 0, width, height);

    gyroChartCtx.strokeStyle = '#1e293b';
    gyroChartCtx.lineWidth = 1;
    for (let i = 0; i < 4; i++) {
        const y = (height / 4) * i;
        gyroChartCtx.beginPath();
        gyroChartCtx.moveTo(0, y);
        gyroChartCtx.lineTo(width, y);
        gyroChartCtx.stroke();
    }

    const midY = height / 2;

    if (chartData.length < 2) return;

    gyroChartCtx.strokeStyle = '#00d4ff';
    gyroChartCtx.lineWidth = 1.5;
    gyroChartCtx.beginPath();

    const maxVal = 200;
    const stepX = width / (CHART_MAX_POINTS - 1);

    for (let i = 0; i < chartData.length; i++) {
        const x = (i - (chartData.length - CHART_MAX_POINTS)) * stepX;
        if (x < 0) continue;

        const y = midY - (chartData[i] / maxVal) * (height / 2 - 5);
        const clampedY = Math.max(5, Math.min(height - 5, y));

        if (i === 0 || x === 0) {
            gyroChartCtx.moveTo(x, clampedY);
        } else {
            gyroChartCtx.lineTo(x, clampedY);
        }
    }

    gyroChartCtx.stroke();

    const gradient = gyroChartCtx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, 'rgba(0, 212, 255, 0.2)');
    gradient.addColorStop(1, 'rgba(0, 212, 255, 0)');

    gyroChartCtx.lineTo(width, midY);
    gyroChartCtx.lineTo(0, midY);
    gyroChartCtx.closePath();
    gyroChartCtx.fillStyle = gradient;
    gyroChartCtx.fill();
}

let simulatedInterval = null;
let simGyroZ = 0;
let simSpeed = 0;
let simThrottle = 0;
let simTime = 0;

function startSimulatedData() {
    if (simulatedInterval) return;

    simulatedInterval = setInterval(() => {
        simTime += 0.05;

        simGyroZ = Math.sin(simTime * 0.8) * 60 + Math.sin(simTime * 2.3) * 20 + (Math.random() - 0.5) * 10;
        simSpeed = 30 + Math.sin(simTime * 0.5) * 10 + (Math.random() - 0.5) * 3;
        simThrottle = 0.5 + Math.sin(simTime * 0.7) * 0.2 + (Math.random() - 0.5) * 0.05;

        const data = {
            ts: Date.now(),
            throttle: Math.max(0, Math.min(1, simThrottle)),
            rpm: simSpeed * 100,
            speed: Math.max(0, simSpeed),
            accel_x: Math.sin(simTime) * 0.5,
            accel_y: Math.cos(simTime) * 0.3,
            accel_z: 1.0,
            gyro_x: (Math.random() - 0.5) * 5,
            gyro_y: (Math.random() - 0.5) * 5,
            gyro_z: simGyroZ,
            gyro_z_rate: simGyroZ * 0.1
        };

        handleData(data);
    }, 20);
}

function stopSimulatedData() {
    if (simulatedInterval) {
        clearInterval(simulatedInterval);
        simulatedInterval = null;
    }
}

console.log('漂移裁判系统已加载。使用 startSimulatedData() 启动模拟数据进行测试。');
