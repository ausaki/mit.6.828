#include "ns.h"

#define MAXTRY 10

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t from_env;
	int perm, r, ntry;
	while(1){
		if((r = ipc_recv(&from_env, &nsipcbuf, &perm)) < 0 || r != NSREQ_OUTPUT){
			continue;
		}
		ntry = 0;
		while((r = sys_net_tx_packet(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) == -E_NETWORK_NO_BUF && ntry < MAXTRY){
			sys_yield();
			ntry++;
		}
	}
}
