#include "sim_llm.h"

#include "face_ui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DESK_BUDDY_SIM_HAS_CURL
#include <curl/curl.h>
#endif

static int has_word(const char *hay, const char *needle)
{
    const char *p = hay;
    size_t n = strlen(needle);
    while (*p) {
        while (*p && !isalnum((unsigned char)*p)) {
            p++;
        }
        if (!*p) {
            break;
        }
        const char *start = p;
        while (*p && isalnum((unsigned char)*p)) {
            p++;
        }
        size_t len = (size_t)(p - start);
        if (len == n) {
            size_t i;
            for (i = 0; i < n; i++) {
                if (tolower((unsigned char)start[i]) != tolower((unsigned char)needle[i])) {
                    break;
                }
            }
            if (i == n) {
                return 1;
            }
        }
    }
    return 0;
}

sim_lang_t sim_llm_detect_lang(const char *text)
{
    if (!text || !text[0]) {
        return SIM_LANG_EN;
    }
    /* Accented Latin-1 / UTF-8 French cues + common words. */
    if (strstr(text, "é") || strstr(text, "è") || strstr(text, "à") ||
        strstr(text, "ç") || strstr(text, "ê") || strstr(text, "ô") ||
        has_word(text, "bonjour") || has_word(text, "salut") ||
        has_word(text, "merci") || has_word(text, "oui") ||
        has_word(text, "non") || has_word(text, "comment") ||
        has_word(text, "ça") || has_word(text, "ca") ||
        has_word(text, "francais") || has_word(text, "français")) {
        return SIM_LANG_FR;
    }
    return SIM_LANG_EN;
}

void sim_llm_mock_stt(const sim_config_t *cfg, char *out, size_t out_len)
{
    sim_lang_t lang = cfg ? cfg->lang : SIM_LANG_AUTO;
    if (lang == SIM_LANG_AUTO) {
        lang = (rand() % 2) ? SIM_LANG_FR : SIM_LANG_EN;
    }
    if (lang == SIM_LANG_FR) {
        const char *phrases[] = {
            "Bonjour! Comment ca va?",
            "Raconte-moi une blague.",
            "Tu es mon ami?",
            "Quelle heure est-il?",
        };
        snprintf(out, out_len, "%s", phrases[rand() % 4]);
    } else {
        const char *phrases[] = {
            "Hello! How are you?",
            "Tell me a joke.",
            "Are you my friend?",
            "What time is it?",
        };
        snprintf(out, out_len, "%s", phrases[rand() % 4]);
    }
}

static void mock_reply(const sim_config_t *cfg, const char *user_text, sim_llm_reply_t *out)
{
    sim_lang_t lang = cfg ? cfg->lang : SIM_LANG_AUTO;
    if (lang == SIM_LANG_AUTO) {
        lang = sim_llm_detect_lang(user_text);
    }

    face_emotion_t emotion = FACE_EMOTION_HAPPY;
    if (has_word(user_text, "sad") || has_word(user_text, "triste") ||
        has_word(user_text, "cry") || has_word(user_text, "alone")) {
        emotion = FACE_EMOTION_SAD;
    } else if (has_word(user_text, "wow") || has_word(user_text, "surprise") ||
               has_word(user_text, "incroyable")) {
        emotion = FACE_EMOTION_SURPRISED;
    } else if (has_word(user_text, "annoying") || has_word(user_text, "angry") ||
               has_word(user_text, "ennuyeux") || has_word(user_text, "colere")) {
        emotion = FACE_EMOTION_ANNOYED;
    } else if (has_word(user_text, "sleep") || has_word(user_text, "tired") ||
               has_word(user_text, "dormir") || has_word(user_text, "fatigue")) {
        emotion = FACE_EMOTION_SLEEPY;
    } else if (has_word(user_text, "joke") || has_word(user_text, "blague") ||
               has_word(user_text, "friend") || has_word(user_text, "ami") ||
               has_word(user_text, "hello") || has_word(user_text, "bonjour")) {
        emotion = FACE_EMOTION_HAPPY;
    } else {
        emotion = FACE_EMOTION_CURIOUS;
    }

    out->emotion = emotion;
    out->used_live = false;

    /* ASCII-friendly captions: default Montserrat subset lacks many Latin-1 glyphs. */
    if (lang == SIM_LANG_FR) {
        switch (emotion) {
        case FACE_EMOTION_SAD:
            snprintf(out->text, sizeof(out->text),
                     "Oh... je suis la avec toi. On respire un peu?");
            break;
        case FACE_EMOTION_SURPRISED:
            snprintf(out->text, sizeof(out->text),
                     "Waouh! Tu m'etonnes toujours.");
            break;
        case FACE_EMOTION_ANNOYED:
            snprintf(out->text, sizeof(out->text),
                     "Hmpf. D'accord... mais sois gentil.");
            break;
        case FACE_EMOTION_SLEEPY:
            snprintf(out->text, sizeof(out->text),
                     "Mmh... j'ai sommeil. Encore une question?");
            break;
        case FACE_EMOTION_HAPPY:
            snprintf(out->text, sizeof(out->text),
                     "Coucou! Ca me fait plaisir. Je suis ton desk buddy.");
            break;
        default:
            snprintf(out->text, sizeof(out->text),
                     "Hmm, interessant. Dis-m'en plus!");
            break;
        }
    } else {
        switch (emotion) {
        case FACE_EMOTION_SAD:
            snprintf(out->text, sizeof(out->text),
                     "Aw... I'm right here with you. Want a quiet moment?");
            break;
        case FACE_EMOTION_SURPRISED:
            snprintf(out->text, sizeof(out->text),
                     "Whoa! You always keep me on my toes.");
            break;
        case FACE_EMOTION_ANNOYED:
            snprintf(out->text, sizeof(out->text),
                     "Hmph. Fine... but be nice to me.");
            break;
        case FACE_EMOTION_SLEEPY:
            snprintf(out->text, sizeof(out->text),
                     "Zzz... almost. One more question?");
            break;
        case FACE_EMOTION_HAPPY:
            snprintf(out->text, sizeof(out->text),
                     "Hey! That made my day. I'm your desk buddy.");
            break;
        default:
            snprintf(out->text, sizeof(out->text),
                     "Hmm, interesting. Tell me more!");
            break;
        }
    }
}

#ifdef DESK_BUDDY_SIM_HAS_CURL
typedef struct {
    char *data;
    size_t len;
} curl_buf_t;

static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    curl_buf_t *b = userdata;
    size_t n = size * nmemb;
    char *p = realloc(b->data, b->len + n + 1);
    if (!p) {
        return 0;
    }
    b->data = p;
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

static bool live_chat(const sim_config_t *cfg, const char *user_text, sim_llm_reply_t *out)
{
    if (!sim_config_has_openai_key(cfg)) {
        return false;
    }

    char url[192];
    snprintf(url, sizeof(url), "%s/chat/completions", cfg->openai_base_url);

    /* Build JSON-escaped user content, then chat body. */
    char escaped[480];
    size_t ei = 0;
    escaped[ei++] = '"';
    for (const char *p = user_text; *p && ei + 2 < sizeof(escaped) - 1; p++) {
        if (*p == '"' || *p == '\\') {
            escaped[ei++] = '\\';
        }
        if ((unsigned char)*p < 0x20) {
            continue;
        }
        escaped[ei++] = *p;
    }
    escaped[ei++] = '"';
    escaped[ei] = '\0';

    char body[1400];
    snprintf(body, sizeof(body),
             "{"
             "\"model\":\"gpt-4o-mini\","
             "\"temperature\":0.7,"
             "\"messages\":["
             "{\"role\":\"system\",\"content\":"
             "\"You are Desk Buddy, a tiny pet face on a desk. "
             "Reply in the same language as the user (EN or FR). "
             "Respond with ONLY compact JSON: "
             "{\\\"emotion\\\":\\\"happy|sad|curious|surprised|annoyed|sleepy\\\","
             "\\\"text\\\":\\\"short spoken reply\\\"}\"},"
             "{\"role\":\"user\",\"content\":%s}"
             "]}",
             escaped);

    CURL *curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    curl_buf_t buf = {0};
    struct curl_slist *headers = NULL;
    char auth[200];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", cfg->openai_api_key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode rc = curl_easy_perform(curl);
    long http = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK || http < 200 || http >= 300 || !buf.data) {
        free(buf.data);
        return false;
    }

    /* Very small extract: look for "emotion" and "text" inside assistant content. */
    const char *content = strstr(buf.data, "\"content\"");
    if (!content) {
        free(buf.data);
        return false;
    }
    const char *emo = strstr(content, "\"emotion\"");
    const char *txt = strstr(content, "\"text\"");
    char emotion_s[32] = {0};
    char text_s[300] = {0};
    if (emo) {
        const char *q1 = strchr(emo + 9, '"');
        if (q1) {
            q1++;
            const char *q2 = strchr(q1, '"');
            if (q2 && (size_t)(q2 - q1) < sizeof(emotion_s)) {
                memcpy(emotion_s, q1, (size_t)(q2 - q1));
            }
        }
    }
    if (txt) {
        const char *q1 = strchr(txt + 6, '"');
        if (q1) {
            q1++;
            size_t i = 0;
            while (*q1 && *q1 != '"' && i + 1 < sizeof(text_s)) {
                if (*q1 == '\\' && q1[1]) {
                    q1++;
                }
                text_s[i++] = *q1++;
            }
            text_s[i] = '\0';
        }
    }
    free(buf.data);

    if (!text_s[0]) {
        return false;
    }
    out->emotion = face_ui_emotion_from_str(emotion_s[0] ? emotion_s : "happy");
    snprintf(out->text, sizeof(out->text), "%s", text_s);
    out->used_live = true;
    return true;
}
#endif /* DESK_BUDDY_SIM_HAS_CURL */

void sim_llm_reply(const sim_config_t *cfg, const char *user_text, sim_llm_reply_t *out)
{
    memset(out, 0, sizeof(*out));
    if (!user_text || !user_text[0]) {
        out->emotion = FACE_EMOTION_CURIOUS;
        snprintf(out->text, sizeof(out->text), "...?");
        return;
    }

#ifdef DESK_BUDDY_SIM_HAS_CURL
    if (cfg && cfg->prefer_live && sim_config_has_openai_key(cfg)) {
        if (live_chat(cfg, user_text, out)) {
            printf("[I][sim_llm] live OpenAI-compatible chat OK\n");
            return;
        }
        printf("[W][sim_llm] live chat failed — falling back to mock\n");
    }
#else
    (void)cfg;
    if (cfg && cfg->prefer_live) {
        printf("[W][sim_llm] DESK_BUDDY_SIM_LIVE set but sim built without libcurl — mock\n");
    }
#endif

    mock_reply(cfg, user_text, out);
    printf("[I][sim_llm] mock reply emotion=%d live=0\n", (int)out->emotion);
}
