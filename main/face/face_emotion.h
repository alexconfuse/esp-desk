#pragma once

/**
 * Face emotions for Desk Buddy.
 * LLM reply "emotion" strings map onto this set (see face_ui_emotion_from_str).
 */
typedef enum {
    FACE_EMOTION_IDLE = 0,
    FACE_EMOTION_CURIOUS,
    FACE_EMOTION_LISTENING,
    FACE_EMOTION_THINKING,
    FACE_EMOTION_SPEAKING,
    FACE_EMOTION_HAPPY,
    FACE_EMOTION_SAD,
    FACE_EMOTION_ANNOYED,
    FACE_EMOTION_SURPRISED,
    FACE_EMOTION_SLEEPY,
    FACE_EMOTION_OFFLINE,
    FACE_EMOTION_BAD_KEY,
} face_emotion_t;
