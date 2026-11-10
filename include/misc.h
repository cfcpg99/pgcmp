#ifndef _H_MISC
#define _H_MISC

// remove white space
extern char *ltrim(char *p_buf);
extern char *rtrim(char *p_buf);
extern char *trim(char *p_buf);

// return elapsed time as double
extern double get_elapsed_time(const struct timeval *p_start, 
                               const struct timeval *p_end);

// once per config file
extern void init_a_target(void);

// show stats
extern void debug(void);
#endif
