#include "output.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct pretty_int { int num; char* suff; } pretty_print(int value)
{

	static char* B   = "B";
	static char* KiB = "KiB";
	static char* MiB = "MiB";

	struct pretty_int res =
	{
		.num  = value,
		.suff = B
	};

	if (res.num == 0) return res;

	if (res.num >= 1024)
	{
		res.num  /= 1024;
		res.suff = KiB;
	}

	if (res.num >= 1024)
	{
		res.num  /= 1024;
		res.suff = MiB;
	}

	return res;
}

void show_files(struct fat_dir *dirs)
{

	struct fat_dir *cur;

	fprintf(stdout, "ATTR  NAME    FMT    SIZE\n-------------------------\n");

    while ((cur = dirs++) != NULL)
	{

		if (cur->name[0] == 0)
			break;

        else if ((cur->name[0] == DIR_FREE_ENTRY) || (cur->attr == DIR_FREE_ENTRY))
            continue;

		else if (cur->attr == 0xf)
			continue;

		struct pretty_int num = pretty_print(cur->file_size);

        fprintf(stdout, "0x%-.*x  %.*s  %4i %s\n", 2, cur->attr, FAT16STR_SIZE, cur->name, num.num, num.suff);
    }

    return;
}

void verbose(struct fat_bpb *bios_pb)
{
    // Exibe informações sobre o BIOS Parameter Block (BPB)
    fprintf(stdout, "Bloco de parâmetros do BIOS:\n");
    fprintf(stdout, "Instrução de salto: ");

    for (int i = 0; i < 3; i++)
        fprintf(stdout, "%hhX ", bios_pb->jmp_instruction[i]); // Exibe os bytes da instrução de salto

    fprintf(stdout, "\n");

    fprintf(stdout, "ID do OEM: %s\n", bios_pb->oem_id);
    fprintf(stdout, "Bytes por setor: %d\n", bios_pb->bytes_p_sect);
    fprintf(stdout, "Setores por cluster: %d\n", bios_pb->sector_p_clust);
    fprintf(stdout, "Setores reservados: %d\n", bios_pb->reserved_sect);
    fprintf(stdout, "Número de cópias da FAT: %d\n", bios_pb->n_fat);
    fprintf(stdout, "Número máximo de entradas no diretório raiz: %d\n", bios_pb->root_entry_count);
    // fprintf(stdout, "Total de setores (16 bits): %d\n", bios_pb->total_sectors_16);
    // fprintf(stdout, "Setores por FAT (16 bits): %d\n", bios_pb->sect_per_fat_16);
    fprintf(stdout, "Descritor de mídia: %hhX\n", bios_pb->media_desc);
    
    fprintf(stdout, "Setores por trilha: %d\n", bios_pb->sect_per_track);
    fprintf(stdout, "Número de cabeças: %d\n", bios_pb->number_of_heads);
    fprintf(stdout, "Setores ocultos: %d\n", bios_pb->hidden_sects);
    fprintf(stdout, "Total de setores (32 bits): %d\n", bios_pb->total_sectors_32);
    fprintf(stdout, "Setores por FAT (32 bits): %d\n", bios_pb->sect_per_fat_32);
    fprintf(stdout, "Cluster raiz (FAT32): %d\n", bios_pb->root_cluster);
    fprintf(stdout, "Setor com informações do sistema de arquivos: %d\n", bios_pb->fs_info);
    fprintf(stdout, "Setor de backup do boot: %d\n", bios_pb->backup_boot_sector);

    fprintf(stdout, "Endereço da FAT: 0x%x\n", bpb_faddress(bios_pb));
    fprintf(stdout, "Endereço do diretório raiz: 0x%x\n", bpb_froot_addr(bios_pb));
    fprintf(stdout, "Endereço da área de dados: 0x%x\n", bpb_fdata_addr(bios_pb));

    return;
}
