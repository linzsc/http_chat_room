let websocket;
let isConnected = false;
const CHAT_SERVER_URL = 'ws://127.0.0.1:12345';

let user_name;
let user_id;
let user_password;
let logined = false;

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
    if (result.success) {
        alert('注册成功');
    } else {
        alert('注册失败');
    }
    console.log(result);
}

// 获取历史消息
async function getHistoryMessage() {
    const response = await sendHttpRequest('POST', '/getmessage', { user_id });
    const result = await response.json();
    result.messages.forEach(message => {
        receiveMessage(message);
    });
    scrollToBottom(); // 获取历史消息后滚动到底部
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
        user_id = result.userId;
        user_name = username;
        user_password = password;
        // 提示用户登录成功
        alert('登录成功');
        logined = true;

        // 登录成功，显示聊天框
        document.getElementById('authForm').classList.add('hidden');
        document.getElementById('chatBox').classList.remove('hidden');

        // 获取历史消息记录
        getHistoryMessage();

        setTimeout(() => {
            connectWebSocket();
        }, 500);
    } else {
        alert('Invalid username or password');
    }
}

// 发送消息
function sendMessage() {
    if (!logined) {
        alert("login first");
        return;
    }
    const messageInput = document.getElementById('messageInput');
    const message = messageInput.value;

    if (message.trim() === '') return;

    // 创建消息对象
    const msg = {
        type: '1', // 消息类型，GroupChat：1,可以扩展为 'PrivateChat':2
        sender_name: user_name, // 发送者用户名，确保 username 已定义
        sender_id: user_id,
        content: message,
        timestamp: Date.now()
    };
    console.log(msg);

    // 发送消息
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify(msg));
        // 将消息显示在右侧
        receiveMessage(msg);
        messageInput.value = '';
    } else {
        alert('WebSocket connection is not open. Please reconnect.');
    }
}

// 滚动到底部
function scrollToBottom() {
    const messageList = document.getElementById('messageList');
    messageList.scrollTop = messageList.scrollHeight;
}

// 接收消息
function receiveMessage(data) {
    console.log("收到消息:", data);
    console.log("当前用户 ID:", user_id);
    console.log("消息发送者 ID:", data.sender_id);
    const messageList = document.getElementById('messageList');
    const li = document.createElement('li');
    const currentUserId = String(user_id);
    const senderId = String(data.sender_id);
    // 判断是否是自己发送的消息
    if (senderId === currentUserId) {
        li.className = 'own-message'; // 添加一个类名用于样式区分
        li.innerHTML = `<strong>${data.sender_name} (You):</strong> ${data.content}`;
    } else {
        li.className = 'other-message'; // 添加一个类名用于样式区分
        li.innerHTML = `<strong>${data.sender_name}:</strong> ${data.content}`;
    }

    messageList.appendChild(li);
    // 滚动到底部
    scrollToBottom();
}

// WebSocket连接
function connectWebSocket() {

    if (websocket) {
        websocket.close();
    }
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
    if (websocket) {
        websocket.close(); // 关闭现有连接
    }
    // 如果需要自动连接WebSocket，可以在这里调用 connectWebSocket()
    // connectWebSocket();
};
/*

window.onbeforeunload = () => {
    if (websocket) {
        websocket.close();
        websocket = null;
    }
};
*/

window.closed = () => {
    if (websocket) {
        websocket.close();
        websocket = null;
    }
};