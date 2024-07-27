#ifndef _SESSION_HEADER_
#define _SESSION_HEADER_

typedef struct {
    char sessionId[SESSION_ID_LENGTH];  // Session ID
    time_t expirationTime;              // Expiration time of the session
    // Other session properties as needed...
} Session;

Session* createSession();
void freeSession(Session *session);
void generateSessionId(Session *session);
void setSessionExpirationTime(Session *session, time_t expirationTime);
const char* getSessionId(const Session *session);
time_t getSessionExpirationTime(const Session *session);

#endif