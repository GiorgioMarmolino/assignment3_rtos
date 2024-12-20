 /* writertest.c
  */
 
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
 
int main() 
{
         int fd, result, len;
         char buf[10];
         const char *str;
         
	 // open file for writing

         str = "cavolfiore"; 
	 len = strlen(str)+1;

	 // write str on driver

	 printf("%d bytes written \n", result);

	 // close file
}
