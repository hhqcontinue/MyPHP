# 阻塞型server(BlockingServer)

在I/O操作里，有两个步骤：

> * 1、等待数据就绪

> * 2、把数据从内核拷贝到程序

阻塞型server在两个步骤都会阻塞，在调用read/recvfrom时，server就开始阻塞，等待客户端数据到来，此时server不能处理其他请求，如果有新的请求到来，server不会响应它，一旦数据到来，server将数据拷贝到用户内存中，处理成功后，server才重新运行，处理下一个请求。


# 多进程server(MultiProcessServer)

BlockingServer的弊端是每次只能处理一个请求，新的请求必须等待前面的请求处理完才能继续。在阻塞型server的基础上增加多进程，新请求过来时，fork一个进程处理新的请求，这样可以同时处理多个请求一起到来的情况。