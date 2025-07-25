#include <iostream>
#include <thread>
#include <latch>
#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <deque>
using namespace std;

#if 0 // Test for fixedthreadpool
#include "FiexdThreadPool.hpp"
FiexdThreadPool pool;
void Add(int a,int b, std::promise<int> &c_promise){
    cout<<"add begin..."<<endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    int c = a + b;
    c_promise.set_value(c);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cout<<"add end..."<<endl;
}

void add_a(){
    std::promise<int> c_promise;
    std::future<int> a_future = c_promise.get_future();
    std::function<void(void)> task = std::bind(Add, 10, 20, std::ref(c_promise));
    pool.AddTask(task);
    cout<<"add_a :"<<a_future.get()<<endl;
}
void add_b(){
    std::promise<int> c_promise;
    std::future<int> a_future = c_promise.get_future();
    std::function<void(void)> task = std::bind(Add, 30, 40, std::ref(c_promise));
    pool.AddTask(task);
    cout<<"add_b :"<<a_future.get()<<endl;
}
void add_c(){
    std::promise<int> c_promise;
    std::future<int> a_future = c_promise.get_future();
    std::function<void(void)> task = std::bind(Add, 50, 60, std::ref(c_promise));
    pool.AddTask(task);
    cout<<"add_c :"<<a_future.get()<<endl;
}
int main(){
    std::thread t1(add_a);
    std::thread t2(add_b);
    std::thread t3(add_c);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
#endif

#if 0 // Test for CachedThread Pool Case 1
#include "CachedThreadPool.hpp"
void func(int index){
    static int num =0 ;
    cout<<"func_"<<index<<"num:"<<++num<<endl;
}
int add(int a,int b){
    return a + b;
}
int main(){
    CachedThreadPool mypool;
    for(int i=0;i<10;i++){
        if(i%2 == 0){
            auto pa = mypool.Submit(add,i,i+1);
            cout<<pa.get()<<endl;
        }else{
            mypool.execute(func,i);
        }
    }
}
#endif

#if 0 // Test for CachedThread Pool Case 2
#include "CachedThreadPool.hpp"
CachedThreadPool pool(2);
int add(int a,int b,int s){
    std::this_thread::sleep_for(std::chrono::seconds(s));
    int c = a+b;
    cout<<"add begin..."<<endl;
    return c;
}
void add_a(){
    auto r = pool.AddTask(add,10,20,4);
    cout<<"add_a:"<<r.get()<<endl;
}
void add_b(){
    auto r = pool.AddTask(add,20,30,6);
    cout<<"add_b:"<<r.get()<<endl;
}
void add_c(){
    auto r = pool.AddTask(add,30,40,1);
    cout<<"add_c:"<<r.get()<<endl;
}
void add_d(){
    auto r = pool.AddTask(add,10,40,9);
    cout<<"add_d:"<<r.get()<<endl;
}
int main(){
    std::thread tha(add_a);
    std::thread thb(add_b);
    std::thread thc(add_c);
    std::thread thd(add_d);

    tha.join();
    thb.join();
    thc.join();
    thd.join();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::thread the(add_a);
    std::thread thf(add_b);
    the.join();
    thf.join();
    return 0;
}
#endif

#if 1 // Test for WorkSteal Thread Pool
#include "WorkStealingPool.hpp"
int add(int a, int b, int s)
{
    clog << "add begin" << endl;
    int c = a + b;
    clog << "add end" << endl;
    return c;
}
void add_a(int ch, int x, int y, WorkStealingPool &mypool)
{
    auto r = mypool.Submit(add, x, y, rand() % 10);
    cout << "add_" << ch << r.get() << endl;
}
int main()
{
    const int n = 2000;
    WorkStealingPool mypool(10000, 8);  // 队列容量 10000，线程数 8
    std::vector<std::thread> tha(n);

    for (int i = 0; i < n; ++i) {
        tha[i] = std::thread(add_a, i, i + 20, i + 10, std::ref(mypool));
    }

    for (auto &t : tha) {
        if (t.joinable()) t.join();
    }

    std::cout << "All tasks completed!" << std::endl;
    return 0;
}

#endif