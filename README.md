# mywebserver
reactor模式 webserver
主线程accept连接  从线程池取出子线程处理连接

channel类注册相应事件回调函数

epoll注册fd对应的事件 事件发生时调用channel类的回调函数

shutdown优雅关闭

小根堆处理超时连接

压力测试结果

![image](https://github.com/zzssee5432/mywebserver/blob/master/image/2021-08-08%2010-04-46%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)
