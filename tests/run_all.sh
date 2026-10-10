#!/usr/bin/env bash
set -e
S="Networking/NetWatch/net_watch.c Networking/NetWatch/net_observe.c Provenance/provenance.c"
C="gcc -DAEVROS_HOST_TEST -Wall -Wextra"

echo "##### provenance core";  $C tests/prov_test.c tests/provenance_host_stubs.c Provenance/provenance.c -o /tmp/t1 && /tmp/t1
echo; echo "##### trace order, cycles, wrap";  $C tests/prov_trace_test.c tests/provenance_host_stubs.c Provenance/provenance.c -o /tmp/t2 && /tmp/t2
echo; echo "##### frame -> netwatch -> provenance"; $C tests/prov_net_test.c $S tests/provenance_host_stubs.c -o /tmp/t3 && /tmp/t3
echo; echo "#### DEMO: the provenance command"; $C tests/prov_demo.c $S Provenance/provenance_report.c tests/host_console_stubs.c tests/provenance_host_stubs.c -o /tmp/t4 && /tmp/t4
echo; echo "ALL DONE"