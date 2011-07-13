// Require our package
require(package "multiwan")

// Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);


// Click interface from outside Tunnel to Tunnel
AddressInfo(tunIn 192.168.20.0/24);
host_in :: KernelTun(tunIn, DEVNAME tun_in);
host_out :: Queue(1024) -> host_in;


// Click interfaces from Tunnel to out of Tunnel
// TODO: Duplicate below two lines for the number of lines in the tunnel, as
// well as in other places this is used.
AddressInfo(tun0 192.168.25.0/24);
ar0 :: ARPResponder(192.168.25.2 DE:AD:BE:EF:25:02);
c0 :: Classifier(12/0806 20/0001, 12/0806 20/0002, -);
tun0 :: KernelTun(tun0, DEVNAME tun0);
tun0_out :: Queue(1024) -> Unqueue -> IPFragmenter(1500) -> tun0;
tun0 -> c0;
c0[0] -> ar0 -> tun0_out; // arp requests
c0[1] -> host_out; // arp responses to linux
c0[2] -> IPReassembler() -> tun0_in :: Null(); // everything else

AddressInfo(tun1 192.168.26.0/24);
ar1 :: ARPResponder(192.168.26.2 DE:AD:BE:EF:26:02);
c1 :: Classifier(12/0806 20/0001, 12/0806 20/0002, -);
tun1 :: KernelTun(tun1, DEVNAME tun1);
tun1_out :: Queue(1024) -> Unqueue -> IPFragmenter(1500) -> tun1;
tun1 -> c1;
c1[0] -> ar1 -> tun1_out;
c1[1] -> host_out;
c1[2] -> IPReassembler() -> tun1_in :: Null();


// Flow for packets going into the tunnel
progSch :: ProgScheduler;
progSch[0] 
    -> AddMWanHeader(DEFAULT_BW 100) 
    -> IPEncap(253, 192.168.25.2, 192.168.35.2) 
    -> tun0_out;
progSch[1] 
    -> AddMWanHeader(DEFAULT_BW 100) 
    -> IPEncap(253, 192.168.26.2, 192.168.36.2) 
    -> tun1_out;


// Flow that processes MWan packet headers
calcLDelta :: CalcLatencyDelta(MAX_PAINT 2, OFFSET 0) 
    -> ProcessMWanHeader(PROGSCHEDULER progSch, MAX_PAINT 2) 
    -> Discard;
procHeader :: SetTimestamp 
    -> StripIPHeader 
    -> calcLDelta;


// Flow for packets coming from the tunnel
tcpReorderer :: AggregateIPFlows 
    -> StripIPHeader 
    -> TCPReorderer(CALCLATENCYDELTA calcLDelta) 
    -> UnstripIPHeader 
    -> host_out;

ipcTcp :: IPClassifier(tcp, -);
ipcTcp[0] -> tcpReorderer;
ipcTcp[1] -> host_out;
outOfTunnel :: StripIPHeader 
    -> Strip(12) 
    -> MarkIPHeader 
    -> ipcTcp;

// Beginning of it all
tun0_in -> dup0 :: Tee;
dup0[0] -> outOfTunnel;
dup0[1] -> Paint(0) -> procHeader;
tun1_in -> dup1 :: Tee;
dup1[0] -> outOfTunnel;
dup1[1] -> Paint(1) -> procHeader;

host_in -> progSch;

