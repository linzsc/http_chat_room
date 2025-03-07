document.addEventListener('DOMContentLoaded', () => {
    // 检查登录状态
    const user_id = localStorage.getItem(STORAGE_KEYS.USER_ID);
    const user_name = localStorage.getItem(STORAGE_KEYS.USER_NAME);
    
    if (!user_id || !user_name) {
        alert('Please login first');
        window.location.href = 'index.html';
        return;
    }

    // 初始化
    connectWebSocket();
    setupEventListeners();
    loadHistory();
});

function setupEventListeners() {
    document.getElementById('sendButton').addEventListener('click', sendMessage);
    document.getElementById('messageInput').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') sendMessage();
    });
}

let websocket;
function connectWebSocket() {
    websocket = new WebSocket(WS_URL);

    websocket.onopen = () => {
        console.log('WebSocket connected');
    };

    websocket.onmessage = (event) => {
        const message = JSON.parse(event.data);
        appendMessage(message);
    };

    websocket.onclose = () => {
        console.log('WebSocket disconnected');
        setTimeout(connectWebSocket, 3000);
    };
}

async function loadHistory() {
    try {
        const response = await fetch(`${API_BASE}/getmessage`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ 
                user_id: localStorage.getItem(STORAGE_KEYS.USER_ID) 
            })
        });
        const data = await response.json();
        data.messages.forEach(appendMessage);
        scrollToBottom();
    } catch (error) {
        console.error('Failed to load history:', error);
    }
}

function sendMessage() {
    const input = document.getElementById('messageInput');
    const content = input.value.trim();
    
    if (content) {
        const message = {
            type: '1',
            sender_id: localStorage.getItem(STORAGE_KEYS.USER_ID),
            sender_name: localStorage.getItem(STORAGE_KEYS.USER_NAME),
            content,
            timestamp: Date.now()
        };
        
        websocket.send(JSON.stringify(message));
        appendMessage(message);
        input.value = '';
        scrollToBottom();
    }
}

function appendMessage(message) {
    const list = document.getElementById('messageList');
    const li = document.createElement('li');
    //const isOwnMessage = message.sender_id === localStorage.getItem(STORAGE_KEYS.USER_ID);
    const isOwnMessage = String(message.sender_id) === localStorage.getItem(STORAGE_KEYS.USER_ID);
    li.className = `message ${isOwnMessage ? 'own-message' : 'other-message'}`;
    li.innerHTML = `<strong>${message.sender_name}${isOwnMessage ? ' (You)' : ''}:</strong> ${message.content}`;
    list.appendChild(li);
}

function scrollToBottom() {
    const list = document.getElementById('messageList');
    list.scrollTop = list.scrollHeight;
}