# Pathpay_optimizer

本项目为金融路径优化系统，包含两个版本：

## 1. C++ 版本
- 位于项目根目录。
- 通过 main.cpp 实现，支持命令行操作，数据文件为 users.txt、nodes.txt、edges.txt。
- 适合本地批量计算和基础功能演示。

## 2. Node.js + MySQL + 前端可视化版本
- 位于 `node.js_version/` 目录。
- 后端基于 Node.js + Express，数据存储为 MySQL，API 兼容原有 C++ 逻辑。
- 前端为多页面（登录、注册、主页、机构管理、路径管理、最优路径查询），风格统一，支持管理员/普通用户分权。
- 首页集成仪表盘统计（ECharts 环形图/数字卡片），最优路径查询结果带动画。

### 主要功能
- 用户注册/登录/身份校验
- 机构与路径管理
- 最优路径查询（含手续费、汇率明细）
- 管理员与普通用户功能分区
- 数据迁移脚本与接口

### 目录结构
```
|-- main.cpp                # C++核心代码
|-- users.txt/nodes.txt/edges.txt  # C++数据文件
|-- node.js_version/
|     |-- server.js         # Node.js后端主程序
|     |-- db.js             # 数据库连接
|     |-- *.html            # 前端页面
|     |-- package.json      # 依赖配置
```

### 启动方式
#### C++ 版本
- 使用 VS/MinGW 编译 main.cpp，命令行运行。

#### Node.js 版本
1. 安装依赖：`npm install`
2. 配置 MySQL 数据库并导入表结构
3. 启动服务：`node server.js`
4. 浏览器访问 `http://localhost:3000/home.html`

---
如需体验可视化、动画和多用户权限管理，推荐使用 Node.js 版本。