#!/bin/bash
#!/bin/bash
if [ $# -eq 4 ]; then
     mode=$1
     ip=$2
     mocketsPort=$3
     msgNumber=$4
else
  echo "Usage: `basename $0` -client | -server <ip> <mockets_port> <msg_number>"
  exit 1
fi

java -Djava.library.path=../bin -cp ../lib/*:../../externals/java/log4j-1.2.17.jar us.ihmc.kryomockets.test.ClientServerTest $mode $ip $mocketsPort $msgNumber
