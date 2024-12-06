#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
// #include "fat16.h"
#include "fat32.h"
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>

/*
Para refatorar a função find_in_root para o padrão FAT32, precisamos
garantir que ela funcione de maneira similar, 
mas levando em consideração as diferenças entre FAT16 e FAT32,
como o tamanho do nome do arquivo (agora usando FAT32STR_SIZE),
e o formato de nome do arquivo.
 */
struct far_dir_searchres find_in_root(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = { .found = false };

    // Itera sobre as entradas de diretório
    for (size_t i = 0; i < bpb->n_fat; i++)
    {
        // Ignora entradas livres ou inválidas
        if (dirs[i].name[0] == DIR_FREE_ENTRY || dirs[i].name[0] == '\0')
            continue;

        // Compara o nome do arquivo armazenado com o nome fornecido
        if (memcmp((char *) dirs[i].name, filename, FAT32STR_SIZE) == 0)
        {
            res.found = true;
            res.fdir  = dirs[i];
            res.idx   = i;
            break;
        }
    }

    return res;
}

/*
Número de entradas: A função agora usa bpb->n_fat (que é o número de entradas
no diretório raiz no FAT32), ao invés de bpb->possible_rentries, para iterar sobre as entradas do diretório.
Verificação de entradas livres: As entradas livres são identificadas por DIR_FREE_ENTRY ou por um byte 
nulo ('\0'), indicando que a entrada está disponível.
Comparação dos nomes dos arquivos: A comparação do nome 
do arquivo agora usa FAT32STR_SIZE para garantir que estamos comparando corretamente o tamanho do nome no formato FAT32.
Resultado da pesquisa: Se o nome do arquivo (filename) for encontrado na entrada do diretório
(dirs[i].name), a função define res.found como true, preenche a estrutura res.fdir com a entrada
correspondente e registra o índice da entrada no diretório com res.idx.
Retorno da função: A função retorna a estrutura far_dir_searchres, que contém o resultado da busca. Caso o arquivo não seja 
encontrado, res.found será false, e as demais informações não serão alteradas.
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    // Calcula o endereço do diretório raiz no FAT32.
    uint32_t root_address = bpb_root_dir_address(bpb);

    // Calcula o tamanho do diretório raiz com base no número de entradas de diretório.
    uint32_t root_size = sizeof(struct fat_dir) * bpb->n_fat;

    // Aloca memória para armazenar as entradas do diretório.
    struct fat_dir *dirs = malloc(root_size);
    if (dirs == NULL) {
        error_at_line(EXIT_FAILURE, ENOMEM, __FILE__, __LINE__, "Erro ao alocar memória para diretórios");
    }

    // Lê as entradas do diretório raiz do disco.
    if (read_bytes(fp, root_address, dirs, root_size) == RB_ERROR) {
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler diretório raiz");
    }

    // Aqui, podemos fazer um loop para listar apenas os arquivos válidos (não livres).
    for (int i = 0; i < bpb->n_fat; i++) {
        // Verifica se a entrada está livre ou não (indicado por DIR_FREE_ENTRY ou '\0').
        if (dirs[i].name[0] != DIR_FREE_ENTRY && dirs[i].name[0] != '\0') {
            // Aqui você pode processar os arquivos encontrados, por exemplo, imprimir o nome
            printf("%s\n", dirs[i].name);
        }
    }

    return dirs;
} 

//REFATORAÇÃO MV
// Conversão de nome para FAT32: A função cstr_to_fat32wnull() é usada para converter 
// os nomes de arquivo para o formato adequado no FAT32.
// Leitura do diretório raiz: O diretório raiz é lido usando o endereço calculado pela 
// função bpb_root_dir_address(bpb). Isso permite encontrar as entradas de diretório no formato fat_dir.
// Busca de arquivos de origem e destino: A função find_in_root() é usada para localizar 
// as entradas de diretório correspondentes aos arquivos de origem (source) e destino (dest).
// Verificação de substituição de arquivo: Caso o arquivo de destino já exista, o comando 
// mv falha (não permitido substituir arquivos). Isso é feito verificando se a entrada do destino já está presente.
// Renomeação (movimento): Se o arquivo de origem é encontrado, seu nome é alterado para o 
// nome do destino (dest_rname), e a entrada do diretório é atualizada.
// Escrita de volta ao disco: A entrada de diretório modificada (com o novo nome) é escrita 
// de volta no disco usando fseek() e fwrite().
// Sem cópia de clusters: Diferente de cp, no mv não há a necessidade de copiar os clusters, 
// pois estamos apenas alterando o nome do arquivo no diretório.
void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    // Converte os nomes dos arquivos para o formato FAT32.
    bool badname = cstr_to_fat32wnull(source, source_rname)
                || cstr_to_fat32wnull(dest, dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    // Calcula o endereço do diretório raiz no FAT32.
    uint32_t root_address = bpb_root_dir_address(bpb);

    // Calcula o tamanho da estrutura que guarda as entradas de diretório da pasta raiz.
    uint32_t root_size = sizeof(struct fat_dir) * bpb->n_fat;

    // Lê as entradas do diretório raiz
    struct fat_dir root[root_size];
    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler diretório raiz");

    // Encontra a entrada do arquivo de origem e destino
    struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);
    struct far_dir_searchres dir2 = find_in_root(root, dest_rname, bpb);

    // Verifica se o destino já existe (não pode substituir)
    if (dir2.found == true)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via mv.", dest);

    // Verifica se a origem existe
    if (dir1.found == false)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", source);

    // Mover o arquivo (renomeando no diretório)
    memcpy(dir1.fdir.name, dest_rname, sizeof(char) * FAT32STR_SIZE);

    // Calcula o endereço da entrada do diretório que será atualizada
    uint32_t source_address = sizeof(struct fat_dir) * dir1.idx + root_address;

    // Escreve a nova entrada de diretório no disco
    (void) fseek(fp, source_address, SEEK_SET);
    (void) fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);

    printf("mv %s → %s.\n", source, dest);
    return;
}

void rm(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    /* Manipulação de diretório */
    char fat32_rname[FAT32STR_SIZE_WNULL];

    // Converte o nome do arquivo para o formato FAT32
    if (cstr_to_fat32wnull(filename, fat32_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_root_dir_address(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->n_fat;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler estrutura do diretório raiz");

    // Encontra a entrada do arquivo a ser removido
    struct far_dir_searchres dir = find_in_root(root, fat32_rname, bpb);
    if (!dir.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", filename);

    // Marca a entrada como livre (define o primeiro byte como DIR_FREE_ENTRY)
    dir.fdir.name[0] = DIR_FREE_ENTRY;

    // Calcula o endereço da entrada de diretório a ser deletada
    uint32_t file_address = sizeof(struct fat_dir) * dir.idx + root_address;

    // Escreve a entrada atualizada de volta ao disco
    (void) fseek(fp, file_address, SEEK_SET);
    (void) fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

    /* Liberação dos clusters */
    uint32_t fat_address = bpb_fat_address(bpb);
    uint32_t cluster_number = dir.fdir.starting_cluster_low;
    uint32_t null = 0x00000000;  // Em FAT32, o valor de cluster livre é 0x00000000
    size_t count = 0;

    // Continua liberando os clusters até encontrar EOF
    while (cluster_number >= 0x00000002 && cluster_number <= 0x0FFFFFF7) 
    {
        uint32_t infat_cluster_address = fat_address + cluster_number * 4;  // Cada entrada FAT32 tem 4 bytes
        uint32_t next_cluster;

        // Lê o próximo cluster da FAT
        read_bytes(fp, infat_cluster_address, &next_cluster, sizeof(uint32_t));

        // Marca o cluster como livre
        (void) fseek(fp, infat_cluster_address, SEEK_SET);
        (void) fwrite(&null, sizeof(uint32_t), 1, fp);

        cluster_number = next_cluster;
        count++;
    }

    printf("rm %s, %li clusters apagados.\n", filename, count);

    return;
}


struct fat32_newcluster_info fat32_find_free_cluster(FILE* fp, struct fat_bpb* bpb)
{
    uint32_t cluster = 2;  // Iniciar a busca a partir do cluster 2
    uint32_t fat_address = bpb_fat_address(bpb);  // Endereço do início da tabela FAT
    uint32_t total_clusters = bpb_fdata_cluster_count(bpb);  // Total de clusters

    // Percorrer todos os clusters disponíveis
    for (cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t entry;
        uint32_t entry_address = fat_address + cluster * 4;  // Cada entrada tem 4 bytes

        // Ler a entrada correspondente no FAT32
        (void) read_bytes(fp, entry_address, &entry, sizeof(uint32_t));

        // Verificar se o cluster está livre (entrada 0x00000000)
        if (entry == 0x00000000)
        {
            // Retornar informações sobre o cluster livre encontrado
            return (struct fat32_newcluster_info) { .cluster = cluster, .address = entry_address };
        }
    }

    // Se nenhum cluster livre for encontrado, retornar um valor nulo ou de erro
    return (struct fat32_newcluster_info) { .cluster = 0, .address = 0 };
}


void cp(FILE *fp, char* source, char* dest, struct fat_bpb *bpb)
{
    /* Manipulação de diretório */
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    bool badname = cstr_to_fat16wnull(source, source_rname)
                || cstr_to_fat16wnull(dest, dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_root_dir_address(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->n_fat;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler estrutura do diretório raiz");

    struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);
    if (!dir1.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", source);

    if (find_in_root(root, dest_rname, bpb).found)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via cp.", dest);

    struct fat_dir new_dir = dir1.fdir;
    memcpy(new_dir.name, dest_rname, FAT32STR_SIZE);

    /* Dentry */

    bool dentry_failure = true;

    /* Procura-se uma entrada livre no diretório raiz */
    for (int i = 0; i < bpb->n_fat; i++) {
        if (root[i].name[0] == DIR_FREE_ENTRY || root[i].name[0] == '\0') {

            uint32_t dest_address = sizeof(struct fat_dir) * i + root_address;

            (void) fseek(fp, dest_address, SEEK_SET);
            (void) fwrite(&new_dir, sizeof(struct fat_dir), 1, fp);

            dentry_failure = false;
            break;
        }
    }

    if (dentry_failure)
        error_at_line(EXIT_FAILURE, ENOSPC, __FILE__, __LINE__, "Não foi possível alocar uma entrada no diretório raiz.");

    /* Agora é necessário alocar os clusters para o novo arquivo. */

    int count = 0;

    /* Clusters */
    {
        struct fat32_newcluster_info next_cluster, prev_cluster = { .cluster = FAT32_EOF_LO };

        uint32_t cluster_count = dir1.fdir.file_size / bpb->bytes_p_sect / bpb->sector_p_clust + 1;

        while (cluster_count--) {
            prev_cluster = next_cluster;
            next_cluster = fat32_find_free_cluster(fp, bpb); // Função para encontrar um cluster livre

            if (next_cluster.cluster == 0x0)
                error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Disco cheio (imagem foi corrompida)");

            (void) fseek(fp, next_cluster.address, SEEK_SET);
            (void) fwrite(&prev_cluster.cluster, sizeof(uint32_t), 1, fp);

            count++;
        }

        /* O cluster de início é guardado na entrada do diretório. */
        new_dir.starting_cluster_low = next_cluster.cluster;
        new_dir.ea_index = prev_cluster.cluster >> 16;  // Ajuste do cluster alto
    }

    /* Copy */
    {
        const uint32_t fat_address = bpb_fat_address(bpb);
        const uint32_t data_region_start = bpb_data_address(bpb);
        const uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

        size_t bytes_to_copy = new_dir.file_size;

        uint32_t source_cluster_number = dir1.fdir.starting_cluster_low;
        uint32_t destin_cluster_number = new_dir.starting_cluster_low;

        while (bytes_to_copy != 0) {

            uint32_t source_cluster_address = cluster_to_address(source_cluster_number, bpb);
            uint32_t destin_cluster_address = cluster_to_address(destin_cluster_number, bpb);

            size_t copied_in_this_sector = MIN(bytes_to_copy, cluster_width);

            char filedata[cluster_width];

            /* Lê da fonte e escreve no destino */
            (void) read_bytes(fp, source_cluster_address, filedata, copied_in_this_sector);
            (void) fseek(fp, destin_cluster_address, SEEK_SET);
            (void) fwrite(filedata, sizeof(char), copied_in_this_sector, fp);

            bytes_to_copy -= copied_in_this_sector;

            uint32_t source_next_cluster_address = fat_address + source_cluster_number * sizeof(uint32_t);
            uint32_t destin_next_cluster_address = fat_address + destin_cluster_number * sizeof(uint32_t);

            (void) read_bytes(fp, source_next_cluster_address, &source_cluster_number, sizeof(uint32_t));
            (void) read_bytes(fp, destin_next_cluster_address, &destin_cluster_number, sizeof(uint32_t));
        }
    }

    printf("cp %s → %s, %i clusters copiados.\n", source, dest, count);

    return;
}

// Definimos a estrutura fat32_newcluster_info para armazenar o número do cluster e o endereço físico correspondente.
// Adicionamos o uso da função fat32_find_free_cluster() para localizar clusters livres.
// Garantimos que os cálculos de alocação de clusters e cópia de dados estejam corretos para o FAT32.

void cat_fat32(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    char rname[FAT32STR_SIZE_WNULL];
    bool badname = cstr_to_fat16wnull(filename, rname); // Converte para o formato de nome compatível
    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_root_dir_address(bpb);
    uint32_t root_size    = sizeof(struct fat_dir) * bpb->n_fat;  // Ajuste para FAT32
    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler estrutura do diretório raiz");

    // Função para buscar no diretório raiz
    struct far_dir_searchres dir = find_in_root(&root[0], rname, bpb);
    if (!dir.found)
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o %s.", filename);

    size_t bytes_to_read = dir.fdir.file_size;
    uint32_t data_region_start = bpb_data_address(bpb);  // Endereço da região de dados
    uint32_t fat_address = bpb_fat_address(bpb);  // Endereço da tabela FAT

    uint32_t cluster_number = dir.fdir.starting_cluster_low; // FAT32 usa 32 bits para o número do cluster

    const uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

    while (bytes_to_read != 0)
    {
        // Calcular o endereço físico do cluster
        uint32_t cluster_address = cluster_to_address(cluster_number, bpb);
        
        size_t read_in_this_sector = MIN(bytes_to_read, cluster_width);
        char filedata[cluster_width];

        // Lê o cluster e imprime no terminal
        read_bytes(fp, cluster_address, filedata, read_in_this_sector);
        printf("%.*s", (signed)read_in_this_sector, filedata);

        bytes_to_read -= read_in_this_sector;

        // Calcular o próximo cluster na FAT32
        uint32_t next_cluster_address = fat_address + cluster_number * 4;  // FAT32 usa 4 bytes por entrada
        read_bytes(fp, next_cluster_address, &cluster_number, sizeof(uint32_t));

        // Verificar se atingiu o final do arquivo
        if (cluster_number >= FAT32_EOF_LO && cluster_number <= FAT32_EOF_HI)
            break;
    }

    return;
}
