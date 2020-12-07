#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t from_env;
	int perm, r;
	while(1){
		if((r = ipc_recv(&from_env, &nsipcbuf, &perm)) < 0 || r != NSREQ_OUTPUT){
			continue;
		}
		if((r = sys_net_tx_packet(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) < 0){
			;
		}
	}
	
}
