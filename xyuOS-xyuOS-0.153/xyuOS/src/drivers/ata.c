#include "ata.h"
#include <stdint.h>

/* ATA I/O ports (primary bus) */
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECCOUNT   0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HI     0x1F5
#define ATA_DRIVE     0x1F6
#define ATA_STATUS    0x1F7
#define ATA_COMMAND   0x1F7

/* Status bits */
#define ATA_SR_BSY     0x80
#define ATA_SR_DRQ     0x08
#define ATA_SR_ERR     0x01

/* Commands */
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

static inline void io_wait(void){
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

static inline uint8_t inb(uint16_t port){
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline void outb(uint16_t port, uint8_t val){
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port){
    uint16_t r;
    __asm__ volatile("inw %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline void outw(uint16_t port, uint16_t val){
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Wait until BSY=0 and DRQ=1 */
static int ata_wait(void){
    uint8_t s;
    while((s = inb(ATA_STATUS)) & ATA_SR_BSY);
    if(s & ATA_SR_ERR) return -1;
    while(!(inb(ATA_STATUS) & ATA_SR_DRQ));
    return 0;
}

/* Public API */

void ata_init(void){
    /* select master */
    outb(ATA_DRIVE, 0xE0);
    io_wait();
}

/* Read one 512-byte sector */
int ata_read_sector(uint32_t lba, uint8_t* buf){
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ);

    if(ata_wait() != 0) return -1;

    for(int i=0;i<256;i++){
        uint16_t w = inw(ATA_DATA);
        buf[i*2]     = (uint8_t)(w & 0xFF);
        buf[i*2 + 1] = (uint8_t)(w >> 8);
    }
    return 0;
}

/* Write one 512-byte sector */
int ata_write_sector(uint32_t lba, const uint8_t* buf){
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if(ata_wait() != 0) return -1;

    for(int i=0;i<256;i++){
        uint16_t w = buf[i*2] | ((uint16_t)buf[i*2 + 1] << 8);
        outw(ATA_DATA, w);
    }

    io_wait();
    return 0;
}
