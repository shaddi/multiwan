//Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

//Click interfaces from Tunnel to out of Tunnel
//TODO: Duplicate below two lines for the number of lines in the tunnel, as wlel as in other places this is used.
AddressInfo(tun0 <TODO: put stuff here>);
kTunLine0 :: KernelTun(tun0, [TODO: MTU <number>], DEVNAME <TODO: fill in name>);


//Click interface from outside Tunnel to Tunnel
AddressInfo(tun0 <TODO: put stuff here>);
kTunOutIn :: KernelTun(tun0, [TODO: MTU <number>], DEVNAME <TODO: fill in name>);


//Flow for packets going into the tunnel
progSch :: ProgScheduler;
progSch[0] -> AddMWanHeader(DEFAULT_BW <TODO: number>) -> IPEncap(<TODO:proto>, <TODO:src>,<TODO:dst>) -> kTunLine0;


//Flow that processes MWan packet headers
calcLDelta :: CalcLatencyDelta(2,0) -> ProcessMWanHeader(progSch, 2) -> Discard;
procHeader :: SetTimestamp -> StripIPHeader -> calcLDelta;


//Flow for packets coming from the tunnel
tcpReorderer :: AggregateIPFlows -> StripIPHeader ->
              TCPReorderer(calcLDelta) -> UnstripIPHeader -> kTunOutIn;
ipcTcp ::  IPClassifier(tcp, -);
ipcTcp[0] -> tcpReorderer;
ipcTcp[1] -> kTunOutIn;
outOfTunnel :: StripIPHeader -> Strip(12) -> MarkIPHeader -> ipcTcp;


//Beginning of it all
kTunLine0 -> dup0 :: Tee;
dup0[0] -> outOfTunnel;
dup0[0] -> Paint(0) -> procHeader;

kTunOutIn -> progSch;

