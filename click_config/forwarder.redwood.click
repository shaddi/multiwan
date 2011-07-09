//Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

AddressInfo(tun0 192.168.25.0/24);
tun :: KernelTun(tun0, DEVNAME tun0);

//Click interface from outside Tunnel to Tunnel
AddressInfo(tunIn 192.168.20.0/24);
kTunOutIn :: KernelTun(tunIn, DEVNAME tun_in);

kTunOutIn -> MarkIPHeader -> IPPrint(TUN_IN, LENGTH true) -> IPEncap(4, 192.168.25.1, 192.168.35.1) -> tun;

tun -> StripIPHeader -> MarkIPHeader -> IPPrint(TUN, LENGTH true) -> kTunOutIn;
