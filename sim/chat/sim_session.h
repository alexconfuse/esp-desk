#pragma once

#include "sim_config.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void sim_session_init(sim_config_t *cfg);

/** Talk: mock listen → STT → think → speak. */
bool sim_session_request_talk(void);

/** Type path with user text through the same pipeline. */
bool sim_session_request_text(const char *text);

void sim_session_cancel_listen(void);

bool sim_session_is_busy(void);
bool sim_session_is_listening(void);

sim_config_t *sim_session_config(void);

#ifdef __cplusplus
}
#endif
