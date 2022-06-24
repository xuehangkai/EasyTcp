cd `dirname $0`

nOpenFile=`ulimit -n`
if [ $nOpenFile -lt 10240 ];then
	echo "重置当前进程可以打开的最大文件数"
	ulimit -n 10240
fi
echo "当前进程可以打开的最大文件数"
ulimit -n

#服务端IP地址
cmd='strIP=any'
#服务端端口
cmd=$cmd' nPort=4567'
#消息处理线程数量
cmd=$cmd" nThread=2"
#客户端连接上限
cmd="$cmd nMaxClient=10000"
#客户端发送缓冲区大小（字节）
cmd=$cmd" nSendBuffSize=20480"
#客户端接收缓冲区大小（字节）
cmd=$cmd" nRecvBuffSize=20480"
#收到消息后将返回应答消息
cmd=$cmd" -sendback"
#提示发送缓冲区已写满
#当出现sendfull提示时，表示当次消息被丢弃
cmd=$cmd" -sendfull"
#检查接收到的客户端消息ID是否连续
cmd=$cmd" -checkMsgID"
#自定义标志 未使用
cmd=$cmd" -p"

#启动程序 传入参数
./server $cmd
#
read -p "..press any key to exit.." var
