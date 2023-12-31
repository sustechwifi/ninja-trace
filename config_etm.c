
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <asm/io.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JunTao You");
MODULE_DESCRIPTION("Confiure ETM registers in syscall tracing");


/*
Present only when FEAT_ETE is implemented and System register access to the trace unit registers is implemented

name                [op0 op1 CRn CRm op2](S<op0>_<op1>_<Cn>_<Cm>_<op2>)     discription     
======================================================================================================================================================
TRCPRGCTLR          [0b10 0b001 0b0000 0b0001 0b000](S2_1_C0_C1_0)          Trace Programming Control Register
TRCVICTLR           [0b10 0b001 0b0000 0b0000 0b010](S2_1_C0_C0_2)          Trace ViewInst Main Control Register
TRCEXTINSELR<0>     [0b10 0b001 0b0000 0b1000 0b100](S2_1_C0_C8_4)          Trace External Input Select Register <n>, 
TRCRSCTLR<2>        [0b10 0b001 0b0001 0b0010 0b000](S2_1_C1_C2_0)          Trace Resource Selection Control Register<n>, n = 2 - 31. 
TRCEVENTCTL0R       [0b10 0b001 0b0000 0b1000 0b000](S2_1_C0_C8_0)          Trace Event Control 0 Register
TRCEVENTCTL1R       [0b10 0b001 0b0000 0b1001 0b000](S2_1_C0_C9_0)          Trace Event Control 1 Register, Controls behavior of TRCEVENTCTL0R.
*/


/*
 * ETMv4 registers:
 * 0x000 - 0x2FC: Trace         registers
 * 0x300 - 0x314: Management    registers
 * 0x318 - 0xEFC: Trace         registers
 * 0xF00: Management		registers
 * 0xFA0 - 0xFA4: Trace		registers
 * 0xFA8 - 0xFFC: Management	registers
 */

// ETM register base address
#define ETM_BASE_ADDRESS	0xE0041000
#define ETM_REGISTER_RANGE  0x1000




// ETM regiser                  offest
//===================================================
#define TRCPRGCTLR              (0x004)
#define TRCEVENTCTL0R			(0x020)
#define TRCEVENTCTL1R			(0x024)
#define	TRCVICTLR		        (0x080)  
#define TRCEXTINSELR	        (0x120)
#define TRCRSCTLRn(n)			(0x200 + (n * 4))


// Bits Fields in ETM registers
// =======================================================================================



/*
TRCPRGCTLR.EN, bit [0]
    0b0     The trace unit is disabled.
    0b1     The trace unit is enabled.
*/
#define EN                      (0x1 << 0)


/*
TRCEVENTCTL0R.EVENT0_SEL, bits [4:0]    choose resource 2 as event 0
    2   EVENT0_SEL; Event element 0 is generated when trace resoure 2 is showed.

Work at selected resource events occurs and TRCEVENTCTL1R.INSTEN[0] == 1
*/
#define EVENT0_SEL              (0xf)



/*
TRCEVENTCTL1R.INSTEN[0], bit [0]. when ETEEvent <0> occurs. The trace unit generates an Event element 0.
    0b1     The trace unit generates an Event element 0

Work at trace unit is in the Idle state, n = 0-3
*/
#define INSTEN                  (0xf)



/*
TRCVICTLR.EXLEVEL_NS_EL<n>, bit [22:20]; EXLEVEL_S_EL<n>, bit [19:16]
    0b0     The trace unit generates instruction trace for EL<n> in Non-secure state.
    0b1     The trace unit does not generate instruction trace for EL<n> in Non-secure state.

Work at trace unit is in either of the Idle or Stable states
*/
#define EXLEVEL_NS_EL           (0x7 << 20)
#define EXLEVEL_S_EL            (0xf << 16)



/*
TRCEXTINSELR<0>.evtCount, bits [15:0].    Set PMU event to select.
    n   PMU event ID

Work at Exist an 'a' that TRCRSCTLR<a>.GROUP == 0b0000 and TRCRSCTLR<a>.EXTIN[n] == 1.
*/
#define EVT_COUNT               (0xffff)


/*
TRCRSCTLR<2>.GROUP, bits [19:16]. Selects a group of resources. 
    0b0000      External Input Selectors.

TRCRSCTLR<2>.SELECT.EXTIN[<m>], bit [m].  m = 3 to 0
    1           EXTIN[0]  

Work at TRCEVENTCTL0R.EVENT0.TYPE == 0 and TRCEVENTCTL0R.EVENT0.SEL == n.
    and  trace unit is in the Idle state
*/
#define GROUP                    (0xf << 16)
#define SELECT_EXTINm(m)          (1 << m)


struct config_etm_pram {
    void __iomem *etm_register;
} t_param;


static void enable_ETM(struct config_etm_pram *param, uint32_t en){
    //asm volatile( "mcr p14, 1, %0, c0, c1, 0" : : "r" (en));
    uint32_t reg = ioread32(param->etm_register + TRCPRGCTLR);
    iowrite32(en & EN,param->etm_register + TRCPRGCTLR);
    uint32_t reg2 = ioread32(param->etm_register + TRCPRGCTLR);
    printk(KERN_INFO "Configure TRCPRGCTLR[EN = %d], TRCPRGCTLR: 0x%x -> 0x%x\n",en,reg,reg2);
}




static void enable_trace_EL0(struct config_etm_pram *param) {
    uint32_t reg = ioread32(param->etm_register + TRCVICTLR);
    uint32_t val = (reg | EXLEVEL_S_EL) & (~EXLEVEL_NS_EL);
    iowrite32(val, param->etm_register + TRCVICTLR);
    printk(KERN_INFO "Configure TRCVICTLR.EXLEVELx[NS = 0;S = 1], TRCVICTLR: 0x%x -> 0x%x\n",reg, val);
}



static void trigger_PMU_event(struct config_etm_pram *param, uint32_t eventID) {
    uint32_t reg = ioread32(param->etm_register + TRCEXTINSELR);
    uint32_t val = (reg & (~EVT_COUNT)) | eventID;
    iowrite32(val, param->etm_register + TRCEXTINSELR);
    printk(KERN_INFO "Configure TRCEXTINSELR[evtCount = 0x%x], TRCEXTINSELR: 0x%x -> 0x%x\n", eventID,reg,val);
}




static void load_as_resource2(struct config_etm_pram *param) {
    uint32_t reg = ioread32(param->etm_register + TRCRSCTLRn(2));
    uint32_t val = (reg | SELECT_EXTINm(0)) & (~GROUP);
    iowrite32(val, param->etm_register + TRCRSCTLRn(2));
    printk(KERN_INFO "Configure TRCRSCTLR<2>.GROUP = 0 and TRCRSCTLR<2>.SELECT.EXTIN[0] = 1, TRCRSCTLR<2>: 0x%x -> 0x%x\n",reg,val);
}





static void select_trace_event0(struct config_etm_pram *param) {
    uint32_t reg = ioread32(param->etm_register + TRCEVENTCTL0R);
    uint32_t val = (reg & ~EVENT0_SEL) | 2; // choose resource 2
    iowrite32(val, param->etm_register + TRCEVENTCTL0R);
    printk(KERN_INFO "Configure TRCEVENTCTL0R.EVENT0_SEL = 2, TRCEVENTCTL0R: 0x%x -> 0x%x\n",reg,val);
}




static void enable_trace_event0(struct config_etm_pram *param) {
    uint32_t reg = ioread32(param->etm_register + TRCEVENTCTL1R);
    uint32_t val = (reg & ~INSTEN) | 1; // enable event 0
    iowrite32(val, param->etm_register + TRCEVENTCTL1R);
    printk(KERN_INFO "Configure TRCEVENTCTL1R.INSTEN[0] = 1, TRCEVENTCTL1R: 0x%x -> 0x%x\n",reg,val);
}



static int do_config(struct config_etm_pram *param){
    enable_ETM(param,0);
    enable_trace_EL0(param);
    trigger_PMU_event(param,0x60);
    load_as_resource2(param);
    select_trace_event0(param);
    enable_trace_event0(param);
    enable_ETM(param,1);
    return 0;
}


static int __init etm_config_init(void) {
    printk(KERN_INFO "Configure ETM begin!\n");
    struct config_etm_pram *param = kmalloc(sizeof(t_param), GFP_KERNEL);
    param->etm_register = ioremap(ETM_BASE_ADDRESS, ETM_REGISTER_RANGE);
    do_config(param);
    iounmap(param->etm_register);
    kfree(param);
    return 0;
}

static void __exit etm_config_exit(void) {
    printk(KERN_INFO "Goodbye!\n");
}

module_init(etm_config_init);
module_exit(etm_config_exit);


/*
make
dmesg
sudo insmod config_etm.ko 
dmesg
sudo rmmod config_etm
*/