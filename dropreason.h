#ifndef LDSEC_TRACE_DROPREASON_H
#define LDSEC_TRACE_DROPREASON_H

/**
  * skb drop reason
  * |  <--  16Bit --> | <--  8Bit  -->  |  <-- 8Bit   -->  | 
  * ------------------------------------------
  * |        LineNo        |     Module         |     Reason          |
  * -------------------------------------------
  */
#define REASON_MASK            0xff
#define MOD_MASK               0xff00
#define LINENO_MASK            0xffff0000
#define MOD_SHIFT              8
#define LINENO_SHIFT           16

#define GET_REASON(r)          ((r) & REASON_MASK)
#define GET_MOD(r)             (((r) & MOD_MASK) >> MOD_SHIFT)
#define GET_LINENO(r)          (((r) & LINENO_MASK) >> LINENO_SHIFT)

#define drop_reason(reason, mod, lineno) \
	(((reason) & REASON_MASK) | ((mod << MOD_SHIFT) & MOD_MASK) | ((lineno << LINENO_SHIFT) & LINENO_MASK))

enum trace_drop_mod {
	MOD_NONE        = 0,
	MOD_DRIVER      = 1,       /* network card */
	MOD_BRIDGE,                /* bridge           */
	MOD_VLAN,                  /* vlan               */
	MOD_BOND,                  /* bond              */
	MOD_NEIGH,                 /* neighbour      */   
	MOD_FASTPATH,              /* fastpath        */
	
	MOD_IP_RCV,                /* ip receiving     */
	MOD_TCP,
	MOD_UDP,                
	MOD_ICMP,
	MOD_IP_XMIT,  
	MOD_IP_FWD,
	MOD_ROUTE,
	
	MOD_RULE,
	MOD_CT,
	MOD_NAT,

	_MAX_MOD            ,      /* Max module */ 
};

enum drop_code_iprcv {
	DROP_IPRCV_NONE    =  0,
		
	DROP_IPRCV_CHKSUM  =  1,
	DROP_IPRCV_LEN         ,
	DROP_IPRCV_DIFFNS      ,
	DROP_IPRCV_XFRM_SP     ,
	DROP_IPRCV_NOPROTO     ,
	DROP_IPRCV_NORT        ,
	DROP_IPRCV_OPT         ,
	DROP_IPRCV_OHOST       ,
	DROP_IPRCV_HDR         ,
	DROP_IPRCV_TRUNC       ,
	DROP_IPRCV_TRIM        ,

	_MAX_IPRCV             ,
};

const char *iprcv_arr[]    =  {
	[DROP_IPRCV_NONE]      =  "[none]",		
	[DROP_IPRCV_CHKSUM]    =  "[incorrect checksum]",
	[DROP_IPRCV_LEN]       =  "[wrong packet length]",   
	[DROP_IPRCV_DIFFNS]    =  "[different net namespace]",
	[DROP_IPRCV_XFRM_SP]   =  "[inbound xfrm policy]",
	[DROP_IPRCV_NOPROTO]   =  "[no protocol handler]",
	[DROP_IPRCV_NORT]      =  "[no route]",
	[DROP_IPRCV_OPT]       =  "[wrong IP option]",
	[DROP_IPRCV_OHOST]     =  "[pakcet type is \"PACKET_OTHERHOST\"]",
	[DROP_IPRCV_HDR]       =  "[wrong packet length when gain IP header]",
	[DROP_IPRCV_TRUNC]     =  "[packet is truncated]",
	[DROP_IPRCV_TRIM]      =  "[trim packet failed]",
	[_MAX_IPRCV]           =  "[Unknown]",
};

enum drop_code_ipfwd {
	DROP_IPFWD_NONE     =  0,
		
	DROP_FWD_LRO        =  1,
	DROP_FWD_XFRM_SP       ,
	DROP_FWD_NOTHOST       ,
	DROP_FWD_TTL0          ,
	DROP_FWD_FRAG_NEEDED   ,
	DROP_FWD_EXPAND_HEAD   ,
	DROP_FWD_SSRT          ,

	_MAX_IPFWD             ,	
};

const char *ipfwd_arr[]    =  {
	[DROP_IPFWD_NONE]      =  "[none]",		
	[DROP_FWD_LRO]         =  "[wrong LRO]",
	[DROP_FWD_XFRM_SP]     =  "[forward xfrm policy]",
	[DROP_FWD_NOTHOST]     =  "[packet type is \"PACKET_HOST\"]",
	[DROP_FWD_TTL0]        =  "[TTL exceeded]",
	[DROP_FWD_FRAG_NEEDED] =  "[need fragment but DF is set]",
	[DROP_FWD_EXPAND_HEAD] =  "[pakcet expand header failed]",
	[DROP_FWD_SSRT]        =  "[Strict routing failed]",

	[_MAX_IPFWD]           =  "[Unknown]",
};

enum drop_code_ipxmit {
	DROP_IPXMIT_NONE      =  0,
		
	DROP_IPXMIT_ALLOC_HDR =  1,
	DROP_IPXMIT_NOHH          ,
	DROP_IPXMIT_FRAG_NEED     ,
	
	_MAX_IPXMIT               ,		
};

const char *ipxmit_arr[]     =  {
	[DROP_IPXMIT_NONE]       =  "[none]",		

	[DROP_IPXMIT_ALLOC_HDR]  =  "[allocate Layer2 header failed]",
	[DROP_IPXMIT_NOHH]       =  "[no header cache and no neighbour]",
	[DROP_IPXMIT_FRAG_NEED]  =  "[handle fragment but DF is set]",

	[_MAX_IPXMIT]            =  "[Unknown]",
};


enum drop_code_RULE {
	DROP_FW_NONE            ,
	DROP_FW_POLICY       = 1,	

	_MAX_FW                 ,
};

const char *rule_arr[]   =  {
	[DROP_FW_NONE]       =  "[none]",
	[DROP_FW_POLICY]     =  "[FW deny policy]",

	[_MAX_FW]            =  "[Unknown]",	
};

enum drop_code_NAT {
	DROP_NAT_NONE           ,
	DROP_NAT_POLICY    =  1,

	_MAX_NAT                ,
};

const char *nat_arr[]   =  {
	[DROP_FW_NONE]      =  "[none]",
	[DROP_NAT_POLICY]   =  "[Nat hide inner net/host policy]",

	[_MAX_FW]           =  "[Unknown]",	
};


enum drop_code_NEIGH {
	DROP_NEIGH_NONE     =  0,
	DROP_PROBE_FAIL     =  1,
	DROP_CREAT_HH_F         ,
	DROP_LEN                ,
	DROP_CHECK              ,
	DROP_BLACKHOLE          ,
	DROP_NOPROBETIMES       ,
	DROP_QUEUE_OVER         ,
	DROP_NO_ATTATCH_DST     ,
	DROP_NEIGH_CREAT_HH_F   ,

	_MAX_NEIGH              ,
};

const char *neigh_arr[]     =  {
	[DROP_NEIGH_NONE]       =  "[none]",
	[DROP_PROBE_FAIL]       =  "[Probe failed]",
	[DROP_CREAT_HH_F]       =  "[Create Layer2 header failed]",
	[DROP_LEN]              =  "[Wrong packet length]",
	[DROP_CHECK]            =  "[check failed]",
	[DROP_BLACKHOLE]        =  "[black hole]",
	[DROP_NOPROBETIMES]     =  "[probe times is zero]",
	[DROP_QUEUE_OVER]       =  "[need resolved packet over the queue length]",
	[DROP_NO_ATTATCH_DST]   =  "[neighbour is not attached route cache]",
	[DROP_NEIGH_CREAT_HH_F] =  "[neighbour create Layer2 header failed]",

	[_MAX_NEIGH]            =  "[Unknown]",
};


enum drop_code_BRIDGE {
	DROP_BRIDGE_NONE    =  0,
	DROP_LEN_MTU        =  1,
	DROP_CP_HEADER          ,
	DROP_LRO                ,
	DROP_NOT_DELIEVER       ,
	DROP_NOP_PDISABLED      ,
	DROP_LEARNING           ,
	DROP_ERR_STATE          ,
	DROP_NOT_BRPORT         ,
	DROP_DNAT_DISFWD        ,
	DROP_DNAT_NOROUTE       ,
	DROP_DNAT_ERRROUTE      ,	

	_MAX_BRIDGE             ,
};

const char *bridge_arr[]  =  {
	[DROP_BRIDGE_NONE]    =  "[none]",
	[DROP_LEN_MTU]        =  "[MTU oversized packet]",
	[DROP_CP_HEADER]      =  "[Copy Mac Header failed]",
	[DROP_LRO]            =  "[LRO error]",
	[DROP_NOT_DELIEVER]   =  "[Cannot deliver on this port]",
	[DROP_NOP_PDISABLED]  =  "[Port is disabled or no port]",
	[DROP_LEARNING]       =  "[Port's state is \"STATE_LEARNING\"]",
	[DROP_ERR_STATE]      =  "[Port can not handle packet in this state]",
	[DROP_NOT_BRPORT]     =  "[Not Bridge port]",
	[DROP_DNAT_DISFWD]    =  "[DNAT,the device is disabled forwarding]",
	[DROP_DNAT_NOROUTE]   =  "[DNAT,no route]",
	[DROP_DNAT_ERRROUTE]  =  "[DNAT,wrong error]",

	[_MAX_BRIDGE]         =  "[Unknown]",	
};

enum drop_code_FASTPATH {
	DROP_FAST_NONE      =  0,
	DROP_FP_CREAT_HH_F  =  1,

	_MAX_FASTPATH           ,
};

const char *fastpath_arr[]   =  {
	[DROP_FAST_NONE]     =  "[none]",
	[DROP_FP_CREAT_HH_F] =  "[Create Layer2 header failed]",

	[_MAX_FASTPATH]      =  "[Unknown]",	
};


enum drop_code_VLAN {
	DROP_VLAN_NONE      =  0,
	DROP_V_CLONE_F      =  1,
	DROP_V_LEN              ,
	DROP_V_NODEV            ,
	DROP_V_EXPAND_HDR       ,
	DROP_V_ISL_LINEAR       ,
	DROP_V_ISL_NOETH        ,
	DROP_V_NONATIVE         ,
	
	_MAX_VLAN               ,
};

const char *vlan_arr[]   =  {
	[DROP_VLAN_NONE]     =  "[none]",
	[DROP_V_CLONE_F]     =  "[Packet clone failed]",
	[DROP_V_LEN]         =  "[wrong packet length]",
	[DROP_V_NODEV]       =  "[No VLAN device for this VLAN ID]",
	[DROP_V_EXPAND_HDR]  =  "[Packet Expand head buffer failed]",
	[DROP_V_ISL_LINEAR]  =  "[ISL linear failed]",
	[DROP_V_ISL_NOETH]   =  "[ISL not ethernet type]",
	[DROP_V_NONATIVE]    =  "[No native VLAN configure for untag Packet]",
	[_MAX_VLAN]          =  "[Unknown]",	
};

enum drop_code_ROUTE {
	DROP_RT_NONE        =  0,
	DROP_RT_UNREACH         ,
	DROP_RT_BUG             ,

	_MAX_ROUTE              ,
};

const char *route_arr[] =  {
	[DROP_RT_NONE]     =  "[none]",
	[DROP_RT_UNREACH]  =  "[Destination Unreachable]",
	[DROP_RT_BUG]      =  "[Route Bug]", 
	[_MAX_ROUTE]       =  "[Unknown]",
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static struct dp_mod_str_t {
	const char *modstr;
	const char **reason_arr;
	int count;
} dp_mod_str[] = {
	[MOD_NONE]       = { 
		.modstr      = "[none]",
		.reason_arr  = NULL, 
		.count = 0,
		},
	[MOD_DRIVER]	 =	{
		.modstr      = "[NIC driver]",
		.reason_arr  = NULL, 
		.count = 0,	
		},
	[MOD_BRIDGE]	 =	{
		.modstr      = "[Bridge]",
		.reason_arr  = (const char **)bridge_arr, 
		.count = ARRAY_SIZE(bridge_arr),	
		},
	[MOD_VLAN]		 =	{
		.modstr      = "[VLAN]",
		.reason_arr  = (const char **)vlan_arr, 
		.count = ARRAY_SIZE(vlan_arr),	
		},
	[MOD_BOND]		 =	{
		.modstr      = "[Bond]",
		.reason_arr  = NULL, 
		.count = 0,	
		},
	[MOD_NEIGH] 	 =	{
		.modstr      = "[Neigh/ARP/NS]",
		.reason_arr  = (const char **)neigh_arr, 
		.count = ARRAY_SIZE(neigh_arr),
		},
	[MOD_FASTPATH]	 =	{
		.modstr      = "[Fastpath]",
		.reason_arr  = (const char **)fastpath_arr, 
		.count = ARRAY_SIZE(fastpath_arr),	
		},
	[MOD_IP_RCV]	 =	{
		.modstr      = "[IPv4 Rcv]",
		.reason_arr  = (const char **)iprcv_arr, 
		.count = ARRAY_SIZE(iprcv_arr),	
		},
	[MOD_TCP]		 =	{
		.modstr      = "[TCP]",
		.reason_arr  = NULL, 
		.count = 0,	
		}, 
	[MOD_UDP]		 =	{
		.modstr      = "[UDP]",
		.reason_arr  = NULL, 
		.count = 0,	
		},
	[MOD_ICMP]		 =	{
		.modstr      = "[ICMP]",
		.reason_arr  = NULL, 
		.count = 0,	
		},
	[MOD_IP_XMIT]	 =	{
		.modstr      = "[IPv4 Xmit]",
		.reason_arr  = (const char **)ipxmit_arr, 
		.count = ARRAY_SIZE(ipxmit_arr),	
		},
	[MOD_IP_FWD]	 =	{
		.modstr      = "[IPv4 Forward]",
		.reason_arr  = (const char **)ipfwd_arr, 
		.count = ARRAY_SIZE(ipfwd_arr),	
		},
	[MOD_ROUTE] 	 =	{
		.modstr      = "[Route]",
		.reason_arr  = (const char **)route_arr, 
		.count = ARRAY_SIZE(route_arr),	
		},
	[MOD_RULE]		 =	{
		.modstr      = "[Rule]",
		.reason_arr  = (const char **)rule_arr, 
		.count = ARRAY_SIZE(rule_arr),	
		},
	[MOD_CT]		 =	{
		.modstr      = "[Conntrack]",
		.reason_arr  = NULL, 
		.count = 0,	
		},
	[MOD_NAT]		 =	{
		.modstr      = "[NAT]",
		.reason_arr  = (const char **)nat_arr, 
		.count = ARRAY_SIZE(nat_arr),	
		},
		
	[_MAX_MOD]		 =	{
		.modstr      = "[unknown]",
		.reason_arr  = NULL, 
		.count = 0,	
		}, 
};

#endif /* LDSEC_TRACE_DROPREASON_H  */

