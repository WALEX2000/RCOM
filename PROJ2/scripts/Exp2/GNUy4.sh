/etc/init.d/networking restart
ifconfig eth0 up
ifconfig eth0 172.16.$10.254/24
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts