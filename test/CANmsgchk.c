/******************************************************************************
* File Name          : CANmsgchk.c
* Date First Issued  : 06/03/2024
* Board              : Linux PC
* Description        : Check ascii/hex format CAN msgs for validity
*******************************************************************************/
/*
gcc CANmsgchk.c -o CANmsgchk -lm && cat <file> | ./CANmsgchk
gcc CANmsgchk.c -o CANmsgchk && cat CANmsgchk-test.txt | ./CANmsgchk
date; gcc CANmsgchk.c -o CANmsgchk && cat /media/deh/*Seagate/CANlog/CAN0_20240513_1435 | ./CANmsgchk;date

*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#define CHECKSUM_INITIAL	0xa5a5	// Initial value for computing checksum

void printbefore(char* pbw, char* pbs, char* pbe);

/* Line buffer size */
#define BSIZE 16
#define LINESIZE 128
char buf[BSIZE][LINESIZE];

/* Lookup table to convert one hex char to binary (4 bits), no checking for illegal incoming hex */
static const int8_t h[256] =
{
/*   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F  */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  0 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  1 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  2 */
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /*  3 */
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  4 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  5 */
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  6 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  7 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  8 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  9 */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  A */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  B */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  C */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  D */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  E */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  /*  F*/
};


/* ************************************************************************************************************ */
/*  Yes, this is where it starts.                                                                               */
/* ************************************************************************************************************ */
int main(int argc, char **argv)
{
	uint32_t linect = 0;
	uint64_t charct = 0;
	uint32_t id;
	uint32_t chkx; 
	uint8_t idx;
	uint8_t ret8;
	int8_t nibhi,niblo;
	uint8_t bintmp;
	int n;
	char* pbs = &buf[0][0]; // Start of circular line buffers
	char* pbe = &buf[BSIZE][LINESIZE]; // End
	char* pbw = pbs; // Working pointer

	while ( (fgets (pbw,LINESIZE,stdin)) != NULL)	// Get a line from stdin
	{
		linect += 1;
		n = strlen(pbw);
		charct += n;
		n -= 1; // No need to deal with '\n
		if ((n > 13) && (n < 31))
		{ // Here, goldilocks: not to big and not too small
			if ((n & 0x1) != 0)
			{ // Here, msg bytes should be even
				printf("%10li %6i Line len not even: %i %s\n",charct,linect, n, pbw);
				continue;
			}
			// Check for checksum and non-hex chars
			chkx = CHECKSUM_INITIAL;
			for (int j = 0; j < n; j += 2 )
			{
				nibhi = h[*(pbw+j)];
				if (nibhi < 0)
				{
					printf("%10li %6i ",charct, linect);
					printf(" Not hex nibhi: %d  %c %d %s\n", nibhi, *(pbw+j), h[*(pbw+j)], pbw);
					break;
				}

				niblo = h[*(pbw+j+1)];
				if (niblo < 0)
				{
					printf("%6i ",linect);
					printf(" Not hex niblo: %d %d %d %s\n", niblo, *(pbw+j+1), h[*(pbw+j+1)], pbw);
					break;
				}
				bintmp= ((nibhi << 4) | niblo);
				if (j < (n-2))
					chkx += ((nibhi << 4) | niblo);
			}
			// Complete checksum generation
			chkx += (chkx >> 16); // Add carries into high half word
			chkx += (chkx >> 16); // Add carry if previous add generated a carry
			chkx += (chkx >> 8);  // Add high byte of low half word
			chkx += (chkx >> 8);  // Add carry if previous add generated a carry
			if ((chkx & 0xFF) != bintmp)
			{ // Here, checksums do not match
				printf("%10li %6i ",charct, linect);
				printf("checksum err: %02X: %02X ",(chkx & 0xFF), bintmp);
				printf( "%s\n",pbw);
			}
		}
		else
		{ // Here we have a bogus line
			printf("%10li %6i ",charct, linect);
			printf("Line length: %i: %s",n+1, pbw);
			printbefore(pbw, pbs, pbe);
		}
		pbw += LINESIZE;
		if (pbw >= pbe) pbw = pbs;
	}
	printf("%10li %6i ---END---\n",charct, linect);
	return 0;
}
/* **************************************************************************************
 * void printbefore(char* pbw, char* pbs, char* pbe);
 * @brief	: Print the previous line
 * @param	: pbw = circular buffer current position
 * @param	: pbs = circular buffer start of array
 * @param	: pbe = circular buffer end+1 of array
 * ************************************************************************************** */
void printbefore(char* pbw, char* pbs, char* pbe)
{
	/* Back up pointer one line in circular buffer. */
	char* pb4 = pbw;
	if (pb4 == pbs)
		pb4 = pbe;
	pb4 -= LINESIZE;
	printf("prev line: %s",pb4);
	return;
}