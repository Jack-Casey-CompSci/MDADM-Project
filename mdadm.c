#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"
#include "net.h"
int rc = 0;
uint32_t sort_op(jbod_cmd_t com, int disk_id, int block_id)
{ uint32_t op;
  op = com << 26 | disk_id << 22 | block_id;
  return op;
}

int mdadm_mount(void) {
uint32_t op = sort_op(JBOD_MOUNT, 0, 0);
  if(rc == 0){
  jbod_client_operation(op, NULL);
  rc = 1;
  return 1;
  }
  else{
    return -1;
  }
  return 0;
}

int mdadm_unmount(void) {
  uint32_t op = sort_op(JBOD_UNMOUNT, 0, 0);
  if(rc == 1){
    jbod_client_operation(op, NULL);
    rc = 0;
    return 1;
  }
  else{
    return -1;
  }
  return 0;
}
void translate_addr(uint32_t lin_addr, int *disk_num, int *block_num, int* offset){
  int disk_addr;
  *disk_num = lin_addr / JBOD_DISK_SIZE;
  disk_addr = lin_addr % JBOD_DISK_SIZE;
  *block_num = disk_addr / JBOD_BLOCK_SIZE;
  *offset = disk_addr % JBOD_BLOCK_SIZE;
}

void seek(int disk_num, int block_num){
  uint32_t op1 = sort_op(JBOD_SEEK_TO_DISK, disk_num, 0);
  uint32_t op2 = sort_op(JBOD_SEEK_TO_BLOCK, 0, block_num);
  jbod_client_operation(op1, NULL);
  jbod_client_operation(op2, NULL);
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  int disk_num, block_num, offset;
  uint32_t op = sort_op(JBOD_READ_BLOCK, 0, 0);
  if(addr > 1048574){
    return -1;
  }
  if(addr + len > 1048574){
    return -1;
  }
  if(len > 1024){
    return -1;
  }
  if(len != 0 && buf == NULL){
    return -1;
  }
  if(rc == 0){
    return -1;
  }
  int curr_addr = addr;
  int curr_len = len;
while(curr_addr < addr + len){
    //translate address and seek                                             
    translate_addr(curr_addr, &disk_num, &block_num, &offset);
    seek(disk_num, block_num);
    uint8_t buf1[JBOD_BLOCK_SIZE];
    if(cache_enabled() == true){
      if(cache_lookup(disk_num, block_num, buf1) == -1){
        jbod_client_operation(op, buf1);
        cache_insert(disk_num, block_num, buf1);
      }
      else{
        //read                                                                   
    jbod_client_operation(op, buf1);
    if(curr_addr == addr){ //first block                                       
      if(offset + len > JBOD_BLOCK_SIZE){//if the offset + len > the 0th block read/copy only whats in the 0th block then move on to the next block       
        memcpy(buf, buf1 + offset, JBOD_BLOCK_SIZE - offset);
	      curr_addr += (JBOD_BLOCK_SIZE - offset);
        curr_len -= curr_addr - addr;
      }
      else{ //if its less than the the size of the 0th block copy starting at offset until len                                                       
        memcpy(buf, buf1 + offset, len);
	      curr_addr += len;
          }

    }
      else if(addr + len - curr_addr <= JBOD_BLOCK_SIZE){//last block            
      memcpy(buf + curr_addr - addr, buf1, curr_len);
      curr_addr += curr_len;

    }
    else{ //normal                                                 
      memcpy(buf + curr_addr - addr, buf1, JBOD_BLOCK_SIZE);
      curr_addr += JBOD_BLOCK_SIZE;
      curr_len -= JBOD_BLOCK_SIZE;
    }
      }
    }
    else{
    //read                                                                   
    jbod_client_operation(op, buf1);
    if(curr_addr == addr){ //first block                                       
      if(offset + len > JBOD_BLOCK_SIZE){//if the offset + len > the 0th block read/copy only whats in the 0th block then move on to the next block       
        memcpy(buf, buf1 + offset, JBOD_BLOCK_SIZE - offset);
	      curr_addr += (JBOD_BLOCK_SIZE - offset);
        curr_len -= curr_addr - addr;
      }
      else{ //if its less than the the size of the 0th block copy starting at offset until len                                                       
        memcpy(buf, buf1 + offset, len);
	      curr_addr += len;
          }

    }
      else if(addr + len - curr_addr <= JBOD_BLOCK_SIZE){//last block            
      memcpy(buf + curr_addr - addr, buf1, curr_len);
      curr_addr += curr_len;

    }
    else{ //normal                                                 
      memcpy(buf + curr_addr - addr, buf1, JBOD_BLOCK_SIZE);
      curr_addr += JBOD_BLOCK_SIZE;
      curr_len -= JBOD_BLOCK_SIZE;
    }
    }
    

  }
  return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  int disk_num, block_num, offset;
  uint32_t op = sort_op(JBOD_READ_BLOCK, 0, 0);
  if(addr > 1048576){
    return -1;
  }
  if(addr + len > 1048576){
    return -1;
  }
  if(len > 1024){
    return -1;
  }
  if(len != 0 && buf == NULL){
    return -1;
  }
  if(rc == 0){
    return -1;
  }
  int curr_addr = addr;
  int curr_len = len;
while(curr_addr < addr + len){
    //translate address and seek                                             
    translate_addr(curr_addr, &disk_num, &block_num, &offset);
    seek(disk_num, block_num);
    uint8_t buf1[JBOD_BLOCK_SIZE];
    //write
    jbod_client_operation(op, buf1);
    if(curr_addr == addr){ //first block                                                                                                              
      if(offset + len > JBOD_BLOCK_SIZE){
      //if the offset + len > the 0th block read/copy only whats in the 0th block then move on to the next block  
        memcpy(buf1 + offset, buf, JBOD_BLOCK_SIZE - offset);
        curr_addr += (JBOD_BLOCK_SIZE - offset);
        curr_len -= curr_addr - addr;
      }
      else{ //if its less than the the size of the 0th block copy starting at offset until len                                                       
        memcpy(buf1 + offset, buf, len);
        curr_addr += len;
          } 

    }
      else if(addr + len - curr_addr <= JBOD_BLOCK_SIZE){//last block      
      memcpy(buf1, buf + curr_addr - addr, curr_len);
      curr_addr += curr_len;

 }
    else{ //normal block                                                      
      memcpy(buf1, buf + curr_addr - addr, JBOD_BLOCK_SIZE);
      curr_addr += JBOD_BLOCK_SIZE;
      curr_len -= JBOD_BLOCK_SIZE;
    }
    uint32_t op1 = sort_op(JBOD_WRITE_BLOCK, 0, 0);
    seek(disk_num, block_num);//points to the disk and block num you want to write
    jbod_client_operation(op1, buf1);//write down the disk and block data
    if(cache_enabled() == true){
      cache_update(disk_num, block_num, buf1);
    }
}
  return len;
}
