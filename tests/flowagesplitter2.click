require(package "multiwan")

// This test only works when debug is on. You need to see when it prints that a
// flow is deleted. Plus probably want to decrete the
// FLOWAGESPLITTER_CLEANUP_INTERVAL 20

//Creates a port where read/write handler calls can be made
ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

fas :: FlowAgeSplitter();

rs0 :: InfiniteSource(ACTIVE false);
rs1 :: InfiniteSource(ACTIVE false);
rs2 :: InfiniteSource(ACTIVE false);
rs3 :: InfiniteSource(ACTIVE false);

miph :: MarkIPHeader -> AggregateIPFlows -> fas;

rs0 -> UDPIPEncap(10.10.10.10, 10, 20.20.20.20, 20) -> miph;
rs1 -> UDPIPEncap(10.10.10.11, 10, 20.20.20.20, 20) -> miph;
rs2 -> UDPIPEncap(10.10.10.12, 10, 20.20.20.20, 20) -> miph;
rs3 -> UDPIPEncap(10.10.10.13, 10, 20.20.20.20, 20) -> miph;

fas[0] -> c0 :: Counter() -> Discard;
fas[1] -> c1 :: Counter() -> Discard;

s :: Script(
     print $(now),

     write rs0.active true,
     write rs1.active true,
     write rs2.active true,
     write rs3.active true,

     set wTime 10,

     wait $(wTime),
     write rs0.active false,
     print $(now) "stop rs0",

     wait $(wTime),
     write rs1.active false,
     print $(now) "stop rs1",

     wait $(wTime),
     write rs2.active false,
     print $(now) "stop rs2",

     wait $(wTime),
     write rs3.active false,
     print $(now) "stop rs3",

     label loop_beg,
     wait $(wTime),
     print $(now),
     goto loop_beg,
     );