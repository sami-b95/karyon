#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

int netconf(in_addr_t, in_addr_t);

int main(int argc, char **argv)
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage: netconf LOCAL_IP GATEWAY_IP\n");
		exit(1);
	}

	if(netconf(inet_addr(argv[1]), inet_addr(argv[2])))
	{
		fprintf(stderr, "failure\n");
		exit(1);
	}

	return EXIT_SUCCESS;
}
