/*
 * doc.h
 */
#ifndef DOC_H_
#define DOC_H_

/* TODO: use doc instead of file definitions */

/* TODO: create doc exist tests */

#include <tiinc.h>

#define DOC_DOCS(__hash) TI_URL"/ThingsDocs/"__hash
#define DOC_SEE(__hash) "; see "DOC_DOCS(__hash)

#define DOC_ADD                     DOC_SEE("#add")
#define DOC_ARRAY                   DOC_SEE("#array")
#define DOC_ASSERT                  DOC_SEE("#assert")
#define DOC_ASSERT_ERR              DOC_SEE("#assert_err")
#define DOC_AUTH_ERR                DOC_SEE("#auth_err")
#define DOC_BAD_DATA_ERR            DOC_SEE("#bad_data_err")
#define DOC_BOOL                    DOC_SEE("#bool")
#define DOC_CALL                    DOC_SEE("#call")
#define DOC_COLLECTION_INFO         DOC_SEE("#collection_info")
#define DOC_COLLECTIONS_INFO        DOC_SEE("#collections_info")
#define DOC_CONTAINS                DOC_SEE("#contains")
#define DOC_COUNTERS                DOC_SEE("#counters")
#define DOC_DEEP                    DOC_SEE("#deep")
#define DOC_DEFINE                  DOC_SEE("#define")
#define DOC_DEL                     DOC_SEE("#del")
#define DOC_DEL_COLLECTION          DOC_SEE("#del_collection")
#define DOC_DEL_EXPIRED             DOC_SEE("#del_expired")
#define DOC_DEL_PROCEDURE           DOC_SEE("#del_procedure")
#define DOC_DEL_TOKEN               DOC_SEE("#del_token")
#define DOC_DEL_USER                DOC_SEE("#del_user")
#define DOC_ENDSWITH                DOC_SEE("#endswith")
#define DOC_ERR                     DOC_SEE("#err")
#define DOC_FORBIDDEN_ERR           DOC_SEE("#forbidden_err")
#define DOC_INT                     DOC_SEE("#int")
#define DOC_LOOKUP_ERR              DOC_SEE("#lookup_err")
#define DOC_MAX_QUOTA_ERR           DOC_SEE("#max_quota_err")
#define DOC_NAMES                   DOC_SEE("#names")
#define DOC_NEW                     DOC_SEE("#new")
#define DOC_NEW_PROCEDURE           DOC_SEE("#new_procedure")
#define DOC_NEW_TYPE                DOC_SEE("#new_type")
#define DOC_NEW_USER                DOC_SEE("#new_user")
#define DOC_NODE_ERR                DOC_SEE("#node_err")
#define DOC_NUM_ARGUMENTS_ERR       DOC_SEE("#num_arguments_err")
#define DOC_OPERATION_ERR           DOC_SEE("#operation_err")
#define DOC_OVERFLOW_ERR            DOC_SEE("#overflow_err")
#define DOC_PROCEDURE_INFO          DOC_SEE("#procedure_info")
#define DOC_PROCEDURES_INFO         DOC_SEE("#procedures_info")
#define DOC_RENAME_COLLECTION       DOC_SEE("#rename_collection")
#define DOC_SPEC                    DOC_SEE("#type-declaration")
#define DOC_SYNTAX_ERR              DOC_SEE("#syntax_err")
#define DOC_TYPE                    DOC_SEE("#type")
#define DOC_TYPE_ERR                DOC_SEE("#type_err")
#define DOC_TYPE_INFO               DOC_SEE("#type_info")
#define DOC_TYPES                   DOC_SEE("#types")
#define DOC_TYPES_INFO              DOC_SEE("#types_info")
#define DOC_USER_INFO               DOC_SEE("#user_info")
#define DOC_USERS_INFO              DOC_SEE("#users_info")
#define DOC_VALUE_ERR               DOC_SEE("#value_err")
#define DOC_ZERO_DIV_ERR            DOC_SEE("#zero_div_err")

#endif  /* TI_DOC_H_ */
