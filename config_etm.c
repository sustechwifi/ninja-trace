
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
TRCPRGCTLR.EN, bit [0]
    0b0     The trace unit is disabled.
    0b1     The trace unit is enabled.

Work at  EL1 && CPACR_EL1.TTA 
*/
static void enable_ETM(uint32_t en){
    uint64_t result;
    asm volatile(
        "mrc p14, 1, %[result], C0, C1, 0\n"  
        "orr %[result], %[result], %[en]\n"      
        "mcr p14, 1, %[result], C0, C1, 0\n"  
        : [result] "=r" (result)         
        : [en]    "r" (en) 
        : "memory"               
    );
    printk(KERN_INFO "Set TRCPRGCTLR[EN = %d], TRCPRGCTLR = %llx!\n",en,result);
}





/*
TRCVICTLR.EXLEVEL_NS_EL<n>, bit [22:20]; EXLEVEL_S_EL<n>, bit [19:16]
    0b0     The trace unit generates instruction trace for EL<n> in Non-secure state.
    0b1     The trace unit does not generate instruction trace for EL<n> in Non-secure state.

Work at trace unit is in either of the Idle or Stable states
*/
static void enable_trace_EL0(void) {
    uint64_t result;
    uint32_t clear_mask = 0xff0fffff;
    uint32_t set_mask = 0x000f0000;
    asm volatile(
        "mrc p14, 1, %[result], C0, C0, 2 \n"  
        "and %[result], %[result], %[clear_mask]\n"      
        "orr %[result], %[result], %[set_mask]\n"      
        "mcr p14, 1, %[result], C0, C0, 2\n"  
        : [result] "=r" (result)         
        :   [clear_mask]    "r" (clear_mask) ,
            [set_mask]      "r" (set_mask)   
        : "memory"             
    );
    printk(KERN_INFO "Set TRCVICTLR.EXLEVELx[NS = 0;S = 1], TRCVICTLR = %llx\n",result);
}


/*
TRCEXTINSELR<0>.evtCount, bits [15:0].    Set PMU event to select.
    n   PMU event ID

Work at Exist an 'a' that TRCRSCTLR<a>.GROUP == 0b0000 and TRCRSCTLR<a>.EXTIN[n] == 1.
*/
static void trigger_PMU_event(void) {
    uint64_t result;
    uint16_t mask = 0x0000;
    uint16_t EXC_SVC_id = 0x60;
    asm volatile(
        "mrc p14, 1, %[result], C0,C8,4 \n"  
        "and %[result], %[result], %[mask]\n"  
        "orr %[result], %[result], %[EXC_SVC_id]\n"          
        "mcr p14, 1, %[result], C0,C8,4\n"  
        : [result] "=r" (result)    
        :   [mask]          "r" (mask) ,     
            [EXC_SVC_id]    "r" (EXC_SVC_id) 
        : "memory"             
    );
    printk(KERN_INFO "Set PMU event %llx as the external input source 0, TRCEXTINSELR[evtCount = %llx], TRCEXTINSELR = %llx\n",result);
}



/*
TRCRSCTLR<2>.GROUP, bits [19:16]. Selects a group of resources. 
    0b0000      External Input Selectors.

TRCRSCTLR<2>.SELECT.EXTIN[<m>], bit [m].  m = 3 to 0
    1           EXTIN[0]  

Work at TRCEVENTCTL0R.EVENT0.TYPE == 0 and TRCEVENTCTL0R.EVENT0.SEL == n.
    and  trace unit is in the Idle state
*/
static void load_as_resource2(void) {
    uint64_t result;
    uint16_t mask = 0x0000;
    asm volatile(
        "mrc p14,1,%[result], C1,C2,0 \n"  
        "and %[result], %[result], %[mask]\n"  
        "orr %[result], %[result], #1\n"          
        "mcr p14,1,%[result], C1,C2,0\n"  
        : [result] "=r" (result)    
        :   [mask]    "r" (mask) 
        : "memory"             
    );
    printk(KERN_INFO "Configure TRCRSCTLR<2>.GROUP = 0 and TRCRSCTLR<2>.SELECT.EXTIN[0] = 1, TRCEXTINSELR = %llx\n",result);
}




/*
TRCEVENTCTL0R.EVENT0_SEL, bits [4:0]    choose resource 2 as event 0
    2   EVENT0_SEL; Event element 0 is generated when trace resoure 2 is showed.

Work at selected resource events occurs and TRCEVENTCTL1R.INSTEN[0] == 1
*/
static void select_trace_event0(void) {
    uint64_t result;
    uint16_t mask = 0x0000;
    asm volatile(
        "mrc p14, 1, %[result], C0, C8, 0\n"  
        "orr %[result], %[result], #2\n"          
        "mcr p14, 1, %[result], C0, C8, 0\n"  
        : [result] "=r" (result)    
        :   
        : "memory"             
    );
    printk(KERN_INFO "Configure TRCRSCTLR<2>.GROUP = 0 and TRCRSCTLR<2>.SELECT.EXTIN[0] = 1, TRCEXTINSELR = %llx\n",result);
}



/*
TRCEVENTCTL1R.INSTEN[0], bit [0]. when ETEEvent <0> occurs. The trace unit generates an Event element 0.
    0b1     The trace unit generates an Event element 0

Work at trace unit is in the Idle state
*/
static void enable_trace_event0(void) {
    uint64_t result;
    uint16_t mask = 0x0000;
    asm volatile(
        "mrc p14, 1, %[result], C0, C9, 0\n"  
        "orr %[result], %[result], #1\n"          
        "mcr p14, 1, %[result], C0, C9, 0\n"  
        : [result] "=r" (result)    
        :   
        : "memory"             
    );
    printk(KERN_INFO "Configure TRCEVENTCTL1R.INSTEN[0] = 1, TRCEVENTCTL1R = %llx\n",result);
}






static int __init etm_config_init(void) {
    printk(KERN_INFO "Configure ETM begin!\n");
    enable_ETM(0);
    enable_trace_EL0();
    trigger_PMU_event();
    load_as_resource2();
    select_trace_event0();
    enable_trace_event0();
    enable_ETM(1);

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