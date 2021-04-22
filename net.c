#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  int n = 0;
  while (n < len){
    int r = read(fd, buf, len - n);
    if (r <= 0){
      return false;
    }
    n += r;
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  int n = 0;
  while (n < len){
    int r = write(fd, buf, len - n);
    if (r <= 0){
      return false;
    }
    n += r;
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  uint8_t header[HEADER_LEN];
  uint16_t len;
  int offset = 0;
  if (nread(fd, HEADER_LEN, header) == true){
    /*check contents of header to see if whole block needs to be read */
    memcpy(&len, header + offset, sizeof(len));
    offset += sizeof(len);
    memcpy(op, header + offset, sizeof(*op));
    offset += sizeof(*op);
    memcpy(ret, header + offset, sizeof(*ret));
    /* convert network value to host value*/
    len = ntohs(len);
    *op = ntohl(*op);
    *ret = ntohs(*ret);
    if (len == HEADER_LEN){
      return true;
    }
    if(len == HEADER_LEN + JBOD_BLOCK_SIZE){
      nread(fd, JBOD_BLOCK_SIZE, block);
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
  return 0;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  /*if it is a write block then increase the length of the packet*/
  jbod_cmd_t opCode = op >> 26;
  uint8_t header[HEADER_LEN];
  int offset = 0;
  uint16_t ret;
  uint16_t len;
  if(nwrite(sd, HEADER_LEN, header) == true){
    len = htons(len);
    op = htonl(op);
    ret = htons(ret);
    memcpy(&len, header + offset, sizeof(len));
    offset += sizeof(len);
    memcpy(op, header + offset, sizeof(op));
    offset += sizeof(op);
    memcpy(ret, header + offset, sizeof(ret));
    if(op == JBOD_WRITE_BLOCK){
      nwrite(sd, HEADER_LEN + JBOD_BLOCK_SIZE, block);
      return true;
    }
    if(op == JBOD_SEEK_TO_BLOCK || JBOD_SEEK_TO_DISK){
      return true;
    }
    if (len == HEADER_LEN){
      return true;
    }
    if(len == HEADER_LEN + JBOD_BLOCK_SIZE){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
  return 0;
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  int sock_id;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_aton(ip, &(addr.sin_addr)) == 0){
    return false;
  }
  sock_id = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(sock_id, (const struct sockaddr *)&addr, sizeof(addr)) == -1){
    return false;
  }
  else{
    cli_sd = sock_id;
    return true;
  }
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
  /* new variable for op and ret*/
  uint16_t ret;
  uint32_t op1;
  if(cli_sd != -1){
    err();
  }
  if(!send_packet(cli_sd, op, block)){
    
  }
  if(!recv_packet(cli_sd, op1, ret, block)){

  }
  
}


