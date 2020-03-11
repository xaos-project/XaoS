# Instructions on internationalization

XaoS has transitioned from gettext to Qt for i18n.

The .ts files can be edited using Qt Linguist. See the
[Qt Linguist manual](https://doc.qt.io/qt-5/qtlinguist-index.html) for more information.

## New or modified texts in XaoS

In case there is a new or updated text in the set of translated texts,
please run the following command from the XaoS top level directory to update the
.ts files with changes from the source code:

```
lupdate -tr-function-alias QT_TRANSLATE_NOOP=TR XaoS.pro
```

The function alias is very important so Qt can find the TR macros used
by XaoS.

## Adding a new language

To add a new translation, do the following:

1. Edit i18n.pri and add a file named $$PWD/XaoS_LL.ts to the TRANSLATIONS list. 
Replace LL with the two
letter ISO 639-1 code for the new language. Save the pri file and
rerun the lupdate command above to create the .ts file.

2. The i18n.pri contains commands to automatically generate the binary .qm
files from the .ts files each time XaoS is built, but the .qm files can
also be manually updated by running the following command from the top
level directory:

```
lrelease XaoS.pro
```

3. Also, add an entry for the new language in the file XaoS.qrc
in the top level directory. This makes sure that the .qm file will be
used by the executable (actually, the .qm file is linked against
the executable).

4. There are some further changes required in src/include/ui_helper.h
by defining the new language with a line

```
#define UIH_LANG_LL XX
```

where LL is the two letter ISO 639-1 code for the new language and the numbers XX
are consecutive in the list. You probably need to renumber the list or a part of it.

5. In src/ui/main.cpp increase the number of internationalized menu items at line

```
#define MAX_MENUITEMS_I18N 30
```

If you compile XaoS in debug mode, you can find the required minimal number here
by checking the message "Filled ... ui menu items out of ...".

6. Also, add the LL code in src/ui/main.cpp to the list languages1 and an appropriate
[ICU](http://userguide.icu-project.org/locale) code to languages2.

7. Finally, extend the file src/ui/main.cpp with a menu entry like

```
MENUINTRB_I("setlang", NULL, "Swedish", "sv", UI, uih_setlanguage, UIH_LANG_SV, ui_languageselected);
```

Here the English name "Swedish" and the two letter code "sv" must be changed, and also
UIH_LANG_SV to UIH_LANG_LL, accordingly (as defined in step 4).

**Important: The languages must follow the same order in steps 4, 6 and 7.**

## Testing your changes

After compilation you can try a translation out by starting XaoS
in the bin folder and change the language in the View menu.
