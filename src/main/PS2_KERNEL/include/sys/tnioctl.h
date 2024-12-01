/* $Header: /src386/usr/include/sys/RCS/tnioctl.h,v 1.1 92/07/31 16:07:28 root Exp $ */
#ifndef	TNIOCTL_H
#define	TNIOCTL_H

#define	TNIOC	('N' << 8)
#define	TNGETA	(TNIOC|1)	/* Get node attributes */
#define	TNGETAF	(TNIOC|2)	/* Get node attributes, clear node stats */

#define	NTNST	8		/* Number of network statistics */

/*
 * Network node attributes.
 * Maintained on a per node id basis.
 * NOTE: Node id 0 is used for totals.
 */
typedef struct tnattr {
	unsigned char	host[6];	/* host id - node id in host[5] */
	unsigned char	bad;		/* non-zero if node is down */
	unsigned char	fill;		/* reserved, also for alignment */
	unsigned long	recons;		/* network reconfigurations */
	unsigned long	stats[NTNST];	/* statistics */
} tnattr_t ;

/*
 * Statistics maintained per node.
 */
#define	TnRxBYTES	0	/* # bytes received from node */
#define	TnTxBYTES	1	/* # bytes transmitted to node */
#define	TnRxPACKS	2	/* # packets received from node */
#define	TnTxPACKS	3	/* # packets transmitted to node */
#define	TnDISCARD	4	/* # packets discarded [to node] */
#define	TnSTATMOD	5	/* # status transitions on node */
#define	TnWRTDLYS	6	/* # delayed writes */
#define	TnELAPSED	7	/* elapsed time for stats in ticks */

#endif
