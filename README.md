# mywebserver

reactor模式服务器 主线程accept连接   从线程池里取出线程处理连接   状态机处理报文  


epoll 注册事件   channel中注册相应的回调函数  根据产生事件的fd找到对应channel 调用对应的回调函数


eventfd 唤醒线程 


小根堆处理超时连接


shutdown优雅关闭连接


wenbench压测结果


![image](https://github.com/zzssee5432/mywebserver/blob/master/image/2021-08-08%2009-43-24%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)
