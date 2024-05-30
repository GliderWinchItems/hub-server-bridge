/******************************************************************************
* File Name          : sand1.c
* Date First Issued  : 05/28/2024
* Board              : PC
* Description        : CAN msg bridging with hub-server distribution
* *************************************************************************** */
#include <syslog.h>
#include "hub-server-sock.h"
#include "hub-server-queue.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../../../bridging/PC/can-bridge-filter.h"
#include "../../../../bridging/PC/can-bridge-filter-lookup.h"

/* Local variables.
 */
static connect_t *ccb_base;				/* Save address of ccb[0] here */
static int ccb_max;						/* Save number of ccbs here */

static volatile connect_t *ccb_tmp;		/* Kludge to supress warnings */
static volatile int int_tmp;			/* Kludge to supress warnings */

static struct CBF_TABLES* pcbf; // Pointer to bridging structs & tables
static int oto = 0;
static char* pinbuf;

extern char eol;
extern FILE* fp;

int hsd_init(connect_t *first_ccb, int num_ccb)
{
	ccb_base = first_ccb;
	ccb_max = num_ccb;
	return 0;
}

int hsd_open(connect_t *ccb)
{
	ccb_tmp = ccb;
	return 0;
}

int hsd_new_in_data(connect_t *in, int size)
{
	if (oto == 0)
	{ // One time setup of CAN bridging tables
		oto = 1;
		pcbf = can_bridge_filter_init(fp);
		if (pcbf == NULL)
		{ 
			printf("ERR: can_bridge_filter_init failed\n");
			exit (1);
		}
	}	
	ccb_tmp = in;
	int_tmp = size;
	pinbuf = in->ptibs; // Beginning if input buffer
	return 0;
}

int hsd_close(connect_t *ccb)
{
	ccb_tmp = ccb;
	return 0;
}

/* ************************************************************************** */
int hsd_new_in_out_pair(connect_t *pin, connect_t *pout, int size)
/* 
This routine is entered when there is one or more complete lines from the 
connect side.

'pin' points to the control block--see 'hub-server-sock.h'
ptibs points to the beginning of the line.
size is the amount buffered, and => may include more than one line <=.
*/
{
	int ct;
	int ret;
	int n,nn;
	char* pchar;

	if ((pin->connex_num == 0) && (pout->connex_num == 0))
	{ // Here, both are listening port connections
		n = hsq_enqueue_chars(&pout->oq, pin->ptibs, size); // Copy all
		if(n != size)
		{
			syslog(LOG_INFO, "[%ld/%d,%ld/%d]hsd_new_in_out_pair -- enqueue S1 err, %d/%d\n",
				pin-ccb_base, pin->socket, pout-ccb_base, pout->socket, n, size);
		}
		return n;
	}
	// Here, one or both are client port connections
	ct = 0;
	while (size > 0)
	{
		/* Find length of one msg (one line). */
		pchar = pinbuf;
		while (*pchar != eol) pchar++;
		nn = pchar - pinbuf; // Size of line
		if (nn > 14)
		{
			/* Check if CAN ID (if valid,  and translated if necessary) should be copied to output. */
			ret = can_bridge_filter_lookup((uint8_t*)pin->ptibs, pcbf, pin->connex_num, pout->connex_num);
			if (ret != 0)
			{ // Copy msg (line) to output
				n = hsq_enqueue_chars(&pout->oq, pin->ptibs, nn );
				if (n != nn)
					syslog(LOG_INFO, "[%ld/%d,%ld/%d]hsd_new_in_out_pair -- enqueue S2 err, %d/%d\n",
						pin-ccb_base, pin->socket, pout-ccb_base, pout->socket, n, size);		
			}
		}
		else
		{ // Here, line too short to be a CAN msg
			n = hsq_enqueue_chars(&pout->oq, pin->ptibs, nn); 
			if(n != nn)
			{
				syslog(LOG_INFO, "[%ld/%d,%ld/%d]hsd_new_in_out_pair -- enqueue S3 err, %d/%d\n",
					pin-ccb_base, pin->socket, pout-ccb_base, pout->socket, nn, size);
			}				
		}
		size -= nn;
		pin->ptibs += nn;
		ct += nn;
	}
	return nn;
}
