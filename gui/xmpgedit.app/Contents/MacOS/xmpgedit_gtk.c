#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *cmd;
    int len = 0;
    int i = 1;

    while (i < argc) {
        len += strlen(argv[i]) + 2;
        i++;
    }

    cmd = (char *) malloc(strlen(argv[0]) + 2 + 4 + len + sizeof(".sh"));
    if (cmd) {
        strcpy(cmd, argv[0]);
        strcat(cmd, "_gtk.sh");
        strcat(cmd, " ");
        i = 1;
        while (i < argc) {
            strcat(cmd, argv[i]);
            strcat(cmd, " ");
            i++;
        }
    }
    return system(cmd);
}
