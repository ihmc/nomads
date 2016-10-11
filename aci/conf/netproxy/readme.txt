Pick a configuration template and copy all the files in IHMC/confs


netproxy.cfg
Add the internal and external interfaces (es. eth0, eth1), you can obtain them by using ifconfig. The interface with an IP is the external one while the one with no IP is the internal.
If both interfaces have an IP you probably should check your etc/interfaces file.
InternalInterfaceName = eth0
ExternalInterfaceName = eth1

ProxyUniqueIDs
Add as many entries (x.x.x.x == remote netproxy IP) as the netproxy that the local one is supposed to connect to. 
x.x.x.x MocketsPort=5800 TCPPort=5800 UDPPort=5801 autoConnect=Mockets reconnectInterval=5000 connectivity=ACTIVE

proxyStaticARPTable.cfg
You should not touch this file.

proxyEndPoints.cfg
You should not touch this file.

proxyAddrMapping.cfg
Add as many entry as IP you need to route to remote netproxy