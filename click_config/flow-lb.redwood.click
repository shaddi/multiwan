/*
 * We want to bind each TCP flow to a single output line, but we need to
 * guarantee the same line is used for both directions of the flow. Thus, we
 * want to hash on the source/dest tuples for each packet, where the fields are
 * sorted by low_ip,high_ip,low_port,high_port. This gives us a canonical
 * representation of both sides of a flow, and if the lines are installed in
 * the same order on both sides of the tunnel then we achieve our goal of
 * binding each flow to the same line. This approach avoids the need to perform
 * any encapsulation and is fully stateless.
 *
 * In addition, we should output directly to the device (bypass the kernel).
 *
 * Unfortunately, I'm lazy, and doing the above would require writing a new
 * element. So what we do instead is run two seperate configurations for both
 * sides of our setup. On one side, we simply take a hash of the
 * source_ip/dest_ip/source_port/dest_port (the end of the IP header and the
 * beginning of the TCP header) and feed that to a hash switch, which directly
 * pushes to an output. On the other side, we first use IPMirror to reverse the
 * source/dest order at the input of the HashSwitch, and then reverse it again
 * at the output.
 *
 * We should allocate flows in a weighted fashion across lines for a fair
 * comparison...
 *
 * Also, note this file is SPECIFIC to redwood, in that it is using the
 * source/dest MAC addresses from the redwood perspective. Otherwise, this conf
 * file is host-agnostic.
 */

// REDWOOD is A, SEQUOIA is B
AddressInfo(mac_a 00:30:48:59:43:99, mac_b 00:30:48:5b:d7:13);
AddressInfo(side_a 192.168.200.0/24, side_b 192.168.100.0/24);
AddressInfo(host_tun 192.168.20.0/24);
AddressInfo(tun0 192.168.25.0/24);
AddressInfo(tun1 192.168.26.0/24);

host :: KernelTun(host_tun, DEVNAME tun_host);
tun0 :: KernelTun(tun0, DEVNAME tun0);
tun1 :: KernelTun(tun1, DEVNAME tun1);

eth1 :: Queue -> EtherEncap(0x0800, mac_a, mac_b) -> ToDevice(eth1)

rw_out1 :: IPAddrPairRewriter(pattern 192.168.25.2 192.168.35.2 0 1)
rw_out2 :: IPAddrPairRewriter(pattern 192.168.26.2 192.168.36.2 0 1)
rw_in1 :: IPAddrPairRewriter(pattern 192.168.100.100 192.168.200.100 0 1)
rw_in2 :: IPAddrPairRewriter(pattern 192.168.100.100 192.168.200.100 0 1)

// input: rewrite to tg source/dest IP (TODO: this should be a range for whole /24)
tun0 -> rw_in1 -> host
tun1 -> rw_in2 -> host

// output: rewrite to NAT'd source/dest IPs
out1 :: Null -> rw_out1 -> eth1
out2 :: Null -> rw_out2 -> eth1

rw_in1[1] -> Discard
rw_in2[1] -> Discard
rw_out1[1] -> Discard
rw_out2[1] -> Discard

switch :: HashSwitch(12,12);

// keep track of which packets need to be mirrored on each line.
ps1 :: PaintSwitch;
ps2 :: PaintSwitch;

dir_classifier :: IPClassifier( src net 192.168.100.0/24,
                                src net 192.168.200.0/24,
                                -);

host -> MarkIPHeader -> dir_classifier;

// packets are in the 200net > 100net direction, go straight to switch
dir_classifier[0] -> Paint(0) -> switch;
dir_classifier[1] -> Paint(1) -> IPMirror -> switch;
dir_classifier[2] -> Discard;

ps1[0] -> out1;
ps1[1] -> IPMirror -> out1;

ps2[0] -> out2;
ps2[1] -> IPMirror -> out2;

switch[0] -> ps1;
switch[1] -> ps2;
