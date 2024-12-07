#include "fat16.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <err.h>

/* calculate FAT address */
uint32_t bpb_faddress(struct fat_bpb *bpb)
{
	return bpb->reserved_sect * bpb->bytes_p_sect;
}

/* calculate FAT root address */
uint32_t bpb_froot_addr(struct fat_bpb *bpb)
{
	return bpb_faddress(bpb) + bpb->n_fat * bpb->sect_per_fat * bpb->bytes_p_sect;
}

/* calculate data address */
uint32_t bpb_fdata_addr(struct fat_bpb *bpb)
{
    // Calcular o endereço de início da região de dados
    uint32_t data_start = bpb->reserved_sect + (bpb->n_fat * bpb->sect_per_fat);
    // O primeiro cluster do diretório raiz
    uint32_t root_cluster = bpb->root_cluster;
    // Calcular o endereço físico real para o primeiro cluster do diretório raiz
    uint32_t root_dir_address = data_start + (root_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;
    return root_dir_address;
}

/* calculate data sector count */
uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb)
{
	return bpb->large_n_sects - bpb_fdata_addr(bpb) / bpb->bytes_p_sect;
}


uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb) {
    // Total de setores da partição - (setores reservados + setores das FATs)
    uint32_t total_sectors = (bpb->large_n_sects == 0) ? bpb->snumber_sect : bpb->large_n_sects;
    uint32_t fat_size = (bpb->sect_per_fat == 0) ? bpb->sect_per_fat32 : bpb->sect_per_fat;
    uint32_t data_sectors = total_sectors - (bpb->reserved_sect + (bpb->n_fat * fat_size));
    // Retorna o número de clusters no volume
    return data_sectors / bpb->sector_p_clust;
}


/*
 * allows reading from a specific offset and writting the data to buff
 * returns RB_ERROR if seeking or reading failed and RB_OK if success
 */
int read_bytes(FILE *fp, unsigned int offset, void *buff, unsigned int len)
{

	if (fseek(fp, offset, SEEK_SET) != 0)
	{
		error_at_line(0, errno, __FILE__, __LINE__, "warning: error when seeking to %u", offset);
		return RB_ERROR;
	}
	if (fread(buff, 1, len, fp) != len)
	{
		error_at_line(0, errno, __FILE__, __LINE__, "warning: error reading file");
		return RB_ERROR;
	}
	return RB_OK;
}

uint32_t get_next_cluster_from_fat(FILE *fp, struct fat_bpb *bpb, uint32_t cluster) {
    // Calcula o endereço de início da FAT
    uint32_t fat_start = bpb_faddress(bpb);

    // Calcula o deslocamento do cluster dentro da FAT
    uint32_t fat_offset = cluster * 4; // 4 bytes por entrada no FAT32

    // Calcula o endereço real na tabela FAT
    uint32_t fat_address = fat_start + fat_offset;

    // Buffer para armazenar o valor do próximo cluster
    uint32_t next_cluster;

    // Lê o próximo cluster da FAT
    if (read_bytes(fp, fat_address, &next_cluster, sizeof(next_cluster)) != RB_OK) {
        fprintf(stderr, "Erro ao obter o próximo cluster na FAT para cluster %u\n", cluster);
        return 0xFFFFFFFF; // Retorna um valor inválido para indicar erro
    }

    // Aplica a máscara para descartar os 4 bits reservados
    return next_cluster & 0x0FFFFFFF;
}
/* read fat16's bios parameter block */
void rfat(FILE *fp, struct fat_bpb *bpb)
{

	read_bytes(fp, 0x0, bpb, sizeof(struct fat_bpb));

	return;
}





