@echo off

::服务端IP地址
set cmd="strIP=any"
::服务端端口
set cmd=%cmd% nPort=4567
::消息处理线程数量
set cmd=%cmd% nThread=4
::客户端连接上限
set cmd=%cmd% nMaxClient=100064
::客户端发送缓冲区大小（字节）
set cmd=%cmd% nSendBuffSize=10240
::客户端接收缓冲区大小（字节）
set cmd=%cmd% nRecvBuffSize=10240
::收到消息后将返回应答消息
set cmd=%cmd% -sendback
::提示发送缓冲区已写满
::当出现sendfull提示时，表示当次消息被丢弃
set cmd=%cmd% -sendfull
::检查接收到的客户端消息ID是否连续
set cmd=%cmd% -checkMsgID
::自定义标志 未使用
set cmd=%cmd% -p

::启动程序 传入参数
EasyTcpServer %cmd%

pause