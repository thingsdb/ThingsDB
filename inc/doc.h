/*
 * doc.h
 */
#ifndef DOC_H_
#define DOC_H_

/* TODO: use doc instead of file definitions */

#include <tiinc.h>

#define DOC_DOCS(__hash) TI_URL"/ThingsDocs/"__hash
#define DOC_SEE(__hash) "; see "DOC_DOCS(__hash)

#define DOC_ADD                     DOC_SEE("#add")
#define DOC_ARRAY                   DOC_SEE("#array")
#define DOC_ASSERT                  DOC_SEE("#assert")
#define DOC_BOOL                    DOC_SEE("#bool")
#define DOC_CALL                    DOC_SEE("#call")
#define DOC_COLLECTION_INFO         DOC_SEE("#collection_info")
#define DOC_COLLECTIONS_INFO        DOC_SEE("#collections_info")
#define DOC_DEFINE                  DOC_SEE("#define")
#define DOC_INT                     DOC_SEE("#int")
#define DOC_NAMES                   DOC_SEE("#names")
#define DOC_NEW                     DOC_SEE("#new")
#define DOC_NEW_PROCEDURE           DOC_SEE("#new_procedure")
#define DOC_NEW_TYPE                DOC_SEE("#new_type")
#define DOC_NEW_USER                DOC_SEE("#new_user")
#define DOC_PROCEDURE_INFO          DOC_SEE("#procedure_info")
#define DOC_PROCEDURES_INFO         DOC_SEE("#procedures_info")
#define DOC_RENAME_COLLECTION       DOC_SEE("#rename_collection")
#define DOC_SPEC                    DOC_SEE("#type-declaration")
#define DOC_TYPE                    DOC_SEE("#type")
#define DOC_TYPE_INFO               DOC_SEE("#type_info")
#define DOC_TYPES                   DOC_SEE("#types")
#define DOC_TYPES_INFO              DOC_SEE("#types_info")
#define DOC_USER_INFO               DOC_SEE("#user_info")
#define DOC_USERS_INFO              DOC_SEE("#users_info")


#endif  /* TI_DOC_H_ */
