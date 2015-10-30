/* UniDOS emulator */
/* By Nguyen Anh Quynh, 2015 */

#include <fcntl.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>

#include "int21.h"

#define FD_TABLE_SIZE 256

static uint8_t str_buf[1024];   // buffer for reading string from memory
static uint16_t dta;    // disk transfer area address
static int fdtable[FD_TABLE_SIZE];
static char buf[64 * 1024];


// read a string of @size bytes
static char *read_str(uc_engine *uc, uint64_t addr, int size)
{
    // do not read exceed the size of str_buf[]
    size = MIN(size, sizeof(str_buf) - 1);
    uc_mem_read(uc, addr, str_buf, size);
    str_buf[size] = '\0';

    return (char *)str_buf;
}


// read a string of @size bytes, or until '$' letter.
static char *read_str_till_char(uc_engine *uc, uint64_t addr, char terminator)
{
    size_t i = 0;

    // read a string from memory until '@terminator
    while(true) {
        uc_mem_read(uc, addr + i, str_buf + i, 1);
        if (str_buf[i] == terminator) {
            str_buf[i] = '\0';
            break;
        }
        i++;
    }

    return (char *)str_buf;
}


// set C flag in EFLAGS register
static void set_flag_C(uc_engine *uc, int flag)
{
    uint32_t r_eflags;

    uc_reg_read(uc, UC_X86_REG_EFLAGS, &r_eflags);

    if (flag)
        r_eflags &= 0xfffffffe; // eflags_C = 0
    else
        r_eflags |= 1; // eflags_C = 1

    uc_reg_write(uc, UC_X86_REG_EFLAGS, &r_eflags);
}


// initialize FD table
static void fdtable_init(void)
{
    int i;

    // enable special fd
    fdtable[0] = 0;
    fdtable[1] = 1;
    fdtable[2] = 2;

    // disable other fd
    for(i = 3; i < FD_TABLE_SIZE; i++)
        fdtable[i] = -1;
}


// clear FD
static void fdtable_clear(int fd)
{
    if (fd < 3 || fd >= FD_TABLE_SIZE)
        return;

    fdtable[fd] = -1;
}


// map host FD to dos FD
static int fdtable_set(int hostfd)
{
    int i;

    for(i = 0; i < FD_TABLE_SIZE; i++) {
        if (fdtable[i] == -1) {
            // found a free slot, which is also a dos FD
            fdtable[i] = hostfd;
            return i;
        }
    }

    // not found
    return -1;
}


// map DOS FD to host FD
static int fdtable_get(int dosfd)
{
    if (dosfd > 0 && dosfd < FD_TABLE_SIZE)
        return fdtable[dosfd];

    return -1;
}


void int21_init(void)
{
    fdtable_init();
}


// callback for handling interrupt
void int21(uc_engine *uc)
{
    uint16_t r_ip;
    uint8_t r_ah;

    uc_reg_read(uc, UC_X86_REG_AH, &r_ah);
    uc_reg_read(uc, UC_X86_REG_IP, &r_ip);

    //printf(">>> 0x%x: interrupt: %x, AH = %02x\n", r_ip, 0x21, r_ah);

    switch(r_ah) {
        default:
            //printf("\n>>> 0x%x: interrupt: %x, AH = %02x\n", r_ip, 0x21, r_ah);
            break;

        case 0x00: //terminate process
            {
                uc_emu_stop(uc);
            }

        case 0x01: // character input with echo
            {
                uint8_t r_al;

                r_al = getchar();

                uc_reg_write(uc, UC_X86_REG_AL, &r_al);

                break;
            }

        case 0x02: // character output
            {
                uint8_t r_dl;

                uc_reg_read(uc, UC_X86_REG_DL, &r_dl);

                printf("%c\n", r_dl);

                break;
            }

        case 0x08: // character input without echo
            {
                // TODO we need a cross-platform function that input without echo
                uint8_t r_al;

                r_al = getchar(); // FIXME

                uc_reg_write(uc, UC_X86_REG_AL, &r_al);

                break;
            }

        case 0x09: // write to screen
            {
                uint16_t r_dx, r_ds;
                char *buf;

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);
                //printf(">>> 0x%x: interrupt: %x, AH: %02x, DX = %02x, DS = %02x, addr = %x\n\n",
                //        r_ip, 0x21, r_ah, r_dx, r_ds, MK_FP(r_ds, r_dx));

                // read until '$'
                buf = read_str_till_char(uc, MK_FP(r_ds, r_dx), '$');
                printf("%s", buf);

                break;
            }

        case 0x0a: // buffered keyboard input
            {
                uint16_t r_dx, r_ds;
                uint8_t max_buf = 0;
                char *buf = NULL;
                char *str = NULL;   

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);
                uc_mem_read(uc, r_dx, &max_buf, 1);

                fscanf(stdin, "%ms", &buf);
                str = strndup(buf, max_buf -1);
                str[max_buf] = '$';

                uc_mem_write(uc, MK_FP(r_ds, r_dx) + 2, str, strlen(str));

                free(buf);
                free(str);

                break;
            }

        case 0x1a: // set disk transfer area address
            {
                uint16_t r_dx;

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);

                dta = r_dx;

                break;
            }

        case 0x30: // return DOS version 7.0
            {
                uint16_t r_ax = 0x07;

                uc_reg_write(uc, UC_X86_REG_AX, &r_ax);

                break;
            }

        case 0x3c: // create a new file (or truncate existing file)
            {
                uint16_t tmp, r_dx, r_ds;
                uint8_t r_al;
                char *fname;
                int hostfd;

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);
                uc_reg_read(uc, UC_X86_REG_AL, &r_al);

                // read until '$'
                fname = read_str_till_char(uc, MK_FP(r_ds, r_dx), '$');
                //printf(">>> Trying to create a new file '%s'\n", fname);

                // TODO ignore attributes
                hostfd = open(fname, O_CREAT | O_TRUNC | O_RDWR);
                if (hostfd < 0) {   // failed to open
                    set_flag_C(uc, 1);
                    tmp = errno;    // FIXME
                } else {
                    int dosfd = fdtable_set(hostfd);
                    if (dosfd < 0) {
                        // system table is full
                        close(hostfd);
                        set_flag_C(uc, 1);
                        tmp = ENFILE;
                    } else {
                        set_flag_C(uc, 0);
                        tmp = dosfd;
                        //printf(">>> OK, dosfd = %u\n", dosfd);
                    }
                }
                uc_reg_write(uc, UC_X86_REG_AX, &tmp);

                break;
            }

        case 0x3d: // open existing file
            {
                uint16_t tmp, r_dx, r_ds;
                uint8_t r_al;
                char *fname;
                int hostfd;

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);
                uc_reg_read(uc, UC_X86_REG_AL, &r_al);

                // read until '$'
                fname = read_str_till_char(uc, MK_FP(r_ds, r_dx), '$');
                //printf(">>> Trying to open file '%s'\n", fname);

                hostfd = open(fname, r_al & 3);
                if (hostfd < 0) {   // failed to open
                    set_flag_C(uc, 1);
                    tmp = errno;    // FIXME
                } else {
                    int dosfd = fdtable_set(hostfd);
                    if (dosfd < 0) {
                        // system table is full
                        close(hostfd);
                        set_flag_C(uc, 1);
                        tmp = ENFILE;
                    } else {
                        set_flag_C(uc, 0);
                        tmp = dosfd;
                        //printf(">>> OK, dosfd = %u\n", dosfd);
                    }
                }
                uc_reg_write(uc, UC_X86_REG_AX, &tmp);

                break;
            }

        case 0x3e: // close file
            {
                uint16_t r_bx, r_ax;
                int fd;

                // read from FD in BX register
                uc_reg_read(uc, UC_X86_REG_BX, &r_bx);
                fd = fdtable_get(r_bx);
                if (fd < 0) {
                    set_flag_C(uc, 1);
                    r_ax = EBADF;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    break;
                }

                if (close(fd)) {
                    set_flag_C(uc, 1);
                    r_ax = errno;   // FIXME
                } else {
                    set_flag_C(uc, 0);
                    fdtable_clear(r_bx);
                }

                //printf(">>> closed file = %u\n", r_bx);

                break;
            }

        case 0x3f: // read from device
            {
                uint16_t r_bx, r_ax, r_cx, r_dx, r_ds;
                int fd;
                ssize_t count;

                // read from FD in BX register
                uc_reg_read(uc, UC_X86_REG_BX, &r_bx);
                fd = fdtable_get(r_bx);
                if (fd < 0) {
                    set_flag_C(uc, 1);
                    r_ax = EBADF;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    break;
                }

                uc_reg_read(uc, UC_X86_REG_CX, &r_cx);
                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);

                //printf(">>> trying to read from devide = %u, len = %u\n", r_bx, r_cx);
                count = read(fd, buf, r_cx);
                if (count < 0) {
                    // failed to read
                    set_flag_C(uc, 1);
                    r_ax = errno;   // FIXME
                } else {
                    // copy read memory to emulator memory
                    uc_mem_write(uc, MK_FP(r_ds, r_dx), buf, count);
                    set_flag_C(uc, 0);
                    r_ax = count;
                }

                uc_reg_write(uc, UC_X86_REG_AX, &r_ax);

                break;
            }

        case 0x40: // Write to device
            {
                uint16_t r_ax, r_bx, r_cx, r_dx, r_ds;
                char *buf;
                int fd;
                ssize_t count;

                // write to FD in BX register
                uc_reg_read(uc, UC_X86_REG_BX, &r_bx);
                //printf(">>> trying to write to devide = %u\n", r_bx);
                fd = fdtable_get(r_bx);
                if (fd < 0) {
                    set_flag_C(uc, 1);
                    r_ax = EBADF;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    break;
                }

                uc_reg_read(uc, UC_X86_REG_CX, &r_cx);
                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);

                buf = read_str(uc, MK_FP(r_ds, r_dx), r_cx);
                //printf("%s", buf);

                count = write(fd, buf, r_cx);
                if (count < 0) {
                    // failed to write
                    set_flag_C(uc, 1);
                    r_ax = errno;   // FIXME
                } else {
                    set_flag_C(uc, 0);
                    r_ax = count;
                }

                uc_reg_write(uc, UC_X86_REG_AX, &r_ax);

                break;
            }

        case 0x41: // delete a file
            {
                uint16_t r_dx, r_ds, r_ax;
                char *fname;

                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_DS, &r_ds);

                // read until '$'
                fname = read_str_till_char(uc, MK_FP(r_ds, r_dx), '$');
                //printf(">>> Trying to delete file '%s'\n", fname);

                if (unlink(fname)) {
                    set_flag_C(uc, 1);
                    r_ax = errno;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                } else {
                    set_flag_C(uc, 0);
                }

                break;
            }
        case 0x42: // lseek to set file position
            {
                uint16_t r_bx, r_cx, r_dx, r_ax;
                uint8_t r_al;
                int fd;
                off_t offset;

                // FD in BX register
                uc_reg_read(uc, UC_X86_REG_BX, &r_bx);
                fd = fdtable_get(r_bx);
                if (fd < 0) {
                    set_flag_C(uc, 1);
                    r_ax = EBADF;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    break;
                }

                uc_reg_read(uc, UC_X86_REG_CX, &r_cx);
                uc_reg_read(uc, UC_X86_REG_DX, &r_dx);
                uc_reg_read(uc, UC_X86_REG_AL, &r_al);

                offset = lseek(fd, (r_cx << 16) | r_dx, r_al);
                //printf(">>> trying to seek in devide = %u, offset = %u, where = %u\n", r_bx, offset, r_al);
                if (offset < 0) {
                    // fail to seek
                    set_flag_C(uc, 1);
                    r_ax = errno;   // FIXME
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    //printf(">>> FAILED\n");
                } else {
                    set_flag_C(uc, 0);
                    r_ax = offset & 0xffff;
                    r_dx = offset >> 16;
                    uc_reg_write(uc, UC_X86_REG_AX, &r_ax);
                    uc_reg_write(uc, UC_X86_REG_DX, &r_dx);
                    //printf(">>> OK\n");
                }

                break;
            }

        case 0x4c: // exit
            {
                uint8_t r_al;

                uc_reg_read(uc, UC_X86_REG_AL, &r_al);
                //printf(">>> 0x%x: interrupt: %x, AH: %02x = EXIT. quit with code = %02x!\n\n",
                //        r_ip, 0x21, r_ah, r_al);

                uc_emu_stop(uc);
                break;
            }

        case 0x57: // get data & time of last write to file
            {
                uint32_t r_eflags;

                //printf(">>> UNIMPLEMENTED datetime\n");
                uc_reg_read(uc, UC_X86_REG_EFLAGS, &r_eflags);
                r_eflags &= 0xfffffffe; // eflags_C = 0
                uc_reg_write(uc, UC_X86_REG_EFLAGS, &r_eflags);

                break;
            }
    }
}
