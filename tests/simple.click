//Creates a port where read/write handler calls can be made
ControlSocket("UNIX", /tmp/clickconstrolsocket);

//ChatterSocket("UNIX", simple.chatter);

rs :: RatedSource(RATE 2);
rs -> MarkIPHeader -> IPPrint(PKT, LENGTH true, CONTENTS HEX) -> Discard;

s :: Script(
     label begin_loop,
     wait 1,
     print "RatedSource.count =" $(rs.count),
     goto begin_loop $(le $(rs.count) 10),
     stop
     );