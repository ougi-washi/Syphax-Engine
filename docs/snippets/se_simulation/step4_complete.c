#include "se_simulation.h"

int main(void) {
	se_simulation_config config = {0};
	se_simulation_handle sim = se_simulation_create(&config);
	se_entity_id entity = se_simulation_entity_create(sim);
	(void)se_simulation_entity_alive(sim, entity);
	u32 count = se_simulation_entity_count(sim);
	(void)count;
	se_simulation_step(sim, 1);
	se_simulation_step(sim, 4);
	se_sim_tick tick = se_simulation_get_tick(sim);
	(void)tick;
	se_simulation_reset(sim);
	se_simulation_destroy(sim);
	return 0;
}
