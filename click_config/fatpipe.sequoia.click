/* 
 */

// REDWOOD is A, SEQUOIA is B
AddressInfo(mac_a 00:30:48:59:43:99, mac_b 00:30:48:5b:d7:13);
AddressInfo(side_a 192.168.200.0/24, side_b 192.168.100.0/24);
AddressInfo(tun0 192.168.30.0/24); // sequoia specific

//tun :: KernelTun(tun0);
//dev :: Queue -> EtherEncap(0x0800, mac_b, mac_a) -> ToDevice(eth1);
//tun -> dev
