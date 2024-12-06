#include "fat32.h"
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <err.h>

/* calcular endereço da FAT */
uint32_t bpb_fat_address(struct fat_bpb *bpb) {
    return bpb->reserved_sect * bpb->bytes_p_sect;
}
/* calculate FAT root address */
uint32_t bpb_froot_addr(struct fat_bpb *bpb) {
    return bpb_data_address(bpb) + (bpb->root_cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
}
uint32_t bpb_faddress(struct fat_bpb *bpb)
{
	return bpb->reserved_sect * bpb->bytes_p_sect;
}
/* calcular endereço do diretório raiz */
uint32_t cluster_to_address(uint32_t cluster, struct fat_bpb *bpb)
{
    // Calcular o offset do FAT para o cluster especificado (FAT32 usa 4 bytes por entrada)
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_address = bpb_fat_address(bpb) + fat_offset;  // Local do FAT no disco

    // Calcular o endereço de dados para o cluster (em relação ao número de setores e clusters)
    uint32_t data_offset = (cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
    uint32_t data_address = bpb_data_address(bpb) + data_offset;

    return data_address;  // Retorna o endereço físico do cluster
}

uint32_t bpb_root_dir_address(struct fat_bpb *bpb) {
    // Calcular o endereço de dados para o cluster da raiz
    return bpb_data_address(bpb) + (bpb->root_cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
}
uint32_t bpb_fdata_addr(struct fat_bpb *bpb) {
    // Considera o endereço dos dados (baseado no setor de dados)
    return bpb_fat_address(bpb) + (bpb->root_cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
}
uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb) {
    // Aqui você pode calcular a quantidade de setores baseando-se nos campos do BPB
    return bpb->large_n_sects - bpb->reserved_sect - (bpb->n_fat * bpb->sect_per_fat);
}
uint32_t bpb_fdata_cluster_count(struct fat_bpb* bpb)
{
	uint32_t sectors = bpb_fdata_sector_count_s(bpb);

	return sectors / bpb->sector_p_clust;
}
uint32_t bpb_fdata_sector_count_s(struct fat_bpb *bpb) {
    // A quantidade de setores de dados seria o total de setores, menos os setores reservados e as FATs
    return bpb->large_n_sects - bpb->reserved_sect - (bpb->n_fat * bpb->sect_per_fat);
}
/* calcular endereço da região de dados */
uint32_t bpb_data_address(struct fat_bpb *bpb) {
    return bpb_fat_address(bpb) + (bpb->n_fat * bpb->sect_per_fat * bpb->bytes_p_sect);
}

/* calcular quantidade de setores de dados */
uint32_t bpb_data_sector_count(struct fat_bpb *bpb) {
    return bpb->large_n_sects - bpb_data_address(bpb) / bpb->bytes_p_sect;
}

/* calcular quantidade de clusters de dados */
uint32_t bpb_data_cluster_count(struct fat_bpb *bpb) {
    uint32_t sectors = bpb_data_sector_count(bpb);
    return sectors / bpb->sector_p_clust;
}

// /* calcular endereço físico de um cluster (FAT32) */
// uint32_t cluster_to_address(uint32_t cluster, struct fat_bpb *bpb) {
//     uint32_t data_offset = (cluster - 2) * bpb->sector_p_clust * bpb->bytes_p_sect;
//     uint32_t data_address = bpb_data_address(bpb) + data_offset;
//     return data_address;
// }

/*
 * lê dados de um offset específico no arquivo
 * retorna RB_ERROR em caso de erro ou RB_OK em caso de sucesso
 */
int read_bytes(FILE *fp, unsigned int offset, void *buff, unsigned int len) {
    if (fseek(fp, offset, SEEK_SET) != 0) {
        error_at_line(0, errno, __FILE__, __LINE__, "Erro ao buscar offset %u", offset);
        return RB_ERROR;
    }
    if (fread(buff, 1, len, fp) != len) {
        error_at_line(0, errno, __FILE__, __LINE__, "Erro ao ler dados do arquivo");
        return RB_ERROR;
    }
    return RB_OK;
}

/* lê o BPB do FAT32 */
void rfat(FILE *fp, struct fat_bpb *bpb) {
    read_bytes(fp, 0x0, bpb, sizeof(struct fat_bpb));
}

/* outras funções auxiliares podem ser implementadas aqui */
