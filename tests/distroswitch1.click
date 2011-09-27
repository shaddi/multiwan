require(package "multiwan")

//Creates a port where read/write handler calls can be made
ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

ds :: DistroSwitch;

rs :: InfiniteSource(ACTIVE false, LIMIT 1000000);
rs -> cs::Counter() -> ds;

ds[0] -> c0::Counter() -> Discard;
ds[1] -> c1::Counter() -> Discard;
ds[2] -> c2::Counter() -> Discard;
ds[3] -> c3::Counter() -> Discard;

s :: Script(
     set o0 10,
     set o1 15,
     set o2 25,
     set o3 50,
     write ds.set_distribution 4 ${o0} ${o1} ${o2} ${o3},
     write rs.active true,
     label loop_beg,
     wait 1,
     goto loop_beg $(lt $(cs.count) 1000000),
     set n0 $(div $(mul $(c0.count) 100) 1000000),
     set n1 $(div $(mul $(c1.count) 100) 1000000),
     set n2 $(div $(mul $(c2.count) 100) 1000000),
     set n3 $(div $(mul $(c3.count) 100) 1000000),
     print $(n0),
     print $(n1),
     print $(n2),
     print $(n3),
     set d0 $(sub $(o0) $(n0)),
     set d1 $(sub $(o1) $(n1)),
     set d2 $(sub $(o2) $(n2)),
     set d3 $(sub $(o3) $(n3)),
     set result $(and $(le $(d0) 1) $(ge $(d0) -1)
         $(le $(d1) 1) $(ge $(d1) -1)
         $(le $(d2) 1) $(ge $(d2) -1)
         $(le $(d3) 1) $(ge $(d3) -1)),
     print "Pass test: " $(result),
     stop
     );