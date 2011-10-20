// Require our package
require(package "multiwan")

// Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

// Define variables for config file.
define($offset 0); // Offset between beginning of packet and our header
define($max_paint 2); // Number of lines

// Define our elements
cld :: CalcLatencyDelta(OFFSET $offset, MAX_PAINT $max_paint);

fs :: FlareSwitch(START_MTBS 10);

ph :: ProcessMWanHeader3(FLARESWITCH fs, MAX_PAINT $max_paint,
                       UPDATE_INT 100, // Update interval for DistroSwitch (ms)
                       DISTRIB_TOTAL 100, // Total "points" giving to lines
                       DISTRIB_INC 1, // Points move b/t lines when redistribute
                       DISTRIB_MIN 5 // Smallest points a line can have
                       );

tcpr :: TCPReorderer(CALCLATENCYDELTA cld);

ccd0 :: CalcCongestionDelta(OFFSET $offset);
ccd1 :: CalcCongestionDelta(OFFSET $offset);

ah0 :: AddMWanHeader3(CALC_CONGESTION_DELTA ccd0, CALC_LATENCY_DELTA cld);
ah1 :: AddMWanHeader3(CALC_CONGESTION_DELTA ccd1, CALC_LATENCY_DELTA cld);

AddressInfo(host_tun 192.168.30.0/24);
AddressInfo(tun0 192.168.35.0/24);
AddressInfo(tun1 192.168.36.0/24);
host :: KernelTun(host_tun, DEVNAME tun_host, MTU 2000);
tun0_dev :: KernelTun(tun0, DEVNAME tun0, MTU 2000);
tun1_dev :: KernelTun(tun1, DEVNAME tun1, MTU 2000);
tun0 :: Null -> tun0_dev;
tun1 :: Null -> tun1_dev;
// tun0 :: Null -> c0 :: Counter -> tun0_dev;
// tun1 :: Null -> c1 :: Counter -> tun1_dev;

// TODO: take Unqueue out
host -> fs;

cld -> ph -> Discard;

// TODO: take Unqueue out
tun0_dev -> tee0 :: Tee();
tun1_dev -> tee1 :: Tee();

tee0[0] -> Paint(0) -> SetTimestamp -> StripIPHeader -> ccd0 -> cld;
tee1[0] -> Paint(1) -> SetTimestamp -> StripIPHeader -> ccd1 -> cld;

ipcTcp :: IPClassifier(tcp, -);
ipcTcp[0] -> MarkIPHeader -> AggregateIPFlows -> StripIPHeader ->
          tcpr -> UnstripIPHeader -> host;
ipcTcp[1] -> MarkIPHeader -> host;

to_internet :: StripIPHeader -> Strip(12) -> ipcTcp;

tee0[1] -> to_internet;
tee1[1] -> to_internet;

// TODO: fix IPEncap
fs[0] -> ah0 -> IPEncap(253, 192.168.35.2, 192.168.25.2) -> tun0;
fs[1] -> ah1 -> IPEncap(253, 192.168.36.2, 192.168.26.2) -> tun1;

// Debug script element comment me out when not needed:
// debug :: Script(
//       label loop_beg,
//       wait .25,
//       set a $(ccd0.get_congestion_score),
//       set b $(ccd1.get_congestion_score),
//       print "c0:" $(c0.count) "c1:" $(c1.count),// "ccd0:" $(a) "ccd1:" $(b),
//       write c0.reset,
//       write c1.reset,
//       goto loop_beg
// );
