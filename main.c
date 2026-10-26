#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "view.h"

/* Check whether the filename ends with .mp3. */
static int valid_mp3_name(const char *name)
{
    size_t len;

    if (name == NULL)
        return 0;

    len = strlen(name);

    if (len < 4)
        return 0;

    return strcasecmp(name + len - 4, ".mp3") == 0;
}

/* Validate the filename and store it in the structure. */
static int set_filename(tag *Tag, const char *name)
{
    if (!valid_mp3_name(name))
    {
        printf("ERROR: Filename must end with .mp3\n");
        return 0;
    }

    if (strlen(name) >= sizeof(Tag->input_mp3_filename))
    {
        printf("ERROR: Filename is too long\n");
        return 0;
    }

    strcpy(Tag->input_mp3_filename, name);
    return 1;
}

int main(int argc, char *argv[])
{
    tag Tag = {0};
    Status ret;
    OperationType operation;

    if (argc < 2)
    {
        printf("ERROR: Insufficient arguments\n");
        display_help_menu();
        return 1;
    }

    operation = check_operation(argv[1]);

    /* -------------------- VIEW OPERATION -------------------- */
    if (operation == e_view)
    {
        if (argc != 3)
        {
            printf("Usage: ./mp3tag -v <song.mp3>\n");
            display_help_menu();
            return 1;
        }

        Tag.operationflag = e_view;

       ret = validate_input(argc, argv, &Tag);      
       if (ret == e_failure)
            return 1;

        ret = open_files(&Tag);
        if (ret == e_failure)
            return 1;

        ret = view_operation(&Tag);

        if (Tag.input_mp3_fptr != NULL)
            fclose(Tag.input_mp3_fptr);

        return ret == e_success ? 0 : 1;
    }

    /* -------------------- EDIT OPERATION -------------------- */
    else if (operation == e_edit)
    {
        if (argc != 5)
        {
            printf("Usage: ./mp3tag -m <Tag> \"New value\" <song.mp3>\n");
            display_help_menu();
            return 1;
        }

        Tag.operationflag = e_edit;

     ret = validate_input(argc, argv, &Tag);        
      if (ret == e_failure)
            return 1;

        ret = open_files(&Tag);
        if (ret == e_failure)
            return 1;

        ret = edit_operation(&Tag);
        if (ret == e_failure)
        {
            if (Tag.input_mp3_fptr != NULL)
                fclose(Tag.input_mp3_fptr);
            if (Tag.temp_mp3 != NULL)
                fclose(Tag.temp_mp3);
            return 1;
        }

    
        return ret == e_success ? 0 : 1;
    }

    /* -------------------- DELETE ONE TAG -------------------- */
    else if (operation == e_delete)
    {
        const char *frame_id;

        if (argc != 4)
        {
            printf("Usage: ./mp3tag -d <Tag> <song.mp3>\n");
            display_help_menu();
            return 1;
        }

        //frame_id = get_parameter_to_edit(argv[2]);
       frame_id = get_parameter_to_edit(argv[2]);          
        if (frame_id == NULL)
        {
            printf("ERROR: Unsupported tag '%s'\n", argv[2]);
            return 1;
        }

        if (!set_filename(&Tag, argv[3]))
            return 1;

        strcpy(Tag.edit_parameter, frame_id);

        ret = open_files(&Tag);
        if (ret == e_failure)
            return 1;

        ret = delete_operation(&Tag);

        if (Tag.input_mp3_fptr != NULL)
            fclose(Tag.input_mp3_fptr);

        return ret == e_success ? 0 : 1;
    }

    /* -------------------- RESTORE BACKUP -------------------- */
    else if (operation == e_restore)
    {
        if (argc != 3)
        {
            printf("Usage: ./mp3tag -r <song.mp3>\n");
            return 1;
        }

        if (!set_filename(&Tag, argv[2]))
            return 1;

        ret = restore_operation(&Tag);

        return ret == e_success ? 0 : 1;
    }

    /* -------------------- DELETE ALL TAGS -------------------- */
    else if (operation == e_delete_all)
    {
        char confirm[10];

        if (argc != 3)
        {
            printf("Usage: ./mp3tag -da <song.mp3>\n");
            return 1;
        }

        if (!set_filename(&Tag, argv[2]))
            return 1;

        printf("This will remove all ID3 tags but keep audio data.\n");
        printf("Type YES to continue: ");

        if (fgets(confirm, sizeof(confirm), stdin) == NULL)
            return 1;

        confirm[strcspn(confirm, "\r\n")] = '\0';

        if (strcmp(confirm, "YES") != 0)
        {
            printf("Operation cancelled.\n");
            return 0;
        }

        ret = delete_all_tags(&Tag);
        return ret == e_success ? 0 : 1;
    }
    else if (operation == e_delete_all)
{
    if (argc != 3)
    {
        printf("Usage: ./mp3tag -da <file.mp3>\n");
        return 1;
    }

    ret = set_filename(&Tag, argv[2]);

    if (ret == e_success)
        ret = delete_all_tags(&Tag);

    return (ret == e_success) ? 0 : 1;
}

    /* -------------------- UNSUPPORTED OPERATION -------------------- */
    printf("ERROR: Unsupported operation '%s'\n", argv[1]);
    display_help_menu();
    return 1;
}
