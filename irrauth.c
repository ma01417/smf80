#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

/* irrauth.c - implementazione di irrauth_check_auth(): formatta
 * entity_name/class_name nei campi a lunghezza fissa richiesti da
 * RACROUTE REQUEST=AUTH e richiama r_admin_check_auth() (racauth_metal.c).
 * Nessun collegamento al codice EXTRACT/EXTRACTN di irrcseq.c - vedi
 * irrauth.h.
 */

#include <string.h>
#include "irrauth.h"

int irrauth_check_auth(const char *entity_name, const char *class_name, unsigned char attr,
                        RacauthStatus *status)
{
    unsigned char entity[246];
    unsigned char cls[8];
    RacauthStatus local_status;
    RacauthStatus *ast = status ? status : &local_status;
    size_t elen, clen;

    memset(entity, ' ', sizeof(entity));
    elen = strlen(entity_name);
    if (elen > sizeof(entity)) elen = sizeof(entity);
    memcpy(entity, entity_name, elen);

    memset(cls, ' ', sizeof(cls));
    clen = strlen(class_name);
    if (clen > sizeof(cls)) clen = sizeof(cls);
    memcpy(cls, class_name, clen);

    return r_admin_check_auth(entity, cls, attr, ast);
}
