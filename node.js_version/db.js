const mysql = require('mysql2/promise');

// 修改为你的数据库配置
const db = mysql.createPool({
    host: 'localhost',
    user: 'root',
    password: 'yan',
    database: 'Pathpay_optimizer',
    waitForConnections: true,
    connectionLimit: 10,
    queueLimit: 0
});

db.getConnection()
  .then(conn => {
    console.log('数据库连接成功');
    conn.release();
  })
  .catch(err => {
    console.error('数据库连接失败:', err.message);
  });

module.exports = db;
