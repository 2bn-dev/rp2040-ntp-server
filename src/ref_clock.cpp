#define XOSC_MHZ 10

#include "common.h"


#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/resets.h"
#include "hardware/watchdog.h"


void configure_clocks_10mhz(void){
    // Before we touch PLLs, switch sys and ref cleanly away from their aux sources.
    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_sys].selected != 0x1)
        tight_loop_contents();
    hw_clear_bits(&clocks_hw->clk[clk_ref].ctrl, CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while (clocks_hw->clk[clk_ref].selected != 0x1)
        tight_loop_contents();

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    //pll_init(pll_sys, 1, 1500 * MHZ, 6, 2);
    // pll_init is hardcoded-ish to 12 Mhz, easier to just manually configure the plls in the same way.
    uint32_t pdiv_sys = (5 << PLL_PRIM_POSTDIV1_LSB) | (2 << PLL_PRIM_POSTDIV2_LSB);
    reset_block(RESETS_RESET_PLL_SYS_BITS);
    unreset_block_wait(RESETS_RESET_PLL_SYS_BITS);
    pll_sys->cs = 1;
    pll_sys->fbdiv_int = 250;
  
    hw_clear_bits(&pll_sys->pwr, PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS);
    while (!(pll_sys->cs & PLL_CS_LOCK_BITS)) tight_loop_contents();
    pll_sys->prim = pdiv_sys;

    hw_clear_bits(&pll_sys->pwr, PLL_PWR_POSTDIVPD_BITS);


    //pll_init(pll_usb, 1, 1200 * MHZ, 5, 5);
    uint32_t pdiv_usb = (5 << PLL_PRIM_POSTDIV1_LSB) | (5 << PLL_PRIM_POSTDIV2_LSB);
    reset_block(RESETS_RESET_PLL_USB_BITS);
    unreset_block_wait(RESETS_RESET_PLL_USB_BITS);
    pll_usb->cs = 1;
    pll_usb->fbdiv_int = 120;
    
    hw_clear_bits(&pll_usb->pwr, PLL_PWR_PD_BITS | PLL_PWR_VCOPD_BITS);
    while (!(pll_usb->cs & PLL_CS_LOCK_BITS)) tight_loop_contents();
    pll_usb->prim = pdiv_usb;

    hw_clear_bits(&pll_usb->pwr, PLL_PWR_POSTDIVPD_BITS);


    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                    0, // No aux mux
                    10 * MHZ,
                    10 * MHZ);

    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    250 * MHZ,
                    250 * MHZ);

    clock_configure(clk_usb,
                    0, // No GLMUX
                    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);

    clock_configure(clk_adc,
                    0, // No GLMUX
                    CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    48 * MHZ);

    clock_configure(clk_rtc,
                    0, // No GLMUX
                    CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48 * MHZ,
                    46875);

    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    250 * MHZ,
                    250 * MHZ);

    watchdog_start_tick(10);

    //testing ref clk output to gpio21
    //clock_gpio_init(21, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_REF, 1);
}

