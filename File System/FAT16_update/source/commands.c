#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
#include "fat16.h"
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>

/*
 * Função de busca na pasta raíz. Codigo original do professor,
 * altamente modificado.
 *
 * Ela itera sobre todas as bpb->possible_rentries do struct fat_dir* dirs, e
 * retorna a primeira entrada com nome igual à filename.
 */
struct far_dir_searchres find_in_root(FILE *fp, struct fat_dir *dirs, char *filename, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = { .found = false };

    // Obter o primeiro cluster do diretório raiz a partir do BPB
    uint32_t cluster = bpb->root_cluster;

    // Calcular o endereço de início da região de dados
    uint32_t data_start = bpb_froot_addr(bpb);

    // Percorrer os clusters do diretório raiz
    while (cluster < 0xFFFFFFF8) { // Verifica se o cluster não é de fim de cadeia (valores especiais)

        // Calcular o endereço real do cluster
        uint32_t real_cluster_address = data_start + (cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;

        // Ler as entradas do diretório
        if (read_bytes(fp, real_cluster_address, dirs, bpb->bytes_p_sect * bpb->sector_p_clust) != RB_OK) {
            fprintf(stderr, "Erro ao ler cluster %u\n", cluster);
            break; // Saia do loop em caso de erro
        }

        // Iterar sobre as entradas do diretório
        for (size_t i = 0; i < (bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)); i++) {
            if (dirs[i].name[0] == '\0') continue; // Ignorar entradas vazias

            // Verificar se o nome do arquivo corresponde ao que foi buscado
            if (memcmp((char *) dirs[i].name, filename, FAT16STR_SIZE) == 0) {
                res.found = true;
                res.fdir  = dirs[i];
                res.idx   = i;
                break;
            }
        }

        // Obter o próximo cluster da tabela FAT
        cluster = get_next_cluster_from_fat(fp, bpb, cluster); // Agora passa 'fp' para a função
        if (res.found) break; // Se encontrou o arquivo, parar a busca
    }

    return res;
}


/*
 * Função de ls
 *
 * Ela itéra todas as bpb->possible_rentries do diretório raiz
 * e as lê via read_bytes().
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    // Verificar se o arquivo foi aberto corretamente
    if (fp == NULL) {
        error_at_line(0, errno, __FILE__, __LINE__, "error: failed to open disk image");
        return NULL;
    }

    // Inicializa a lista de diretórios
    struct fat_dir *dirs = malloc(sizeof(struct fat_dir) * 128);  // Aloca espaço para um número inicial de diretórios
    if (dirs == NULL) {
        error_at_line(0, errno, __FILE__, __LINE__, "error: memory allocation failed");
        return NULL;
    }
    
    size_t dir_count = 0;

    // Começar a ler a partir do primeiro cluster do diretório raiz
    uint32_t current_cluster = bpb->root_cluster;
    uint32_t data_start = bpb->reserved_sect + (bpb->n_fat * bpb->sect_per_fat);

    // Verifica se a posição inicial é válida
    if (data_start >= bpb->large_n_sects) {
        free(dirs);
        error_at_line(0, errno, __FILE__, __LINE__, "error: invalid data start sector");
        return NULL;
    }

    // Verifica o tamanho do arquivo
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);

    while (current_cluster != 0) {
        // Calcular o endereço real do cluster
        uint32_t cluster_address = (bpb_fdata_cluster_count(bpb) - 2) * bpb->sector_p_clust;


        // Verificar se o endereço calculado está dentro dos limites do arquivo
        if (cluster_address >= file_size) {
            free(dirs);
            error_at_line(0, errno, __FILE__, __LINE__, "error: cluster address exceeds file size. Cluster address: %u, File size: %ld", cluster_address, file_size);
            return NULL;
        }

        // Lê as entradas de diretório a partir do cluster atual
        size_t i = 0;
        while (i < bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)) {
            // Calcula o offset para cada entrada do diretório
            uint32_t offset = cluster_address + i * sizeof(struct fat_dir);
            struct fat_dir entry;

            // Chama a função de leitura
            if (read_bytes(fp, offset, &entry, sizeof(entry)) == RB_ERROR) {
                // Se ocorrer erro, sai da função
                free(dirs);
                return NULL;
            }

            // Verifica se a entrada é válida
            if (entry.name[0] == 0x00) {
                break;  // Fim das entradas do diretório
            }

            // Armazena a entrada no buffer de diretórios
            dirs[dir_count++] = entry;

            // Se a lista de diretórios estiver cheia, aumenta o espaço alocado
            if (dir_count % 128 == 0) {
                dirs = realloc(dirs, sizeof(struct fat_dir) * (dir_count + 128));
                if (dirs == NULL) {
                    error_at_line(0, errno, __FILE__, __LINE__, "error: memory reallocation failed");
                    return NULL;
                }
            }

            i++;
        }

        // Verifica o próximo cluster através da tabela FAT
        current_cluster = get_next_cluster_from_fat(fp, bpb, current_cluster);
    }

    return dirs;
}


void mv(FILE *fp, char *source, char* dest, struct fat_bpb *bpb)
{
    char source_rname[FAT16STR_SIZE_WNULL], dest_rname[FAT16STR_SIZE_WNULL];

    // Converte os nomes dos arquivos de C-String para o formato usado pelo FAT32
    bool badname = cstr_to_fat16wnull(source, source_rname)
                || cstr_to_fat16wnull(dest,   dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t data_start = bpb_froot_addr(bpb);
    // Localiza o primeiro cluster do diretório raiz (FAT32 trata o diretório raiz como um arquivo)
    uint32_t root_cluster = bpb->root_cluster;
    uint32_t root_address = data_start + (root_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;

    // Lê o diretório raiz (potencialmente com múltiplos clusters)
    struct fat_dir *root = ls(fp, bpb);  // Função ls adaptada para FAT32

    // Busca os arquivos source e dest no diretório raiz
    struct far_dir_searchres dir1 = find_in_root(fp,root, source_rname, bpb);
    struct far_dir_searchres dir2 = find_in_root(fp,root, dest_rname, bpb);

    // Verifica se o destino já existe (não pode substituir)
    if (dir2.found == true)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via mv.", dest);

    // Verifica se o arquivo de origem existe
    if (dir1.found == false)
        error(EXIT_FAILURE, 0, "Não foi possivel encontrar o arquivo %s.", source);

    // A partir daqui, o arquivo já existe, e vamos mover ele para o destino.
    
    // 1. Remove o arquivo de origem do diretório (marca como deletado ou apaga entrada)
    memset(&dir1.fdir, 0, sizeof(struct fat_dir));  // Marca o arquivo como apagado (entry vazio)

    uint32_t source_address = root_address + dir1.idx * sizeof(struct fat_dir);
    fseek(fp, source_address, SEEK_SET);
    fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp); // Apaga o arquivo do diretório de origem

    // 2. Adiciona a entrada de destino no diretório raiz
    memcpy(dir1.fdir.name, dest_rname, FAT16STR_SIZE_WNULL);  // Copia o novo nome para o diretório
    dir1.fdir.starting_cluster = dir1.fdir.starting_cluster;  // Mantém o mesmo cluster do arquivo (não muda)

    // Adiciona o arquivo movido ao diretório de destino
    struct far_dir_searchres dir_dest = find_in_root(fp,root, dest_rname, bpb);
    uint32_t dest_address = root_address + dir_dest.idx * sizeof(struct fat_dir);

    fseek(fp, dest_address, SEEK_SET);
    fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);  // Adiciona o arquivo no diretório de destino

    // Agora, o arquivo foi movido. Verifica se a tabela FAT precisa ser atualizada.

    printf("mv %s → %s.\n", source, dest);
}


void rm(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    char fat16_rname[FAT16STR_SIZE_WNULL];

    // Converte o nome do arquivo para o formato FAT32.
    if (cstr_to_fat16wnull(filename, fat16_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Localiza o primeiro cluster do diretório raiz (FAT32 trata o diretório raiz como um arquivo).
    uint32_t root_cluster = bpb->root_cluster;
    uint32_t data_start = bpb_froot_addr(bpb);
    uint32_t root_address = data_start + (root_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;

    // Lê o diretório raiz, que pode estar fragmentado em múltiplos clusters.
    struct fat_dir *root = ls(fp, bpb);  // Função ls adaptada para FAT32.

    // Encontra a entrada do diretório correspondente ao arquivo.
    struct far_dir_searchres dir = find_in_root(fp,root, fat16_rname, bpb);

    // Verifica se o arquivo foi encontrado.
    if (dir.found == false)
    {
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", filename);
    }

    // Marca a entrada como livre.
    dir.fdir.name[0] = DIR_FREE_ENTRY; // DIR_FREE_ENTRY é o valor que indica que a entrada está livre.

    // Calcula o endereço da entrada do diretório a ser deletada.
    uint32_t file_address = root_address + dir.idx * sizeof(struct fat_dir);

    // Escreve a entrada atualizada de volta ao disco.
    fseek(fp, file_address, SEEK_SET);
    fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

    // Leitura da tabela FAT32 (endereços de clusters).
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t cluster_number = dir.fdir.starting_cluster;
    uint32_t null = 0x00000000;  // O valor para indicar que o cluster está livre (em FAT32).
    size_t count = 0;

    // Continua a zerar os clusters até chegar no End Of File (FAT32 usa 0x0FFFFFFF para EOF).
    while (cluster_number < 0x0FFFFFF8)  // 0x0FFFFFF8 é o valor que indica o fim de uma cadeia de clusters.
    {
        uint32_t infat_cluster_address = fat_address + cluster_number * sizeof(uint32_t);
        read_bytes(fp, infat_cluster_address, &cluster_number, sizeof(uint32_t));

        // Setar o cluster number como NULL (0x00000000).
        fseek(fp, infat_cluster_address, SEEK_SET);
        fwrite(&null, sizeof(uint32_t), 1, fp);

        count++;
    }

    printf("rm %s, %zu clusters apagados.\n", filename, count);

    return;
}


struct fat16_newcluster_info fat16_find_free_cluster(FILE* fp, struct fat_bpb* bpb)
{

	/* Essa implementação de FAT16 não funciona com discos grandes. */
	assert(bpb->large_n_sects == 0);

	uint16_t cluster        = 0x0;
	uint32_t fat_address    = bpb_faddress(bpb);
	uint32_t total_clusters = bpb_fdata_cluster_count(bpb);

	for (cluster = 0x2; cluster < total_clusters; cluster++)
	{
		uint16_t entry;
		uint32_t entry_address = fat_address + cluster * 2;

		(void) read_bytes(fp, entry_address, &entry, sizeof (uint16_t));

		if (entry == 0x0)
			return (struct fat16_newcluster_info) { .cluster = cluster, .address = entry_address };
	}

	return (struct fat16_newcluster_info) {0};
}
void cp(FILE *fp, char* source, char* dest, struct fat_bpb *bpb)
{
    char source_rname[FAT16STR_SIZE_WNULL], dest_rname[FAT16STR_SIZE_WNULL];

    // Converte os nomes de arquivo para o formato FAT32.
    if (cstr_to_fat16wnull(source, source_rname) || cstr_to_fat16wnull(dest, dest_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Localiza o primeiro cluster do diretório raiz.
    uint32_t root_cluster = bpb->root_cluster;
    uint32_t data_start = bpb_froot_addr(bpb);
    uint32_t root_address = data_start + (root_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;

    // Lê o diretório raiz, que pode estar fragmentado em múltiplos clusters.
    struct fat_dir *root = ls(fp, bpb);  // Função ls adaptada para FAT32.

    // Encontra a entrada do diretório correspondente ao arquivo fonte.
    struct far_dir_searchres dir1 = find_in_root(fp,root, source_rname, bpb);
    if (!dir1.found)
    {
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", source);
    }

    // Verifica se o arquivo destino já existe.
    if (find_in_root(fp,root, dest_rname, bpb).found)
    {
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via cp.", dest);
    }

    // Cria uma nova entrada de diretório para o arquivo de destino.
    struct fat_dir new_dir = dir1.fdir;
    memcpy(new_dir.name, dest_rname, FAT16STR_SIZE);

    // Procura por uma entrada livre no diretório raiz.
    uint32_t cluster_address = root_address;
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t bytes_read = 0;
    bool dentry_failure = true;
    struct fat_dir *root_cluster_data = malloc(cluster_width);

    while (dentry_failure && bytes_read < bpb->bytes_p_sect * bpb->sector_p_clust)
    {
        // Lê um cluster do diretório raiz.
        read_bytes(fp, cluster_address, root_cluster_data, cluster_width);

        // Procura por uma entrada livre no diretório.
        for (int i = 0; i < cluster_width / sizeof(struct fat_dir); i++)
        {
            if (root_cluster_data[i].name[0] == DIR_FREE_ENTRY || root_cluster_data[i].name[0] == '\0')
            {
                uint32_t dest_address = (cluster_address - data_start) + (i * sizeof(struct fat_dir));

                // Escreve a nova entrada de diretório.
                fseek(fp, dest_address, SEEK_SET);
                fwrite(&new_dir, sizeof(struct fat_dir), 1, fp);

                dentry_failure = false;
                break;
            }
        }

        cluster_address += cluster_width;
        bytes_read += cluster_width;
    }

    if (dentry_failure)
    {
        error_at_line(EXIT_FAILURE, ENOSPC, __FILE__, __LINE__, "Não foi possível alocar uma entrada no diretório raiz.");
    }

    // Agora, aloca-se os clusters para o novo arquivo.

    int count = 0;
    struct fat16_newcluster_info next_cluster, prev_cluster = { .cluster = FAT16_EOF_HI };

    uint32_t cluster_count = dir1.fdir.file_size / bpb->bytes_p_sect / bpb->sector_p_clust + 1;

    // Aloca os clusters, escrevendo-os na FAT.
    while (cluster_count--)
    {
        prev_cluster = next_cluster;
        next_cluster = fat16_find_free_cluster(fp, bpb);

        if (next_cluster.cluster == 0x0)
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Disco cheio (imagem foi corrompida)");

        fseek(fp, next_cluster.address, SEEK_SET);
        fwrite(&prev_cluster.cluster, sizeof(uint16_t), 1, fp);

        count++;
    }

    // Atualiza a entrada de diretório com o primeiro cluster do novo arquivo.
    new_dir.starting_cluster = next_cluster.cluster;

    // Agora faz a cópia dos dados.

    const uint32_t fat_address = bpb_faddress(bpb);
    const uint32_t data_region_start = bpb_fdata_addr(bpb);

    size_t bytes_to_copy = new_dir.file_size;

    uint16_t source_cluster_number = dir1.fdir.starting_cluster;
    uint16_t destin_cluster_number = new_dir.starting_cluster;

    // Itera sobre os clusters do arquivo fonte e copia os dados para o destino.
    while (bytes_to_copy != 0)
    {
        uint32_t source_cluster_address = (source_cluster_number - 2) * cluster_width + data_region_start;
        uint32_t destin_cluster_address = (destin_cluster_number - 2) * cluster_width + data_region_start;

        size_t copied_in_this_cluster = MIN(bytes_to_copy, cluster_width);

        char filedata[cluster_width];

        // Lê os dados do arquivo fonte e escreve no arquivo destino.
        read_bytes(fp, source_cluster_address, filedata, copied_in_this_cluster);
        fseek(fp, destin_cluster_address, SEEK_SET);
        fwrite(filedata, sizeof(char), copied_in_this_cluster, fp);

        bytes_to_copy -= copied_in_this_cluster;

        // Avança para o próximo cluster.
        uint32_t source_next_cluster_address = fat_address + source_cluster_number * sizeof(uint32_t);
        uint32_t destin_next_cluster_address = fat_address + destin_cluster_number * sizeof(uint32_t);

        read_bytes(fp, source_next_cluster_address, &source_cluster_number, sizeof(uint32_t));
        read_bytes(fp, destin_next_cluster_address, &destin_cluster_number, sizeof(uint32_t));
    }

    printf("cp %s → %s, %i clusters copiados.\n", source, dest, count);

    return;
}


void cat(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    char fat32_rname[FAT16STR_SIZE_WNULL];

    // Converte o nome do arquivo para o formato FAT32.
    if (cstr_to_fat16wnull(filename, fat32_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Localiza o primeiro cluster do diretório raiz (FAT32 trata o diretório raiz como um arquivo).
    uint32_t root_cluster = bpb->root_cluster;
    uint32_t data_start = bpb_froot_addr(bpb);
    uint32_t root_address = data_start + (root_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;

    // Lê o diretório raiz, que pode estar fragmentado em múltiplos clusters.
    struct fat_dir *root = ls(fp, bpb);  // Função ls adaptada para FAT32.

    // Encontra a entrada do diretório correspondente ao arquivo.
    struct far_dir_searchres dir = find_in_root(fp,root, fat32_rname, bpb);

    // Verifica se o arquivo foi encontrado.
    if (dir.found == false)
    {
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", filename);
    }

    /*
     * Descobre o tamanho do arquivo.
     */
    size_t bytes_to_read = dir.fdir.file_size;

    /*
     * Endereço da região de dados e da tabela de alocação.
     */
    uint32_t data_region_start = bpb_fdata_addr(bpb);
    uint32_t fat_address = bpb_faddress(bpb);

    /*
     * O primeiro cluster do arquivo está armazenado na estrutura fat_dir.
     */
    uint32_t cluster_number = dir.fdir.starting_cluster;

    const uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

    // Lê os clusters e exibe o conteúdo do arquivo.
    while (bytes_to_read != 0)
    {
        uint32_t cluster_address = (cluster_number - 2) * cluster_width + data_region_start;
        size_t read_in_this_cluster = MIN(bytes_to_read, cluster_width);

        char filedata[cluster_width];

        // Lê o cluster atual.
        read_bytes(fp, cluster_address, filedata, read_in_this_cluster);
        printf("%.*s", (signed)read_in_this_cluster, filedata);

        bytes_to_read -= read_in_this_cluster;

        // Calcula o próximo cluster da cadeia de clusters.
        uint32_t next_cluster_address = fat_address + cluster_number * sizeof(uint32_t);

        // Lê o próximo número de cluster.
        read_bytes(fp, next_cluster_address, &cluster_number, sizeof(uint32_t));

        // Em FAT32, um valor de cluster >= 0x0FFFFFF8 indica o final da cadeia de clusters (EOF).
        if (cluster_number >= 0x0FFFFFF8)
        {
            break;
        }
    }

    return;
}
