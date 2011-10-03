require(package "multiwan")

//Creates a port where read/write handler calls can be made
//ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

ccd :: CalcCongestionDelta(OFFSET 0);
ah :: AddMWanHeader2(CALC_CONGESTION_DELTA ccd);

rs :: InfiniteSource(ACTIVE false, LIMIT 1);
rs -> SetTimestamp -> ah -> SetTimestamp -> ccd -> Discard;
//rs -> SetTimestamp -> ah -> Queue -> duq :: DelayUnqueue(1);
//duq -> SetTimestamp -> ccd -> cs::Counter() -> Discard;

s :: Script(
     set x 0,
     label loop_beg,
     write rs.active true,
     wait .1,
     write rs.active false,
     write rs.reset,
     set x $(add $(x) 1),
     goto loop_beg $(lt $(x) 17),
     print $(ccd.get_congestion_score),
     print "Pass test: " $(eq $(ccd.get_congestion_score) "0"),

     stop,

     write rs.active false, write rs.limit 1, write rs.reset,
     write cs.reset,
     write duq.delay .5,
     write rs.active true,
     label l1, wait .1,
     //print "a",
     goto l1 $(lt $(cs.count) 1),
     
     write rs.active false, write rs.limit 1, write rs.reset,
     write cs.reset,
     write duq.delay 1,
     write rs.active true,
     label l2, wait .1,
     //print "b",
     goto l2 $(lt $(cs.count) 1),

     write rs.active false, write rs.limit 1, write rs.reset,
     write cs.reset,
     write duq.delay .5,
     write rs.active true,
     label l3, wait .1,
     //print "c",
     goto l3 $(lt $(cs.count) 1),

     print $(ccd.get_congestion_score),
     stop
     );