#ifndef NETEMU_H
#define NETEMU_H

#include <stdint.h>

typedef struct channel_state_info {
	//double duration;
	uint32_t delay; 
	uint32_t jitter;
	uint32_t loss_rate; 
	uint32_t duplicate_rate;
	uint32_t reorder_gap;
	uint32_t queue_size;
} channel_state_info_t;

void init_netem (void);
void setup_netem (const channel_state_info_t *csi);
void close_netem (void);

#endif /* NETEMU_H */
