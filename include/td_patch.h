#ifndef _H_TD_PATCH
#define _H_TD_PATCH

#include "pgcmp.h"

// patch 'b' target table 
extern void td_open_patch(void);
extern void td_connect_patch(target_t *p_target);
extern void td_patch(int32_t p_target_id,
                     const char *p_keydata1,
                     const char *p_keydata2);
extern void td_disconnect_patch(target_t *p_target);
#endif
