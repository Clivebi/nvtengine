cmake_minimum_required(VERSION 3.12.0)

add_library(masscan STATIC 
crypto-base64.c
crypto-blackrock2.c
event-timeout.c
hostscan.c
in-binary.c
in-filter.c
in-report.c
logger.c
main-conf.c
main-dedup.c
#main-initadapter.c
main-listscan.c
main-ptrace.c
main-readrange.c
main-status.c
main-throttle.c
masscan-app.c
massip-addr.c
massip-parse.c
massip-rangesv4.c
massip-rangesv6.c
massip.c
misc-rstfilter.c
out-binary.c
out-certs.c
out-grepable.c
out-hostonly.c
out-json.c
out-ndjson.c
out-null.c
out-redis.c
out-tcp-services.c
out-text.c
out-unicornscan.c
out-xml.c
output.c
pixie-backtrace.c
pixie-file.c
pixie-threads.c
pixie-timer.c
proto-arp.c
proto-banner1.c
proto-banout.c
proto-coap.c
proto-dns.c
proto-ftp.c
proto-http.c
proto-icmp.c
proto-imap4.c
proto-interactive.c
proto-memcached.c
proto-netbios.c
proto-ntlmssp.c
proto-ntp.c
proto-oproto.c
proto-pop3.c
proto-preprocess.c
proto-sctp.c
proto-smb.c
proto-smtp.c
proto-snmp.c
proto-ssh.c
proto-ssl-test.c
proto-ssl.c
proto-tcp-rdp.c
proto-tcp-telnet.c
proto-tcp.c
proto-udp.c
proto-vnc.c
proto-x509.c
proto-zeroaccess.c
rand-blackrock.c
rand-lcg.c
rand-primegen.c
rawsock-getif.c
rawsock-getip.c
rawsock-getip6.c
rawsock-getmac.c
rawsock-getroute.c
rawsock-pcapfile.c
rawsock.c
read-service-probes.c
rte-ring.c
scripting-banner.c
scripting-masscan.c
scripting.c
siphash24.c
smack1.c
smackqueue.c
stack-arpv4.c
stack-if.c
stack-ndpv6.c
stack-queue.c
stack-src.c
string_s.c
stub-lua.c
stub-pcap.c
stub-pfring.c
syn-cookie.c
templ-payloads.c
templ-pkt.c
util-checksum.c
util-malloc.c
versioning.c
vulncheck-heartbleed.c
vulncheck-ntp-monlist.c
vulncheck-sslv3.c
vulncheck.c
xring.c
)