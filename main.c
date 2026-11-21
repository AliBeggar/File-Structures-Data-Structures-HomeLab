#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define BLOCK_SIZE 100
#define ID_SIZE 5
#define COMMUNE_SIZE 25
#define DAIRA_SIZE 22
#define WILAYA_CODE_SIZE 3
#define WILAYA_NAME_SIZE 21
#define RANGE 20
#define MAX_SIZE 200
#define NAME_SIZE 40


typedef struct RECORD
{
    char id[ID_SIZE];
    char commune[COMMUNE_SIZE];
    char daira[DAIRA_SIZE];
    char wilaya_code[WILAYA_CODE_SIZE];
    char wilaya_name[WILAYA_NAME_SIZE];
} record;

typedef struct Tblock
{
    record tab[BLOCK_SIZE];
} tblock;

typedef struct HEADER
{
    unsigned int last_block;
    unsigned int nb;
    unsigned int nb_records;
}theader;

typedef struct file
{
    FILE* F;
    theader h;
}*file;

tblock buffer,write_buffer,*buffers;

void clear_screen()
{
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void wait_for_enter()
{
    printf("\nPress Enter to continue...");
    getchar();
    getchar();
    clear_screen();
}

file open_file(char file_name[NAME_SIZE],char mode)
{
    file F= malloc(sizeof(struct file));
    if (F == NULL) {
        return NULL;
    }
    if (mode=='N')
    {
        (*F).F=fopen(file_name,"w");
        if ((*F).F == NULL) {
            free(F);
            return NULL;
        }
        (*F).h.last_block=0;
        (*F).h.nb=0;
        (*F).h.nb_records=0;
        fwrite(&((*F).h),sizeof(theader),1,(*F).F);
    }else
    {
        if (mode=='A')
        {
            (*F).F=fopen(file_name,"r+");
            if ((*F).F == NULL) {
                free(F); 
                return NULL;
            }
            fread(&((*F).h),sizeof(theader),1,(*F).F);
        }else
        {
            printf("ERROR No such mode....");
            free(F);
            return NULL;
        }
    };
    return F;
}

FILE * close_file(file F)
{
    if ((*F).F==NULL)
    {
        return NULL;
    }
    fseek((*F).F,0,SEEK_SET);
    fwrite(&((*F).h),sizeof(theader),1,(*F).F);
    fclose((*F).F);
}

void read_block(FILE * F,int i,tblock* buf)
{
    if (F==NULL)
    {
        return;
    }
    fseek(F,i*sizeof(*buf)+sizeof(theader),SEEK_SET);
    fread(buf,sizeof(*buf),1,F);
}

void write_block(FILE * F,int i,tblock* buf)
{
    if (F==NULL)
    {
        return;
    }
    fseek(F,i*sizeof(*buf)+sizeof(theader),SEEK_SET);
    fwrite(buf,sizeof(*buf),1,F);
}

unsigned int get_header(file F,unsigned int i)
{
    switch (i)
    {
    case 1: return (*F).h.last_block;
    case 2: return (*F).h.nb;
    case 3: return (*F).h.nb_records;
    };
    return 0;
}

void set_header(file F,unsigned int i,void* value)
{
    switch (i)
    {
    case 1: (*F).h.last_block=*(int *)value;
    break;
    case 2: (*F).h.nb=*(int *)value;
    break;
    case 3: (*F).h.nb_records=*(int *)value;
    break;
    };
}

void show_header(file F)
{
    printf("Last Block: %d\n", F->h.last_block);
    printf("NB: %d\n", F->h.nb);
    printf("NB Records: %d\n", F->h.nb_records);
}

void read_and_display_metadata(char file_name[NAME_SIZE])
{
    file F = open_file(file_name, 'A');
    if (F == NULL || F->F == NULL) {
        printf("Error: could not open file %s\n", file_name);
        return;
    }

    fseek(F->F, 0, SEEK_END);
    long file_size = ftell(F->F);

    unsigned int num_records = F->h.nb_records;

    int min_id = 0;
    int max_id = 0;
    bool first_record = true;

    tblock buf;
    for (unsigned int b = 0; b <= F->h.last_block; b++) {
        read_block(F->F, b, &buf);
        unsigned int limit = (b == F->h.last_block) ? F->h.nb : BLOCK_SIZE;

        for (unsigned int p = 0; p < limit; p++) {
            int current_id = atoi(buf.tab[p].id);
            if (first_record) {
                min_id = current_id;
                max_id = current_id;
                first_record = false;
            } else {
                if (current_id < min_id) {
                    min_id = current_id;
                }
                if (current_id > max_id) {
                    max_id = current_id;
                }
            }
        }
    }

    printf("\n");
    printf("======================================================================================\n");
    printf("                           FILE METADATA: %-37s  \n", file_name);
    printf("======================================================================================\n");
    printf("                        Number of Records: %-37u  \n", num_records);
    printf("                             Minimum ID: %-39d  \n", min_id);
    printf("                             Maximum ID: %-39d  \n", max_id);
    printf("                             File Size: %ld bytes (%.2f KB)\n", file_size, file_size / 1024.0);
    printf("                        Number of Blocks: %-37u  \n", F->h.last_block + 1);
    printf("                   Records in Last Block: %-37u  \n", F->h.nb);
    printf("======================================================================================\n\n");

    close_file(F);
}

void load_file(FILE* original_F,file F)
{
    int k=0;
    unsigned int n_block=0;
    unsigned int n_pos=0;
    char s[MAX_SIZE];
    record r;
    int nb=0;
    fgets(s,MAX_SIZE,original_F);
    while (fgets(s,MAX_SIZE,original_F))
    {
        char* token=strtok(s,",\n");
        int i=1;
        while (token!=NULL)
        {
            switch (i)
            {
            case 1: strcpy(r.id,token);
                break;
            case 2: strcpy(r.commune,token);
                break;
            case 3: strcpy(r.daira,token);
                break;
            case 4: strcpy(r.wilaya_code,token);
                break;
            case 5: strcpy(r.wilaya_name,token);
                break;
            }
            token=strtok(NULL,",\n");
            i++;
        }
        if (n_pos==BLOCK_SIZE)
        {
            write_block((*F).F,n_block,&buffer);
            n_block++;
            n_pos=0;
        }
        buffer.tab[n_pos]=r;
        nb = nb+1;
        n_pos++;
    };
    set_header(F,1,&n_block);
    set_header(F,2,&n_pos);
    set_header(F,3,&nb);
    write_block((*F).F,n_block,&buffer);
}

void displayrecord(record r)
{
    printf("| %-4s | %-23s | %-20s | %-4s | %-19s |\n",
           r.id, r.commune, r.daira, r.wilaya_code, r.wilaya_name);
}
void show_file(file F)
{

    unsigned int last_block = F->h.last_block;
    unsigned int last_pos = F->h.nb;

    printf("\n");
    printf("======================================================================================\n");
    printf("| %-4s | %-23s | %-20s | %-4s | %-19s |\n",
           "ID", "COMMUNE", "DAIRA", "CODE", "WILAYA");
    printf("======================================================================================\n");

    tblock buf;
    int record_count = 0;

    for (unsigned int b = 0; b <= last_block; ++b) {
        read_block(F->F, b, &buf);
        unsigned int limit = (b == last_block) ? last_pos : BLOCK_SIZE;
        for (unsigned int p = 0; p < limit; ++p) {
            displayrecord(buf.tab[p]);
            record_count++;
        }
    }

    printf("======================================================================================\n");
    printf("Total records: %d\n\n", record_count);
}

void show_file_by_name(char file_name[NAME_SIZE])
{
    file F = open_file(file_name, 'A');
    if (F == NULL || F->F == NULL) {
        printf("Error: file does not exist %s\n", file_name);
        return;
    }

    show_file(F);
    close_file(F);
}

int fragmentate_by_id(file files[],  int step ){
    char main_file[NAME_SIZE] = "cities_binary_file.bin";
    file F = open_file(main_file, 'A');
    if (F == NULL) {
        printf("Error opening file\n");
        return 0;
    }

    if (step <= 0) {
        printf("Ranges must contain atleast one record\n");
        return 0;
    }

    // calculate number of files needed
    int nb_files = (1541 / step) + ((1541 % step) ? 1 : 0);
    // maximum number of files is 1541 if step is 1

    // this is the standard name format for the files to be created
    char str[NAME_SIZE] = "file____.bin";
    for (int i = 0; i < nb_files; i++) {
        strcpy(str, "file____.bin");
        str[4]= '0' + (int)((i+1)/1000);
        str[5]= '0' + (int)(((i+1)%1000) / 100);
        str[6]= '0' + (int)( ((i+1)/10) % 10);
        str[7]= '0' + (int)((i+1)%10);
        if ( str[6] == '4' && str[5] == '5' && str[4] == '1'){printf("haha get lost!");} 
        files[i] = open_file(str, 'N');
        if (files[i] == NULL) {
            printf("Error creating file %s\n", str);
            return 0;
        }
    }

    int i = 0;
    int j = 0;
    int k = 0;
    int stop = 0;
    int cost = 0;
    tblock *buffers = malloc(sizeof(tblock) * nb_files);
    if (buffers == NULL) {
        printf("Memory allocation failed\n");
        for (int x = 0; x < nb_files; x++) {
            close_file(files[x]);
        }
        close_file(F);
        return 0;
    }
    tblock buf;

    for (i = 0; i <= F->h.last_block; i++) {
        read_block(F->F, i, &buf);
        cost++;
        int limit = (i == F->h.last_block) ? F->h.nb : BLOCK_SIZE;

        for (j = 0; j < limit; j++) {
            stop = 0;
            int id = atoi(buf.tab[j].id);

            for (k = 0; (k < nb_files && !stop); k++) {
                if ((k*step + 1) > 1541) { // case where lower bound exceeds max id
                    stop = 1;
                } else {
                    if (id >= (k*step + 1) && id <= (k+1)*step) {
                        buffers[k].tab[files[k]->h.nb] = buf.tab[j];

                        files[k]->h.nb++;
                        files[k]->h.nb_records++;

                        if (files[k]->h.nb == BLOCK_SIZE) {
                            write_block(files[k]->F, files[k]->h.last_block, &buffers[k]);
                            cost++;
                            files[k]->h.last_block++;
                            files[k]->h.nb = 0;
                        }
                        stop = 1;
                    }
                }
            }
        }
    }

    // write remaining records in buffers to their respective files
    for (int i = 0; i < nb_files; i++) {
        if (files[i]->h.nb > 0) {
            write_block(files[i]->F, files[i]->h.last_block, &buffers[i]);
            cost++;
        }
        close_file(files[i]);
    }
    free(buffers);
    close_file(F);
    return cost;
}

int fragmentate_by_wilaya_code(file files[], int nb_files)
{
    char main_file[NAME_SIZE] = "cities_binary_file.bin";
    file F = open_file(main_file, 'A');
    if (F == NULL) {
        printf("Error opening file\n");
        return 0;
    }

    if (nb_files <= 0) {
        printf("Number of files must be positive\n");
        return 0;
    }

    // calculate step size for wilaya code ranges
    int step = (int)(58/nb_files) + 1; // min step is 1

    // this is the standard name format for the files to be created
    char str[NAME_SIZE] = "file__.bin";
    for (int i = 0; i < nb_files; i++) {
        strcpy(str, "file__.bin");
        if (i >= 9) {
            str[4] = '0' + (int)((i+1)/10);
            str[5] = '0' + (int)((i+1)%10);
        } else {
            str[5] = '1' + i;
        }
        files[i] = open_file(str, 'N');
        if (files[i] == NULL) {
            printf("Error creating file %s\n", str);
            return 0;
        }
    }

    int i = 0;
    int j = 0;
    int k = 0;
    int stop = 0;
    int cost = 0;
    tblock buffers[nb_files];
    tblock buf;

    for (i = 0; i <= F->h.last_block; i++) {
        read_block(F->F, i, &buf);
        cost++;

        int limit = (i == F->h.last_block) ? F->h.nb : BLOCK_SIZE;

        for (j = 0; j < limit; j++) {
            stop = 0;
            int wilaya_code = atoi(buf.tab[j].wilaya_code);

            for (k = 0; (k < nb_files && !stop); k++) {
                if ((k*step + 1) > 58) {
                    stop = 1;
                } else {
                    if (wilaya_code >= (k*step + 1) && wilaya_code <= (k+1)*step) {
                        buffers[k].tab[files[k]->h.nb] = buf.tab[j];

                        files[k]->h.nb++;
                        files[k]->h.nb_records++;

                        if (files[k]->h.nb == BLOCK_SIZE) {
                            write_block(files[k]->F, files[k]->h.last_block, &buffers[k]);
                            cost++;
                            files[k]->h.last_block++;
                            files[k]->h.nb = 0;
                        }
                        stop = 1;
                    }
                }
            }
        }
    }

    // write remaining records in buffers to their respective files
    for (int i = 0; i < nb_files; i++) {
        if (files[i]->h.nb > 0) {
            write_block(files[i]->F, files[i]->h.last_block, &buffers[i]);
            cost++;
        }
        close_file(files[i]);
    }

    close_file(F);
    return cost;
}



int fragmentate_by_firstLetter_Wilaya(char input_file[NAME_SIZE])
{
    file F = open_file(input_file, 'A');
    if (F == NULL || F->F == NULL) {
        printf("Error: could not open file\n");
        return 0;
    }

    tblock buf_r, buf_w;
    unsigned int last_n_block = F->h.last_block;
    unsigned int last_n_pos = F->h.nb;

    char file_name[NAME_SIZE] = "*.bin";
    char character = 'A';
    int cost = 0;
    while (character <= 'Z') {
        file_name[0] = character;
        file output_file = open_file(file_name, 'N');

        if (output_file == NULL) {
            printf("Error creating file %s\n", file_name);
            character++;
            continue;
        }

        bool stop = false;

        for (int i = 0; i <= last_n_block ; i++) {
            read_block(F->F, i, &buf_r);
            cost++;
            int limit = (i == last_n_block) ? F->h.nb : BLOCK_SIZE;

            for (int j = 0; j < limit ; j++) {
                if (buf_r.tab[j].wilaya_name[0] == character) {
                    output_file->h.nb_records++;
                    buf_w.tab[output_file->h.nb] = buf_r.tab[j];
                    output_file->h.nb++;
                    if(output_file->h.nb == BLOCK_SIZE){
                        write_block(output_file->F, output_file->h.last_block, &buf_w);
                        cost++;
                        output_file->h.last_block++;
                        output_file->h.nb = 0;
                    }
                }
            }
        }

        if (output_file->h.nb > 0) {
            write_block(output_file->F, output_file->h.last_block, &buf_w);
            cost++;
        }
        close_file(output_file);
        character++;
    }

    close_file(F);
    character = 'A';
    for (int i = 0; i < 26; i++) {
        file_name[0] = character;
        file output_file = open_file(file_name, 'A');
        if (output_file != NULL) {
            if (get_header(output_file, 3) == 0) {
                remove(file_name);
            }
            close_file(output_file);
        }
        character++;
    }
    return cost;
}

void delete_by_first_letter () {
    char file_name[NAME_SIZE] = "*.bin";
    char character = 'A';
    while (character <= 'Z') {
        file_name[0] = character;
        remove(file_name);
        character++;
    }
}

void delete_by_id_range () {
    char file_name[NAME_SIZE];
    for (int i = 1; i <= 1541; i++) {
        strcpy(file_name, "file____.bin");
        file_name[4] = '0' + (int)((i)/1000);
        file_name[5] = '0' + (int)(((i)%1000) / 100);
        file_name[6] = '0' + (int)( ((i)/10) % 10);
        file_name[7] = '0' + (int)((i)%10);
        remove(file_name);
    }
}

void delete_by_willaya_code () {
    char file_name[NAME_SIZE];
    for (int i = 0; i < 99; i++) {
        strcpy(file_name, "file__.bin");
        if (i >= 9) {
            file_name[4] = '0' + (int)((i+1)/10);
            file_name[5] = '0' + (int)((i+1)%10);
        } else {
            file_name[5] = '1' + i;
        }
        remove(file_name);
    }
}

void delete_all_files()
{
    // Delete main binary file
    remove("cities_binary_file.bin");

    // Delete fragmented files by first letter (A.bin to Z.bin)
    delete_by_first_letter();

    // Delete fragmented files by wilaya code (file_01.bin to file_99.bin)
    delete_by_willaya_code();

    // Delete fragmented files by ID range (file_0001.bin to file_1541.bin)
    delete_by_id_range();

    printf("All generated files have been deleted.\n");
}


int main()
{
    int choice;
    FILE* Text_file;
    file F;
    char main_file[NAME_SIZE] = "cities_binary_file.bin";
    bool choice_1_done = (access(main_file, F_OK) == 0);
    while(1)
    {
        printf("\n");
        printf("======================================================================================\n");
        printf("|                              FILE MANAGEMENT SYSTEM                                |\n");
        printf("======================================================================================\n");
        printf("| 1. Load CSV and create binary file                                                 |\n");
        printf("| 2. Show file header                                                                |\n");
        printf("| 3. Show file metadata                                                              |\n");
        printf("| 4. Display file records                                                            |\n");
        printf("| 5. Fragmentate by first letter of Wilaya                                           |\n");
        printf("| 6. Fragmentate by Wilaya code                                                      |\n");
        printf("| 7. Fragmentate by id range                                                         |\n");
        printf("| 8. Display a specific file by name                                                 |\n");
        printf("| 9. Delete all generated files                                                      |\n");
        printf("| 0. Exit                                                                            |\n");
        printf("======================================================================================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if(choice == 0)
            break;

        switch(choice)
        {
            case 1:
                choice_1_done = true;
                Text_file = fopen("algeria_cities_geocoded.csv","r");
                if(Text_file == NULL)
                {
                    printf("Error opening CSV file\n");
                    wait_for_enter();
                    break;
                }
                F = open_file(main_file,'N');
                load_file(Text_file,F);
                close_file(F);
                fclose(Text_file);
                printf("Binary file created successfully\n");
                wait_for_enter();
                break;

            case 2:
                F = open_file(main_file,'A');
                if(F == NULL)
                {
                    printf("Error opening file\n");
                    wait_for_enter();
                    break;
                }
                show_header(F);
                close_file(F);
                wait_for_enter();
                break;

            case 3:
            {
                char filename[NAME_SIZE];
                printf("Enter the file name (e.g., cities_binary_file.bin, file_1.bin): ");
                scanf("%s", filename);
                read_and_display_metadata(filename);
                wait_for_enter();
                break;
            }

            case 4:
                F = open_file(main_file,'A');
                if(F == NULL)
                {
                    printf("Error opening file\n");
                    wait_for_enter();
                    break;
                }
                show_file(F);
                close_file(F);
                wait_for_enter();
                break;

            case 5:
                if(!choice_1_done){printf("You should treat choice 1 first!\n"); wait_for_enter(); break;}
                char confirm;
                printf("Do you want to delete all previously generated files? (y/n): ");
                scanf(" %c", &confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    delete_by_first_letter();
                } else {
                    printf("Operation cancelled to avoid conflicts between files.\n");
                    wait_for_enter();
                    break;
                }
                int cost = fragmentate_by_firstLetter_Wilaya(main_file);
                printf("Fragmentation by first letter completed\n");
                printf("I/O operations cost: %d\n", cost);
                wait_for_enter();
                break;

            case 6:
            {
                if(!choice_1_done){printf("You should treat choice 1 first!\n"); wait_for_enter(); break;}
                char confirm;
                printf("Do you want to delete all previously generated files? (y/n): ");
                scanf(" %c", &confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    delete_by_willaya_code();
                } else {
                    printf("Operation cancelled to avoid conflicts between files.\n");
                    wait_for_enter();
                    break;
                }
                int nb_files;
                printf("Enter number of files (<100): ");
                scanf("%d", &nb_files);
                if(nb_files >= 100) { printf("Number of files should be <= 100"); wait_for_enter();}
                file *wilaya_files = malloc(sizeof(file) * nb_files);
                int cost = fragmentate_by_wilaya_code(wilaya_files, nb_files);
                printf("Fragmentation by Wilaya code completed\n");
                printf("I/O operations cost: %d\n", cost);

                free(wilaya_files);
                wait_for_enter();
                break;
            }

            case 7:
            {
                if(!choice_1_done){printf("You should treat choice 1 first!\n"); wait_for_enter(); break;}
                char confirm;                    choice_1_done = false;
                printf("Do you want to delete all previously generated files? (y/n): ");
                scanf(" %c", &confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    delete_by_id_range();
                } else {
                    printf("Operation cancelled to avoid conflicts between files.\n");
                    wait_for_enter();
                    break;
                }
                int step ;
                printf("Enter step size for id range ( example step = 99 , intervals : 1-99, 100-198, ...): ");
                scanf("%d", &step);
                int nb_files = (1541 / step) + ((1541 % step) ? 1 : 0);
                file *id_files = malloc(sizeof(file) * nb_files);
                int cost = fragmentate_by_id(id_files, step);
                printf("Fragmentation by ID range completed\n");
                printf("I/O operations cost: %d\n", cost);

                free(id_files);
                wait_for_enter();
                break;
            }

            case 8:
            {
                char filename[NAME_SIZE];
                printf("Enter the file name (e.g., file_1.bin or A.bin): ");
                scanf("%s", filename);
                show_file_by_name(filename);
                wait_for_enter();
                break;
            }

            case 9:
            {
                char confirm;
                printf("Are you sure you want to delete all generated files? (y/n): ");
                scanf(" %c", &confirm);
                if (confirm == 'y' || confirm == 'Y') {
                    delete_all_files();
                    choice_1_done = false;
                } else {
                    printf("Deletion cancelled.\n");
                }
                wait_for_enter();
                break;
            }

            default:
                printf("Invalid choice\n");
                wait_for_enter();
                break;
        }
    }

    return 0;
}
