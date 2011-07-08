//Require our package
require(package "multiwan")

//Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

//Click interfaces from Tunnel to out of Tunnel
//TODO: Duplicate below two lines for the number of lines in the tunnel, as wlel as in other places this is used.
AddressInfo(tun0 192.168.35.0/24);
kTunLine0 :: KernelTun(tun0, DEVNAME tun0);
AddressInfo(tun1 192.168.36.0/24);
kTunLine1 :: KernelTun(tun1, DEVNAME tun1);


//Click interface from outside Tunnel to Tunnel
AddressInfo(tunIn 192.168.30.0/24);
kTunOutIn :: KernelTun(tunIn, DEVNAME tun_in);


//Flow for packets going into the tunnel
progSch :: ProgScheduler;
progSch[0] -> AddMWanHeader(DEFAULT_BW 100) -> IPEncap(4, 192.168.35.1, 192.168.25.1) -> kTunLine0;
progSch[1] -> AddMWanHeader(DEFAULT_BW 100) -> IPEncap(4, 192.168.36.1, 192.168.26.1) -> kTunLine1;


//Flow that processes MWan packet headers
calcLDelta :: CalcLatencyDelta(MAX_PAINT 2, OFFSET 0) -> ProcessMWanHeader(PROGSCHEDULER progSch, MAX_PAINT 2) -> Discard;
procHeader :: SetTimestamp -> StripIPHeader -> calcLDelta;


//Flow for packets coming from the tunnel
tcpReorderer :: AggregateIPFlows -> StripIPHeader ->
              TCPReorderer(CALCLATENCYDELTA calcLDelta) -> UnstripIPHeader ->
              kTunOutIn;
ipcTcp ::  IPClassifier(tcp, -);
ipcTcp[0] -> tcpReorderer;
ipcTcp[1] -> kTunOutIn;
outOfTunnel :: StripIPHeader -> Strip(12) -> MarkIPHeader -> ipcTcp;


//Beginning of it all
kTunLine0 -> dup0 :: Tee;
dup0[0] -> outOfTunnel;
dup0[1] -> Paint(0) -> procHeader;
kTunLine1 -> dup1 :: Tee;
dup1[0] -> outOfTunnel;
dup1[1] -> Paint(1) -> procHeader;

kTunOutIn -> progSch;

