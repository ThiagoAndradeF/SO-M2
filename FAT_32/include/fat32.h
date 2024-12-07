#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5

#define DIR_ATTR_READONLY 1 << 0 /* file is read only */
#define DIR_ATTR_HIDDEN 1 << 1 /* file is hidden */
#define DIR_ATTR_SYSTEM 1 << 2 /* system file (also hidden) */
#define DIR_ATTR_VOLUMEID 1 << 3 /* special entry containing disk volume label */
#define DIR_ATTR_DIRECTORY 1 << 4 /* describes a subdirectory */
#define DIR_ATTR_ARCHIVE 1 << 5 /* archive flag (always set when file is modified */
#define DIR_ATTR_LFN 0xf /* not used */

#define SIG 0xAA55 /* boot sector signature -- sector is executable */

#pragma pack(push, 1)

/* FAT32 Directory Entry */
struct fat_dir {
    unsigned char name[11]; /* Short name + file extension */
    uint8_t attr;           /* file attributes */
    uint8_t ntres;          /* reserved for Windows NT, set to 0 when a file is created */
    uint8_t creation_stamp; /* millisecond timestamp at file creation time */
    uint16_t creation_time; /* time file was created */
    uint16_t creation_date; /* date file was created */
    uint16_t last_access_date; /* last access date (last read/written) */
    uint16_t ea_index;         /* high word of the first cluster (FAT32 only) */
    uint16_t last_write_time;  /* time of last write */
    uint16_t last_write_date;  /* date of last write */
    uint16_t starting_cluster_low; /* low word of starting cluster */
    uint32_t file_size; /* file size in bytes */
};

/* FAT32 Boot Sector and BPB */
struct fat_bpb {
    uint8_t jmp_instruction[3]; /* code to jump to the bootstrap code */
    unsigned char oem_id[8];    /* OEM ID: name of the formatting OS */

    uint16_t bytes_p_sect;      /* bytes per sector */
    uint8_t sector_p_clust;     /* sectors per cluster */
    uint16_t reserved_sect;     /* reserved sectors */
    uint8_t n_fat;              /* number of FAT copies */
    uint16_t unused;            /* zero for FAT32 */
    uint16_t snumber_sect;      /* small number of sectors (if less than 65536) */
    uint8_t media_desc;         /* media descriptor */
    uint16_t unused_sect_per_fat16; /* unused for FAT32 */
    uint16_t sect_per_track;    /* sectors per track */
    uint16_t number_of_heads;   /* number of heads */
    uint32_t hidden_sects;      /* hidden sectors */
    uint32_t large_n_sects;     /* total sectors for volumes > 32MB */

    /* FAT32 specific fields */
    uint32_t sect_per_fat;      /* sectors per FAT */
    uint16_t ext_flags;         /* extended flags */
    uint16_t fs_version;        /* filesystem version */
    uint32_t root_cluster;      /* starting cluster of the root directory */
    uint16_t fs_info;           /* sector number of the FSINFO structure */
    uint16_t backup_boot_sector; /* backup boot sector location */
    uint8_t reserved[12];       /* reserved for future use */
    uint8_t drive_number;       /* drive number (used by BIOS) */
    uint8_t reserved1;          /* reserved */
    uint8_t boot_signature;     /* extended boot signature (0x29 if present) */
    uint32_t volume_id;         /* volume ID (serial number) */
    unsigned char volume_label[11]; /* volume label */
    unsigned char fs_type[8];   /* filesystem type ("FAT32   ") */
};

#pragma pack(pop)

/* Prototypes for reading and manipulating FAT32 */
int read_bytes(FILE *, unsigned int, void *, unsigned int);
void rfat(FILE *, struct fat_bpb *);

/* Prototypes for calculating FAT32 offsets and addresses */
uint32_t bpb_fat_address(struct fat_bpb *);
uint32_t bpb_root_dir_address(struct fat_bpb *);
uint32_t next_cluster(FILE *fp, struct fat_bpb *bpb, uint32_t cluster);
uint32_t bpb_data_address(struct fat_bpb *);
uint32_t bpb_data_sector_count(struct fat_bpb *);
uint32_t bpb_data_cluster_count(struct fat_bpb *bpb);
uint32_t bpb_froot_addr(struct fat_bpb *bpb);  // Apenas a declaração, sem definição.
uint32_t bpb_faddress(struct fat_bpb *);
// Função que encontra um cluster livre e retorna informações sobre ele
uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb);
uint32_t bpb_fdata_addr(struct fat_bpb *bpb);
uint32_t bpb_fdata_sector_count(struct fat_bpb *);
// Função para calcular o endereço físico de um cluster (FAT32)
uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb);
uint32_t cluster_to_address(uint32_t cluster, struct fat_bpb *bpb);
uint32_t bpb_fdata_sector_count_s(struct fat_bpb *bpb);

///

#define FAT32STR_SIZE       11
#define FAT32STR_SIZE_WNULL 12

#define RB_ERROR -1
#define RB_OK     0

#define FAT32_EOF_LO 0x0FFFFFF8
#define FAT32_EOF_HI 0x0FFFFFFF

#endif


// Mudanças Realizadas:
// fat_dir:

// Adicionado o campo ea_index para armazenar a parte alta do cluster inicial (necessário em FAT32).
// Dividido starting_cluster em dois campos: starting_cluster_low e ea_index.
// fat_bpb:

// Incluídos campos específicos para FAT32, como:
// sect_per_fat (setores por FAT para volumes grandes).
// root_cluster (cluster inicial do diretório raiz).
// fs_info e backup_boot_sector.
// Definições Específicas para FAT32:

// Alterado o intervalo EOF para FAT32 (0x0FFFFFF8 a 0x0FFFFFFF).
// Adicionados campos específicos para FAT32, como FAT32STR_SIZE e bpb_fat_address().
// Compatibilidade com Arquitetura FAT32:

// Reservei espaço para campos não usados em FAT16, como ext_flags e fs_version.