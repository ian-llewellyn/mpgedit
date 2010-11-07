#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *cmd;
    int sts = 0;

    cmd = (char *) malloc(strlen(argv[0]) + 5 + sizeof(".term"));
    if (cmd) {
        strcpy(cmd, "open ");
        strcat(&cmd[5], argv[0]);
        strcat(cmd, ".term");
        sts = system(cmd);
    }
    return sts;
}
