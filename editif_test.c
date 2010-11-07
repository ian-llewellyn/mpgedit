#include "editif.h"


int main(void)
{
    editspec_t *edarray;
    int sts;
    void *ctx;

    unlink("editif_test.mp3");

    /* Index input file */
    ctx = mpgedit_edit_index_init("test1.mp3");
    while (!mpgedit_edit_index(ctx));
    mpgedit_edit_index_free(ctx);

    edarray = mpgedit_editspec_init();
    mpgedit_editspec_append(edarray, "test1.mp3", "15.986-19.983");
    mpgedit_editspec_append(edarray, "test1.mp3", "11.989-15.986");
    mpgedit_editspec_append(edarray, "test1.mp3", "7.993-11.989");
    mpgedit_editspec_append(edarray, "test1.mp3", "3.996-7.993");
    mpgedit_editspec_append(edarray, "test1.mp3", "0.0-3.996");
    mpgedit_editspec_append(edarray, "test1.mp3", "27.976-31.999");
    mpgedit_editspec_append(edarray, "test1.mp3", "23.979-27.976");
    mpgedit_editspec_append(edarray, "test1.mp3", "19.983-23.979");
    mpgedit_editspec_append(edarray, "test1.mp3", "31.999-");

    ctx = mpgedit_edit_files_init(edarray, "editif_test.mp3", 0, &sts);
    while (sts==0 && mpgedit_edit_files(ctx, &sts))
        ;
    mpgedit_edit_files_free(ctx);
    mpgedit_editspec_free(edarray);

    if (sts) {
        printf("mpgedit_edit_files failed %d\n", sts);
    }

    unlink("editif_test_2.mp3");
    edarray = mpgedit_editspec_init();
    mpgedit_editspec_append(edarray, "editif_test.mp3", "15.986-19.983");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "11.989-15.986");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "7.993-11.989");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "3.996-7.993");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "0.0-3.996");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "28.002-31.999");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "24.006-28.002");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "19.983-24.006");
    mpgedit_editspec_append(edarray, "editif_test.mp3", "31.999-");

    /* Index input file */
    ctx = mpgedit_edit_index_init("editif_test.mp3");
    while (!mpgedit_edit_index(ctx));
    mpgedit_edit_index_free(ctx);

    ctx = mpgedit_edit_files_init(edarray, "editif_test_2.mp3", 0, &sts);
    while (sts==0 && mpgedit_edit_files(ctx, &sts))
        ;
    mpgedit_edit_files_free(ctx);
    if (sts) {
        printf("mpgedit_edit_files failed\n");
    }

    return 0;
}
