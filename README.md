# 多种类线程池实现（C++）

本项目设计并实现了多种类的高性能线程池，包括固定式线程池、动态线程池与工作窃取式线程池，支持高并发任务调度处理。项目采用“三层结构”设计：同步服务层、同步队列层、异步服务器层，实现半同步半异步任务调度模型。

---

## 📌 项目亮点

- ✅ 多种线程池架构支持（固定 / 动态 / 工作窃取）
- ✅ 完善的线程同步与通信机制
- ✅ 支持任务完美转发与资源自动管理
- ✅ 面向高并发和高吞吐量场景设计

---

## 🧱 项目结构
```
ThreadPool/
├── bin/ # 可执行文件
├── build
├── test/ # 测试示例
├── threadpool/ # 核心代码
├── CMakeLists.txt # CMake 构建配置
└── README.md # 项目说明文件
```
---

## 🧩 功能模块说明

### 🧵 固定式线程池（FixedThreadPool）

- 固定数量的工作线程
- 主线程提交任务到同步队列，由工作线程轮询执行
- 适用于 **CPU 使用率高** 的任务场景

### 🔄 动态线程池（DynamicThreadPool）

- 可根据任务量**动态创建和销毁线程**
- 避免资源浪费，适用于 **大量短生命周期异步任务**

### 🕸️ 工作窃取线程池（WorkStealingThreadPool）

- 每个线程维护私有任务队列
- 空闲线程可从其他线程“窃取”任务执行
- 提高线程利用率，适用于 **高吞吐 / 递归 / CPU 密集型任务**

---

## 🔧 技术栈

- C++17 并发支持库（`<thread>`、`<mutex>`、`<condition_variable>`、`<future>` 等）
- 智能指针（`std::shared_ptr` / `std::unique_ptr`）
- Lambda 表达式与完美转发（`std::forward`）
- STL 容器（`std::queue`、`std::vector`、`std::deque` 等）
- 原子操作（`std::atomic`）确保线程安全

---

## ⚙️ 编译方式

项目使用 CMake 进行构建：

```bash
cd ThreadPool

mkdir build && cd build

cmake ..

make
