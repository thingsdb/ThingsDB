///*
// * back.h
// */
//#ifndef TI_BACK_H_
//#define TI_BACK_H_
//
//typedef enum
//{
//    TI_BACK_PING,          // status
//    TI_BACK_AUTH,          // [id, version, min_version]
//    TI_BACK_EVENT_REG,     // id
//    TI_BACK_EVENT_UPD,     // [id, new_id]
//    TI_BACK_EVENT_READY,   // [id, raw]
//    TI_BACK_EVENT_CANCEL,  // id
//    TI_BACK_MAINT_REG,     // maintenance_counter
//} ti_back_req_e;
//
//typedef struct ti_back_s  ti_back_t;
//
//#include <ti/stream.h>
//struct ti_back_s
//{
//    ti_stream_t * sock;
//};
//
//ti_back_t * ti_back_create(void);
//void ti_back_destroy(ti_back_t * back);
//int ti_back_listen(ti_back_t * back);
//const char * ti_back_req_str(ti_back_req_e tp);
//
//#endif /* TI_BACK_H_ */
//
//
