/******************************************************************************
* File Name          : sand1.c
* Date First Issued  : 05/28/2024
* Board              : PC
* Description        : CAN msg bridging with hub-server distribution
* *************************************************************************** */
#include <syslog.h>
#include "hub-server-sock.h"
#include "hub-server-queue.h"
#include "hub-server-util.h"

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
extern FILE* fpS;

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
	/* Upon first data (which means client sockets have been 
	   connected, init bridging tables. */
	if (oto == 0)
	{ // One time setup of CAN bridging tables
		oto = 1;
		if (fpS == NULL)
		{
			printf("ERR: bridging file pointer is NULL. Is command line --file <bridge file> missing?\n");
			hs_exit(12);
		}
		/* If fpS is NULL segmentation error. */
		pcbf = can_bridge_filter_init(fpS);
		if (pcbf == NULL)
		{ 
			printf("ERR: can_bridge_filter_init failed\n");
			exit (1);
		}
		/* The connection count has to match the bridging file.
What happens when there has been no listening socket connection? */		
		int ctr = 1;
		for (int i = 0; i < ccb_max; i++)
		{
			if ((ccb_base+i)->connex_num !=  0)
				ctr += 1;
printf("%2i socket %i, connex_num %i\n",i+1,(ccb_base+i)->socket,(ccb_base+i)->connex_num);
		}
		if (pcbf->n != ctr)
		{
			printf("\nERR: bridge file matrix size \"N\" is %i and,\n"
			         " it does not match hub-server connections %i\n",pcbf->n, ctr);
			printf(  " Command line client connection count (plus one) should match bridge file size\n");
			hs_exit(13);			
		}

	}	
	ccb_tmp = in;
	int_tmp = size;
	pinbuf = in->ptibs; // Beginning of input buffer
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
	char* pinwk = pin->ptibs; // Working pinbuf

printf("A: pin->connex_num %i pout->connex_num %i size %i\n",pin->connex_num, pout->connex_num, size);
	if ((pin->connex_num == 0) && (pout->connex_num == 0))
	{ // Here, both are listening port connections
printf("LtoL\n");
		n = hsq_enqueue_chars(&pout->oq, pinwk, size); // Copy all
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
		/* Find length of what might be(!) one CAN msg (one line). */
		pchar = pinwk;
		while (*pchar != eol) pchar++;
		nn = pchar - pinbuf + 1; // Size of line
printf("B: pin->connex_num %i pout->connex_num %i size %i\n",pin->connex_num, pout->connex_num, nn);		
		if ((nn > 14) && (nn < 32))
		{ // Here, goldilocks: not too short, not too long
			/* Check if CAN ID (if valid,  and translated if necessary) should be copied to output. */
			ret = can_bridge_filter_lookup((uint8_t*)pinwk, pcbf, pin->connex_num, pout->connex_num);
			if (ret != 0)
			{ // Copy msg (line) to output
				n = hsq_enqueue_chars(&pout->oq, pinwk, size); 
				if (n != nn)
					syslog(LOG_INFO, "[%ld/%d,%ld/%d]hsd_new_in_out_pair -- enqueue S2 err, %d/%d\n",
						pin-ccb_base, pin->socket, pout-ccb_base, pout->socket, n, size);		
			}
		}
		else
		{ // Here, line too short or too long to be a CAN msg
			n = hsq_enqueue_chars(&pout->oq, pinwk, size); 
			if(n != nn)
			{
				syslog(LOG_INFO, "[%ld/%d,%ld/%d]hsd_new_in_out_pair -- enqueue S3 err, %d/%d\n",
					pin-ccb_base, pin->socket, pout-ccb_base, pout->socket, nn, size);
			}				
		}
		size -= n;
		pinwk += n;
		ct += n;
	}
	return nn;
}
