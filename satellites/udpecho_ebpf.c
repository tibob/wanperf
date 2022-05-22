#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/udp.h>

int udpecho (struct __sk_buff * skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;
    u32 header_length = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr);
  
    /* Check if our frame is big enough to include an UDP datagram. If not, pass to the next filter */
    if (data + header_length > data_end) {
       /* The linear data is perhaps to short. Pull the non-linear data we need */
        bpf_skb_pull_data(skb, header_length);
        data = (void *)(long)skb->data;
        data_end = (void *)(long)skb->data_end;
        
        /* Still too small? This frame is not for us */
        if (data + header_length > data_end) {
            return TC_ACT_OK;
        }
    }

    /* Access the different layer headers */
    struct ethhdr *ethernet =  data;
    struct iphdr  *ip  = (data + sizeof(struct ethhdr));
    struct udphdr *udp = (data + sizeof(struct ethhdr) + sizeof(struct iphdr));   
      
    /* If this is not an IPv4 packet, return */
    if (ethernet->h_proto != __constant_htons(ETH_P_IP))
        return TC_ACT_OK;

    /* If this is not an UDP datagram, return */
    if (ip->protocol != IPPROTO_UDP)
        return TC_ACT_OK;

    /* Is this isn't an unicast frame (multicast bit is set), return */
    if ((ethernet->h_dest[0] & 0b1) == 0b1)
        return TC_ACT_OK;

#ifdef PORT
    /* Is this an UDP port we want to respond to? */
    if (udp->dest != bpf_htons(PORT))
        return TC_ACT_OK;
#endif
    
#ifdef PORTMIN
    /* Is this a port range we want to respond to? */
    int port = bpf_ntohs(udp->dest);
    if (port < PORTMIN || port > PORTMAX)
        return TC_ACT_OK;
#endif

    /* We are done with our checks.
     * Now prepare the outgoing frame.
     * No checksum recompute is needed, as the bytes in the headers do not
     * change, just their order.
     */
    
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
