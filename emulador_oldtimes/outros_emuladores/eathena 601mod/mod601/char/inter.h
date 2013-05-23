#ifndef _INTER_H_
#define _INTER_H_

int inter_init(const char *file);
int inter_save();
int inter_parse_frommap(int fd);

int inter_check_length(int fd,int length);

#define inter_cfgName "conf/inter_athena.conf"

#endif
