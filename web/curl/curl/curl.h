// libcurl's names that RecompFrontend's mod store uses, for the page, where it is off (../FindCURL.cmake): no handle
// is ever made, so the store's code throws before it fetches anything.
#pragma once

typedef void CURL;

typedef enum { CURLE_OK = 0, CURLE_FAILED_INIT = 2 } CURLcode;

typedef enum {
    CURLOPT_URL, CURLOPT_FOLLOWLOCATION, CURLOPT_USERAGENT, CURLOPT_SSL_VERIFYPEER, CURLOPT_SSL_VERIFYHOST,
    CURLOPT_WRITEFUNCTION, CURLOPT_WRITEDATA, CURLOPT_TIMEOUT, CURLOPT_HTTPHEADER
} CURLoption;

typedef enum { CURLINFO_RESPONSE_CODE } CURLINFO;

#define CURL_GLOBAL_DEFAULT 3L

struct curl_slist {
    char* data;
    struct curl_slist* next;
};

static inline CURLcode curl_global_init(long) { return CURLE_FAILED_INIT; }
static inline CURL* curl_easy_init(void) { return nullptr; }
static inline void curl_easy_cleanup(CURL*) {}
static inline CURLcode curl_easy_setopt(CURL*, CURLoption, ...) { return CURLE_FAILED_INIT; }
static inline CURLcode curl_easy_perform(CURL*) { return CURLE_FAILED_INIT; }
static inline CURLcode curl_easy_getinfo(CURL*, CURLINFO, ...) { return CURLE_FAILED_INIT; }
static inline const char* curl_easy_strerror(CURLcode) { return "no curl on the page"; }
static inline struct curl_slist* curl_slist_append(struct curl_slist* list, const char*) { return list; }
static inline void curl_slist_free_all(struct curl_slist*) {}
