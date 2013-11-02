//
//  libintl.h
//  libintl
//
//  Created by Jeong YunWon on 12. 10. 25..
//  Copyright (c) 2012 youknowone.org. All rights reserved.
//

/*!
 *  @header libintl.h
 *  Project:
 *
 *  This file is used to replace libintl part of gettext runtime with
 *      NSBundle localizedString: for OSX/iOS.
 *  Visit gettext ( http://www.gnu.org/software/gettext/gettext.html ) for
 *  original implementation.
 *
 *  NOTE: This implementation does full-support gettext and dgettext only.
 *      And also support ngettext and dngettext with only singler/plural.
 *      You should use original gettext for the best.
 *
 *  Honor your unix-based i18n code without gettext dependency!
 *
 *  On compile time:
 *      1. Set this dummy libintl.h be header path.
 *      2. Link you project to this dummy libintl.a
 *
 *  On build time:
 *      1. Put your <domain>.strings localization files in <locale>.lproj
 *      2. Bundle this directories.
 *
 *      Oops, no .strings files? Here is po_to_strings.rb in repository:
 *          1. Put your po files in po/ directory.
 *              like: po/ko.po
 *          2. Run po_to_strings.rb
 *          3. Get result in po/ directory.
 *              like: po/ko.strings
 *
 *  On run time:
 *      1. Be lazy and see your gettext-based code is localized very well!
 *      2. Or suppose your Localizable.strings is:
 *
 *          "My Message" = "내 글쪽";
 *
 *         Write code as like:
 *
 *          gettext("My Message"); // returns "내 글쪽" in default domain
 *
 *      3. Or suppose your libintl.strings is:
 *
 *          "My Message" = "내 글쪽";
 *
 *          "My Messages" = "내 글쪽들";
 *
 *         Write code as like:
 *
 *          dgettext("libintl", "My Message");  // returns "내 글쪽"
 *
 *          dgettext("libintl", "My Messages"); // returns "내 글쪽들"
 *
 *         Write code as like:
 *
 *          dngettext("libintl", "My Message", "My Messages", 1); // returns "내 글쪽"
 *
 *          dngettext("libintl", "My Message", "My Messages", 2); // returns "내 글쪽들"
 *
 *          dngettext("libintl", "My Message", "My Messages", 3); // returns "내 글쪽들"
 *
 *
 *         Write code as like:
 *
 *          textdomain("libintl"); // returns "libintl", meaningless
 *
 *          textdomain("");        // returns "libintl", current domain
 *
 *          gettext("My Message"); // returns "내 글쪽", in current domain
 *
 *  NOTE: Only system locale is supported.
 */

#ifndef __LIBINTL_ALTERNATIVE__
#define __LIBINTL_ALTERNATIVE__

#include <xlocale.h>

/*!
 *  @define
 *  @brief Fake gettext version 0.18.2
 */
#ifndef LIBINTL_VERSION
#define LIBINTL_VERSION 0x001202
#endif
extern int libintl_version;

#ifdef __cplusplus
extern "C" {
#endif
    
    /*!
     *  @brief gettext implementation
     *  @see gettext
     */
    extern char *libintl_gettext (const char *__msgid);
    /*!
     *  @brief NSLocalizedStringFromTable with table from textdomain
     *  @details This function works as like NSLocalizedStringFromTable(__msgid, textdomain())
     *  @param __msgid
     *      id to find localized string.
     *  @return Localized string.
     *  @see textdomain
     *  @see NSLocalizedStringFromTable
     */
    static inline char *gettext (const char *__msgid)
    {
        return libintl_gettext (__msgid);
    }
    
    /*!
     *  @brief dgettext implementation
     *  @see dgettext
     */
    extern char *libintl_dgettext(const char *__domainname, const char *__msgid);
    /*!
     *  @brief NSLocalizedStringFromTable
     *  @details This function works as like NSLocalizedStringFromTable(__msgid, __domainname)
     *  @see NSLocalizedStringFromTable
     */
    static inline char *dgettext(const char *__domainname, const char *__msgid) {
        return libintl_dgettext(__domainname, __msgid);
    }
    
    /*!
     *  @brief dcgettext implementation
     *  @see dcgettext
     */
    extern char *libintl_dcgettext (const char *__domainname, const char *__msgid,
                                    int __category);
    /*!
     *  @brief Redirect to libintl_dgettext
     *  @details Dummy implementation about category. __category would be ignored always.
     *  @see dgettext
     */
    static inline char *dcgettext (const char *__domainname, const char *__msgid,
                                   int __category) {
        return libintl_dcgettext(__domainname, __msgid, __category);
    }
    
    /*!
     *  @brief ngettext implementation
     *  @see ngettext
     */
    extern char *libintl_ngettext (const char *__msgid1, const char *__msgid2,
                                   unsigned long int __n);
    /*!
     *  @brief NSLocalizedStringFromTable with table from textdomain, detecting singular/plural
     *  @details Rough implementation about plural decision. This function works as like NSLocalizedStringFromTable(__msgid1, textdomain()). Replace __msgid1 with __msgid2 if __n is not 1. Use gettext-runtime for more complex plural system.
     *  @see textdomain
     *  @see NSLocalizedStringFromTable
     */
    static inline char *ngettext (const char *__msgid1, const char *__msgid2,
                                  unsigned long int __n) {
        return libintl_ngettext(__msgid1, __msgid2, __n);
    }
    
    /*!
     *  @brief dngettext implementation
     *  @see dngettext
     */
    extern char *libintl_dngettext (const char *__domainname,
                                    const char *__msgid1, const char *__msgid2,
                                    unsigned long int __n);
    /*!
     *  @brief NSLocalizedStringFromTable with detecting singular/plural
     *  @details Rough implementation about plural decision. This function works as like NSLocalizedStringFromTable(__msgid1, __domainname). Replace __msgid1 with __msgid2 if __n is not 1. Use gettext-runtime for more complex plural system.
     *  @see NSLocalizedStringFromTable
     */
    static inline char *dngettext (const char *__domainname,
                                   const char *__msgid1, const char *__msgid2,
                                   unsigned long int __n) {
        return libintl_dngettext(__domainname, __msgid1, __msgid2, __n);
    }
    
    /*!
     *  @brief dcngettext implementation
     *  @see dcngettext
     */
    extern char *libintl_dcngettext (const char *__domainname,
                                     const char *__msgid1, const char *__msgid2,
                                     unsigned long int __n, int __category);
    /*!
     *  @brief Redirect to libintl_dngettext
     *  @details Dummy implementation about category. Rough implementation about plural decision. __category would be ignored always.
     *  @see dngettext
     */
    static inline char *dcngettext (const char *__domainname,
                                    const char *__msgid1, const char *__msgid2,
                                    unsigned long int __n, int __category) {
        return libintl_dcngettext(__domainname, __msgid1, __msgid2, __n, __category);
    }
    
    
    /*!
     *  @brief textdomain implementation
     *  @see textdomain
     */
    extern char *libintl_textdomain (const char *__domainname);
    /*!
     *  @brief Set or get current text domain name.
     *  @param __domainname
     *      NULL to get. Or set current text domain name.
     *  @return Current domain name if __donaminname is NULL. Or set __domainname as current domain name and returns it.
     */
    static inline char *textdomain (const char *__domainname) {
        return libintl_textdomain(__domainname);
    }
    
    /*!
     *  @brief bindtextdomain implementation
     *  @see bindtextdomain
     */
    extern char *libintl_bindtextdomain (const char *__domainname,
                                         const char *__dirname);
    /*!
     *  @brief Dummy implementation.
     *  @return __dirname
     *  @details Dummy implementation except return value. This returns __dirname for code compatibility. But does nothing else. If your code depends on this function, use gettext-runtime.
     */
    static inline char *bindtextdomain (const char *__domainname,
                                        const char *__dirname) {
        return libintl_bindtextdomain(__domainname, __dirname);
    }
    
    /*!
     *  @brief bind_textdomain_codeset implementation
     *  @see bind_textdomain_codeset
     */
    extern char *libintl_bind_textdomain_codeset (const char *__domainname,
                                                  const char *__codeset);
    /*!
     *  @brief Dummy implementation.
     *  @return __codeset
     *  @details Dummy implementation except return value. This returns __codeset for code compatibility. But does nothing else. If your code depends on this function, use gettext-runtime.
     */
    static inline char *bind_textdomain_codeset (const char *__domainname,
                                                 const char *__codeset) {
        return libintl_bind_textdomain_codeset(__domainname, __codeset);
    }
    
    
    /*!
     *  @brief Redirect to setlocale in <locale.h>
     *  @see setlocale
     */
    extern char *libintl_setlocale (int, const char *);
    
    /*!
     *  @brief Redirect to newlocale in <xlocale.h>
     *  @see newlocale
     */
    extern locale_t libintl_newlocale (int, const char *, locale_t);
    
    /*!
     *  @brief Dummy implementation! This funciton does nothing.
     *  @details Dummy implementation. If your code depends on this function, use gettext-runtime.
     */
    extern void libintl_set_relocation_prefix (const char *orig_prefix,
                                               const char *curr_prefix);
    
#ifdef __cplusplus
}
#endif

#endif
