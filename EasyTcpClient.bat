@echo off
::服务端ip地址
::set strIP=any
::服务端端口
::set nPort=4567
@echo off

set cmd="strIP=192.168.31.247"

set cmd=%cmd% nPort=4567
set cmd=%cmd% nThread=4
set cmd=%cmd% nClient=64000

set cmd=%cmd% nMsg=1
set cmd=%cmd% nSendSleep=1500
set cmd=%cmd% nSendBuffSize=10240
set cmd=%cmd% nRecvBuffSize=10240
set cmd=%cmd% -checkMsgID

EasyTcpClient %cmd%
 
pause