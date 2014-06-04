/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */


#include <sys/types.h>
#include <sys/sysctl.h>
#include <iostream>
#include <sstream>

//#include <Platform/osx/OpenPorts.hpp>

/**
 * Return the network port
 */
int getPort(int port) {
	return ntohs((u_short)port);
}

/*
 * Pretty print the internet address to a string
 */
std::string getHost(struct in_addr *inp) {
	std::ostringstream oss;

	inp->s_addr = ntohl(inp->s_addr);
	
	oss << ((u_int)((inp->s_addr >> 24) & 0xff)) << ".";
	oss << ((u_int)((inp->s_addr >> 16) & 0xff)) << ".";
	oss << ((u_int)((inp->s_addr >> 8) & 0xff)) << ".";
	oss << ((u_int)((inp->s_addr) & 0xff));

	return oss.str();
}

/**
 * Platform-dependent function to list listening ports
 */
std::vector< std::pair< std::string, int > > getListeningPorts() {
	size_t len = 0;
	struct tcpcb *tp = NULL;
	struct inpcb *inp;
	struct xinpgen *xig, *oxig;
	struct xsocket *so;
	char *buf = NULL;

	// Prepare response buffers
	std::vector< std::pair< std::string, int > > ans;

	// Probe to get length
	if (sysctlbyname("net.inet.tcp.pcblist", 0, &len, 0, 0) < 0) {
	    perror("sysctlbyname");
	    return ans;
	}

	// Get buffer
    buf = (char*)malloc(len);
    sysctlbyname("net.inet.tcp.pcblist", buf, &len, 0, 0);

    // Bail-out to avoid logic error in the loop below when
    // there is in fact no more control block to process
    if (len <= sizeof(struct xinpgen)) {
        free(buf);
        return ans;
    }

    // Process entries in the table
	oxig = xig = (struct xinpgen *)buf;
	for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
	     xig->xig_len > sizeof(struct xinpgen);
	     xig = (struct xinpgen *)((char *)xig + xig->xig_len)) {

		// We are doing TCP trace
		tp = &((struct xtcpcb *)xig)->xt_tp;
		inp = &((struct xtcpcb *)xig)->xt_inp;
		so = &((struct xtcpcb *)xig)->xt_socket;

		/* Ignore PCBs which were freed during copyout. */
		if (inp->inp_gencnt > oxig->xig_gen)
			continue;

		/* Use only IP4 */
		if ((inp->inp_vflag & INP_IPV4) == 0)
		    continue;

		/* Keep only linstening ports */
        if (tp->t_state != TCPS_LISTEN)
            continue;

        /* Put pair in the answer stack */
        ans.push_back( 
        	std::make_pair(
        		getHost(&inp->inp_laddr), 
        		getPort((int)inp->inp_lport)
        	)
        );

	}

	// Release buffer
	free(buf);

	// Return ans
	return ans;

}