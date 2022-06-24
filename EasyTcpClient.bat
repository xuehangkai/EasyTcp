@echo off
::服务端ip地址
::set strIP=any
::服务端端口
::set nPort=4567
@echo off

set cmd="strIP=192.168.31.250"

set cmd=%cmd% nPort=4567
set cmd=%cmd% nThread=1
set cmd=%cmd% nClient=10000

set cmd=%cmd% nMsg=10
set cmd=%cmd% nSendSleep=1000
set cmd=%cmd% nSendBuffSize=20480
set cmd=%cmd% nRecvBuffSize=20480
set cmd=%cmd% -checkMsgID

EasyTcpClient %cmd%
 
pause