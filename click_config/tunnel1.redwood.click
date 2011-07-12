//Require our package
require(package "multiwan")

//Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

//Click interfaces from Tunnel to out of Tunnel
//TODO: Duplicate below two lines for the number of lines in the tunnel, as
// well as in other places this is used.
AddressInfo(tun0 192.168.25.0/24);
tun0 :: KernelTun(tun0, DEVNAME tun0);
tun0_out :: Queue(1024) -> tun0;

AddressInfo(tun1 192.168.26.0/24);
tun1 :: KernelTun(tun1, DEVNAME tun1);
tun1_out :: Queue(1024) -> tun1;

//Click interface from outside Tunnel to Tunnel
AddressInfo(tunIn 192.168.20.0/24);
in :: KernelTun(tunIn, DEVNAME tun_in);
out :: Queue(1024) -> in;


//Flow for packets going into the tunnel
progSch :: ProgScheduler;
progSch[0]  
    -> AddMWanHeader(DEFAULT_BW 100) 
    -> IPEncap(253, 192.168.25.2, 192.168.35.2) 
    -> tun0_out;

progSch[1]  
    -> AddMWanHeader(DEFAULT_BW 100) 
    -> IPEncap(253, 192.168.26.2, 192.168.36.2) 
    -> tun1_out;

//Flow that processes MWan packet headers
calcLDelta :: CalcLatencyDelta(MAX_PAINT 2, OFFSET 0) 
    -> ProcessMWanHeader(PROGSCHEDULER progSch, MAX_PAINT 2) 
    -> Discard;
procHeader :: SetTimestamp 
    -> StripIPHeader 
    -> calcLDelta;


//Flow for packets coming from the tunnel
tcpReorderer :: AggregateIPFlows 
    -> StripIPHeader 
    -> TCPReorderer(CALCLATENCYDELTA calcLDelta) 
    -> UnstripIPHeader 
    -> out;

ipcTcp ::  IPClassifier(tcp, -);
ipcTcp[0] -> tcpReorderer;
ipcTcp[1] -> out;
outOfTunnel :: StripIPHeader 
    -> Strip(12) 
    -> MarkIPHeader 
    -> ipcTcp;


//Beginning of it all
tun0 -> dup0 :: Tee;
dup0[0] -> outOfTunnel;
dup0[1] -> Paint(0) -> procHeader;
tun11 -> dup1 :: Tee;
dup1[0] -> outOfTunnel;
dup1[1] -> Paint(1) -> procHeader;

in -> progSch;

