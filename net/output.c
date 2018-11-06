
#include "ns.h"

extern union Nsipc nsipcbuf;

    void
output(envid_t ns_envid)
{
    binaryname = "ns_output";


    int r;

    while (1) {
        int32_t req, whom;
        req = ipc_recv(&whom, &nsipcbuf, NULL);
        assert(whom == ns_envid);
        assert(req == NSREQ_OUTPUT);
        if ((r = sys_net_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0)
            cprintf("Failed to transmit packet: %e\n", r);
    }

}
