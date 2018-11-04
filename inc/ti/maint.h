///*
// * maint.h
// */
//#ifndef TI_MAINT_H_
//#define TI_MAINT_H_
//
//#define TI_MAINT_STAT_NIL 0
//#define TI_MAINT_STAT_READY 1
//#define TI_MAINT_STAT_REG 2
//#define TI_MAINT_STAT_WAIT 3
//#define TI_MAINT_STAT_BUSY 4
//
//typedef struct ti_maint_s  ti_maint_t;
//
//#include <uv.h>
//#include <stdint.h>
//
//ti_maint_t * ti_maint_new(void);
//int ti_maint_start(ti_maint_t * maint);
//void ti_maint_stop(ti_maint_t * maint);
//
//struct ti_maint_s
//{
//    uv_work_t work;
//    uv_timer_t timer;
//    uint8_t status;
//    uint64_t last_commit;
//};
//
//#endif /* TI_MAINT_H_ */
//
//
