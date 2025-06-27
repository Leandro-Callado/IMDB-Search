#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILENAME "action.csv"
#define HASH_TABLE_SIZE 19997

typedef struct {
    char movie_id[20];
    char filme_nome[255];
    int ano_lanc;
    char idade_etaria[10];
    int tempo_filme;
    char genero[50];
    float avaliacao;
    char descricao[500];
    char diretor[255];
    int diretor_id;
    char star[500];
    int star_id;
    int votos;
    float receita;
} MovieRecord;

typedef struct {
    char *key;
    int value;
} HashNode;

typedef struct {
    HashNode **table;
    int size;
    int count;
} HashTable;

unsigned int hash_function(const char *key, int size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

HashTable* ht_create(int size) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    ht->size = size;
    ht->count = 0;
    ht->table = (HashNode **)calloc(size, sizeof(HashNode *));
    return ht;
}

void ht_insert(HashTable *ht, const char *key, int value) {
    if (ht->count >= ht->size - 1) {
        return;
    }
    HashNode *new_node = (HashNode *)malloc(sizeof(HashNode));
    new_node->key = strdup(key);
    new_node->value = value;
    unsigned int index = hash_function(key, ht->size);
    while (ht->table[index] != NULL) {
        index = (index + 1) % ht->size;
    }
    ht->table[index] = new_node;
    ht->count++;
}

HashNode* ht_search(HashTable *ht, const char *key, int *accesses_count) {
    unsigned int index = hash_function(key, ht->size);
    *accesses_count = 0;
    while (ht->table[index] != NULL) {
        (*accesses_count)++;
        if (strcmp(ht->table[index]->key, key) == 0) {
            return ht->table[index];
        }
        index = (index + 1) % ht->size;
        if (*accesses_count >= ht->size) break;
    }
    return NULL;
}

void ht_free(HashTable *ht) {
    if (!ht) return;
    for (int i = 0; i < ht->size; i++) {
        if (ht->table[i] != NULL) {
            free(ht->table[i]->key);
            free(ht->table[i]);
        }
    }
    free(ht->table);
    free(ht);
}

void load_movie_data(MovieRecord **records, int *count, HashTable **index_table) {
    FILE *fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo '%s'!\n", FILENAME);
        return;
    }

    if (*records) free(*records);
    if (*index_table) ht_free(*index_table);
    *records = NULL;
    *count = 0;
    *index_table = ht_create(HASH_TABLE_SIZE);

    char Linha[4096];

    if (fgets(Linha, sizeof(Linha), fp) == NULL) {
        fclose(fp);
        return;
    }

    while (fgets(Linha, sizeof(Linha), fp) != NULL) {
        if ((*index_table)->count >= (*index_table)->size - 1) {
            printf("Aviso: Tabela Hash cheia. Interrompendo o carregamento de novos dados.\n");
            break;
        }

        Linha[strcspn(Linha, "\r\n")] = 0;

        (*count)++;
        *records = (MovieRecord *)realloc(*records, (*count) * sizeof(MovieRecord));
        MovieRecord *current_record = &(*records)[*count - 1];
        memset(current_record, 0, sizeof(MovieRecord));

        char *line_ptr = Linha;
        int campo = 0;

        while (*line_ptr && campo <= 13) {
            char token[4096] = {0};
            char *token_ptr = token;

            if (*line_ptr == '"') {
                line_ptr++;
                while (*line_ptr) {
                    if (*line_ptr == '"') {
                        if (*(line_ptr + 1) == '"') {
                            *token_ptr++ = '"';
                            line_ptr += 2;
                        } else {
                            line_ptr++;
                            if (*line_ptr == ',') line_ptr++;
                            break;
                        }
                    } else {
                        *token_ptr++ = *line_ptr++;
                    }
                }
            } else {
                while (*line_ptr && *line_ptr != ',') {
                    *token_ptr++ = *line_ptr++;
                }
                if (*line_ptr == ',') {
                    line_ptr++;
                }
            }

            switch (campo) {
                case 0: strncpy(current_record->movie_id, token, sizeof(current_record->movie_id) - 1); break;
                case 1: strncpy(current_record->filme_nome, token, sizeof(current_record->filme_nome) - 1); break;
                case 2: current_record->ano_lanc = (token[0] == '\0') ? 0 : atoi(token); break;
                case 3: strncpy(current_record->idade_etaria, token, sizeof(current_record->idade_etaria) - 1); break;
                case 4: current_record->tempo_filme = (token[0] == '\0') ? 0 : atoi(token); break;
                case 5: strncpy(current_record->genero, token, sizeof(current_record->genero) - 1); break;
                case 6: current_record->avaliacao = (token[0] == '\0') ? 0.0f : strtof(token, NULL); break;
                case 7: strncpy(current_record->descricao, token, sizeof(current_record->descricao) - 1); break;

                // NOVO: Lógica para limpar o campo do diretor
                case 8: {
                    char *pattern_pos = strstr(token, "/name/nm");
                    if (pattern_pos != NULL) {
                        // Trunca a string onde o padrão começa
                        *pattern_pos = '\0';

                        // Limpa vírgulas ou espaços em branco que sobraram no final
                        int len = strlen(token);
                        while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == ',')) {
                            token[len - 1] = '\0';
                            len--;
                        }
                    }
                    strncpy(current_record->diretor, token, sizeof(current_record->diretor) - 1);
                    break;
                }

                case 9: current_record->diretor_id = (token[0] == '\0') ? 0 : atoi(token); break;

                // NOVO: Lógica para limpar o campo da(s) estrela(s)
                case 10: {
                    char *pattern_pos = strstr(token, "/name/nm");
                    if (pattern_pos != NULL) {
                        // Trunca a string onde o padrão começa
                        *pattern_pos = '\0';

                        // Limpa vírgulas ou espaços em branco que sobraram no final
                        int len = strlen(token);
                        while (len > 0 && (token[len - 1] == ' ' || token[len - 1] == ',')) {
                            token[len - 1] = '\0';
                            len--;
                        }
                    }
                    strncpy(current_record->star, token, sizeof(current_record->star) - 1);
                    break;
                }

                case 11: current_record->star_id = (token[0] == '\0') ? 0 : atoi(token); break;
                case 12: current_record->votos = (token[0] == '\0') ? 0 : atoi(token); break;
                case 13: current_record->receita = (token[0] == '\0') ? 0.0f : strtof(token, NULL); break;
            }
            campo++;
        }

        ht_insert(*index_table, current_record->movie_id, *count - 1);
    }

    fclose(fp);
    printf(">> %d registros de filmes foram carregados e indexados com sucesso.\n", *count);
}

void perform_search(HashTable *index_table, MovieRecord *records) {
    if (!index_table || !records) {
        printf("\n!! Erro: Os dados ainda nao foram carregados. Use a Opcao 1 primeiro.\n");
        return;
    }

    char search_id[50];
    printf("\nDigite o ID do Filme para buscar (ex: tt0092099): ");
    scanf("%49s", search_id);

    int accesses = 0;
    clock_t start_time = clock();
    HashNode *found_node = ht_search(index_table, search_id, &accesses);
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("\n---------- RESULTADO DA CONSULTA ----------\n");
    if (found_node) {
        MovieRecord *record = &records[found_node->value];
        printf("Filme Encontrado!\n");
        printf("  - ID:           %s\n", record->movie_id);
        printf("  - Titulo:       %s\n", record->filme_nome);
        printf("  - Ano:          %d\n", record->ano_lanc);
        printf("  - Genero:       %s\n", record->genero);
        printf("  - Faixa etaria: %s\n", record->idade_etaria);
        printf("  - Duracao:      %d minutos\n", record->tempo_filme);
        printf("  - Diretor:      %s\n", record->diretor);
        printf("  - Estrela(s):   %s\n", record->star);
        printf("  - Avaliacao:    %.1f\n", record->avaliacao);
        printf("  - Votos:        %d\n", record->votos);
        printf("  - Receita:      $%.2f M\n", record->receita);
        printf("  - Descricao:    %s\n", record->descricao);
    } else {
        printf("Filme com o ID '%s' nao foi encontrado na base de dados.\n", search_id);
    }

    printf("\n------- METRICAS DE DESEMPENHO -------\n");
    printf("  - Quantidade de acessos na tabela: %d\n", accesses);
    printf("  - Tempo de busca: %f segundos\n", time_spent);
    printf("----------------------------------------\n");
}

void cleanup(MovieRecord *records, HashTable *index_table) {
    printf("\nLiberando toda a memoria alocada...\n");
    if (records) free(records);
    if (index_table) ht_free(index_table);
    printf("Programa finalizado.\n");
}

int main() {
    MovieRecord *movie_records = NULL;
    HashTable *index_table = NULL;
    int record_count = 0;
    int choice = 0;

    do {
        printf("\n========= MENU DE CONSULTA DE FILMES =========\n");
        printf("1. Carregar e Indexar Dados de Filmes\n");
        printf("2. Consultar Filme por ID\n");
        printf("3. Sair\n");
        printf("============================================\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            choice = 0;
        }

        switch (choice) {
            case 1:
                load_movie_data(&movie_records, &record_count, &index_table);
                break;
            case 2:
                perform_search(index_table, movie_records);
                break;
            case 3:
                cleanup(movie_records, index_table);
                break;
            default:
                printf("\n!! Opcao invalida. Por favor, tente novamente.\n");
        }
    } while (choice != 3);

    return 0;
}
