document.addEventListener('DOMContentLoaded', () => {
    // 事件绑定
    document.querySelector('.login-btn').addEventListener('click', loginUser);
    document.querySelector('.register-btn').addEventListener('click', registerUser);
});

async function sendRequest(endpoint, method, data) {
    const response = await fetch(`${API_BASE}${endpoint}`, {
        method,
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
    });
    return await response.json();
}

async function registerUser() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const result = await sendRequest('/register', 'POST', {username, password});
        if (result.success) {
            alert('Registration successful! Please login');
            document.getElementById('password').value = '';
        } else {
            throw new Error(result.message || 'Registration failed');
        }
    } catch (error) {
        alert(error.message);
    }
}

async function loginUser() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const result = await sendRequest('/login', 'POST', {username, password});
        if (result.success) {
            // 存储用户信息
            localStorage.setItem(STORAGE_KEYS.USER_ID, result.userId);
            localStorage.setItem(STORAGE_KEYS.USER_NAME, username);
            
            // 跳转到聊天页面
            window.location.href = 'chat.html';
        } else {
            throw new Error(result.message || 'Invalid credentials');
        }
    } catch (error) {
        alert(error.message);
    }
}