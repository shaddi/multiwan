require(package "multiwan")

//Creates a port where read/write handler calls can be made
//ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

fs :: FlareSwitch(START_MTBS 50);

ccd1 :: CalcCongestionDelta(OFFSET 0);
ccd2 :: CalcCongestionDelta(OFFSET 0);
cld :: CalcLatencyDelta(OFFSET 0, MAX_PAINT 2);
ah1 :: AddMWanHeader3(CALC_CONGESTION_DELTA ccd1, CALC_LATENCY_DELTA cld);
ah2 :: AddMWanHeader3(CALC_CONGESTION_DELTA ccd2, CALC_LATENCY_DELTA cld);
ph :: ProcessMWanHeader3(FLARESWITCH fs, MAX_PAINT 2, UPDATE_INT 100,
      DISTRIB_TOTAL 100, DISTRIB_INC 10, DISTRIB_MIN 5) -> Discard;

rs1 :: InfiniteSource(ACTIVE false, LIMIT 1);
rs2 :: InfiniteSource(ACTIVE false, LIMIT 1);
//rsd :: InfiniteSource(ACTIVE false, LIMIT 1);
rsd :: InfiniteSource();

rs1 -> SetTimestamp -> ah1 -> Queue -> duq1::DelayUnqueue(.05) -> ccd1 -> Paint(0) -> SetTimestamp -> c1::Counter() -> cld -> ph;

rs2 -> SetTimestamp -> ah2 -> Queue -> duq2::DelayUnqueue(.05) -> ccd2 -> Paint(1) -> SetTimestamp -> cld -> ph;


rsd -> MarkIPHeader -> SetTimestamp -> fs;
fs[0] -> cd0::Counter() -> Discard;
fs[1] -> cd1::Counter() -> Discard;

// s :: Script(
//      wait 1,
//      print "ACTIVATING!",
//      write rsd.active true,
//      wait 1,
//      stop
//      );


rateS1 :: Script(
          set wTime .1,
          set dTime .05,
          label loop_beg,
          write rs1.active true,
//          print "rs1 Send packet",
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
//          print "rs1 Send packet",
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

rateS2 :: Script(
          set wTime .1,
          set dTime .05,
          label loop_beg,
          write rs2.active true,
//          print "rs2 Send packet",
          wait $(wTime),
          write rs2.active false,
          write rs2.reset,
          goto loop_beg $(lt $(c1.count) 20),

          print "RateS2 Increasing",

          label loop_beg2,
          write rs2.active true,
//          print "rs2 Send packet",
          wait $(wTime),
          write rs2.active false,
          write rs2.reset,
          set d $(if $(le $(c1.count) 5) 0 0.05),
          set wTime $(add $(wTime) $(d)),
          set dTime $(add $(dTime) $(d)),
//          print "------ " $(wTime) " " $(dTime),
          write duq2.delay $(dTime),
          goto loop_beg2 $(lt $(c1.count) 75),
          );

// rateSD :: Script(
//           label loop_beg,
//           write rsd.active true,
//           wait .01,
//           write rsd.active false,,
//           write rsd.reset,
//           goto loop_beg
//           );

rateC :: Script(
         label loop_beg,
         wait .25,
         set a $(ccd1.get_congestion_score),
         set b $(ccd2.get_congestion_score),
         print $(cd0.count) $(cd1.count)  $(a) $(b),
         write cd0.reset,
         write cd1.reset,
         goto loop_beg
);