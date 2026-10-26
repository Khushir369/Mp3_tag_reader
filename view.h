
#ifndef VIEW_H
#define VIEW_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/*-------------------- CONSTANTS --------------------*/

#define MP3_FILENAME_MAX       512
#define MP3_BACKUP_SUFFIX      ".bak"
#define MP3_TEMP_SUFFIX        ".tmp"
#define MP3_TAG_DATA_MAX       4096

#define ID3_HEADER_SIZE        10
#define ID3_FRAME_HEADER_SIZE  10
#define ID3_MAX_TAG_SIZE      268435455U

/*-------------------- STATUS --------------------*/

typedef enum
{
    e_success = 0,
    e_failure
} Status;

/*-------------------- OPERATIONS --------------------*/

typedef enum
{
    e_view = 1,
    e_edit,
    e_delete,
    e_restore,
    e_delete_all,
    e_backup,
    e_help,
    e_exit,
    e_unsupported
} OperationType;

/*-------------------- TAG PARAMETERS --------------------*/

typedef enum
{
    p_title,
    p_artist,
    p_album,
    p_year,
    p_genre,
    p_track,
    p_disc,
    p_composer,
    p_album_artist,
    p_comment,
    p_lyrics,
    p_copyright,
    p_publisher,
    p_encoder,
    p_bpm,
    p_original_artist,
    p_conductor,
    p_grouping,
    p_subtitle,
    p_language,
    p_unknown
} TagParameter;

/*-------------------- MP3 FILE DETAILS --------------------*/

typedef struct _tag
{
    /* File paths */
    char input_mp3_filename[MP3_FILENAME_MAX];
    char backup_filename[MP3_FILENAME_MAX + sizeof(MP3_BACKUP_SUFFIX)];
    char temp_filename[MP3_FILENAME_MAX + sizeof(MP3_TEMP_SUFFIX)];

    /* File pointers */
    FILE *input_mp3_fptr;
    FILE *temp_mp3;

    /* Selected operation */
    OperationType operationflag;

    /* ID3 tag information */
    uint32_t mp3_file_size;
    uint32_t frame_size;

    /* Editing information */
    char edit_parameter[5];
    char newdata[MP3_TAG_DATA_MAX];
    size_t size_of_newdata;

    /* Backup state */
    int backup_exists;

} tag;

/*-------------------- MENU AND HELP --------------------*/

void display_menu(void);
void display_help_menu(void);

/*-------------------- INPUT VALIDATION --------------------*/

OperationType check_operation(const char *argument);
Status validate_input(int argc, char **argv, tag *Tag);
Status validate_mp3_file(const char *filename);

/*-------------------- TAG MAPPING --------------------*/

const char *get_parameter_to_edit(const char *parameter);
const char *get_frame_id(TagParameter parameter);

/*-------------------- FILE HANDLING --------------------*/

Status open_files(tag *Tag);
void close_files(tag *Tag);

/*-------------------- BYTE CONVERSION --------------------*/

uint32_t read_big_endian(const unsigned char *buffer);
void write_big_endian(uint32_t value, unsigned char *buffer);

uint32_t synchsafe_to_int(const unsigned char *buffer);
void int_to_synchsafe(uint32_t size, unsigned char *buffer);

/*-------------------- VIEW AND EDIT --------------------*/

Status view_operation(tag *Tag);
Status edit_operation(tag *Tag);

/*-------------------- DELETE OPERATIONS --------------------*/

Status delete_operation(tag *Tag);
Status delete_all_tags(tag *Tag);

/*-------------------- BACKUP AND RESTORE --------------------*/

Status create_backup(tag *Tag);
Status validate_backup(tag *Tag);
Status restore_operation(tag *Tag);

/*-------------------- SAFE FILE REPLACEMENT --------------------*/

Status rename_and_delete(tag *Tag);
Status copy_file(const char *source, const char *destination);
void cleanup_temp_file(tag *Tag);

#endif
