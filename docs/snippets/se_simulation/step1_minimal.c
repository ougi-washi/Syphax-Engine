#include "se_simulation.h"

int main(void) {
	se_simulation_config config = {0};
	se_simulation_handle sim = se_simulation_create(&config);
	se_entity_id entity = se_simulation_entity_create(sim);
	return 0;
}
