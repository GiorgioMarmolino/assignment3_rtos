 /* readtertest.c
  */
 
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
 
int main() 
{
         int fd, result;
	 int len = 11;
         char buf[len];

	 //The first call uses the device /dev/simple with minor number 0, the 
	//second call uses the device /dev/simple1 with minor number 1
	//the read and the write must use the same device to exchange 
	//data, since two different devices correspond to different areas of
	//memory

         if ((fd = open ("/dev/simple", O_RDWR )) == -1) {

         //if ((fd = open ("/dev/simple1", O_RDWR)) == -1) {
                  perror("open failed");
                  return -1;
         }

	  if ((result = read (fd, &buf, sizeof(buf))) != len) {
                 perror("read failed");
                 return -1;
          } 
 
          fprintf (stdout, "Read %s \n", buf);
          close(fd);
          return 0;
  
}
