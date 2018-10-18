/*
 * nodes.h
 */
#ifndef THINGSDB_NODES_H_
#define THINGSDB_NODES_H_

int thingsdb_nodes_create(void);
void thingsdb_nodes_destroy(void);
_Bool thingsdb_nodes_has_quorum(void);

#endif /* THINGSDB_NODES_H_ */
