require(package "multiwan")

//Creates a port where read/write handler calls can be made
//ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

ccd1 :: CalcCongestionDelta(OFFSET 0);
ccd2 :: CalcCongestionDelta(OFFSET 0);
ah1 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd1);
ah2 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd2);
cld :: CalcLatencyDelta(OFFSET 0, MAX_PAINT 2) -> Discard;

rs1 :: InfiniteSource(ACTIVE false, LIMIT 1);
rs2 :: InfiniteSource(ACTIVE false, LIMIT 1);

rs1 -> SetTimestamp -> ah1 -> ccd1 -> Print(line1A, TIMESTAMP true) -> Queue -> DelayUnqueue(.1) -> Paint(0) -> SetTimestamp -> Print(line1B, TIMESTAMP true) -> cld;
rs2 -> SetTimestamp -> ah2 -> ccd2 -> Print(line2A, TIMESTAMP true) -> Queue -> DelayUnqueue(.15) -> Paint(1) -> SetTimestamp -> Print(line2B, TIMESTAMP true) -> cld;

s :: Script(
     set sum 0,
     set c 0,
     label loop_beg,
     write rs1.active true,
     write rs2.active true,
     wait .2,
     write rs1.active false,
     write rs2.active false,
     write rs1.reset,
     write rs2.reset,
     set c $(add $(c) 1),
     print $(div $(cld.get_max_delta) 1000000),
     set sum $(add $(sum) $(cld.get_max_delta)),
     goto loop_beg $(lt $(c) 20),
     print $(div $(div $(sum) 1000000) 20),
     print "Pass test: " $(eq $(div $(div $(sum) 1000000) 20) 50),

     stop
     );