require(package "multiwan")

fs :: FlareSwitch(START_MTBS 0);

miph :: MarkIPHeader -> SetTimestamp -> fs;

i00::InfiniteSource() -> UDPIPEncap(10.10.10.00, 10, 20.20.20.20, 20) -> miph;
i10::InfiniteSource() -> UDPIPEncap(10.10.10.10, 10, 20.20.20.20, 20) -> miph;
i20::InfiniteSource() -> UDPIPEncap(10.10.10.20, 10, 20.20.20.20, 20) -> miph;
i30::InfiniteSource() -> UDPIPEncap(10.10.10.30, 10, 20.20.20.20, 20) -> miph;
i40::InfiniteSource() -> UDPIPEncap(10.10.10.40, 10, 20.20.20.20, 20) -> miph;
i50::InfiniteSource() -> UDPIPEncap(10.10.10.50, 10, 20.20.20.20, 20) -> miph;
i60::InfiniteSource() -> UDPIPEncap(10.10.10.60, 10, 20.20.20.20, 20) -> miph;
i70::InfiniteSource() -> UDPIPEncap(10.10.10.70, 10, 20.20.20.20, 20) -> miph;
i80::InfiniteSource() -> UDPIPEncap(10.10.10.80, 10, 20.20.20.20, 20) -> miph;
i90::InfiniteSource() -> UDPIPEncap(10.10.10.90, 10, 20.20.20.20, 20) -> miph;

i01::InfiniteSource() -> UDPIPEncap(10.10.10.01, 10, 20.20.20.20, 20) -> miph;
i11::InfiniteSource() -> UDPIPEncap(10.10.10.11, 10, 20.20.20.20, 20) -> miph;
i21::InfiniteSource() -> UDPIPEncap(10.10.10.21, 10, 20.20.20.20, 20) -> miph;
i31::InfiniteSource() -> UDPIPEncap(10.10.10.31, 10, 20.20.20.20, 20) -> miph;
i41::InfiniteSource() -> UDPIPEncap(10.10.10.41, 10, 20.20.20.20, 20) -> miph;
i51::InfiniteSource() -> UDPIPEncap(10.10.10.51, 10, 20.20.20.20, 20) -> miph;
i61::InfiniteSource() -> UDPIPEncap(10.10.10.61, 10, 20.20.20.20, 20) -> miph;
i71::InfiniteSource() -> UDPIPEncap(10.10.10.71, 10, 20.20.20.20, 20) -> miph;
i81::InfiniteSource() -> UDPIPEncap(10.10.10.81, 10, 20.20.20.20, 20) -> miph;
i91::InfiniteSource() -> UDPIPEncap(10.10.10.91, 10, 20.20.20.20, 20) -> miph;

fs[0] -> Discard;
fs[1] -> Discard;
fs[2] -> Discard;
fs[3] -> Discard;

s :: Script(
     wait 5,
     write i00.active false,
     print "i00 deactive============================================",
     wait 1,
     write i10.active false,
     print "i10 deactive===========================================",
     wait 1,
     write i20.active false,
     print "i20 deactive===========================================",
     wait 1,
     write i30.active false,
     print "i30 deactive===========================================",
     wait 1,
     write i40.active false,
     print "i40 deactive===========================================",
     wait 1,
     write i50.active false,
     print "i50 deactive===========================================",
     wait 1,
     write i60.active false,
     print "i60 deactive===========================================",
     wait 1,
     write i70.active false,
     print "i70 deactive===========================================",
     wait 1,
     write i80.active false,
     print "i80 deactive===========================================",
     wait 1,
     write i90.active false,
     print "i90 deactive===========================================",
     print "Wait 2 minutes",
     wait 120,
     stop
     );