#include "pgcmp.h"
#include "misc.h"

/* 
   right trim whitespace
*/
char *rtrim(char *p_buf)
{
 int32_t i;

 // backup until printable char
 for(i = (int32_t ) strlen(p_buf)-1; isspace(p_buf[i]); i--);
 
 p_buf[i+1] = '\0'; 

 return(p_buf); 
}

/*
  left trim whitespace
*/
char *ltrim(char *p_buf)
{
 int32_t i;
 int32_t j;

 // forward until printable char
 for(i = 0; isspace(p_buf[i]); i++);

 if (i > 0)
 {
    j = 0; 
    while(p_buf[i] != '\0')
    {
       p_buf[j++] = p_buf[i++]; 
    }
    p_buf[j] = '\0';
 }

 return(p_buf);
}

/*
   trim left and right whitespace
*/
char *trim(char *p_buf)
{
 return(ltrim(rtrim(p_buf)));
}

/*
  return elapsed time 
*/
double get_elapsed_time(const struct timeval *p_start, 
                        const struct timeval *p_end)
{
  return(p_end->tv_sec - p_start->tv_sec +
         (1.0e-6) * p_end->tv_usec - (1.0e-6) * p_start->tv_usec);
}
