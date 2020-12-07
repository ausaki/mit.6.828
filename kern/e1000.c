#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000_regs;

struct tx_desc tx_desc_arr[TDLEN];
uint32_t tdt;

struct rx_desc rx_desc_arr[RDLEN];
uint32_t rdt;

void test_tx_packet(void);

int e1000_attach(struct pci_func *pcif){
    pci_func_enable(pcif);
    uint32_t base = pcif->reg_base[0];
    uint32_t size = pcif->reg_size[0];

    e1000_regs = mmio_map_region(base, size);
    cprintf("E1000 device status register 0: %08x\n", e1000_regs[E1000_STATUS]);
    e1000_init_tx();
    e1000_init_rx();
    return 0;
}

/*
 * see 14.5  
 */
void e1000_init_tx(void){
    struct PageInfo *pp;
    int i, j, m ;
    m = PGSIZE / TDBUFLEN;
    tdt = 0;
    
    memset(tx_desc_arr, 0, sizeof(tx_desc_arr));

    cprintf("e1000_init_tx: tx_desc_arr: 0x%08x\n", tx_desc_arr);

    for(i = 0, j = m; i < TDLEN; i++){
        if(j == m){
            if((pp = page_alloc(1)) == NULL){
                panic("e1000_init: no memory");
            }
            pp->pp_ref++;
            j = 0;
        }
        tx_desc_arr[i].addr = page2pa(pp) + j * TDBUFLEN;
        tx_desc_arr[i].status |= TXD_STA_DD;
        tx_desc_arr[i].length = 0;
        tx_desc_arr[i].cmd |= TXD_CMD_RS;
        j++;
    }

    e1000_regs[E1000_TDLEN] = TDLEN * sizeof(struct tx_desc);
    e1000_regs[E1000_TDBAL] = PADDR(tx_desc_arr);
    e1000_regs[E1000_TDBAH] = 0x0;
    e1000_regs[E1000_TDH] = 0;
    e1000_regs[E1000_TDT] = 0;
    
    e1000_regs[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP | TCTL_COLD;
    e1000_regs[E1000_TIPG] = TIPG_VAL;


    // test_tx_packet();
}


void e1000_init_rx(void){
    struct PageInfo *pp;
    int i, j, m ;
    m = PGSIZE / RDBUFLEN;
    tdt = 0;
    
    memset(rx_desc_arr, 0, sizeof(rx_desc_arr));

    cprintf("e1000_init_rx: rx_desc_arr: 0x%08x\n", rx_desc_arr);

    for(i = 0, j = m; i < RDLEN; i++){
        if(j == m){
            if((pp = page_alloc(1)) == NULL){
                panic("e1000_init: no memory");
            }
            pp->pp_ref++;
            j = 0;
        }
        rx_desc_arr[i].addr = page2pa(pp) + j * RDBUFLEN;
        rx_desc_arr[i].length = RDBUFLEN;
        j++;
    }

    e1000_regs[E1000_RAL] = MACADDRL;
    e1000_regs[E1000_RAH] = MACADDRH | (1 << 31) ;
    e1000_regs[E1000_RDLEN] = RDLEN * sizeof(struct rx_desc);
    e1000_regs[E1000_RDBAL] = PADDR(rx_desc_arr);
    e1000_regs[E1000_RDBAH] = 0x0;
    e1000_regs[E1000_RDH] = 1;
    e1000_regs[E1000_RDT] = 0;
    
    e1000_regs[E1000_RCTL] = E1000_RCTL_EN | E1000_RCTL_LBM_NO | E1000_RCTL_RDMTS_EIGTH | E1000_RCTL_BAM | E1000_RCTL_SZ_2048 | E1000_RCTL_SECRC;
}


/*
 * send a packet, return 0 on success.
 * error:
 *   -E_INVAL: buf is too large
 *   -E_NETWORK_NO_BUF: tx desc arr is full
 */
int e1000_tx_packet(void *buf, size_t len){
    if(len > TDBUFLEN){
        return -E_INVAL;
    }
    if(!(tx_desc_arr[tdt].status & TXD_STA_DD_MASK)){
        return -E_NETWORK_NO_BUF;
    }

    // cprintf("e1000_tx_packet: tdt = %d, tx_desc.addr = 0x%08x, len = %d \n", tdt, (uint32_t)tx_desc_arr[tdt].addr, len);

    memmove(KADDR(tx_desc_arr[tdt].addr), buf, len);
    tx_desc_arr[tdt].length = len;
    tx_desc_arr[tdt].status &= ~TXD_STA_DD;
    tx_desc_arr[tdt].cmd |= TXD_CMD_RS | TXD_CMD_EOP;
    tdt = (tdt + 1) % TDLEN;
    e1000_regs[E1000_TDT] = tdt;
    return 0;
}

int e1000_rx_packet(void *buf, size_t len){
    int new_rdt = (rdt + 1) % RDLEN;
    int stat = E1000_RXD_STAT_DD | E1000_RXD_STAT_EOP;
    if((rx_desc_arr[new_rdt].status & stat) != stat){
        return -E_NETWORK_NO_DATA;
    }
    if(len < rx_desc_arr[new_rdt].length){
        return -E_INVAL;
    }
    memmove(buf, KADDR(rx_desc_arr[new_rdt].addr), rx_desc_arr[new_rdt].length);
    rdt = new_rdt;
    rx_desc_arr[rdt].status &= ~stat;
    e1000_regs[E1000_RDT] = rdt;
    return rx_desc_arr[new_rdt].length;
}

void test_tx_packet(void){
    char *packet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for(int i = 0; i < 10; i++){
        e1000_tx_packet(packet, strlen(packet));
    }
}