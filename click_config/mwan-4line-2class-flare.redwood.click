// Require our package
require(package "multiwan")

// Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

// Define non class specific elements
fas :: FlowAgeSplitter();

// Define variables for config file.
define($offset 0); // Offset between beginning of packet and our header
define($c0_max_paint 2); // Number of lines for class 0
define($c1_max_paint 2); // Number of lines for class 1

// Define our elements for Class 0
c0_cld :: CalcLatencyDelta(OFFSET $offset, MAX_PAINT $c0_max_paint);

c0_fs :: FlareSwitch(START_MTBS 10);

c0_ph :: ProcessMWanHeader3(FLARESWITCH c0_fs, MAX_PAINT $c0_max_paint,
                       UPDATE_INT 100, // Update interval for DistroSwitch (ms)
                       DISTRIB_TOTAL 100, // Total "points" giving to lines
                       DISTRIB_INC 1, // Points move b/t lines when redistribute
                       DISTRIB_MIN 5, // Smallest points a line can have
                       FLOWAGESPLITTER fas, // Splits between classes
                       // FLOWSPLIT_NUM , // Using default (1) for now
                       FLOWSPLIT_THRESHOLD 3 // Using defualt (4 of 16)  for now
                       );

c0_ccd0 :: CalcCongestionDelta(OFFSET $offset);
c0_ccd1 :: CalcCongestionDelta(OFFSET $offset);

c0_ah0 :: AddMWanHeader3(CALC_CONGESTION_DELTA c0_ccd0, CALC_LATENCY_DELTA c0_cld);
c0_ah1 :: AddMWanHeader3(CALC_CONGESTION_DELTA c0_ccd1, CALC_LATENCY_DELTA c0_cld);

// Define our elements for Class 1
c1_cld :: CalcLatencyDelta(OFFSET $offset, MAX_PAINT $c1_max_paint);

c1_fs :: FlareSwitch(START_MTBS 10);

c1_ph :: ProcessMWanHeader3(FLARESWITCH c1_fs, MAX_PAINT $c1_max_paint,
                       UPDATE_INT 100, // Update interval for DistroSwitch (ms)
                       DISTRIB_TOTAL 100, // Total "points" giving to lines
                       DISTRIB_INC 1, // Points move b/t lines when redistribute
                       DISTRIB_MIN 5 // Smallest points a line can have
                       // Since this is the "lower" class doesn't see flow stuff
                       );

c1_ccd0 :: CalcCongestionDelta(OFFSET $offset);
c1_ccd1 :: CalcCongestionDelta(OFFSET $offset);

c1_ah0 :: AddMWanHeader3(CALC_CONGESTION_DELTA c1_ccd0, CALC_LATENCY_DELTA c1_cld);
c1_ah1 :: AddMWanHeader3(CALC_CONGESTION_DELTA c1_ccd1, CALC_LATENCY_DELTA c1_cld);

// Address and tun stuff
AddressInfo(host_tun 192.168.20.0/24);
AddressInfo(tun0 192.168.25.0/24);
AddressInfo(tun1 192.168.26.0/24);
AddressInfo(tun2 192.168.27.0/24);
AddressInfo(tun3 192.168.28.0/24);
host :: KernelTun(host_tun, DEVNAME tun_host, MTU 2000); // TODO: Fix me.
tun0_dev :: KernelTun(tun0, DEVNAME tun0, MTU 2000);
tun1_dev :: KernelTun(tun1, DEVNAME tun1, MTU 2000);
tun2_dev :: KernelTun(tun2, DEVNAME tun2, MTU 2000);
tun3_dev :: KernelTun(tun3, DEVNAME tun3, MTU 2000);
tun0 :: Null -> tun0_dev;
tun1 :: Null -> tun1_dev;
tun2 :: Null -> tun2_dev;
tun3 :: Null -> tun3_dev;

// TODO: take Unqueue out
host -> MarkIPHeader -> aipf :: AggregateIPFlows -> fas;

fas[0] -> MarkIPHeader -> SetTimestamp -> c0_fs;
fas[1] -> MarkIPHeader -> SetTimestamp -> c1_fs;

aipf[1] -> MarkIPHeader -> SetTimestamp -> c0_fs; //TODO:Mild hack, but just for now

c0_cld -> c0_ph -> Discard;
c1_cld -> c1_ph -> Discard;

// TODO: take Unqueue out
tun0_dev -> c0_tee0 :: Tee();
tun1_dev -> c0_tee1 :: Tee();
tun2_dev -> c1_tee0 :: Tee();
tun3_dev -> c1_tee1 :: Tee();

c0_tee0[0] -> Paint(0) -> SetTimestamp -> StripIPHeader -> c0_ccd0 -> c0_cld;
c0_tee1[0] -> Paint(1) -> SetTimestamp -> StripIPHeader -> c0_ccd1 -> c0_cld;
c1_tee0[0] -> Paint(0) -> SetTimestamp -> StripIPHeader -> c1_ccd0 -> c1_cld;
c1_tee1[0] -> Paint(1) -> SetTimestamp -> StripIPHeader -> c1_ccd1 -> c1_cld;

c0_to_internet :: StripIPHeader -> Strip(14) -> MarkIPHeader -> host;
c1_to_internet :: StripIPHeader -> Strip(14) -> MarkIPHeader -> host;

c0_tee0[1] -> c0_to_internet;
c0_tee1[1] -> c0_to_internet;
c1_tee0[1] -> c1_to_internet;
c1_tee1[1] -> c1_to_internet;

// TODO: fix IPEncap
c0_fs[0] -> c0_ah0 -> IPEncap(253, 192.168.25.2, 192.168.35.2) -> tun0;
c0_fs[1] -> c0_ah1 -> IPEncap(253, 192.168.26.2, 192.168.36.2) -> tun1;
c1_fs[0] -> c1_ah0 -> IPEncap(253, 192.168.27.2, 192.168.37.2) -> tun2;
c1_fs[1] -> c1_ah1 -> IPEncap(253, 192.168.28.2, 192.168.38.2) -> tun3;

// s :: Script(
//      label loop_beg,
//      wait .25,
//      print "---------------------------------- FLOWAGESPLITTER: flow count" $(fas.get_flow_count),
//      goto loop_beg
//      );
