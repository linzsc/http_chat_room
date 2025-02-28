let websocket;
let isConnected = false;
const CHAT_SERVER_URL = 'ws://127.0.0.1:12345';

let clinet_name;
let client_password;

// 发送HTTP请求
function sendHttpRequest(method, path, data) {
    return fetch(`http://127.0.0.1:12345${path}`, {
        method,
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    });
}

// 用户注册
async function registerUser() {
    const username = document.getElementById('registerUsername').value;
    const password = document.getElementById('registerPassword').value;

    const response = await sendHttpRequest('POST', '/register', {
        username,
        password
    });

    const result = await response.json();
    if(result.success) {
        alert('注册成功');
    } else {
        alert('注册失败');
    }
    console.log(result);
}

// 用户登录
async function loginUser() {
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;

    const response = await sendHttpRequest('POST', '/login', {
        username,
        password
    });

    const result = await response.json();
    console.log(result);
    if (result.success) {
        //提示用户登录成功
        alert('登录成功');
        // 登录成功，显示聊天框
        document.getElementById('authForm').classList.add('hidden');
        document.getElementById('chatBox').classList.remove('hidden');

        clinet_name = username;
        client_password = password;
        setTimeout(() => {
            connectWebSocket();
        }, 500);
        // 连接WebSocket
        //connectWebSocket();
    } else {
        alert('Invalid username or password');
    }
}

// 发送消息
function sendMessage() {
    const messageInput = document.getElementById('messageInput');
    const message = messageInput.value;

    if (message.trim() === '') return;

    // 创建消息对象
    const msg = {
        type: '1', // 消息类型，GroupChat：1,可以扩展为 'PrivateChat':2
        sender: clinet_name, // 发送者用户名，确保 username 已定义
        content: message,
        timestamp: Date.now()
    };
    console.log(msg);
    // 构建非 JSON 格式的消息字符串
    //const serializedMsg = `${msg.type}|${msg.sender}|${msg.content}|${msg.timestamp}`;

    // 发送消息
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(msg));
        messageInput.value = '';
    } else {
        alert('WebSocket connection is not open. Please reconnect.');
    }
}

function resetHeartbeat() {
    clearTimeout(heartbeatTimeout);
}

function startHeartbeat() {
    heartbeatTimeout = setInterval(() => {
        if (websocket.readyState === WebSocket.OPEN) {
            const heartbeatMessage = { type: "HEARTBEAT" };
            websocket.send(JSON.stringify(heartbeatMessage));
            resetHeartbeat();
        } else {
            console.log("WebSocket connection lost. Attempting to reconnect...");
            connectWebSocket(); // 重连逻辑
        }
    }, 5000); // 每5秒发送一次心跳消息
}

// 接收消息
function receiveMessage(data) {
    const messageList = document.getElementById('messageList');
    const li = document.createElement('li');
    li.textContent = `[${new Date(data.timestamp)}] ${data.sender}: ${data.content}`;
    messageList.appendChild(li);
}

// WebSocket连接
function connectWebSocket() {
    console.log("尝试连接 WebSocket:", CHAT_SERVER_URL);
    websocket = new WebSocket(CHAT_SERVER_URL);

    websocket.onopen = () => {
        console.log('Connected to server');
        isConnected = true;
    };

    websocket.onmessage = (event) => {
        console.log("收到 WebSocket 消息:", event.data);
        const data = JSON.parse(event.data);
        receiveMessage(data);
    };

    websocket.onclose = () => {
        console.log('Disconnected from server');
        isConnected = false;
    };

    websocket.onerror = (error) => {
        console.error('WebSocket error:', error);
        alert('WebSocket connection error. Please check the server.');
    };
}

// 页面加载时
window.onload = () => {
    // 如果需要自动连接WebSocket，可以在这里调用 connectWebSocket()
    //connectWebSocket();
};

