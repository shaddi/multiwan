require(package "multiwan")

//Creates a port where read/write handler calls can be made
ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

fas :: FlowAgeSplitter();

rs0 :: InfiniteSource(ACTIVE false, LIMIT 100);
rs1 :: InfiniteSource(ACTIVE false, LIMIT 100);
rs2 :: InfiniteSource(ACTIVE false, LIMIT 100);
rs3 :: InfiniteSource(ACTIVE false, LIMIT 100);

miph :: MarkIPHeader -> AggregateIPFlows -> fas;

rs0 -> UDPIPEncap(10.10.10.10, 10, 20.20.20.20, 20) -> miph;
rs1 -> UDPIPEncap(10.10.10.11, 10, 20.20.20.20, 20) -> miph;
rs2 -> UDPIPEncap(10.10.10.12, 10, 20.20.20.20, 20) -> miph;
rs3 -> UDPIPEncap(10.10.10.13, 10, 20.20.20.20, 20) -> miph;

fas[0] -> c0 :: Counter() -> Discard;
fas[1] -> c1 :: Counter() -> Discard;

s :: Script(
     write rs0.active true,
     write rs1.active true,
     write rs2.active true,
     write rs3.active true,
     label loop_beg, wait .1,
     goto loop_beg $(lt $(c0.count) 400),

     write rs0.active false,
     write rs1.active false,
     write rs2.active false,
     write rs3.active false,

     write rs0.reset,
     write rs1.reset,
     write rs2.reset,
     write rs3.reset,

     write fas.bump_flows 2,

     write rs0.active true,
     write rs1.active true,
     write rs2.active true,
     write rs3.active true,
     label loop_beg2, wait .1,
     goto loop_beg2 $(lt $(add $(c0.count) $(c1.count)) 800),

     print $(c0.count) $(c1.count),
     print "Pass test: " $(and $(eq $(c0.count) 600) $(eq $(c1.count) 200)),

     stop
     );