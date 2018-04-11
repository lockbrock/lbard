#include <strings.h>

#include "fakecsmaradio.h"

unsigned char barrett_e0_string[6]={0x13,'E','0',13,10,0x11};

int hfbarrett_read_byte(int i,unsigned char c)
{
  if (c==0x15) {
    // Control-U -- clear input buffer
    clients[i].buffer_count=0;
  } else if ((c!='\n')&&(c!='\r')&&c) {
    // fprintf(stderr,"Radio #%d received character 0x%02x\n",i,c);

    // First echo the character back
    write(clients[i].socket,&c,1);
    
    if (clients[i].buffer_count<(CLIENT_BUFFER_SIZE-1))
      clients[i].buffer[clients[i].buffer_count++]=c;
  } else {
    if (clients[i].buffer_count) {

      // Print CRLF
      write(clients[i].socket,"\r\n",2);
      
      clients[i].buffer[clients[i].buffer_count]=0;
      fprintf(stderr,"Barrett HF Radio #%d sent command '%s'\n",i,clients[i].buffer);

      // Process the command here
      if (!strncasecmp("AXNMSG",(char *)clients[i].buffer,6)) {
	// Send ALE message
	// XXX- implement me!
      } else {
	// Complain about unknown commands
	write(clients[i].socket,barrett_e0_string,6);
      }

      // Reset buffer ready for next command
      clients[i].buffer_count=0;
    }    
  }

  return 0;
}
