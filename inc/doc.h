/*
 * doc.h
 */
#ifndef DOC_H_
#define DOC_H_

#include <tiinc.h>

#define DOC_DOCS(__hash) TI_URL"/ThingsDocs/"__hash
#define DOC_SEE(__hash) "; see "DOC_DOCS(__hash)

#define DOC_DEFINE                  DOC_SEE("#define")
#define DOC_NEW_TYPE                DOC_SEE("#new_type")
#define DOC_NEW_USER                DOC_SEE("#new_user")
#define DOC_RENAME_COLLECTION       DOC_SEE("#rename_collection")
#define DOC_SPEC                    DOC_SEE("#type-declaration")


#endif  /* TI_DOC_H_ */
