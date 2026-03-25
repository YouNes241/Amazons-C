#ifndef I18N_H
#define I18N_H

#include <libintl.h>
#include <locale.h>

// La fameuse macro qui raccourcit "gettext" en "_"
#define _(String) gettext (String)

#endif // I18N_H
