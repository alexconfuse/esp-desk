#pragma once

#include "desk_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Start worker task; uses NVS-backed config pointer (must outlive session). */
esp_err_t chat_session_start(const desk_config_t *cfg);

/** Talk: listen → STT → LLM → TTS → face. One turn at a time. */
esp_err_t chat_session_request_talk(void);

/** Type path: user text → LLM → TTS → face. */
esp_err_t chat_session_request_text(const char *text);

/** Cancel in-progress listen (second tap / UI). */
void chat_session_cancel_listen(void);

bool chat_session_is_busy(void);
bool chat_session_is_listening(void);

#ifdef __cplusplus
}
#endif
