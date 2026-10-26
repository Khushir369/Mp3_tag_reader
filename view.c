#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include "view.h"

char *tags[] = {"TIT2", "TPE1", "TALB", "TYER", "TCOM", "TCON", "COMM", NULL};

/* -------------------- HELPER FUNCTIONS -------------------- */

int big_to_little_endian(char *buffer)
{
    unsigned char *p = (unsigned char *)buffer;

    return ((int)p[0] << 24) |
           ((int)p[1] << 16) |
           ((int)p[2] << 8)  |
           (int)p[3];
}

int little_to_big_endian(int val)
{
    return ((val & 0x000000FF) << 24) |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0xFF000000) >> 24);
}

uint32_t synchsafe_to_int(const unsigned char *buffer)
{
    uint32_t size = 0;

    size = ((uint32_t)(buffer[0] & 0x7F) << 21) |
           ((uint32_t)(buffer[1] & 0x7F) << 14) |
           ((uint32_t)(buffer[2] & 0x7F) << 7)  |
           ((uint32_t)(buffer[3] & 0x7F));

    return size;
}
void int_to_synchsafe(uint32_t size, unsigned char *buffer)
{
    buffer[0] = (size >> 21) & 0x7F;
    buffer[1] = (size >> 14) & 0x7F;
    buffer[2] = (size >> 7) & 0x7F;
    buffer[3] = size & 0x7F;
}

const char *get_parameter_to_edit(const char *parameter)
{
    if (strcasecmp(parameter, "Title") == 0)
        return "TIT2";
    else if (strcasecmp(parameter, "Artist") == 0)
        return "TPE1";
    else if (strcasecmp(parameter, "Album") == 0)
        return "TALB";
    else if (strcasecmp(parameter, "Year") == 0)
        return "TYER";
    else if (strcasecmp(parameter, "Composer") == 0)
        return "TCOM";
    else if (strcasecmp(parameter, "Genre") == 0 ||
             strcasecmp(parameter, "Content") == 0)
        return "TCON";
    else if (strcasecmp(parameter, "Comment") == 0 ||
             strcasecmp(parameter, "Comments") == 0)
        return "COMM";

    return NULL;
}

OperationType check_operation(const char *argument)
{
    if (strcmp(argument, "-v") == 0)
        return e_view;
    else if (strcmp(argument, "-m") == 0)
        return e_edit;
    else if (strcmp(argument, "-d") == 0)
        return e_delete;
    else if (strcmp(argument, "-r") == 0)
        return e_restore;
    if (strcmp(argument, "-da") == 0)
    return e_delete_all;

    return e_unsupported;
}

void display_help_menu(void)
{
    printf("\n========== MP3 TAG READER / EDITOR ========================================\n");
    printf("View tags    : ./a.out -v song.mp3\n");
    printf("Edit a tag   : ./a.out -m Title \"New title\" song.mp3\n");
    printf("Delete a tag : ./a.out -d Title song.mp3\n");
    printf("Restore file : ./a.out -r song.mp3\n");
    printf("Delete all tags : ./mp3tag -da song.mp3\n");
    printf("\nSupported tags: Title, Artist, Album, Year, Composer, Genre, Comment\n");
    printf("=============================================================================\n");
}

static int valid_mp3_name(char *filename)
{
    size_t len = strlen(filename);

    if (len < 5)
        return 0;

    return strcasecmp(filename + len - 4, ".mp3") == 0;
}

Status validate_input(int argc, char **argv, tag *Tag)
{
    if (argc < 3)
    return e_failure;
    const char *frame_id;

    if (argv[1] == NULL || argv[2] == NULL)
        return e_failure;

    if (check_operation(argv[1]) == e_view)
    {
        if (!valid_mp3_name(argv[2]))
        {
            printf("ERROR: File extension must be .mp3\n");
            return e_failure;
        }
        strcpy(Tag->input_mp3_filename, argv[2]);
    }
    else if (check_operation(argv[1]) == e_edit)
    {
        if (argv[2] == NULL || argv[3] == NULL || argv[4] == NULL)
            return e_failure;

        frame_id = get_parameter_to_edit(argv[2]);
        if (frame_id == NULL)
        {
            printf("ERROR: Unsupported tag name\n");
            return e_failure;
        }

        if (!valid_mp3_name(argv[4]))
        {
            printf("ERROR: File extension must be .mp3\n");
            return e_failure;
        }

        strcpy(Tag->edit_parameter, frame_id);
        strncpy(Tag->newdata, argv[3], sizeof(Tag->newdata) - 1);
        Tag->newdata[sizeof(Tag->newdata) - 1] = '\0';
        Tag->size_of_newdata = (int)strlen(Tag->newdata);
        strcpy(Tag->input_mp3_filename, argv[4]);
    }
    else if (check_operation(argv[1]) == e_delete ||
             check_operation(argv[1]) == e_restore)
    {
        if (!valid_mp3_name(argv[2]))
        {
            printf("ERROR: File extension must be .mp3\n");
            return e_failure;
        }
        strcpy(Tag->input_mp3_filename, argv[2]);
    }
    else
    {
        return e_failure;
    }

    return e_success;
}

Status open_files(tag *Tag)
{
    Tag->input_mp3_fptr = fopen(Tag->input_mp3_filename, "rb");

    if (Tag->input_mp3_fptr == NULL)
    {
        perror("ERROR: Unable to open MP3 file");
        return e_failure;
    }

    return e_success;
}

/* -------------------- VIEW OPERATION -------------------- */

Status view_operation(tag *Tag)
{
    unsigned char header[10];
    int tag_size, position, count = 0;

    if (fread(header, 1, 10, Tag->input_mp3_fptr) != 10)
    {
        printf("ERROR: Unable to read ID3 header\n");
        return e_failure;
    }

    if (memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: ID3 header not found\n");
        return e_failure;
    }

    if (header[3] != 3)
    {
        printf("ERROR: This program supports ID3v2.3 only\n");
        return e_failure;
    }

    tag_size = synchsafe_to_int(&header[6]);
    position = 10;

    printf("\n------------------------------------\n");
    printf("Tag ID\tValue\n");
    printf("------------------------------------\n");

    while (position + 10 <= 10 + tag_size)
    {
        unsigned char frame_header[10];
        char frame_id[5];
        int frame_size, i;

        if (fseek(Tag->input_mp3_fptr, position, SEEK_SET) != 0)
            return e_failure;

        if (fread(frame_header, 1, 10, Tag->input_mp3_fptr) != 10)
            break;

        if (frame_header[0] == 0)
            break;

        memcpy(frame_id, frame_header, 4);
        frame_id[4] = '\0';

        frame_size = big_to_little_endian((char *)&frame_header[4]);

        if (frame_size < 0 ||
            position + 10 + frame_size > 10 + tag_size)
        {
            printf("ERROR: Invalid frame size\n");
            return e_failure;
        }

        /* Skip album artwork and other binary data */
        if (strcmp(frame_id, "APIC") == 0)
        {
            printf("%s\t[Album artwork skipped]\n", frame_id);
        }
        else if (frame_id[0] == 'T' && frame_size > 0)
        {
            unsigned char encoding;

            encoding = (unsigned char)fgetc(Tag->input_mp3_fptr);

            printf("%s\t", frame_id);

            for (i = 1; i < frame_size; i++)
            {
                int ch = fgetc(Tag->input_mp3_fptr);

                if (ch == EOF)
                    break;

                if (ch >= 32 || ch == '\t')
                    putchar(ch);
            }

            if (encoding != 0)
                printf(" [text encoding %u]", encoding);

            putchar('\n');
        }
        else if (strcmp(frame_id, "COMM") == 0)
        {
            printf("%s\t[Comment frame]\n", frame_id);
        }
        else
        {
            printf("%s\t[Non-text frame skipped]\n", frame_id);
        }

        /* Move to the next frame */
        position += 10 + frame_size;
        count++;
    }

    if (count == 0)
        printf("No frames found.\n");

    printf("------------------------------------\n");

    return e_success;
}
/* -------------------- EDIT / DELETE OPERATION -------------------- */

static Status change_tag(tag *Tag, int delete_frame)
{
    FILE *temp;
    unsigned char header[10], frame_header[10];
    int old_tag_size, new_tag_size = 0, position = 10;
    int found = 0, ch;
    long tag_end;

    if (fseek(Tag->input_mp3_fptr, 0, SEEK_SET) != 0 ||
        fread(header, 1, 10, Tag->input_mp3_fptr) != 10)
    {
        printf("ERROR: Cannot read MP3 header\n");
        return e_failure;
    }

    if (memcmp(header, "ID3", 3) != 0 || header[3] != 3)
    {
        printf("ERROR: ID3v2.3 file required\n");
        return e_failure;
    }

  old_tag_size = synchsafe_to_int((const unsigned char *)&header[6]);
    tag_end = 10L + old_tag_size;

    temp = fopen("temp.mp3", "wb");
    if (temp == NULL)
    {
        perror("ERROR: Cannot create temporary file");
        return e_failure;
    }

    /* Write a placeholder header. We update the size after processing frames. */
    if (fwrite(header, 1, 10, temp) != 10)
        goto error;

    while (position + 10 <= tag_end)
    {
        int frame_size;
        long frame_start = position;

        if (fseek(Tag->input_mp3_fptr, frame_start, SEEK_SET) != 0 ||
            fread(frame_header, 1, 10, Tag->input_mp3_fptr) != 10)
            goto error;

        if (frame_header[0] == 0)
            break;

        frame_size = big_to_little_endian((char *)&frame_header[4]);
        if (frame_size < 0 || position + 10 + frame_size > tag_end)
            goto error;

        if (memcmp(frame_header, Tag->edit_parameter, 4) == 0)
        {
            found = 1;

            if (!delete_frame)
            {
                int data_size = Tag->size_of_newdata + 1;
                int be_size = little_to_big_endian(data_size);

                if (fwrite(frame_header, 1, 4, temp) != 4)
                    goto error;
                if (fwrite(&be_size, 1, 4, temp) != 4)
                    goto error;
                if (fwrite(frame_header + 8, 1, 2, temp) != 2)
                    goto error;

                /* Encoding byte 0 means a single-byte text string. */
                fputc(0, temp);
                if (fwrite(Tag->newdata, 1, Tag->size_of_newdata, temp) !=
                    (size_t)Tag->size_of_newdata)
                    goto error;

                new_tag_size += 10 + data_size;
            }
            /* For delete, skip the matching frame. */
        }
        else
        {
            if (fwrite(frame_header, 1, 10, temp) != 10)
                goto error;

            if (frame_size > 0)
            {
                int i;
                for (i = 0; i < frame_size; i++)
                {
                    ch = fgetc(Tag->input_mp3_fptr);
                    if (ch == EOF || fputc(ch, temp) == EOF)
                        goto error;
                }
            }

            new_tag_size += 10 + frame_size;
        }

        position += 10 + frame_size;
    }

    if (!found)
    {
        printf("ERROR: Requested tag was not found\n");
        goto error;
    }

    /* Copy any padding still inside the old tag. */
    while (position < tag_end)
    {
        ch = fgetc(Tag->input_mp3_fptr);
        if (ch == EOF)
            break;
        if (fputc(ch, temp) == EOF)
            goto error;
        position++;
        new_tag_size++;
    }

    /* Copy the audio bytes after the ID3 tag unchanged. */
    if (fseek(Tag->input_mp3_fptr, tag_end, SEEK_SET) != 0)
        goto error;

    while ((ch = fgetc(Tag->input_mp3_fptr)) != EOF)
    {
        if (fputc(ch, temp) == EOF)
            goto error;
    }

     int_to_synchsafe((uint32_t)new_tag_size,
                 (unsigned char *)&header[6]);
                     if (fseek(temp, 0, SEEK_SET) != 0 ||
        fwrite(header, 1, 10, temp) != 10)
        goto error;

    if (fclose(temp) != 0)
{
    fclose(Tag->input_mp3_fptr);
    Tag->input_mp3_fptr = NULL;
    remove("temp.mp3");
    return e_failure;
}

if (fclose(Tag->input_mp3_fptr) != 0)
{
    Tag->input_mp3_fptr = NULL;
    remove("temp.mp3");
    return e_failure;
}

Tag->input_mp3_fptr = NULL;

if (rename("temp.mp3", Tag->input_mp3_filename) != 0)
{
    perror("ERROR: Could not replace original file");
    printf("Temporary file is temp.mp3; original file is %s\n",
           Tag->input_mp3_filename);
    return e_failure;
}

printf("INFO: Operation completed successfully\n");
return e_success;

    printf("INFO: Operation completed successfully\n");
    return e_success;

error:
    fclose(temp);
    remove("temp.mp3");
    printf("ERROR: Could not process the ID3 frames\n");
    return e_failure;
}

Status edit_operation(tag *Tag)
{
    return change_tag(Tag, 0);
}

Status delete_operation(tag *Tag)
{
    const char *frame_id = get_parameter_to_edit(Tag->edit_parameter);

    /* The CLI stores a frame ID for edit. Accept either a name or a frame ID. */
    if (frame_id == NULL)
    {
        int i;
        for (i = 0; tags[i] != NULL; i++)
        {
            if (strcmp(tags[i], Tag->edit_parameter) == 0)
                break;
        }
        if (tags[i] == NULL)
        {
            printf("ERROR: Unsupported tag\n");
            return e_failure;
        }
    }
    else
    {
        strcpy(Tag->edit_parameter, frame_id);
    }
    if (copy_file(Tag->input_mp3_filename, "deleted_tag.bak") != e_success)
{
    printf("ERROR: Could not create backup\n");
    return e_failure;
}

    return change_tag(Tag, 1);
}

Status rename_and_delete(tag *Tag)
{
    if (Tag->input_mp3_fptr != NULL)
    {
        fclose(Tag->input_mp3_fptr);
        Tag->input_mp3_fptr = NULL;
    }

    if (Tag->temp_mp3 != NULL)
    {
        fclose(Tag->temp_mp3);
        Tag->temp_mp3 = NULL;
    }

    if (rename("temp.mp3", Tag->input_mp3_filename) != 0)
    {
        perror("ERROR: Unable to replace MP3 file");
        return e_failure;
    }

    return e_success;
}

/* Simple backup/restore: save and restore the complete MP3 file. */
Status copy_file(const char *source, const char *destination){
    FILE *src = fopen(source, "rb");
    FILE *dst;
    int ch;

    if (src == NULL)
    {
        perror("ERROR: Cannot open source file");
        return e_failure;
    }

    dst = fopen(destination, "wb");
    if (dst == NULL)
    {
        perror("ERROR: Cannot create destination file");
        fclose(src);
        return e_failure;
    }

    while ((ch = fgetc(src)) != EOF)
    {
        if (fputc(ch, dst) == EOF)
        {
            fclose(src);
            fclose(dst);
            return e_failure;
        }
    }

    fclose(src);
    fclose(dst);
    return e_success;
}

Status restore_operation(tag *Tag)
{
    if (Tag == NULL || Tag->input_mp3_filename[0] == '\0')
    {
        printf("ERROR: Invalid MP3 filename\n");
        return e_failure;
    }

    if (copy_file("deleted_tag.bak",
                  Tag->input_mp3_filename) == e_success)
    {
        printf("INFO: MP3 file restored successfully\n");
        return e_success;
    }

    printf("ERROR: Backup not found or restore failed\n");
    return e_failure;
}
Status delete_all_tags(tag *Tag)
{
    FILE *src, *temp;
    unsigned char header[10];
    uint32_t size;
    char temp_name[] = "temp.mp3";
    int ch;

    src = fopen(Tag->input_mp3_filename, "rb");

    if (src == NULL)
    {
        printf("ERROR: Unable to open MP3 file\n");
        return e_failure;
    }

    if (fread(header, 1, 10, src) != 10 ||
        memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: Invalid ID3 file\n");
        fclose(src);
        return e_failure;
    }

    size = synchsafe_to_int(&header[6]);

    if (fseek(src, 10L + size, SEEK_SET) != 0)
    {
        printf("ERROR: Invalid ID3 tag size\n");
        fclose(src);
        return e_failure;
    }

    temp = fopen(temp_name, "wb");

    if (temp == NULL)
    {
        printf("ERROR: Unable to create temporary file\n");
        fclose(src);
        return e_failure;
    }

    while ((ch = fgetc(src)) != EOF)
    {
        if (fputc(ch, temp) == EOF)
        {
            printf("ERROR: Unable to write audio data\n");
            fclose(src);
            fclose(temp);
            remove(temp_name);
            return e_failure;
        }
    }

    if (ferror(src))
    {
        printf("ERROR: Unable to read audio data\n");
        fclose(src);
        fclose(temp);
        remove(temp_name);
        return e_failure;
    }

    fclose(src);

    if (fclose(temp) != 0)
    {
        remove(temp_name);
        return e_failure;
    }

if (rename(temp_name, Tag->input_mp3_filename) != 0)
{
    perror("ERROR: Unable to replace original file");
    return e_failure;
}


    printf("All ID3 tags deleted successfully\n");
    return e_success;
}

