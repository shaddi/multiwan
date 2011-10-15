require(package "multiwan")

// What this does:
// The line ProcessMWanHeader is listening to is slowly made more and more congested
// and then decreased to no congestion. It in turn will bump both of the flows to
// go out of FlowAgeSplitter's port 1 instead of port 0.
// All random unexplained stuff is to make things work and not complain.

//Creates a port where read/write handler calls can be made
//ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

ds :: DistroSwitch;
miph :: MarkIPHeader -> AggregateIPFlows -> fas :: FlowAgeSplitter;

ccd1 :: CalcCongestionDelta(OFFSET 0);
ah1 :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd1);
ph :: ProcessMWanHeader2(DISTROSWITCH ds, MAX_PAINT 1, UPDATE_INT 500,
      DISTRIB_TOTAL 100, DISTRIB_INC 10, DISTRIB_MIN 5, FLOWAGESPLITTER fas,
      FLOWSPLIT_THRESHOLD 8) -> Discard;

rs1 :: InfiniteSource(ACTIVE false, LIMIT 1);

rs2 :: InfiniteSource();
rs3 :: InfiniteSource();

rs1 -> SetTimestamp -> ah1 -> Queue -> duq1::DelayUnqueue(.05) -> ccd1 -> Paint(0) -> SetTimestamp -> c1::Counter() -> ph;

rs2 -> UDPIPEncap(10.10.10.2, 10, 20.20.20.20, 20) -> miph;
rs3 -> UDPIPEncap(10.10.10.3, 10, 20.20.20.20, 20) -> miph;

fas[0] -> cf0::Counter() -> ds -> Discard;
fas[1] -> cf1::Counter() -> Discard;

rateS1 :: Script(
          set wTime .1,
          set dTime .05,
          label loop_beg,
          write rs1.active true,
          wait $(wTime),
          write rs1.active false,
          write rs1.reset,
          set d $(if $(le $(c1.count) 5) 0 0.05),
          set wTime $(add $(wTime) $(d)),
          set dTime $(add $(dTime) $(d)),
          write duq1.delay $(dTime),
          goto loop_beg $(lt $(c1.count) 20),

          print "RateS1 Decreasing",

          label loop_beg2,
          write rs1.active true,
          wait $(wTime),
          write rs1.active false,
          write rs1.reset,
          set d $(if $(le $(dTime) 0) 0.0 0.05),
          set wTime $(sub $(wTime) $(d)),
          set dTime $(sub $(dTime) $(d)),
//          print "------ " $(d) " " $(wTime) " " $(dTime),
          write duq1.delay $(dTime),
          goto loop_beg2 $(lt $(c1.count) 75),
          stop
          );

rateC :: Script(
         label loop_beg,
         wait .25,
         set a $(ccd1.get_congestion_score),
         print $(cf0.count) $(cf1.count)  $(a),
         write cf0.reset,
         write cf1.reset,
         goto loop_beg
);