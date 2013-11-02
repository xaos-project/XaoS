//
//  libintl.m
//  libintl
//
//  Created by Jeong YunWon on 12. 10. 25..
//  Copyright (c) 2012 youknowone.org. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "libintl.h"

NSString *__boundDomain = nil;

static void __setBoundDomain(NSString *domain) {
    __boundDomain = domain;
}


int libintl_version = LIBINTL_VERSION;

char *libintl_gettext (const char *__msgid) {
    return libintl_dgettext(__boundDomain.UTF8String, __msgid);
}

char *libintl_dgettext(const char *__domainname, const char *__msgid) {
    NSString *_domainname = [NSString stringWithUTF8String:__domainname];
    NSString *_msgid = [NSString stringWithUTF8String:__msgid];
    NSString *result = NSLocalizedStringFromTable(_msgid, _domainname, nil);
    return (char *)result.UTF8String;
}

char *libintl_dcgettext (const char *__domainname, const char *__msgid, int __category) {
    return libintl_dgettext(__domainname, __msgid);
}

char *libintl_ngettext (const char *__msgid1, const char *__msgid2,
                        unsigned long int __n) {
    return libintl_dngettext(__boundDomain.UTF8String, __msgid1, __msgid2, __n);
}

char *libintl_dngettext (const char *__domainname,
                         const char *__msgid1, const char *__msgid2,
                         unsigned long int __n) {
    const char *__msgid = (__n == 1) ? __msgid1 : __msgid2;
    return libintl_dgettext(__domainname, __msgid);
}

char *libintl_textdomain (const char *__domainname) {
    if (__domainname == NULL) {
        return (char *)__boundDomain.UTF8String;
    }
    __setBoundDomain([NSString stringWithUTF8String:__domainname]);
    return (char *)__domainname;
}

char *libintl_bindtextdomain (const char *__domainname,
                              const char *__dirname) {
    return (char *)__dirname; // dummy implementation;
}

char *libintl_bind_textdomain_codeset (const char *__domainname,
                                       const char *__codeset) {
    return (char *)__codeset; // dummy implementation;
}

char *libintl_setlocale (int category, const char *locale) {
    return setlocale(category, locale);
}

locale_t libintl_newlocale(int mask, const char * locale, locale_t base) {
    return newlocale(mask, locale, base);
}

void libintl_set_relocation_prefix (const char *orig_prefix,
                                    const char *curr_prefix) {
    
}
