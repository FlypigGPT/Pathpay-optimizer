const express = require('express');
const cors = require('cors');
const axios = require('axios');
const db = require('./db');
const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(__dirname));

// 用户注册
app.post('/api/register', async (req, res) => {
    const { username, password, role } = req.body;
    try {
        const [rows] = await db.query('SELECT id FROM users WHERE username = ?', [username]);
        if (rows.length > 0) {
            return res.status(400).json({ msg: '用户已存在' });
        }
        await db.query('INSERT INTO users (username, password, role) VALUES (?, ?, ?)', [username, password, role]);
        res.json({ msg: '注册成功' });
    } catch (e) {
        res.status(500).json({ msg: '注册失败', error: e.message });
    }
});

// 用户登录
app.post('/api/login', async (req, res) => {
    const { username, password } = req.body;
    try {
        const [rows] = await db.query('SELECT role FROM users WHERE username = ? AND password = ?', [username, password]);
        if (rows.length === 0) return res.status(401).json({ msg: '用户名或密码错误' });
        res.json({ role: rows[0].role });
    } catch (e) {
        res.status(500).json({ msg: '登录失败', error: e.message });
    }
});

// 添加机构
app.post('/api/institution', async (req, res) => {
    const { name, country } = req.body;
    try {
        const [rows] = await db.query('SELECT id FROM institutions WHERE name = ?', [name]);
        if (rows.length > 0) {
            return res.status(400).json({ msg: '机构已存在' });
        }
        await db.query('INSERT INTO institutions (name, country) VALUES (?, ?)', [name, country]);
        res.json({ msg: '添加成功' });
    } catch (e) {
        res.status(500).json({ msg: '添加失败', error: e.message });
    }
});

// 查询机构
app.get('/api/institutions', async (req, res) => {
    try {
        const [rows] = await db.query('SELECT name, country FROM institutions');
        res.json(rows);
    } catch (e) {
        res.status(500).json({ msg: '查询失败', error: e.message });
    }
});

// 添加金融流路径
app.post('/api/edge', async (req, res) => {
    const { from, to, feeRate, fromCur, toCur } = req.body;
    try {
        await db.query('INSERT INTO edges (from_institution, to_institution, feeRate, fromCur, toCur) VALUES (?, ?, ?, ?, ?)', [from, to, feeRate, fromCur, toCur]);
        res.json({ msg: '添加成功' });
    } catch (e) {
        res.status(500).json({ msg: '添加失败', error: e.message });
    }
});

// 查询所有路径
app.get('/api/edges', async (req, res) => {
    try {
        const [rows] = await db.query('SELECT from_institution as `from`, to_institution as `to`, feeRate, fromCur, toCur FROM edges');
        res.json(rows);
    } catch (e) {
        res.status(500).json({ msg: '查询失败', error: e.message });
    }
});

// 获取汇率
app.get('/api/rate', async (req, res) => {
    const { from, to } = req.query;
    try {
        const apiKey = 'RGBaQ6phT9oTNYbWnfsWacN28mahcOJj';
        const url = `https://api.apilayer.com/currency_data/convert?from=${from}&to=${to}&amount=1`;
        const response = await axios.get(url, { headers: { apikey: apiKey } });
        if (response.data.success) {
            res.json({ rate: response.data.result });
        } else {
            res.status(400).json({ msg: 'API返回失败' });
        }
    } catch (e) {
        res.status(500).json({ msg: '汇率获取失败', error: e.message });
    }
});

// Dijkstra算法查询最优路径
app.post('/api/bestpath', async (req, res) => {
    const { start, end, amount } = req.body;
    try {
        const [edges] = await db.query('SELECT from_institution as `from`, to_institution as `to`, feeRate, fromCur, toCur FROM edges');
        // 1. 先用无权重/无汇率的Dijkstra找可达路径
        let graph = {};
        edges.forEach(e => {
            if (!graph[e.from]) graph[e.from] = [];
            graph[e.from].push(e);
        });
        let visited = new Set();
        let prev = {};
        let queue = [start];
        visited.add(start);
        let found = false;
        while (queue.length) {
            let node = queue.shift();
            if (node === end) { found = true; break; }
            for (let edge of (graph[node] || [])) {
                if (!visited.has(edge.to)) {
                    visited.add(edge.to);
                    prev[edge.to] = node;
                    queue.push(edge.to);
                }
            }
        }
        if (!found) return res.json({ msg: '无法到达目标机构' });
        // 2. 还原路径
        let path = [], at = end;
        while (at) {
            path.unshift(at);
            at = prev[at];
        }
        // 3. 只对路径上的边查汇率并计算金额
        let curAmount = amount;
        let apiKey = 'RGBaQ6phT9oTNYbWnfsWacN28mahcOJj';
        for (let i = 0; i < path.length - 1; i++) {
            let fromNode = path[i];
            let toNode = path[i + 1];
            let edge = edges.find(e => e.from === fromNode && e.to === toNode);
            let rate = 1;
            if (edge && edge.fromCur !== edge.toCur) {
                try {
                    const url = `https://api.apilayer.com/currency_data/convert?from=${edge.fromCur}&to=${edge.toCur}&amount=1`;
                    const response = await axios.get(url, { headers: { apikey: apiKey } });
                    if (response.data.success) rate = response.data.result;
                } catch {}
            }
            curAmount = curAmount * rate * (1 - (edge ? edge.feeRate : 0));
        }
        res.json({ path, finalAmount: curAmount });
    } catch (e) {
        res.status(500).json({ msg: '最优路径查询失败', error: e.message });
    }
});

// 系统统计接口
app.get('/api/stats', async (req, res) => {
    try {
        const [[{count: institutionCount}]] = await db.query('SELECT COUNT(*) as count FROM institutions');
        const [[{count: edgeCount}]] = await db.query('SELECT COUNT(*) as count FROM edges');
        const [[{count: userCount}]] = await db.query('SELECT COUNT(*) as count FROM users');
        res.json({ institutionCount, edgeCount, userCount });
    } catch (e) {
        res.status(500).json({ msg: '统计失败', error: e.message });
    }
});

app.listen(PORT, () => {
    console.log(`Server running at http://localhost:${PORT}`);
});
