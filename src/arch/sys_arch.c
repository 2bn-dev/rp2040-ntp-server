#include "pico/critical_section.h"
#include "pico/time.h"

typedef int sys_prot_t;

static critical_section_t g_lwip_sec;
static bool cs_init = false;


sys_prot_t sys_arch_protect(void){
  if(!cs_init) critical_section_init(&g_lwip_sec);
  critical_section_enter_blocking(&g_lwip_sec);
}

void sys_arch_unprotect(sys_prot_t pval){
  critical_section_exit(&g_lwip_sec);
}

// lwip needs a millisecond time source, and the TinyUSB board support code has one available */

uint32_t sys_now(void)
{
  return to_ms_since_boot(get_absolute_time());
}

