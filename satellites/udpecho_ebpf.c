#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>

int udpecho (struct __sk_buff * skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;
    u32 header_length = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
  
    /* Check if our frame is big enought to include an UDP Datagram. If not, pass to the next filter */
   if (data + header_length > data_end) {
       /* perhaps the stored data is to short. Pull the data we need */
        bpf_skb_pull_data(skb, header_length);
        data = (void *)(long)skb->data;
        data_end = (void *)(long)skb->data_end;
        
        /* Stil to small? This frame is not for us */
        if (data + header_length > data_end) {
            return TC_ACT_OK;
        }
   }
    
    /* Access the different Layer headers */
    struct ethhdr *ethernet =  data;
    struct iphdr  *ip  = (data + sizeof(struct ethhdr));
    struct udphdr *udp = (data + sizeof(struct ethhdr) + sizeof(struct iphdr));   
      
    /* Is this an IP Packet? */
    if (ethernet->h_proto != __constant_htons(ETH_P_IP))
        return TC_ACT_OK;
    
    /* Is this an UDP Datagram? */
    if (ip->protocol != IPPROTO_UDP)
        return TC_ACT_OK;

    /* Is this an UDP Port we want to respond to? */
/*    if (*dport != DPORT) */
/*    if (udp->dest != 12345)
        return TC_ACT_UNSPEC; */

/* Filter on my source/destination IP Address, so I do not repond to everyone */
    
    /* Now prepare the outgoing frame. No checksum recompute is needed, as the bytes in the headers do noch change, just the order */
    
    /* Swap the MAC adresses */
    u8 tmp_mac[ETH_ALEN];
    memcpy(tmp_mac,            ethernet->h_source, ETH_ALEN);
    memcpy(ethernet->h_source, ethernet->h_dest,   ETH_ALEN);
    memcpy(ethernet->h_dest,   tmp_mac,            ETH_ALEN);
    
    /* Swap IP addresses */
    u32 tmp_ip = ip->saddr;
    ip->saddr = ip->daddr;
    ip->daddr = tmp_ip;
    
    /* Swap UDP ports */
    u16 tmp_port = udp->source;
    udp->source  = udp->dest;
    udp->dest    = tmp_port;
    
    /* Now redirecting the modified skb on the same interface in egress direction (BPF_F_INGRESS is not present*/
    bpf_clone_redirect(skb, skb->ifindex, 0);
    
    /* UDP Datagram has been handled, drop it an stop processing */
    return TC_ACT_SHOT;
}
