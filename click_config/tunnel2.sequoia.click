// Require our package
require(package "multiwan")

// Creates a port where read/write handler calls can be made
ControlSocket("TCP", 50001);
ChatterSocket("TCP", 50002);

// Define variables for config file.
define($offset 0); // Offset between beginning of packet and our header
define($max_paint 4); // Number of lines

// Define our elements
cld :: CalcLatencyDelta(OFFSET $offset, MAX_PAINT $max_paint);

ds :: DistroSwitch;

ph :: ProcessMWanHeader2(DISTROSWITCH ds, MAX_PAINT $max_paint,
                       UPDATE_INT 100, // Update interval for DistroSwitch (ms)
                       DISTRIB_TOTAL 100, // Total "points" giving to lines
                       DISTRIB_INC 10, // Points move b/t lines when redistribute
                       DISTRIB_MIN 5 // Smallest points a line can have
                       );

tcpr :: TCPReorderer(CALCLATENCYDELTA cld);

ccd0 :: CalcCongestionDelta(OFFSET $offset);
ccd1 :: CalcCongestionDelta(OFFSET $offset);
ccd2 :: CalcCongestionDelta(OFFSET $offset);
ccd3 :: CalcCongestionDelta(OFFSET $offset);

ah0 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd0);
ah1 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd1);
ah2 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd2);
ah3 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd3);

AddressInfo(host_tun 192.168.30.0/24);
AddressInfo(tun0 192.168.35.0/24);
AddressInfo(tun1 192.168.36.0/24);
AddressInfo(tun2 192.168.37.0/24);
AddressInfo(tun3 192.168.38.0/24);
host :: KernelTun(host_tun, DEVNAME tun_host); // TODO: Fix me.
tun0_dev :: KernelTun(tun0, DEVNAME tun0);
tun1_dev :: KernelTun(tun1, DEVNAME tun1);
tun2_dev :: KernelTun(tun2, DEVNAME tun2);
tun3_dev :: KernelTun(tun3, DEVNAME tun3);
tun0 :: Null -> tun0_dev;
tun1 :: Null -> tun1_dev;
tun2 :: Null -> tun2_dev;
tun3 :: Null -> tun3_dev;

// TODO: take Unqueue out
host -> ds;

cld -> ph -> Discard;

// TODO: take Unqueue out
tun0_dev -> tee0 :: Tee();
tun1_dev -> tee1 :: Tee();
tun2_dev -> tee2 :: Tee();
tun3_dev -> tee3 :: Tee();

tee0[0] -> Paint(0) -> SetTimestamp -> StripIPHeader -> ccd0 -> cld;
tee1[0] -> Paint(1) -> SetTimestamp -> StripIPHeader -> ccd1 -> cld;
tee2[0] -> Paint(2) -> SetTimestamp -> StripIPHeader -> ccd2 -> cld;
tee3[0] -> Paint(3) -> SetTimestamp -> StripIPHeader -> ccd3 -> cld;

ipcTcp :: IPClassifier(tcp, -);
ipcTcp[0] -> MarkIPHeader -> AggregateIPFlows -> StripIPHeader ->
          tcpr -> UnstripIPHeader -> host;
ipcTcp[1] -> MarkIPHeader -> host;

to_internet :: StripIPHeader -> Strip(10) -> ipcTcp;

tee0[1] -> to_internet;
tee1[1] -> to_internet;
tee2[1] -> to_internet;
tee3[1] -> to_internet;

// TODO: fix IPEncap
ds[0] -> ah0 -> IPEncap(253, 192.168.35.2, 192.168.25.2) -> tun0;
ds[1] -> ah1 -> IPEncap(253, 192.168.36.2, 192.168.26.2) -> tun1;
ds[2] -> ah2 -> IPEncap(253, 192.168.37.2, 192.168.27.2) -> tun2;
ds[3] -> ah3 -> IPEncap(253, 192.168.38.2, 192.168.28.2) -> tun3;
