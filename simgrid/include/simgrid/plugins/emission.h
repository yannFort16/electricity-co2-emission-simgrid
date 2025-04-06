#ifndef SIMGRID_PLUGINS_EMISSION_H_
#define SIMGRID_PLUGINS_EMISSION_H_

#include <simgrid/config.h>
#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_PUBLIC void sg_host_emission_plugin_init();
XBT_PUBLIC void sg_host_emission_update_all();
XBT_PUBLIC double sg_host_get_emission(const_sg_host_t host);
void sg_host_setCO2(const_sg_host_t host, int newCO2);

SG_END_DECL

#endif
