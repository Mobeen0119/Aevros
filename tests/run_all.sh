#!/usr/bin/env bash
set -e
S="Networking/NetWatch/net_watch.c Networking/NetWatch/net_observe.c Networking/NetTrace/net_trace.c Provenance/provenance.c"
C="gcc -DAEVROS_HOST_TEST -Wall -Wextra"

echo "##### provenance core";  $C tests/prov_test.c tests/provenance_host_stubs.c Provenance/provenance.c -o /tmp/t1 && /tmp/t1
echo; echo "##### trace order, cycles, wrap";  $C tests/prov_trace_test.c tests/provenance_host_stubs.c Provenance/provenance.c -o /tmp/t2 && /tmp/t2
echo; echo "##### frame -> netwatch -> provenance"; $C tests/prov_net_test.c $S tests/provenance_host_stubs.c -o /tmp/t3 && /tmp/t3
echo; echo "###### NetTrace request/reply -> provenance"; $C tests/prov_pair_test.c $S tests/provenance_host_stubs.c -o /tmp/t5 && /tmp/t5
echo; echo "##### postbox: queue wrap and DHCP-sized datagrams"; $C -fsanitize=address,undefined tests/postbox_test.c Networking/IPv4/Postbox/postbox.c -o /tmp/t6 && /tmp/t6
echo; echo "##### interrupt-path stack depth (4 KB task stacks)"; python3 tests/stack_depth.py
echo; echo "##### foyer: hold until the MAC is known, then send with it filled in"; $C -fsanitize=address,undefined tests/foyer_test.c Networking/Foyer/foyer.c -o /tmp/t7 2>/dev/null && /tmp/t7
echo; echo "#### DEMO: the provenance command"; $C tests/prov_demo.c $S Provenance/provenance_report.c tests/host_console_stubs.c tests/provenance_host_stubs.c -o /tmp/t4 && /tmp/t4
echo; echo "ALL DONE"