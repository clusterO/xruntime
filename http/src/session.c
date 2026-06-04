#include "session.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Create a new session with a generated session ID
 */
Session* createSession()
{
    Session *session = (Session *)malloc(sizeof(Session));
    if (session == NULL) {
        return NULL;
    }
    
    generateSessionId(session);
    session->createdTime = time(NULL);
    // Default expiration: 24 hours from creation
    session->expirationTime = session->createdTime + (24 * 60 * 60);
    
    return session;
}

/**
 * Free the memory allocated for a session
 */
void freeSession(Session *session)
{
    if (session == NULL) {
        return;
    }
    
    free(session);
}

/**
 * Generate a random session ID using system randomness
 */
void generateSessionId(Session *session)
{
    if (session == NULL) {
        return;
    }
    
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charset_size = sizeof(charset) - 1;
    
    // Use process ID and time as seed for randomness
    unsigned int seed = (unsigned int)(getpid() ^ time(NULL));
    srand(seed);
    
    for (int i = 0; i < SESSION_ID_LENGTH - 1; i++) {
        session->sessionId[i] = charset[rand() % charset_size];
    }
    session->sessionId[SESSION_ID_LENGTH - 1] = '\0';
}

/**
 * Set the expiration time for the session
 */
void setSessionExpirationTime(Session *session, time_t expirationTime)
{
    if (session == NULL) {
        return;
    }
    
    session->expirationTime = expirationTime;
}

/**
 * Get the session ID
 */
const char* getSessionId(const Session *session)
{
    return session ? session->sessionId : NULL;
}

/**
 * Get the expiration time of the session
 */
time_t getSessionExpirationTime(const Session *session)
{
    return session ? session->expirationTime : 0;
}

/**
 * Check if the session is expired
 */
int isSessionExpired(const Session *session)
{
    if (session == NULL) {
        return 1;
    }
    
    return time(NULL) >= session->expirationTime;
}

/**
 * Get the creation time of the session
 */
time_t getSessionCreatedTime(const Session *session)
{
    return session ? session->createdTime : 0;
}
