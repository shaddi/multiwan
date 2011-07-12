//Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

AddressInfo(tun0 192.168.35.0/24);
tun :: KernelTun(tun0, DEVNAME tun0);
tun_out :: Queue(1024) -> tun;

//Click interface from outside Tunnel to Tunnel
AddressInfo(tunIn 192.168.30.0/24);
kTunIn :: KernelTun(tunIn, DEVNAME tun_in);
kTunIn_out :: Queue(1024) -> kTunIn;

ar :: ARPResponder(192.168.35.2 DE:AD:BE:EF:35:02);

c0 :: Classifier(12/0806 20/0001,
		 12/0806 20/0002,
		 -);

kTunIn -> MarkIPHeader -> IPPrint(TUN_IN, LENGTH true) -> IPEncap(253, 192.168.35.2, 192.168.25.2) -> tun_out;

// classify arp and non-arp from tunnel
tun -> c0;

// arp requests...
c0[0] -> ar -> tun_out;

// arp responses to linux
c0[1] -> kTunIn_out;

// everything else
c0[2] -> StripIPHeader -> MarkIPHeader -> IPPrint(TUN, LENGTH true) -> kTunIn_out;
